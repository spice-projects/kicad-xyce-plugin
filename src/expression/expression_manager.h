#pragma once

#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "expression.h"
#include "xyce_parser.h"
#include "xyce_value.h"

class ExpressionManager
{
public:
    ExpressionManager() = default;

    ExpressionManager(const ExpressionManager&) = delete;

    ExpressionManager(ExpressionManager&&) noexcept = default;

    ExpressionManager(std::vector<AnyExpression>& expressions, std::vector<std::pair<size_t, size_t>>& step_slices);

    ~ExpressionManager() = default;

    ExpressionManager& operator=(const ExpressionManager&) = delete;

    ExpressionManager& operator=(ExpressionManager&&) noexcept = default;

    [[nodiscard]] Expression<double>& abscissa();

    [[nodiscard]] std::vector<AnyExpression*> expressions();

    [[nodiscard]] std::vector<std::string> expression_names() const;

    [[nodiscard]] const std::vector<std::pair<size_t, size_t>>& step_slices() const;

    [[nodiscard]] AnyExpression* evaluate(const std::string& expression, const std::optional<std::string>& name = std::nullopt);

    [[nodiscard]] std::string infer_unit(const std::string& expression);

private:
    std::deque<AnyExpression> m_expressions;
    std::vector<std::pair<size_t, size_t>> m_step_slices;
    std::unordered_map<std::string, size_t> m_context;
    std::unordered_map<std::string, XyceValue> m_expression_data;
    XyceParser m_parser;

    AnyExpression* build_expression(XyceValue& value, const std::string& name, const std::string& unit);

    XyceValue rematerialize(XyceValue& value, const ExpressionNode& ast);

    template <typename T>
    void fill_step_spans(const std::vector<T>& data, std::vector<std::span<const T>>& spans) {
        // check there are multiple steps
        if (m_step_slices.size() > 1) {
            // reserve the number of spans
            spans.reserve(m_step_slices.size());
            // pointer
            auto ptr = data.data();
            // create a span for each step slice
            for (const auto& [start, end] : m_step_slices)
                spans.emplace_back(ptr + start, end - start);
        }
        else {
            // single step, create a span for the entire data vector
            spans.emplace_back(data);
        }
    }
};
