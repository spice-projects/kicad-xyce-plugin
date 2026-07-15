#include <algorithm>
#include <cmath>
#include <complex>
#include <span>
#include <stdexcept>

#include "xyce_evaluator.h"

namespace
{
    // determine whether the value is already a complex scalar
    bool is_complex_scalar(const XyceValue& value) {
        return std::holds_alternative<std::complex<double>>(value);
    }

    // determine whether the value is a vector
    bool is_vector_value(const XyceValue& value) {
        return std::holds_alternative<std::vector<double>>(value) || std::holds_alternative<std::vector<std::complex<double>>>(value);
    }

    struct RealBroadcastInput
    {
        std::span<const double> values;
        bool scalar = false;
        double scalar_value = 0.0;
    };

    struct ComplexBroadcastInput
    {
        std::span<const std::complex<double>> values;
        bool scalar = false;
        std::complex<double> scalar_value = {0.0, 0.0};
    };

    RealBroadcastInput to_real_input(const XyceValue& value, std::vector<double>& scratch) {
        if (std::holds_alternative<double>(value)) {
            return {.scalar = true, .scalar_value = std::get<double>(value)};
        }
        if (std::holds_alternative<std::complex<double>>(value)) {
            return {.scalar = true, .scalar_value = std::get<std::complex<double>>(value).real()};
        }
        if (std::holds_alternative<std::vector<double>>(value)) {
            return {.values = std::get<std::vector<double>>(value)};
        }
        const auto& complex_values = std::get<std::vector<std::complex<double>>>(value);
        scratch.clear();
        scratch.reserve(complex_values.size());
        for (const auto& element : complex_values) {
            scratch.push_back(element.real());
        }
        return {.values = scratch};
    }

    ComplexBroadcastInput to_complex_input(const XyceValue& value, std::vector<std::complex<double>>& scratch) {
        if (std::holds_alternative<double>(value)) {
            return {.scalar = true, .scalar_value = {std::get<double>(value), 0.0}};
        }
        if (std::holds_alternative<std::complex<double>>(value)) {
            return {.scalar = true, .scalar_value = std::get<std::complex<double>>(value)};
        }
        if (std::holds_alternative<std::vector<std::complex<double>>>(value)) {
            return {.values = std::get<std::vector<std::complex<double>>>(value)};
        }
        const auto& real_values = std::get<std::vector<double>>(value);
        scratch.clear();
        scratch.reserve(real_values.size());
        for (const auto element : real_values) {
            scratch.emplace_back(element, 0.0);
        }
        return {.values = scratch};
    }

    double pick_real(const RealBroadcastInput& input, const size_t index) {
        if (input.scalar) {
            return input.scalar_value;
        }
        if (input.values.size() == 1) {
            return input.values[0];
        }
        return input.values[index];
    }

    std::complex<double> pick_complex(const ComplexBroadcastInput& input, const size_t index) {
        if (input.scalar) {
            return input.scalar_value;
        }
        if (input.values.size() == 1) {
            return input.values[0];
        }
        return input.values[index];
    }

    class ScopedVariableBindings
    {
    public:
        explicit ScopedVariableBindings(EvaluationContext& context) :
            m_context(context) {
        }

        void bind(std::string key, XyceValue value) {
            m_keys.push_back(key);
            m_previous.push_back(m_context.variables.extract(key));
            m_context.variables.insert_or_assign(std::move(key), std::move(value));
        }

        ~ScopedVariableBindings() {
            for (const auto& key : m_keys) {
                m_context.variables.erase(key);
            }
            for (auto& node : m_previous) {
                if (!node.empty()) {
                    m_context.variables.insert(std::move(node));
                }
            }
        }

        ScopedVariableBindings(const ScopedVariableBindings&) = delete;
        ScopedVariableBindings& operator=(const ScopedVariableBindings&) = delete;

    private:
        EvaluationContext& m_context;
        std::vector<std::string> m_keys;
        std::vector<std::unordered_map<std::string, XyceValue>::node_type> m_previous;
    };

    // collapse a complex scalar to a scalar when possible
    XyceValue make_scalar(std::complex<double> value) {
        if (std::abs(value.imag()) < 1e-15) {
            return value.real();
        }
        return value;
    }

