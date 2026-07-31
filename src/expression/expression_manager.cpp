#include <algorithm>
#include <deque>
#include <iterator>
#include <memory>
#include <optional>
#include <vector>

#include "expression_manager.h"
#include "unit_utils.h"
#include "util.h"

ExpressionManager::ExpressionManager(std::vector<AnyExpression>& expressions, std::vector<std::pair<size_t, size_t>>& step_slices) :
    m_expressions(std::make_move_iterator(expressions.begin()), std::make_move_iterator(expressions.end())), m_step_slices(std::move(step_slices)) {
    // index each expression by its lowercased name for fast lookup
    for (size_t idx = 0; idx < m_expressions.size(); ++idx)
        std::visit([this, &idx](auto&& expression) { this->m_context[to_lower(expression.name())] = idx; }, m_expressions[idx]);
}

Expression<double>& ExpressionManager::abscissa() { return std::get<Expression<double>>(m_expressions[0]); }

std::vector<AnyExpression*> ExpressionManager::expressions() {
    // result
    std::vector<AnyExpression*> result;
    // reserve space for all expressions
    result.reserve(m_expressions.size());
    // transform each expression into a pointer to the expression
    std::ranges::transform(m_expressions.begin(), m_expressions.end(), std::back_inserter(result), [](auto& expression) { return &expression; });
    // exit
    return result;
}

std::vector<std::string> ExpressionManager::expression_names() const {
    // result
    std::vector<std::string> names;
    // reserve space for all expression names
    names.reserve(m_expressions.size());
    // transform each expression into its name
    for (const auto& expression : m_expressions)
        names.push_back(std::visit([](auto&& exp) { return exp.name(); }, expression));
    // exit
    return names;
}

const std::vector<std::pair<size_t, size_t>>& ExpressionManager::step_slices() const { return m_step_slices; }

AnyExpression* ExpressionManager::evaluate(const std::string& expression, const std::optional<std::string>& name) {
    try {
        // determine the lookup key
        const std::string key = to_lower(name.value_or(expression));
        // return the existing expression if already evaluated
        if (const auto it = m_context.find(key); it != m_context.end())
            return &m_expressions.at(it->second);
        // parse the expression string into an AST
        auto ast = m_parser.parse_expression(expression);
        // expressions data
        std::unordered_map<std::string, XyceValue> expression_data;
        // reserve space for all expressions
        expression_data.reserve(m_expressions.size());
        // loop expressions
        for (auto& expression : m_expressions) {
            // create value
            auto value = from_expression(expression);
            // append to map
            expression_data[to_lower(std::visit([](auto&& expr) { return expr.name(); }, expression))] = std::move(value);
        }
        // attach step slice metadata when not empty
        std::optional<std::vector<std::pair<size_t, size_t>>> step_opt;
        if (!m_step_slices.empty())
            step_opt = m_step_slices;
        // evaluate the parsed expression
        auto evaluated = evaluate_expression(*ast, expression_data, {}, {}, step_opt);
        // rematerialize the result if step slices require tiling
        evaluated = rematerialize(evaluated, *ast);
        // unix context
        std::unordered_map<std::string, std::string> unit_context;
        // reserve space for all expressions
        unit_context.reserve(m_expressions.size());
        // loop expressions
        for (auto& expression : m_expressions) {
            // append to map
            std::visit([&unit_context](auto&& expr) { unit_context[to_lower(expr.name())] = expr.unit(); }, expression);
        }
        // infer the unit of the result expression
        auto inferred_unit = ::infer_unit(*ast, unit_context);
        // derive the result name
        auto result_name = name.value_or(format_expression(*ast));
        // create expression
        auto* result = build_expression(evaluated, result_name, inferred_unit);
        // update context
        m_context[key] = m_expressions.size() - 1;
        // exit
        return result;
    }
    catch (const std::exception&) {
        // return nullptr on any error
        return nullptr;
    }
}

std::string ExpressionManager::infer_unit(const std::string& expression) {
    // determine the lookup key
    const std::string key = to_lower(expression);

    // return unit from an already evaluated expression
    if (const auto it = m_context.find(key); it != m_context.end()) {
        const auto& any_expr = m_expressions.at(it->second);
        return std::visit([](auto&& expr) { return expr.unit(); }, any_expr);
    }

    try {
        // parse and infer unit from the expression AST
        auto ast = m_parser.parse_expression(expression);

        // build a unit-context map from all stored expressions
        std::unordered_map<std::string, std::string> unit_context;
        unit_context.reserve(m_expressions.size());
        for (const auto& any_expr : m_expressions) {
            std::visit([&](auto&& expr) { unit_context[to_lower(expr.name())] = expr.unit(); }, any_expr);
        }

        return ::infer_unit(*ast, unit_context);
    }
    catch (const std::exception&) {
        return "";
    }
}

