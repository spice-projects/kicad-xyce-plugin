#ifndef EXPRESSION_MANAGER_H
#define EXPRESSION_MANAGER_H

#include <optional>
#include <vector>
#include <unordered_map>

#include "expression.h"

class ExpressionManager {
public:
    // default constructor
    ExpressionManager() = default;

    // parameterized constructor
    explicit ExpressionManager(std::vector<Expression> expressions, std::optional<std::vector<std::pair<size_t, size_t>>> step_slices = std::nullopt);

    // expressions getter
    [[nodiscard]] const std::vector<Expression>& expressions() const;

    // expression names getter
    [[nodiscard]] std::vector<std::string> expression_names() const;

    // step slices getter
    [[nodiscard]] const std::optional<std::vector<std::pair<size_t, size_t>>>& step_slices() const;

    // evaluate method
    [[nodiscard]] std::optional<Expression> evaluate(const std::string& expression, const std::optional<std::string>& name = std::nullopt) const;

private:
    // expressions list
    std::vector<Expression> m_expressions;
    // step slices metadata
    std::optional<std::vector<std::pair<size_t, size_t>>> m_step_slices;
    // expression context map
    std::unordered_map<std::string, Expression> m_context;

    // casefold helper
    static std::string casefold(std::string str);
};

#endif