    // collapse a complex vector to a real vector when possible
    XyceValue make_vector(std::vector<std::complex<double>> value) {
        bool all_real = true;
        for (const auto& element : value) {
            if (std::abs(element.imag()) >= 1e-15) {
                all_real = false;
                break;
            }
        }
        if (all_real) {
            std::vector<double> out;
            out.reserve(value.size());
            for (const auto& element : value) {
                out.push_back(element.real());
            }
            return out;
        }
        return value;
    }

    // apply a real-valued binary operation with scalar broadcasting
    XyceValue broadcast_binary_real(const XyceValue& left, const XyceValue& right, const std::function<double(double, double)>& fn) {
        if (!is_vector_value(left) && !is_vector_value(right)) {
            const auto lhs = std::holds_alternative<double>(left) ? std::get<double>(left) : std::get<std::complex<double>>(left).real();
            const auto rhs = std::holds_alternative<double>(right) ? std::get<double>(right) : std::get<std::complex<double>>(right).real();
            return fn(lhs, rhs);
        }
        std::vector<double> left_scratch;
        std::vector<double> right_scratch;
        const auto left_values = to_real_input(left, left_scratch);
        const auto right_values = to_real_input(right, right_scratch);
        const auto left_size = left_values.scalar ? size_t{1} : left_values.values.size();
        const auto right_size = right_values.scalar ? size_t{1} : right_values.values.size();
        const auto size = std::max(left_size, right_size);
        std::vector<double> out;
        out.reserve(size);
        for (size_t index = 0; index < size; ++index) {
            const auto lhs = pick_real(left_values, index);
            const auto rhs = pick_real(right_values, index);
            out.push_back(fn(lhs, rhs));
        }
        return out;
    }

    // apply a complex-valued binary operation with scalar broadcasting
    XyceValue broadcast_binary_complex(const XyceValue& left, const XyceValue& right, const std::function<std::complex<double>(std::complex<double>, std::complex<double>)>& fn) {
        if (!is_vector_value(left) && !is_vector_value(right)) {
            const auto lhs = std::holds_alternative<std::complex<double>>(left) ? std::get<std::complex<double>>(left) : std::complex<double>(std::get<double>(left), 0.0);
            const auto rhs = std::holds_alternative<std::complex<double>>(right) ? std::get<std::complex<double>>(right) : std::complex<double>(std::get<double>(right), 0.0);
            return make_scalar(fn(lhs, rhs));
        }
        std::vector<std::complex<double>> left_scratch;
        std::vector<std::complex<double>> right_scratch;
        const auto left_values = to_complex_input(left, left_scratch);
        const auto right_values = to_complex_input(right, right_scratch);
        const auto left_size = left_values.scalar ? size_t{1} : left_values.values.size();
        const auto right_size = right_values.scalar ? size_t{1} : right_values.values.size();
        const auto size = std::max(left_size, right_size);
        std::vector<std::complex<double>> out;
        out.reserve(size);
        for (size_t index = 0; index < size; ++index) {
            const auto lhs = pick_complex(left_values, index);
            const auto rhs = pick_complex(right_values, index);
            out.push_back(fn(lhs, rhs));
        }
        return make_vector(std::move(out));
    }

    // apply a unary real-valued operation across a scalar or vector
    XyceValue broadcast_unary_real(const XyceValue& value, const std::function<double(double)>& fn) {
        if (!is_vector_value(value)) {
            const auto scalar = std::holds_alternative<double>(value) ? std::get<double>(value) : std::get<std::complex<double>>(value).real();
            return fn(scalar);
        }
        std::vector<double> scratch;
        const auto values = to_real_input(value, scratch);
        const auto size = values.scalar ? size_t{1} : values.values.size();
        std::vector<double> out;
        out.reserve(size);
        for (size_t index = 0; index < size; ++index) {
            out.push_back(fn(pick_real(values, index)));
        }
        return out;
    }

