#include <algorithm>
#include <deque>
#include <iterator>
#include <optional>
#include <vector>

#include "expression_manager.h"

ExpressionManager::ExpressionManager(std::vector<AnyExpression>& expressions, std::vector<std::pair<size_t, size_t>>& step_slices) :
    m_expressions(std::make_move_iterator(expressions.begin()), std::make_move_iterator(expressions.end())), m_step_slices(std::move(step_slices)) {
    // pre-process expressions
    for (size_t idx = 0; idx < m_expressions.size(); ++idx)
        std::visit([this, &idx](auto&& expression) { this->m_context[casefold(expression.name())] = idx; }, m_expressions[idx]);
}

Expression<double>& ExpressionManager::abscissa() { return std::get<Expression<double>>(m_expressions[0]); }

std::vector<AnyExpression*> ExpressionManager::expressions() {
    // result
    std::vector<AnyExpression*> result;
    // reserve memory
    result.reserve(m_expressions.size());
    // transform deque to vector of pointers
    std::ranges::transform(m_expressions.begin(), m_expressions.end(), std::back_inserter(result), [](auto& expression) { return &expression; });
    // exit
    return result;
}

std::vector<std::string> ExpressionManager::expression_names() const {
    // create list
    std::vector<std::string> names;
    // reserve memory
    names.reserve(m_expressions.size());
    // loop expressions, collect names
    for (const auto& expression : m_expressions)
        names.push_back(std::visit([](auto&& exp) { return exp.name(); }, expression));
    // return list
    return names;
}

const std::vector<std::pair<size_t, size_t>>& ExpressionManager::step_slices() const { return m_step_slices; }

AnyExpression* ExpressionManager::evaluate(const std::string& expression, const std::optional<std::string>& name) {
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
