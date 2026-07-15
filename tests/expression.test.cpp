#include <complex>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "expression.h"

// helper to create a View from a const buffer, working around View storing a non-const pointer
template <typename T>
View<T> make_view(const std::vector<T>& buf, const size_t size, const size_t stride, const size_t offset = 0) {
    return View<T>(const_cast<T*>(buf.data()) + offset, size, stride);
}

// ========================================================================================
// type traits
// ========================================================================================

static_assert(!std::is_default_constructible_v<View<double>>);
static_assert(!std::is_copy_constructible_v<View<double>>);
static_assert(!std::is_copy_assignable_v<View<double>>);

static_assert(!std::is_default_constructible_v<Expression<double>>);
static_assert(!std::is_copy_constructible_v<Expression<double>>);
static_assert(!std::is_copy_assignable_v<Expression<double>>);

// ========================================================================================
// View
// ========================================================================================

TEST(ExpressionChecks, view_index_operator_respects_stride) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view = make_view(buffer, 3, 2);
    // act
    const std::vector<double> values = {view[0], view[1], view[2]};
    // assert
    ASSERT_EQ(values[0], 1);
    ASSERT_EQ(values[1], 2);
    ASSERT_EQ(values[2], 3);
}

TEST(ExpressionChecks, view_size_returns_constructor_size) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view = make_view(buffer, 3, 2);
    // act
    const auto size = view.size();
    // assert
    ASSERT_EQ(size, 3);
}

TEST(ExpressionChecks, view_vector_constructor_owns_data) {
    // arrange
    std::vector<double> owned = {4, 5, 6};
    // act
    const View<double> view(owned);
    // assert
    ASSERT_EQ(view.size(), 3);
    ASSERT_EQ(view[0], 4);
    ASSERT_EQ(view[1], 5);
    ASSERT_EQ(view[2], 6);
}

// ========================================================================================
// constructor / metadata
// ========================================================================================

TEST(ExpressionChecks, constructor_stores_metadata_for_span_constructor) {
    // arrange
    std::vector<double> data = {1, 2, 3};
    std::vector<std::span<const double>> steps = {{data.data(), data.size()}};
    // act
    const Expression<double> expr("net_voltage", data, steps, "V", "R1", "node");
    // assert
    ASSERT_EQ(expr.name(), "net_voltage");
    ASSERT_EQ(expr.unit(), "V");
    ASSERT_EQ(expr.source(), "R1");
    ASSERT_EQ(expr.variable_type(), "node");
}

TEST(ExpressionChecks, constructor_defaults_source_and_variable_type_to_empty) {
    // arrange
    std::vector<double> data = {1, 2, 3};
    std::vector<std::span<const double>> steps = {{data.data(), data.size()}};
    // act
    const Expression<double> expr("net_voltage", data, steps, "V");
    // assert
    ASSERT_TRUE(expr.source().empty());
    ASSERT_TRUE(expr.variable_type().empty());
}

// ========================================================================================
// step_count
// ========================================================================================

TEST(ExpressionChecks, step_count_returns_correct_count_for_view_steps) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    std::vector<View<double>> steps;
    steps.reserve(2);
    steps.emplace_back(make_view(buffer, 3, 2));
    steps.emplace_back(make_view(buffer, 3, 2, 1));
    const Expression<double> expr("V1", steps, "V");
    // act
    const auto step_count = expr.step_count();
    // assert
    ASSERT_EQ(step_count, 2);
}

TEST(ExpressionChecks, step_count_returns_correct_count_for_span_steps) {
    // arrange
    std::vector<std::complex<double>> data = {1, 2, 3, 4, 5, 6};
    std::vector<std::span<const std::complex<double>>> steps = {{data.data(), 2}, {data.data() + 2, 2}, {data.data() + 4, 2}};
    const Expression<std::complex<double>> expr("I1", data, steps, "A");
    // act
    const auto step_count = expr.step_count();
    // assert
    ASSERT_EQ(step_count, 3);
}

// ========================================================================================
// data / step_data / step_indices
// ========================================================================================