    // apply a unary complex-valued operation across a scalar or vector
    XyceValue broadcast_unary_complex(const XyceValue& value, const std::function<std::complex<double>(std::complex<double>)>& fn) {
        if (!is_vector_value(value)) {
            const auto scalar = std::holds_alternative<std::complex<double>>(value) ? std::get<std::complex<double>>(value) : std::complex<double>(std::get<double>(value), 0.0);
            return make_scalar(fn(scalar));
        }
        std::vector<std::complex<double>> scratch;
        const auto values = to_complex_input(value, scratch);
        const auto size = values.scalar ? size_t{1} : values.values.size();
        std::vector<std::complex<double>> out;
        out.reserve(size);
        for (size_t index = 0; index < size; ++index) {
            out.push_back(fn(pick_complex(values, index)));
        }
        return make_vector(std::move(out));
    }

    // read the scalar real part of the value
    double scalar_real(const XyceValue& value) {
        if (std::holds_alternative<double>(value)) {
            return std::get<double>(value);
        }
        if (std::holds_alternative<std::complex<double>>(value)) {
            return std::get<std::complex<double>>(value).real();
        }
        if (std::holds_alternative<std::vector<double>>(value)) {
            return std::get<std::vector<double>>(value).at(0);
        }
        return std::get<std::vector<std::complex<double>>>(value).at(0).real();
    }

    // read the scalar complex value
    std::complex<double> scalar_complex(const XyceValue& value) {
        if (std::holds_alternative<double>(value)) {
            return {std::get<double>(value), 0.0};
        }
        if (std::holds_alternative<std::complex<double>>(value)) {
            return std::get<std::complex<double>>(value);
        }
        if (std::holds_alternative<std::vector<double>>(value)) {
            return {std::get<std::vector<double>>(value).at(0), 0.0};
        }
        return std::get<std::vector<std::complex<double>>>(value).at(0);
    }
}

XyceValue XyceEvaluator::evaluate(const ExpressionNode& expression, const std::unordered_map<std::string, XyceValue>& expressions, const std::unordered_map<std::string, FunctionDefinitionNode>& functions, const std::unordered_map<std::string, XyceValue>& constants, const std::optional<std::vector<std::pair<size_t, size_t>>>& step_slices) {
    // build the evaluation context
    EvaluationContext context;
    // normalize expression keys
    for (const auto& [name, value] : expressions) {
        context.expressions[casefold(name)] = &value;
    }
    // normalize function keys
    for (const auto& [name, value] : functions) {
        context.functions.insert_or_assign(casefold(name), &value);
    }
    // normalize constant keys and seed the builtin constants
    for (const auto& [name, value] : BUILTIN_CONSTANTS) {
        context.constants[casefold(name)] = &value;
    }
    for (const auto& [name, value] : constants) {
        context.constants[casefold(name)] = &value;
    }
    // store step slices
    context.step_slices = step_slices;
    // evaluate the tree
    return evaluate(expression, context, {});
}

XyceValue XyceEvaluator::evaluate(const ExpressionNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
    if (const auto* node = dynamic_cast<const NumberNode*>(&expression); node != nullptr) {
        return std::stod(node->text);
    }
    if (const auto* node = dynamic_cast<const IdentifierNode*>(&expression); node != nullptr) {
        return lookup_name(node->name, context);
    }
    if (const auto* node = dynamic_cast<const UnaryOperationNode*>(&expression); node != nullptr) {
        return evaluate_unary(*node, context, call_stack);
    }
    if (const auto* node = dynamic_cast<const BinaryOperationNode*>(&expression); node != nullptr) {
        return evaluate_binary(*node, context, call_stack);
    }
    if (const auto* node = dynamic_cast<const TernaryOperationNode*>(&expression); node != nullptr) {
        return evaluate_ternary(*node, context, call_stack);
    }
    if (const auto* node = dynamic_cast<const FunctionCallNode*>(&expression); node != nullptr) {
        return evaluate_function_call(*node, context, call_stack);
    }
    if (const auto* node = dynamic_cast<const StepSelectorNode*>(&expression); node != nullptr) {
        return evaluate_step_selector(*node, context, call_stack);
    }
    throw std::invalid_argument("Unsupported expression node");
}