AnyExpression* ExpressionManager::build_expression(XyceValue& value, const std::string& name, const std::string& unit) {
    // factory
    auto l = [this, &name, &unit]<typename T0>(T0&& arg) -> AnyExpression* {
        // actual parameter type
        using TX = std::decay_t<T0>;
        // scalar double
        if constexpr (std::is_same_v<TX, double>) {
            return nullptr;
        }
        // scalar complex
        else if constexpr (std::is_same_v<TX, std::complex<double>>) {
            return nullptr;
        }
        // View<double>
        else if constexpr (std::is_same_v<TX, std::shared_ptr<View<double>>>) {
            // create expression, append it to expressions
            m_expressions.emplace_back(Expression<double>{name, std::move(*arg), m_step_slices, unit});
            // return expression
            return &m_expressions.back();
        }
        // vector<complex>
        else if constexpr (std::is_same_v<TX, std::shared_ptr<View<std::complex<double>>>>) {
            // create expression, append it to expressions
            m_expressions.emplace_back(Expression<std::complex<double>>{name, std::move(*arg), m_step_slices, unit});
            // return expression
            return &m_expressions.back();
        }
        // not possible value type
        throw std::invalid_argument("unsupported type");
    };
    // convert value to expression
    return std::visit(l, value);

    // // dispatch on the variant type and construct the matching expression
    // return std::visit(
    //     [&](auto&& arg) -> AnyExpression* {
    //         using T = std::decay_t<decltype(arg)>;
    //         if constexpr (std::is_same_v<T, double>) {
    //             // scalar double
    //             std::vector<double> data = {std::move(arg)};
    //             std::vector<std::span<const double>> steps;
    //             fill_step_spans(data, steps);
    //             m_expressions.emplace_back(std::in_place_type<Expression<double>>, name, data, steps, unit, "expression manager");
    //         }
    //         else if constexpr (std::is_same_v<T, std::complex<double>>) {
    //             // scalar complex
    //             std::vector<std::complex<double>> data = {std::move(arg)};
    //             std::vector<std::span<const std::complex<double>>> steps;
    //             fill_step_spans(data, steps);
    //             m_expressions.emplace_back(std::in_place_type<Expression<std::complex<double>>>, name, data, steps, unit, "expression manager");
    //         }
    //         else if constexpr (std::is_same_v<T, std::vector<double>>) {
    //             // vector of doubles
    //             std::vector<std::span<const double>> steps;
    //             fill_step_spans(arg, steps);
    //             m_expressions.emplace_back(std::in_place_type<Expression<double>>, name, arg, steps, unit, "expression manager");
    //         }
    //         else {
    //             // vector of complex doubles
    //             std::vector<std::span<const std::complex<double>>> steps;
    //             fill_step_spans(arg, steps);
    //             m_expressions.emplace_back(std::in_place_type<Expression<std::complex<double>>>, name, arg, steps, unit, "expression manager");
    //         }
    //         return &m_expressions.back();
    //     },
    //     std::move(value));
}

XyceValue ExpressionManager::rematerialize(XyceValue& value, const ExpressionNode& ast) {
    // no steps in data
    if (m_step_slices.size() == 1)
        return value;
    // scalar values never need tiling
    if (is_scalar(value))
        return value;
    // no step selector in the AST means no tiling is needed
    if (!has_step_selector(ast))
        return value;
    // compute the total number of points across all steps
    size_t total_points = 0;
    for (const auto& [start, end] : m_step_slices)
        total_points += (end - start);
    // determine the current vector size
    size_t current_size = std::holds_alternative<std::shared_ptr<View<double>>>(value) ? std::get<std::shared_ptr<View<double>>>(value)->size() : std::get<std::shared_ptr<View<std::complex<double>>>>(value)->size();
    // already the right size
    if (current_size == total_points)
        return value;
    // the value must match a single step length to be tileable
    const auto step_length = m_step_slices[0].second - m_step_slices[0].first;
    if (current_size != step_length)
        return value;
    // tile the single-step data across all steps
    if (std::holds_alternative<std::shared_ptr<View<double>>>(value)) {
        // view
        auto& view = std::get<std::shared_ptr<View<double>>>(value);
        // data
        std::vector<double> tiled;
        // reserve space for all steps
        tiled.reserve(view->size() * m_step_slices.size());
        // loop steps, append the same vector for each step
        for (size_t i = 0; i < m_step_slices.size(); ++i)
            tiled.insert(tiled.end(), view->begin(), view->end());
        // exit
        return std::make_shared<View<double>>(tiled);
    }
    // view
    auto& view = std::get<std::shared_ptr<View<std::complex<double>>>>(value);
    // data
    std::vector<std::complex<double>> tiled;
    // reserve space for all steps
    tiled.reserve(view->size() * m_step_slices.size());
    // loop steps, append the same vector for each step
    for (size_t i = 0; i < m_step_slices.size(); ++i)
        tiled.insert(tiled.end(), view->begin(), view->end());
    // exit
    return std::make_shared<View<std::complex<double>>>(tiled);
}
