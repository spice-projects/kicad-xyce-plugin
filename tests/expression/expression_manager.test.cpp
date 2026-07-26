#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "expression/expression.h"
#include "expression/expression_manager.h"

namespace
{
    Expression<double> make_real_expression(const std::string& name, const std::vector<double>& values, const std::string& unit = "V") {
        std::vector<double> data(values);
        std::vector<std::span<const double>> steps = {{data.data(), data.size()}};
        return {name, data, steps, unit};
    }
} // namespace

// ========================================================================================
// type traits
// ========================================================================================

static_assert(!std::is_copy_constructible_v<ExpressionManager>);
static_assert(!std::is_copy_assignable_v<ExpressionManager>);

// ========================================================================================
// constructor / accessors
// ========================================================================================

TEST(ExpressionManagerChecks, default_constructor_initializes_empty_state) {
    // arrange
    ExpressionManager manager;
    // act
    const auto result = manager.evaluate("v(out)");
    // assert
    ASSERT_TRUE(manager.expressions().empty());
    ASSERT_TRUE(manager.expression_names().empty());
    ASSERT_TRUE(manager.step_slices().empty());
    ASSERT_EQ(result, nullptr);
}

TEST(ExpressionManagerChecks, constructor_stores_expressions_and_step_slices) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("time", {0.0, 1.0, 2.0}, "s"));
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0, 3.0}, "V"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}, {3, 6}};
    // act
    ExpressionManager manager(expressions, slices);
    // assert
    ASSERT_EQ(manager.expressions().size(), 2);
    ASSERT_EQ(std::get<Expression<double>>(*manager.expressions()[0]).name(), "time");
    ASSERT_EQ(std::get<Expression<double>>(*manager.expressions()[1]).name(), "V(out)");
}

TEST(ExpressionManagerChecks, expression_names_returns_names_in_insertion_order) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("time", {0.0, 1.0}, "s"));
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V"));
    expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}};
    // act
    ExpressionManager manager(expressions, slices);
    const auto names = manager.expression_names();
    // assert
    ASSERT_EQ(names, std::vector<std::string>({"time", "V(out)", "I(R1)"}));
}

// ========================================================================================
// evaluate
// ========================================================================================

TEST(ExpressionManagerChecks, evaluate_finds_expression_by_expression_name_case_insensitively) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(Out)", {1.0, 2.0}, "V"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}};
    ExpressionManager manager(expressions, slices);
    // act
    const auto result = manager.evaluate("v(out)");
    // assert
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(std::get<Expression<double>>(*result).name(), "V(Out)");
    ASSERT_EQ(std::get<Expression<double>>(*result).unit(), "V");
}

TEST(ExpressionManagerChecks, evaluate_uses_name_override_for_lookup_when_provided) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}};
    ExpressionManager manager(expressions, slices);
    // act
    const auto result = manager.evaluate("not used", "i(r1)");
    // assert
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(std::get<Expression<double>>(*result).name(), "I(R1)");
    ASSERT_EQ(std::get<Expression<double>>(*result).unit(), "A");
}

TEST(ExpressionManagerChecks, evaluate_returns_nullopt_when_lookup_key_is_missing) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(in)", {1.0, 2.0}, "V"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}};
    ExpressionManager manager(expressions, slices);
    // act
    const auto result = manager.evaluate("V(out)");
    // assert
    ASSERT_EQ(result, nullptr);
}

TEST(ExpressionManagerChecks, evaluate_with_name_override_returns_nullopt_when_override_key_missing) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}};
    ExpressionManager manager(expressions, slices);
    // act
    const auto result = manager.evaluate("V(out)", "missing");
    // assert
    ASSERT_EQ(result, nullptr);
}

TEST(ExpressionManagerChecks, evaluate_returns_last_expression_when_casefolded_keys_collide) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(OUT)", {1.0, 1.0}, "V"));
    expressions.emplace_back(make_real_expression("v(out)", {2.0, 2.0}, "A"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}};
    ExpressionManager manager(expressions, slices);
    // act
    const auto result = manager.evaluate("V(out)");
    // assert
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(std::get<Expression<double>>(*result).name(), "v(out)");
    ASSERT_EQ(std::get<Expression<double>>(*result).unit(), "A");
}
