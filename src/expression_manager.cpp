#include <complex>
#include <vector>
#include <algorithm>
#include <optional>

#include "expression_manager.h"

// parameterized constructor
ExpressionManager::ExpressionManager(std::vector<Expression> expressions, std::optional<std::vector<std::pair<size_t, size_t>>> step_slices)
    : m_expressions(std::move(expressions)), m_step_slices(std::move(step_slices))
{
    for (const auto& expr : m_expressions) {
        // insert casefolded key
        m_context[casefold(expr.name())] = expr;
    }
}


// expressions getter
const std::vector<Expression>& ExpressionManager::expressions() const {
    // return list
    return m_expressions;
}


// expression names getter
std::vector<std::string> ExpressionManager::expression_names() const {
    // create list
    std::vector<std::string> names;
    // reserve memory
    names.reserve(m_expressions.size());
    for (const auto& expr : m_expressions) {
        // append name
        names.push_back(expr.name());
    }
    // return list
    return names;
}


// step slices getter
const std::optional<std::vector<std::pair<size_t, size_t>>>& ExpressionManager::step_slices() const {
    // return slices
    return m_step_slices;
}


// evaluate method
std::optional<Expression> ExpressionManager::evaluate(const std::string& expression, const std::optional<std::string>& name) const {
    // build search key
    std::string key = casefold(name.value_or(expression));
    // find key in map
    auto it = m_context.find(key);
    if (it != m_context.end()) {
        // return match
        return it->second;
    }
    // fallback
    return std::nullopt;
}


// casefold helper
std::string ExpressionManager::casefold(std::string str) {
    // transform to lowercase
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    // return result
    return str;
}