TEST(ExpressionChecks, data_and_step_data_initialize_and_return_values_for_view_steps) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    std::vector<View<double>> steps;
    steps.reserve(2);
    steps.emplace_back(make_view(buffer, 3, 2));
    steps.emplace_back(make_view(buffer, 3, 2, 1));
    Expression<double> expr("V1", steps, "V");
    // act
    const auto data = expr.data();
    const auto step = expr.step_data(1);
    const auto slices = expr.step_indices();
    // assert
    ASSERT_EQ(data.size(), 6);
    ASSERT_EQ(data[0], 1);
    ASSERT_EQ(data[1], 2);
    ASSERT_EQ(data[2], 3);
    ASSERT_EQ(data[3], 10);
    ASSERT_EQ(data[4], 20);
    ASSERT_EQ(data[5], 30);
    ASSERT_EQ(step.size(), 3);
    ASSERT_EQ(step[0], 10);
    ASSERT_EQ(step[1], 20);
    ASSERT_EQ(step[2], 30);
    ASSERT_EQ(slices.size(), 2);
    ASSERT_EQ(slices[0], (std::pair<size_t, size_t>{0, 3}));
    ASSERT_EQ(slices[1], (std::pair<size_t, size_t>{3, 6}));
}

TEST(ExpressionChecks, data_and_step_data_return_values_for_span_steps) {
    // arrange
    std::vector<std::complex<double>> data_buffer = {{1, 1}, {2, 2}, {3, 3}};
    std::vector<std::span<const std::complex<double>>> steps = {{data_buffer.data(), 1}, {data_buffer.data() + 1, 2}};
    Expression<std::complex<double>> expr("I1", data_buffer, steps, "A");
    // act
    const auto data = expr.data();
    const auto step = expr.step_data(1);
    const auto slices = expr.step_indices();
    // assert
    ASSERT_EQ(data.size(), 3);
    ASSERT_EQ(data[0], std::complex<double>(1, 1));
    ASSERT_EQ(data[2], std::complex<double>(3, 3));
    ASSERT_EQ(step.size(), 2);
    ASSERT_EQ(step[0], std::complex<double>(2, 2));
    ASSERT_EQ(step[1], std::complex<double>(3, 3));
    ASSERT_EQ(slices.size(), 2);
    ASSERT_EQ(slices[0], (std::pair<size_t, size_t>{0, 1}));
    ASSERT_EQ(slices[1], (std::pair<size_t, size_t>{1, 3}));
}

TEST(ExpressionChecks, step_data_throws_out_of_range_for_invalid_index) {
    // arrange
    std::vector<double> data_buffer = {1, 2};
    std::vector<std::span<const double>> steps = {{data_buffer.data(), 2}};
    Expression<double> expr("V1", data_buffer, steps, "V");
    // act
    const auto read_step = [&expr]() { (void)expr.step_data(1); };
    // assert
    ASSERT_THROW(read_step(), std::out_of_range);
}

// ========================================================================================
// transform
// ========================================================================================

TEST(ExpressionChecks, transform_updates_metadata_and_values_for_real_expression) {
    // arrange
    std::vector<double> data_buffer = {1, 2, 3, 4};
    std::vector<std::span<const double>> steps = {{data_buffer.data(), 1}, {data_buffer.data() + 1, 3}};
    Expression<double> expr("V1", data_buffer, steps, "V", "R1", "node");
    std::string transformed_name = "V1_scaled";
    std::string transformed_unit = "mV";
    std::string transformed_variable_type = "derived";
    // act
    expr.transform([](double v) { return v * 1000.0; });
    const auto data = expr.data();
    const auto step = expr.step_data(1);
    // assert
    ASSERT_EQ(data.size(), 4);
    ASSERT_EQ(data[0], 1000.0);
    ASSERT_EQ(data[1], 2000.0);
    ASSERT_EQ(data[2], 3000.0);
    ASSERT_EQ(data[3], 4000.0);
    ASSERT_EQ(step.size(), 3);
    ASSERT_EQ(step[0], 2000.0);
    ASSERT_EQ(step[1], 3000.0);
    ASSERT_EQ(step[2], 4000.0);
}

// ========================================================================================
// AnyExpression
// ========================================================================================

TEST(ExpressionChecks, any_expression_variant_holds_real_expression) {
    // arrange
    std::vector<double> data = {0.0, 1.0};
    std::vector<std::span<const double>> steps = {{data.data(), data.size()}};
    Expression<double> expr("time", data, steps, "s");
    // act
    AnyExpression any = std::move(expr);
    // assert
    ASSERT_TRUE(std::holds_alternative<Expression<double>>(any));
}

TEST(ExpressionChecks, any_expression_variant_holds_complex_expression) {
    // arrange
    std::vector<std::complex<double>> data = {{1, 0}, {0, 1}};
    std::vector<std::span<const std::complex<double>>> steps = {{data.data(), data.size()}};
    Expression<std::complex<double>> expr("I1", data, steps, "A");
    // act
    AnyExpression any = std::move(expr);
    // assert
    ASSERT_TRUE(std::holds_alternative<Expression<std::complex<double>>>(any));
}
