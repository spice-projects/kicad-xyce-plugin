#include <algorithm>
#include <complex>
#include <deque>
#include <iterator>
#include <optional>
#include <vector>

#include "expression_manager.h"

ExpressionManager::ExpressionManager(std::vector<Expression>& expressions, std::vector<std::pair<size_t, size_t>>& step_slices)
    : m_expressions(std::make_move_iterator(expressions.begin()), std::make_move_iterator(expressions.end())), m_step_slices(std::move(step_slices)) {
    // insert case folded key for expressions
    for (size_t idx = 0; idx < m_expressions.size(); ++idx)
        m_context[casefold(m_expressions[idx].name())] = idx;
}

const std::deque<Expression>& ExpressionManager::expressions() const {
    // return list
    return m_expressions;
}

std::vector<std::string> ExpressionManager::expression_names() const {
    // create list
    std::vector<std::string> names;
    // reserve memory
    names.reserve(m_expressions.size());
    // loop expressions, collect names
    for (const auto& expr : m_expressions)
        names.push_back(expr.name());
    // return list
    return names;
}

const std::vector<std::pair<size_t, size_t>>& ExpressionManager::step_slices() const {
    // return slices
    return m_step_slices;
}

Expression* ExpressionManager::evaluate(const std::string& expression, const std::optional<std::string>& name) {
    // build search key
    const std::string key = casefold(name.value_or(expression));
    // find key in map
    if (const auto it = m_context.find(key); it != m_context.end()) {
        // expression at index
        return &m_expressions.at(it->second);
    }
    return nullptr;
}

std::string ExpressionManager::casefold(std::string str) {
    // transform to lowercase
    std::ranges::transform(str, str.begin(), [](unsigned char c) { return std::tolower(c); });
    // return result
    return str;
}
