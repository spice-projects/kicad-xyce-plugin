#include <algorithm>
#include <cmath>
#include <complex>
#include <memory>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include "probe_names.h"
#include "util.h"
#include "view.h"
#include "xyce_evaluator.h"
#include "xyce_value.h"

namespace
{
    class ScopedVariableBindings
    {
    public:
        explicit ScopedVariableBindings(EvaluationContext& context) :
            m_context(context) {}

        void bind(std::string key, XyceValue value) {
            m_keys.push_back(key);
            m_previous.push_back(m_context.variables.extract(key));
            m_context.variables.insert_or_assign(std::move(key), std::move(value));
        }

        ~ScopedVariableBindings() {
            // restore previous variable bindings
            for (const auto& key : m_keys)
                m_context.variables.erase(key);
            // restore previous variable values
            for (auto& node : m_previous) {
                // check if the node is not empty before inserting it back into the map
                if (!node.empty())
                    m_context.variables.insert(std::move(node));
            }
        }

        ScopedVariableBindings(const ScopedVariableBindings&) = delete;
        ScopedVariableBindings& operator=(const ScopedVariableBindings&) = delete;

    private:
        EvaluationContext& m_context;
        std::vector<std::string> m_keys;
        std::vector<std::unordered_map<std::string, XyceValue>::node_type> m_previous;
    };

    XyceValue broadcast_binary_real(const XyceValue& left, const XyceValue& right, const std::function<double(double, double)>& fn) {
        // find left & right operand type
        bool left_is_scalar = is_scalar(left);
        bool right_is_scalar = is_scalar(right);
        // check both operands are scalars
        if (left_is_scalar && right_is_scalar) {
            // apply the function and return the result
            return fn(scalar_value<double>(left), scalar_value<double>(right));
        }
        // check both are vectors
        if (!left_is_scalar && !right_is_scalar) {
            // vectors
            auto left_vector = to_real_vector(left);
            auto right_vector = to_real_vector(right);
            // check sizes match
            if (left_vector->size() != right_vector->size())
                throw std::invalid_argument("Vector sizes do not match for binary operation");
            // result
            std::vector<double> out;
            // reserve space for the result
            out.reserve(left_vector->size());
            // loop vectors
            for (size_t i = 0; i < left_vector->size(); ++i)
                out.push_back(fn((*left_vector)[i], (*right_vector)[i]));
            // exit
            return std::make_shared<View<double>>(out);
        }
        // check left is scalar and right is vector
        if (left_is_scalar) {
            // operands
            auto left_scalar = scalar_value<double>(left);
            auto right_vector = to_real_vector(right);
            // result
            std::vector<double> out;
            // reserve space for the result
            out.reserve(right_vector->size());
            // loop vector
            for (size_t i = 0; i < right_vector->size(); ++i)
                out.push_back(fn(left_scalar, (*right_vector)[i]));
            // exit
            return std::make_shared<View<double>>(out);
        }
        // operands
        auto left_vector = to_real_vector(left);
        auto right_scalar = scalar_value<double>(right);
        // result
        std::vector<double> out;
        // reserve space for the result
        out.reserve(left_vector->size());
        // loop vector
        for (size_t i = 0; i < left_vector->size(); ++i)
            out.push_back(fn((*left_vector)[i], right_scalar));
        // exit
        return std::make_shared<View<double>>(out);
    }