XyceValue XyceEvaluator::evaluate_unary(const UnaryOperationNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
    // evaluate the operand first
    const auto value = evaluate(*expression.operand, context, call_stack);
    // apply unary plus
    if (expression.operator_value == UnaryOperator::POS) {
        return value;
    }
    // apply unary minus
    if (expression.operator_value == UnaryOperator::NEG) {
        if (std::holds_alternative<std::vector<double>>(value)) {
            return apply_unary_elementwise(value, [](double element) { return -element; });
        }
        if (std::holds_alternative<std::vector<std::complex<double>>>(value)) {
            return apply_unary_elementwise_complex(value, [](std::complex<double> element) { return -element; });
        }
        if (std::holds_alternative<std::complex<double>>(value)) {
            return make_scalar(-std::get<std::complex<double>>(value));
        }
        return -std::get<double>(value);
    }
    // apply logical not
    if (expression.operator_value == UnaryOperator::NOT) {
        const auto mask = truth_mask(value);
        if (std::holds_alternative<double>(mask)) {
            return std::get<double>(mask) == 0.0 ? 1.0 : 0.0;
        }
        const auto& values = std::get<std::vector<double>>(mask);
        std::vector<double> out;
        out.reserve(values.size());
        for (const auto element : values) {
            out.push_back(element == 0.0 ? 1.0 : 0.0);
        }
        return out;
    }
    throw std::invalid_argument("Unsupported unary operator");
}

XyceValue XyceEvaluator::evaluate_binary(const BinaryOperationNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
    // short-circuit logical and
    if (expression.operator_value == BinaryOperator::LOGICAL_AND) {
        const auto left_value = evaluate(*expression.left, context, call_stack);
        if (!is_vector_value(left_value) && scalar_real(left_value) == 0.0) {
            return 0.0;
        }
        const auto right_value = evaluate(*expression.right, context, call_stack);
        if (!is_vector_value(left_value) && !is_vector_value(right_value)) {
            return (scalar_real(left_value) != 0.0 && scalar_real(right_value) != 0.0) ? 1.0 : 0.0;
        }
        return apply_binary_real(truth_mask(left_value), truth_mask(right_value), [](double lhs, double rhs) { return (lhs != 0.0 && rhs != 0.0) ? 1.0 : 0.0; });
    }
    // short-circuit logical or
    if (expression.operator_value == BinaryOperator::LOGICAL_OR) {
        const auto left_value = evaluate(*expression.left, context, call_stack);
        if (!is_vector_value(left_value) && scalar_real(left_value) != 0.0) {
            return 1.0;
        }
        const auto right_value = evaluate(*expression.right, context, call_stack);
        if (!is_vector_value(left_value) && !is_vector_value(right_value)) {
            return (scalar_real(left_value) != 0.0 || scalar_real(right_value) != 0.0) ? 1.0 : 0.0;
        }
        return apply_binary_real(truth_mask(left_value), truth_mask(right_value), [](double lhs, double rhs) { return (lhs != 0.0 || rhs != 0.0) ? 1.0 : 0.0; });
    }
    // evaluate both operands for the remaining operators
    const auto left_value = evaluate(*expression.left, context, call_stack);
    const auto right_value = evaluate(*expression.right, context, call_stack);
    // arithmetic
    if (expression.operator_value == BinaryOperator::ADD) {
        if (is_complex_scalar(left_value) || is_complex_scalar(right_value) || std::holds_alternative<std::vector<std::complex<double>>>(left_value) || std::holds_alternative<std::vector<std::complex<double>>>(right_value)) {
            return apply_binary_numeric(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs + rhs; });
        }
        return apply_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs + rhs; });
    }
    if (expression.operator_value == BinaryOperator::SUB) {
        if (is_complex_scalar(left_value) || is_complex_scalar(right_value) || std::holds_alternative<std::vector<std::complex<double>>>(left_value) || std::holds_alternative<std::vector<std::complex<double>>>(right_value)) {
            return apply_binary_numeric(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs - rhs; });
        }
        return apply_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs - rhs; });
    }
    if (expression.operator_value == BinaryOperator::MUL) {
        if (is_complex_scalar(left_value) || is_complex_scalar(right_value) || std::holds_alternative<std::vector<std::complex<double>>>(left_value) || std::holds_alternative<std::vector<std::complex<double>>>(right_value)) {
            return apply_binary_numeric(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs * rhs; });
        }
        return apply_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs * rhs; });
    }
    if (expression.operator_value == BinaryOperator::DIV) {
        if (is_complex_scalar(left_value) || is_complex_scalar(right_value) || std::holds_alternative<std::vector<std::complex<double>>>(left_value) || std::holds_alternative<std::vector<std::complex<double>>>(right_value)) {
            return apply_binary_numeric(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs / rhs; });
        }
        return apply_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs / rhs; });
    }
    if (expression.operator_value == BinaryOperator::POW) {
        if (is_complex_scalar(left_value) || is_complex_scalar(right_value) || std::holds_alternative<std::vector<std::complex<double>>>(left_value) || std::holds_alternative<std::vector<std::complex<double>>>(right_value)) {
            return apply_binary_numeric(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return std::pow(lhs, rhs); });
        }
        return apply_binary_real(left_value, right_value, [](double lhs, double rhs) { return std::pow(lhs, rhs); });
    }
    if (expression.operator_value == BinaryOperator::MOD) {
        return apply_binary_real(left_value, right_value, [](double lhs, double rhs) {
            return lhs - std::floor(lhs / rhs) * rhs;
        });
    }
    if (expression.operator_value == BinaryOperator::EQUAL) {
        return apply_compare(left_value, right_value, [](double lhs, double rhs) { return lhs == rhs; });
    }
    if (expression.operator_value == BinaryOperator::NOT_EQUAL) {
        return apply_compare(left_value, right_value, [](double lhs, double rhs) { return lhs != rhs; });
    }
    if (expression.operator_value == BinaryOperator::LESS) {
        return apply_compare(left_value, right_value, [](double lhs, double rhs) { return lhs < rhs; });
    }
    if (expression.operator_value == BinaryOperator::LESS_EQUAL) {
        return apply_compare(left_value, right_value, [](double lhs, double rhs) { return lhs <= rhs; });
    }
    if (expression.operator_value == BinaryOperator::GREATER) {
        return apply_compare(left_value, right_value, [](double lhs, double rhs) { return lhs > rhs; });
    }
    if (expression.operator_value == BinaryOperator::GREATER_EQUAL) {
        return apply_compare(left_value, right_value, [](double lhs, double rhs) { return lhs >= rhs; });
    }
    if (expression.operator_value == BinaryOperator::LOGICAL_XOR) {
        return apply_binary_real(truth_mask(left_value), truth_mask(right_value), [](double lhs, double rhs) { return ((lhs != 0.0) ^ (rhs != 0.0)) ? 1.0 : 0.0; });
    }
    throw std::invalid_argument("Unsupported binary operator");
}

