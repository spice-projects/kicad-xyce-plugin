#ifndef EXPRESSION_MANAGER_H
#define EXPRESSION_MANAGER_H

#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "expression.h"

class ExpressionManager
{
public:
    ExpressionManager() = default;

    ExpressionManager(const ExpressionManager&) = delete;

    ExpressionManager(ExpressionManager&&) noexcept = default;

    ExpressionManager(std::vector<Expression>& expressions, std::vector<std::pair<size_t, size_t>>& step_slices);

    ~ExpressionManager() = default;

    ExpressionManager& operator=(const ExpressionManager&) = delete;

    ExpressionManager& operator=(ExpressionManager&&) noexcept = default;

    [[nodiscard]] const std::deque<Expression>& expressions() const;

    [[nodiscard]] std::vector<std::string> expression_names() const;

    [[nodiscard]] const std::vector<std::pair<size_t, size_t>>& step_slices() const;

    [[nodiscard]] Expression* evaluate(const std::string& expression, const std::optional<std::string>& name = std::nullopt);

private:
    std::deque<Expression> m_expressions;
    std::vector<std::pair<size_t, size_t>> m_step_slices;
    std::unordered_map<std::string, size_t> m_context;

    static std::string casefold(std::string str);
};

#endif