    XyceValue broadcast_binary_complex(const XyceValue& left, const XyceValue& right, const std::function<std::complex<double>(std::complex<double>, std::complex<double>)>& fn) {
        // find left & right operand type
        bool left_is_scalar = !is_vector(left);
        bool right_is_scalar = !is_vector(right);
        // check both operands are scalars
        if (left_is_scalar && right_is_scalar) {
            // apply the function and return the result
            return fn(scalar_value<std::complex<double>>(left), scalar_value<std::complex<double>>(right));
        }
        // check both are vectors
        if (!left_is_scalar && !right_is_scalar) {
            // vectors
            auto left_vector = to_complex_vector(left);
            auto right_vector = to_complex_vector(right);
            // check sizes match
            if (left_vector->size() != right_vector->size())
                throw std::invalid_argument("Vector sizes do not match for binary operation");
            // result
            std::vector<std::complex<double>> out;
            // reserve space for the result
            out.reserve(left_vector->size());
            // loop vectors
            for (size_t i = 0; i < left_vector->size(); ++i)
                out.push_back(fn((*left_vector)[i], (*right_vector)[i]));
            // exit
            return std::make_shared<View<std::complex<double>>>(out);
        }
        // check left is scalar and right is vector
        if (left_is_scalar) {
            // operands
            auto left_scalar = scalar_value<std::complex<double>>(left);
            auto right_vector = to_complex_vector(right);
            // result
            std::vector<std::complex<double>> out;
            // reserve space for the result
            out.reserve(right_vector->size());
            // loop vector
            for (size_t i = 0; i < right_vector->size(); ++i)
                out.push_back(fn(left_scalar, (*right_vector)[i]));
            // exit
            return std::make_shared<View<std::complex<double>>>(out);
        }
        // operands
        auto left_vector = to_complex_vector(left);
        auto right_scalar = scalar_value<std::complex<double>>(right);
        // result
        std::vector<std::complex<double>> out;
        // reserve space for the result
        out.reserve(left_vector->size());
        // loop vector
        for (size_t i = 0; i < left_vector->size(); ++i)
            out.push_back(fn((*left_vector)[i], right_scalar));
        // exit
        return std::make_shared<View<std::complex<double>>>(out);
    }

    XyceValue broadcast_unary_real(const XyceValue& value, const std::function<double(double)>& fn) {
        // check if the value is a scalar and apply the function directly
        if (is_scalar(value)) {
            // apply the function and return the result
            return fn(scalar_value<double>(value));
        }
        // it is a vector
        auto operand = to_real_vector(value);
        // result
        std::vector<double> out;
        // reserve space for the result
        out.reserve(operand->size());
        // loop vectors
        for (size_t i = 0; i < operand->size(); ++i)
            out.push_back(fn(operand->operator[](i)));
        // exit
        return std::make_shared<View<double>>(out);
    }

    XyceValue broadcast_unary_complex(const XyceValue& value, const std::function<std::complex<double>(std::complex<double>)>& fn) {
        // check if the value is a scalar and apply the function directly
        if (!is_vector(value)) {
            // apply the function and return the result
            return fn(scalar_value<std::complex<double>>(value));
        }
        // it is a vector
        auto operand = to_complex_vector(value);
        // result
        std::vector<std::complex<double>> out;
        // reserve space for the result
        out.reserve(operand->size());
        // loop vectors
        for (size_t i = 0; i < operand->size(); ++i)
            out.push_back(fn(operand->operator[](i)));
        // exit
        return std::make_shared<View<std::complex<double>>>(out);
    }

    XyceValue evaluate(const ExpressionNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack);