XyceValue XyceEvaluator::evaluate_ternary(const TernaryOperationNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
    // evaluate the condition value
    const auto condition = evaluate(*expression.condition, context, call_stack);
    // branch for scalar conditions
    if (!is_vector_value(condition)) {
        return scalar_real(condition) != 0.0 ? evaluate(*expression.if_true, context, call_stack) : evaluate(*expression.if_false, context, call_stack);
    }
    // evaluate both branches
    const auto if_true = evaluate(*expression.if_true, context, call_stack);
    const auto if_false = evaluate(*expression.if_false, context, call_stack);
    // select elementwise
    std::vector<double> mask_scratch;
    std::vector<double> true_scratch;
    std::vector<double> false_scratch;
    const auto mask = to_real_input(condition, mask_scratch);
    const auto true_values = to_real_input(if_true, true_scratch);
    const auto false_values = to_real_input(if_false, false_scratch);
    const auto mask_size = mask.scalar ? size_t{1} : mask.values.size();
    std::vector<double> out;
    out.reserve(mask_size);
    for (size_t index = 0; index < mask_size; ++index) {
        const auto t = pick_real(true_values, index);
        const auto f = pick_real(false_values, index);
        out.push_back(pick_real(mask, index) != 0.0 ? t : f);
    }
    return out;
}

XyceValue XyceEvaluator::evaluate_function_call(const FunctionCallNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
    // probe calls are handled specially
    if (is_probe_call(expression)) {
        return evaluate_probe(expression, context);
    }
    // resolve a builtin function
    const auto builtin_key = casefold(expression.name);
    if (const auto it = BUILTIN_FUNCTIONS.find(builtin_key); it != BUILTIN_FUNCTIONS.end()) {
        std::vector<XyceValue> args;
        args.reserve(expression.args.size());
        for (const auto& arg : expression.args) {
            args.push_back(evaluate(*arg, context, call_stack));
        }
        return it->second(args);
    }
    // resolve a user function
    const auto function_key = casefold(expression.name);
    const auto it = context.functions.find(function_key);
    if (it == context.functions.end()) {
        throw std::invalid_argument("Unknown function: " + expression.name);
    }
    if (std::find(call_stack.begin(), call_stack.end(), function_key) != call_stack.end()) {
        throw std::invalid_argument("Recursive function call detected: " + expression.name);
    }
    const auto& definition = *it->second;
    if (expression.args.size() != definition.params.size()) {
        throw std::invalid_argument("Function '" + expression.name + "' expects " + std::to_string(definition.params.size()) + " arguments, got " + std::to_string(expression.args.size()));
    }
    ScopedVariableBindings parameter_bindings(context);
    for (size_t index = 0; index < definition.params.size(); ++index) {
        parameter_bindings.bind(casefold(definition.params[index]), evaluate(*expression.args[index], context, call_stack));
    }
    auto next_stack = call_stack;
    next_stack.push_back(function_key);
    return evaluate(*definition.body, context, next_stack);
}

XyceValue XyceEvaluator::evaluate_step_selector(const StepSelectorNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
    if (!context.step_slices.has_value()) {
        throw std::invalid_argument("Step selector @N requires step metadata in the evaluation context");
    }
    const auto& steps = context.step_slices.value();
    if (expression.step_index < 1 || expression.step_index > steps.size()) {
        throw std::invalid_argument("Step selector @" + std::to_string(expression.step_index) + " is out of range: file has " + std::to_string(steps.size()) + " step(s)");
    }
    const auto base_value = evaluate(*expression.base, context, call_stack);
    const auto [begin, end] = steps.at(expression.step_index - 1);
    if (std::holds_alternative<std::vector<double>>(base_value)) {
        const auto& values = std::get<std::vector<double>>(base_value);
        return std::vector<double>(values.begin() + static_cast<std::ptrdiff_t>(begin), values.begin() + static_cast<std::ptrdiff_t>(end));
    }
    if (std::holds_alternative<std::vector<std::complex<double>>>(base_value)) {
        const auto& values = std::get<std::vector<std::complex<double>>>(base_value);
        return std::vector<std::complex<double>>(values.begin() + static_cast<std::ptrdiff_t>(begin), values.begin() + static_cast<std::ptrdiff_t>(end));
    }
    return base_value;
}

XyceValue XyceEvaluator::lookup_name(const std::string& name, EvaluationContext& context) {
    const auto key = casefold(name);
    if (const auto it = context.variables.find(key); it != context.variables.end()) {
        return it->second;
    }
    if (const auto it = context.expressions.find(key); it != context.expressions.end()) {
        return *it->second;
    }
    if (const auto it = context.constants.find(key); it != context.constants.end()) {
        return *it->second;
    }
    throw std::invalid_argument("Unknown identifier: " + name);
}