    XyceValue evaluate_unary(const UnaryOperationNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
        // evaluate the operand first
        auto value = evaluate(*expression.operand, context, call_stack);
        // apply unary plus
        if (expression.operator_value == UnaryOperator::POS) {
            return value;
        }
        // apply unary minus
        if (expression.operator_value == UnaryOperator::NEG) {
            // processor
            auto l = []<typename T0>(T0&& arg) {
                // actual parameter type
                using TX = std::decay_t<T0>;
                // double
                if constexpr (std::is_same_v<TX, double>) {
                    // apply -
                    return XyceValue{-arg};
                }
                // complex
                if constexpr (std::is_same_v<TX, std::complex<double>>) {
                    // apply -
                    return XyceValue{-arg};
                }
                // View<double>
                if constexpr (std::is_same_v<TX, std::shared_ptr<View<double>>>) {
                    // vector
                    std::vector<double> out;
                    // allocate space
                    out.reserve(arg->size());
                    // append mapped values
                    std::ranges::transform(arg->begin(), arg->end(), std::back_inserter(out), [](auto& v) { return -v; });
                    // exit
                    return XyceValue{std::make_shared<View<double>>(out)};
                }
                // View<complex>
                if constexpr (std::is_same_v<TX, std::shared_ptr<View<std::complex<double>>>>) {
                    // vector
                    std::vector<std::complex<double>> out;
                    // allocate space
                    out.reserve(arg->size());
                    // append mapped values
                    std::ranges::transform(arg->begin(), arg->end(), std::back_inserter(out), [](auto& v) { return -v; });
                    // exit
                    return XyceValue{std::make_shared<View<std::complex<double>>>(out)};
                }
                // not possible value type
                throw std::invalid_argument("unsupported type");
            };
            return std::visit(l, value);
        }
        // apply logical not
        if (expression.operator_value == UnaryOperator::NOT) {
            // processor
            auto l = []<typename T0>(T0&& arg) {
                // actual parameter type
                using TX = std::decay_t<T0>;
                // double
                if constexpr (std::is_same_v<TX, double>) {
                    // apply not
                    return XyceValue{arg != 0.0 ? 0.0 : 1.0};
                }
                // complex
                if constexpr (std::is_same_v<TX, std::complex<double>>) {
                    // apply not
                    return XyceValue{arg.real() != 0.0 ? 0.0 : 1.0};
                }
                // View<double>
                if constexpr (std::is_same_v<TX, std::shared_ptr<View<double>>>) {
                    // vector
                    std::vector<double> out;
                    // allocate space
                    out.reserve(arg->size());
                    // append mapped values
                    std::ranges::transform(arg->begin(), arg->end(), std::back_inserter(out), [](auto& v) { return v != 0.0 ? 0.0 : 1.0; });
                    // exit
                    return XyceValue{std::make_shared<View<double>>(out)};
                }
                // View<complex>
                if constexpr (std::is_same_v<TX, std::shared_ptr<View<std::complex<double>>>>) {
                    // vector
                    std::vector<double> out;
                    // allocate space
                    out.reserve(arg->size());
                    // append mapped values
                    std::ranges::transform(arg->begin(), arg->end(), std::back_inserter(out), [](auto& v) { return v.real() != 0.0 ? 0.0 : 1.0; });
                    // exit
                    return XyceValue{std::make_shared<View<double>>(out)};
                }
                // not possible value type
                throw std::invalid_argument("unsupported type");
            };
            return std::visit(l, value);
        }
        throw std::invalid_argument("Unsupported unary operator");
    }