XyceValue XyceEvaluator::evaluate_probe(const FunctionCallNode& expression, EvaluationContext& context) {
    const auto probe_name = reconstruct_probe_name(expression);
    if (const auto it = context.variables.find(casefold(probe_name)); it != context.variables.end()) {
        return it->second;
    }
    if (const auto it = context.expressions.find(casefold(probe_name)); it != context.expressions.end()) {
        return *it->second;
    }
    if (expression.args.size() == 2) {
        const auto node_a_name = extract_node_name(*expression.args[0]);
        const auto node_b_name = extract_node_name(*expression.args[1]);
        if (casefold(node_b_name) == "0") {
            const auto probe_a_key = casefold("V(" + node_a_name + ")");
            if (const auto it = context.variables.find(probe_a_key); it != context.variables.end()) {
                return it->second;
            }
            if (const auto jt = context.expressions.find(probe_a_key); jt != context.expressions.end()) {
                return *jt->second;
            }
        }
        if (casefold(node_a_name) == "0") {
            const auto probe_b_key = casefold("V(" + node_b_name + ")");
            if (const auto it = context.variables.find(probe_b_key); it != context.variables.end()) {
                return broadcast_unary_real(it->second, [](double value) { return -value; });
            }
            if (const auto jt = context.expressions.find(probe_b_key); jt != context.expressions.end()) {
                return broadcast_unary_real(*jt->second, [](double value) { return -value; });
            }
        }
        const auto probe_a_key = casefold("V(" + node_a_name + ")");
        const auto probe_b_key = casefold("V(" + node_b_name + ")");
        const auto it_a = context.variables.find(probe_a_key);
        const auto it_b = context.variables.find(probe_b_key);
        const auto jt_a = context.expressions.find(probe_a_key);
        const auto jt_b = context.expressions.find(probe_b_key);
        const XyceValue* value_a = nullptr;
        if (it_a != context.variables.end()) {
            value_a = &it_a->second;
        }
        else if (jt_a != context.expressions.end()) {
            value_a = jt_a->second;
        }
        const XyceValue* value_b = nullptr;
        if (it_b != context.variables.end()) {
            value_b = &it_b->second;
        }
        else if (jt_b != context.expressions.end()) {
            value_b = jt_b->second;
        }
        if (value_a != nullptr) {
            if (value_b != nullptr) {
                return broadcast_binary_real(*value_a, *value_b, [](double lhs, double rhs) { return lhs - rhs; });
            }
            return *value_a;
        }
    }
    throw std::invalid_argument("Unknown probe: " + probe_name);
}

bool XyceEvaluator::is_probe_call(const FunctionCallNode& expression) {
    const auto name = casefold(expression.name);
    return (name == "v" || name == "i") && !expression.args.empty();
}

bool XyceEvaluator::has_simple_probe_args(const FunctionCallNode& expression) {
    for (const auto& arg : expression.args) {
        if (!dynamic_cast<const IdentifierNode*>(arg.get()) && !dynamic_cast<const NumberNode*>(arg.get())) {
            return false;
        }
    }
    return true;
}

std::string XyceEvaluator::reconstruct_probe_name(const FunctionCallNode& expression) {
    std::string result = expression.name;
    result += "(";
    for (size_t index = 0; index < expression.args.size(); ++index) {
        if (index > 0) {
            result += ", ";
        }
        result += node_name_from_expr(*expression.args[index]);
    }
    result += ")";
    return result;
}

std::string XyceEvaluator::node_name_from_expr(const ExpressionNode& expression) {
    if (const auto* node = dynamic_cast<const IdentifierNode*>(&expression); node != nullptr) {
        return node->name;
    }
    if (const auto* node = dynamic_cast<const NumberNode*>(&expression); node != nullptr) {
        return node->text;
    }
    throw std::invalid_argument("Cannot extract node name from complex expression");
}

std::string XyceEvaluator::extract_node_name(const ExpressionNode& expression) {
    return node_name_from_expr(expression);
}

bool XyceEvaluator::is_scalar_value(const XyceValue& value) {
    return std::holds_alternative<double>(value) || std::holds_alternative<std::complex<double>>(value);
}

size_t XyceEvaluator::value_size(const XyceValue& value) {
    if (std::holds_alternative<std::vector<double>>(value)) {
        return std::get<std::vector<double>>(value).size();
    }
    if (std::holds_alternative<std::vector<std::complex<double>>>(value)) {
        return std::get<std::vector<std::complex<double>>>(value).size();
    }
    return 1;
}

double XyceEvaluator::scalar_real(const XyceValue& value) {
    return ::scalar_real(value);
}

std::complex<double> XyceEvaluator::scalar_complex(const XyceValue& value) {
    return ::scalar_complex(value);
}

std::vector<double> XyceEvaluator::boolean_result(const std::vector<double>& values) {
    return values;
}

XyceValue XyceEvaluator::boolean_result(const bool value) {
    return value ? 1.0 : 0.0;
}