    XyceValue evaluate_binary(const BinaryOperationNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
        // short-circuit logical and
        if (expression.operator_value == BinaryOperator::LOGICAL_AND) {
            // eveluate left, extract type
            const auto left_value = evaluate(*expression.left, context, call_stack);
            const auto left_is_scalar = is_scalar(left_value);
            // shortcut if left is scalar and false
            if (left_is_scalar && scalar_value<double>(left_value) == 0.0)
                return 0.0;
            // evaluate right, extract type
            const auto right_value = evaluate(*expression.right, context, call_stack);
            const auto right_is_scalar = is_scalar(right_value);
            // shortcut if right is scalar and false
            if (right_is_scalar && scalar_value<double>(right_value) == 0.0)
                return 0.0;
            // easiest case: both are scalars, return 1.0 since both are non-zero
            if (left_is_scalar && right_is_scalar)
                return 1.0;
            // at least one value is a vector
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return (lhs != 0.0 && rhs != 0.0) ? 1.0 : 0.0; });
        }
        // short-circuit logical or
        if (expression.operator_value == BinaryOperator::LOGICAL_OR) {
            // eveluate left, extract type
            const auto left_value = evaluate(*expression.left, context, call_stack);
            const auto left_is_scalar = is_scalar(left_value);
            // shortcut if left is scalar and true
            if (left_is_scalar && scalar_value<double>(left_value) != 0.0)
                return 1.0;
            // evaluate right, extract type
            const auto right_value = evaluate(*expression.right, context, call_stack);
            const auto right_is_scalar = is_scalar(right_value);
            // shortcut if right is scalar and true
            if (right_is_scalar && scalar_value<double>(right_value) != 0.0)
                return 1.0;
            // easiest case: both are scalars, return 0.0 since both are zero
            if (left_is_scalar && right_is_scalar)
                return 0.0;
            // at least one value is a vector
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return (lhs != 0.0 || rhs != 0.0) ? 1.0 : 0.0; });
        }
        // evaluate both operands for the remaining operators
        const auto left_value = evaluate(*expression.left, context, call_stack);
        const auto right_value = evaluate(*expression.right, context, call_stack);
        // +
        if (expression.operator_value == BinaryOperator::ADD) {
            // check at least one operand is complex or a complex vector
            if (is_complex(left_value) || is_complex(right_value))
                return broadcast_binary_complex(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs + rhs; });
            // it is safe to assume both operands are real-valued
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs + rhs; });
        }
        // -
        if (expression.operator_value == BinaryOperator::SUB) {
            // check at least one operand is complex or a complex vector
            if (is_complex(left_value) || is_complex(right_value))
                return broadcast_binary_complex(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs - rhs; });
            // it is safe to assume both operands are real-valued
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs - rhs; });
        }
        // *
        if (expression.operator_value == BinaryOperator::MUL) {
            // check at least one operand is complex or a complex vector
            if (is_complex(left_value) || is_complex(right_value))
                return broadcast_binary_complex(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs * rhs; });
            // it is safe to assume both operands are real-valued
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs * rhs; });
        }
        // /
        if (expression.operator_value == BinaryOperator::DIV) {
            // check at least one operand is complex or a complex vector
            if (is_complex(left_value) || is_complex(right_value))
                return broadcast_binary_complex(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs / rhs; });
            // it is safe to assume both operands are real-valued
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs / rhs; });
        }
        // pow(x, y)
        if (expression.operator_value == BinaryOperator::POW) {
            // check at least one operand is complex or a complex vector
            if (is_complex(left_value) || is_complex(right_value))
                return broadcast_binary_complex(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return std::pow(lhs, rhs); });
            // it is safe to assume both operands are real-valued
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return std::pow(lhs, rhs); });
        }
        // x % y
        if (expression.operator_value == BinaryOperator::MOD) {
            // perform operation on real values only
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs - std::floor(lhs / rhs) * rhs; });
        }
        // x == y
        if (expression.operator_value == BinaryOperator::EQUAL) {
            // check at least one operand is complex or a complex vector
            if (is_complex(left_value) || is_complex(right_value))
                return broadcast_binary_complex(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs == rhs; });
            // it is safe to assume both operands are real-valued
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs == rhs; });
        }
        // x != y
        if (expression.operator_value == BinaryOperator::NOT_EQUAL) {
            // check at least one operand is complex or a complex vector
            if (is_complex(left_value) || is_complex(right_value))
                return broadcast_binary_complex(left_value, right_value, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs != rhs; });
            // it is safe to assume both operands are real-valued
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs != rhs; });
        }
        // <
        if (expression.operator_value == BinaryOperator::LESS) {
            // check log level
            if (spdlog::get_level() <= spdlog::level::warn) {
                // check at least one operand is complex or a complex vector
                if (is_complex(left_value) || is_complex(right_value))
                    spdlog::warn("Evaluating binary operator '<' with complex operands");
            }
            // xyce evaluates `<, <=, >, >=` using the real part of complex numbers
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs < rhs; });
        }
        // <=
        if (expression.operator_value == BinaryOperator::LESS_EQUAL) {
            // check log level
            if (spdlog::get_level() <= spdlog::level::warn) {
                // check at least one operand is complex or a complex vector
                if (is_complex(left_value) || is_complex(right_value))
                    spdlog::warn("Evaluating binary operator '<=' with complex operands");
            }
            // xyce evaluates `<, <=, >, >=` using the real part of complex numbers
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs <= rhs; });
        }
        // >
        if (expression.operator_value == BinaryOperator::GREATER) {
            // check log level
            if (spdlog::get_level() <= spdlog::level::warn) {
                // check at least one operand is complex or a complex vector
                if (is_complex(left_value) || is_complex(right_value))
                    spdlog::warn("Evaluating binary operator '>' with complex operands");
            }
            // xyce evaluates `<, <=, >, >=` using the real part of complex numbers
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs > rhs; });
        }
        // >=
        if (expression.operator_value == BinaryOperator::GREATER_EQUAL) {
            // check log level
            if (spdlog::get_level() <= spdlog::level::warn) {
                // check at least one operand is complex or a complex vector
                if (is_complex(left_value) || is_complex(right_value))
                    spdlog::warn("Evaluating binary operator '>=' with complex operands");
            }
            // xyce evaluates `<, <=, >, >=` using the real part of complex numbers
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return lhs >= rhs; });
        }
        // x ^ y
        if (expression.operator_value == BinaryOperator::LOGICAL_XOR) {
            // apply on the real part of complex numbers, and return 1.0 for true and 0.0 for false
            return broadcast_binary_real(left_value, right_value, [](double lhs, double rhs) { return ((lhs != 0.0) ^ (rhs != 0.0)) ? 1.0 : 0.0; });
        }
        // unsupported operator
        throw std::invalid_argument("Unsupported binary operator");
    }

    XyceValue evaluate_ternary(const TernaryOperationNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
        // evaluate condition
        const auto condition = evaluate(*expression.condition, context, call_stack);
        // branch for scalar condition (short-circuit non-taken branch)
        if (is_scalar(condition)) {
            return scalar_value<double>(condition) != 0.0 ? evaluate(*expression.if_true, context, call_stack) : evaluate(*expression.if_false, context, call_stack);
        }
        // evaluate both branches for vector condition
        const auto if_true = evaluate(*expression.if_true, context, call_stack);
        const auto if_false = evaluate(*expression.if_false, context, call_stack);
        // elementwise conditional selection using builtin if
        return BUILTIN_FUNCTIONS.at("if")({condition, if_true, if_false});
    }

    std::string node_name_from_expr(const ExpressionNode& expression) {
        if (const auto* node = dynamic_cast<const IdentifierNode*>(&expression); node != nullptr) {
            return node->name;
        }
        if (const auto* node = dynamic_cast<const NumberNode*>(&expression); node != nullptr) {
            return node->text;
        }
        throw std::invalid_argument("Cannot extract node name from complex expression");
    }

    std::string reconstruct_probe_name(const FunctionCallNode& expression) {
        std::string result = expression.name;
        result += ")";
        // Note: original reconstruct_probe_name implementation reconstructed from args, keeping exact style
        result = expression.name;
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

    std::string extract_node_name(const ExpressionNode& expression) { return node_name_from_expr(expression); }

    XyceValue evaluate_probe(const FunctionCallNode& expression, EvaluationContext& context) {
        const auto probe_name = reconstruct_probe_name(expression);
        if (const auto it = context.variables.find(to_lower(probe_name)); it != context.variables.end()) {
            return it->second;
        }
        if (const auto it = context.expressions.find(to_lower(probe_name)); it != context.expressions.end()) {
            return *it->second;
        }
        if (expression.args.size() == 2) {
            const auto node_a_name = extract_node_name(*expression.args[0]);
            const auto node_b_name = extract_node_name(*expression.args[1]);
            if (to_lower(node_b_name) == "0") {
                const auto probe_a_key = to_lower("V(" + node_a_name + ")");
                if (const auto it = context.variables.find(probe_a_key); it != context.variables.end()) {
                    return it->second;
                }
                if (const auto jt = context.expressions.find(probe_a_key); jt != context.expressions.end()) {
                    return *jt->second;
                }
            }
            if (to_lower(node_a_name) == "0") {
                const auto probe_b_key = to_lower("V(" + node_b_name + ")");
                const XyceValue* val = nullptr;
                if (const auto it = context.variables.find(probe_b_key); it != context.variables.end()) {
                    val = &it->second;
                }
                else if (const auto jt = context.expressions.find(probe_b_key); jt != context.expressions.end()) {
                    val = jt->second;
                }
                if (val != nullptr) {
                    if (is_complex(*val))
                        return broadcast_unary_complex(*val, [](std::complex<double> value) { return -value; });
                    return broadcast_unary_real(*val, [](double value) { return -value; });
                }
            }
            const auto probe_a_key = to_lower("V(" + node_a_name + ")");
            const auto probe_b_key = to_lower("V(" + node_b_name + ")");
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
                    if (is_complex(*value_a) || is_complex(*value_b))
                        return broadcast_binary_complex(*value_a, *value_b, [](std::complex<double> lhs, std::complex<double> rhs) { return lhs - rhs; });
                    return broadcast_binary_real(*value_a, *value_b, [](double lhs, double rhs) { return lhs - rhs; });
                }
                return *value_a;
            }
        }
        throw std::invalid_argument("Unknown probe: " + probe_name);
    }

    bool has_simple_probe_args(const FunctionCallNode& expression) {
        for (const auto& arg : expression.args) {
            if (!dynamic_cast<const IdentifierNode*>(arg.get()) && !dynamic_cast<const NumberNode*>(arg.get())) {
                return false;
            }
        }
        return true;
    }

    bool is_probe_call(const FunctionCallNode& expression, const EvaluationContext& context) {
        const auto name = to_lower(expression.name);
        // canonical SPICE probe families: v, i, id
        if ((name == "v" || name == "i" || name == "id") && !expression.args.empty())
            return true;
        // network parameter probes: Sxy, Zxy, Yxy, Hxy — only when the key exists in context
        if (is_network_parameter_probe_name(name) && !expression.args.empty()) {
            // network parameter probes must have simple arguments to be unambiguous
            if (has_simple_probe_args(expression)) {
                // reconstruct the probe name and check if it exists in the context
                const auto key = to_lower(reconstruct_probe_name(expression));
                // check if the probe name exists in the context variables or expressions
                return context.variables.contains(key) || context.expressions.contains(key);
            }
        }
        return false;
    }

    XyceValue evaluate_function_call(const FunctionCallNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
        // probe calls are handled specially
        if (is_probe_call(expression, context)) {
            return evaluate_probe(expression, context);
        }
        // resolve a builtin function
        const auto builtin_key = to_lower(expression.name);
        if (const auto it = BUILTIN_FUNCTIONS.find(builtin_key); it != BUILTIN_FUNCTIONS.end()) {
            std::vector<XyceValue> args;
            args.reserve(expression.args.size());
            for (const auto& arg : expression.args) {
                args.push_back(evaluate(*arg, context, call_stack));
            }
            return it->second(args);
        }
        // resolve a user function
        const auto function_key = to_lower(expression.name);
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
            parameter_bindings.bind(to_lower(definition.params[index]), evaluate(*expression.args[index], context, call_stack));
        }
        auto next_stack = call_stack;
        next_stack.push_back(function_key);
        return evaluate(*definition.body, context, next_stack);
    }

    XyceValue evaluate_step_selector(const StepSelectorNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
        // steps from context
        if (expression.step_index < 1 || expression.step_index > context.step_slices.size())
            throw std::invalid_argument("Step selector @" + std::to_string(expression.step_index) + " is out of range: file has " + std::to_string(context.step_slices.size()) + " step(s)");
        // evaluate the base expression
        const auto base_value = evaluate(*expression.base, context, call_stack);
        // slice @ steo index
        const auto [begin, end] = context.step_slices.at(expression.step_index - 1);
        // processor
        auto l = [&begin, &end]<typename T0>(T0& arg) -> XyceValue {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // View<double>
            if constexpr (std::is_same_v<TX, std::shared_ptr<View<double>>>) {
                // return slice on original argument to avoid copying data
                return std::make_shared<View<double>>(arg->data() + static_cast<std::ptrdiff_t>(begin), end - begin, 1, arg);
            }
            // view<complex>
            else if constexpr (std::is_same_v<TX, std::shared_ptr<View<std::complex<double>>>>) {
                // return slice on original argument to avoid copying data
                return std::make_shared<View<std::complex<double>>>(arg->data() + static_cast<std::ptrdiff_t>(begin), end - begin, 1, arg);
            }
            // return argument when scalar
            return {arg};
        };
        return std::visit(l, base_value);
    }

    XyceValue lookup_name(const std::string& name, EvaluationContext& context) {
        // lowercase the name for case-insensitive lookup
        const auto key = to_lower(name);
        // find in context variables
        if (const auto it = context.variables.find(key); it != context.variables.end())
            return it->second;
        // find in context expressions
        if (const auto it = context.expressions.find(key); it != context.expressions.end())
            return *it->second;
        // find in context constants
        if (const auto it = context.constants.find(key); it != context.constants.end())
            return *it->second;
        // not found
        throw std::invalid_argument("Unknown identifier: " + name);
    }

    XyceValue evaluate(const ExpressionNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack) {
        // literal number
        if (const auto* node = dynamic_cast<const NumberNode*>(&expression); node != nullptr)
            return std::stod(node->text);
        // literal string
        if (const auto* node = dynamic_cast<const IdentifierNode*>(&expression); node != nullptr)
            return lookup_name(node->name, context);
        // unary expression
        if (const auto* node = dynamic_cast<const UnaryOperationNode*>(&expression); node != nullptr)
            return evaluate_unary(*node, context, call_stack);
        // binary expression
        if (const auto* node = dynamic_cast<const BinaryOperationNode*>(&expression); node != nullptr)
            return evaluate_binary(*node, context, call_stack);
        // ternary expression
        if (const auto* node = dynamic_cast<const TernaryOperationNode*>(&expression); node != nullptr)
            return evaluate_ternary(*node, context, call_stack);
        // function call expression
        if (const auto* node = dynamic_cast<const FunctionCallNode*>(&expression); node != nullptr)
            return evaluate_function_call(*node, context, call_stack);
        // step selector expression
        if (const auto* node = dynamic_cast<const StepSelectorNode*>(&expression); node != nullptr)
            return evaluate_step_selector(*node, context, call_stack);
        // unsupported expression node
        throw std::invalid_argument("Unsupported expression node");
    }
} // namespace

XyceValue evaluate_expression(const ExpressionNode& expression, const std::unordered_map<std::string, XyceValue>& expressions, const std::unordered_map<std::string, FunctionDefinitionNode>& functions, const std::unordered_map<std::string, XyceValue>& constants, const std::vector<std::pair<size_t, size_t>>& step_slices) {
    // build the evaluation context
    EvaluationContext context;
    // normalize expression keys
    for (const auto& [name, value] : expressions)
        context.expressions[to_lower(name)] = &value;
    // normalize function keys
    for (const auto& [name, value] : functions)
        context.functions.insert_or_assign(to_lower(name), &value);
    // normalize constant keys and seed the builtin constants
    for (const auto& [name, value] : BUILTIN_CONSTANTS)
        context.constants[to_lower(name)] = &value;
    for (const auto& [name, value] : constants)
        context.constants[to_lower(name)] = &value;
    // store step slices
    context.step_slices = step_slices;
    // evaluate the tree
    return evaluate(expression, context, {});
}