XyceValue XyceEvaluator::truth_mask(const XyceValue& value) {
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value) != 0.0 ? 1.0 : 0.0;
    }
    if (std::holds_alternative<std::complex<double>>(value)) {
        return std::get<std::complex<double>>(value).real() != 0.0 ? 1.0 : 0.0;
    }
    if (std::holds_alternative<std::vector<double>>(value)) {
        std::vector<double> out;
        out.reserve(std::get<std::vector<double>>(value).size());
        for (const auto element : std::get<std::vector<double>>(value)) {
            out.push_back(element != 0.0 ? 1.0 : 0.0);
        }
        return out;
    }
    std::vector<double> out;
    out.reserve(std::get<std::vector<std::complex<double>>>(value).size());
    for (const auto& element : std::get<std::vector<std::complex<double>>>(value)) {
        out.push_back(element.real() != 0.0 ? 1.0 : 0.0);
    }
    return out;
}

// apply a unary numeric operation to a scalar or vector
XyceValue XyceEvaluator::apply_unary_numeric(const XyceValue& value, const std::function<std::complex<double>(std::complex<double>)>& fn) {
    if (is_scalar_value(value)) {
        return make_scalar(fn(scalar_complex(value)));
    }
    if (std::holds_alternative<std::vector<double>>(value)) {
        std::vector<double> out;
        out.reserve(std::get<std::vector<double>>(value).size());
        for (const auto element : std::get<std::vector<double>>(value)) {
            out.push_back(fn(std::complex<double>(element, 0.0)).real());
        }
        return out;
    }
    std::vector<std::complex<double>> out;
    out.reserve(std::get<std::vector<std::complex<double>>>(value).size());
    for (const auto& element : std::get<std::vector<std::complex<double>>>(value)) {
        out.push_back(fn(element));
    }
    return make_vector(std::move(out));
}

// apply a numeric binary operation
XyceValue XyceEvaluator::apply_binary_numeric(const XyceValue& left, const XyceValue& right, const std::function<std::complex<double>(std::complex<double>, std::complex<double>)>& fn) {
    if (is_scalar_value(left) && is_scalar_value(right)) {
        return make_scalar(fn(scalar_complex(left), scalar_complex(right)));
    }
    return broadcast_binary_complex(left, right, fn);
}

// apply a real-valued binary operation
XyceValue XyceEvaluator::apply_binary_real(const XyceValue& left, const XyceValue& right, const std::function<double(double, double)>& fn) {
    return broadcast_binary_real(left, right, fn);
}

// apply a comparison and normalize the result to 0/1
XyceValue XyceEvaluator::apply_compare(const XyceValue& left, const XyceValue& right, const std::function<bool(double, double)>& fn) {
    return broadcast_binary_real(left, right, [fn](double lhs, double rhs) { return fn(lhs, rhs) ? 1.0 : 0.0; });
}

// apply a real elementwise helper
XyceValue XyceEvaluator::apply_elementwise(const XyceValue& left, const XyceValue& right, const std::function<double(double, double)>& fn) {
    return broadcast_binary_real(left, right, fn);
}

// apply a complex elementwise helper
XyceValue XyceEvaluator::apply_elementwise_complex(const XyceValue& left, const XyceValue& right, const std::function<std::complex<double>(std::complex<double>, std::complex<double>)>& fn) {
    return broadcast_binary_complex(left, right, fn);
}

// apply a real unary helper
XyceValue XyceEvaluator::apply_unary_elementwise(const XyceValue& value, const std::function<double(double)>& fn) {
    return broadcast_unary_real(value, fn);
}

// apply a complex unary helper
XyceValue XyceEvaluator::apply_unary_elementwise_complex(const XyceValue& value, const std::function<std::complex<double>(std::complex<double>)>& fn) {
    return broadcast_unary_complex(value, fn);
}

// normalize a name for case-insensitive lookups
std::string XyceEvaluator::casefold(std::string text) {
    std::ranges::transform(text, text.begin(), [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return text;
}

// keep the value in array-friendly form
XyceValue XyceEvaluator::to_array(const XyceValue& value) {
    return value;
}

XyceValue evaluate_expression(
    const ExpressionNode& expression,
    const std::unordered_map<std::string, XyceValue>& expressions,
    const std::unordered_map<std::string, FunctionDefinitionNode>& functions,
    const std::unordered_map<std::string, XyceValue>& constants,
    const std::optional<std::vector<std::pair<size_t, size_t>>>& step_slices) {
    return XyceEvaluator{}.evaluate(expression, expressions, functions, constants, step_slices);
}
