#include <complex>
#include <span>
#include <stdexcept>
#include <string>
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

static_assert(!std::is_default_constructible_v<Expression>);
static_assert(!std::is_copy_constructible_v<Expression>);
static_assert(!std::is_copy_assignable_v<Expression>);

// ========================================================================================
// parameterized constructor — metadata
// ========================================================================================

TEST(ExpressionChecks, constructor_stores_name) {
    // arrange
    std::vector<double> data = {1.0, 2.0, 3.0};
    std::vector<std::span<const double>> steps = {{data}};
    // act
    const Expression expr("net_voltage", data, steps, "V");
    // assert
    ASSERT_EQ(expr.name(), "net_voltage");
}

TEST(ExpressionChecks, constructor_stores_unit) {
    // arrange
    std::vector<double> data = {1.0, 2.0, 3.0};
    std::vector<std::span<const double>> steps = {{data}};
    // act
    const Expression expr("net_voltage", data, steps, "V");
    // assert
    ASSERT_EQ(expr.unit(), "V");
}

TEST(ExpressionChecks, constructor_stores_source) {
    // arrange
    std::vector<double> data = {1.0, 2.0, 3.0};
    std::vector<std::span<const double>> steps = {{data}};
    // act
    const Expression expr("net_voltage", data, steps, "V", "R1");
    // assert
    ASSERT_EQ(expr.source(), "R1");
}

TEST(ExpressionChecks, constructor_stores_variable_type) {
    // arrange
    std::vector<double> data = {1.0, 2.0, 3.0};
    std::vector<std::span<const double>> steps = {{data}};
    // act
    const Expression expr("net_voltage", data, steps, "V", "R1", "node");
    // assert
    ASSERT_EQ(expr.variable_type(), "node");
}

TEST(ExpressionChecks, constructor_source_defaults_to_empty) {
    // arrange
    std::vector<double> data = {1.0, 2.0, 3.0};
    std::vector<std::span<const double>> steps = {{data}};
    // act
    const Expression expr("net_voltage", data, steps, "V");
    // assert
    ASSERT_TRUE(expr.source().empty());
}

TEST(ExpressionChecks, constructor_variable_type_defaults_to_empty) {
    // arrange
    std::vector<double> data = {1.0, 2.0, 3.0};
    std::vector<std::span<const double>> steps = {{data}};
    // act
    const Expression expr("net_voltage", data, steps, "V");
    // assert
    ASSERT_TRUE(expr.variable_type().empty());
}

// ========================================================================================
// is_complex detection
// ========================================================================================

TEST(ExpressionChecks, real_view_steps_are_not_complex) {
    // arrange
    std::vector<View<double>> steps;
    // act
    const Expression expr("V1", steps, "V");
    // assert
    ASSERT_FALSE(expr.is_complex());
}

TEST(ExpressionChecks, complex_view_steps_are_complex) {
    // arrange
    std::vector<View<std::complex<double>>> steps;
    // act
    const Expression expr("V1", steps, "V");
    // assert
    ASSERT_TRUE(expr.is_complex());
}

TEST(ExpressionChecks, real_span_steps_are_not_complex) {
    // arrange
    constexpr std::vector<double> buffer;
    std::vector data(buffer);
    std::vector<std::span<const double>> steps;
    // act
    const Expression expr("V1", data, steps, "V");
    // assert
    ASSERT_FALSE(expr.is_complex());
}

TEST(ExpressionChecks, complex_span_steps_are_complex) {
    // arrange
    constexpr std::vector<std::complex<double>> buffer;
    std::vector data(buffer);
    std::vector<std::span<const std::complex<double>>> steps;
    // act
    const Expression expr("V1", data, steps, "V");
    // assert
    ASSERT_TRUE(expr.is_complex());
}

// ========================================================================================
// step_count
// ========================================================================================

TEST(ExpressionChecks, step_count_returns_correct_count_for_real_views) {
    // arrange
    const std::vector<double> buffer = {1, 2, 3};
    std::vector<View<double>> steps;
    steps.reserve(3);
    steps.emplace_back(make_view(buffer, 3, 1));
    steps.emplace_back(make_view(buffer, 3, 1));
    steps.emplace_back(make_view(buffer, 3, 1));
    const Expression expr("V1", steps, "V");
    // act/assert
    ASSERT_EQ(expr.step_count(), 3);
}

TEST(ExpressionChecks, step_count_returns_correct_count_for_complex_views) {
    // arrange
    using Complex = std::complex<double>;
    const std::vector<Complex> buffer = {Complex(1, 0), Complex(2, 0), Complex(3, 0)};
    std::vector<View<Complex>> steps;
    steps.reserve(3);
    steps.emplace_back(make_view(buffer, 3, 1));
    steps.emplace_back(make_view(buffer, 3, 1));
    steps.emplace_back(make_view(buffer, 3, 1));
    const Expression expr("V1", steps, "V");
    // act/assert
    ASSERT_EQ(expr.step_count(), 3);
}

TEST(ExpressionChecks, step_count_returns_correct_count_for_real_spans) {
    // arrange
    std::vector<double> data = {1, 2, 3, 4, 5, 6};
    std::vector<std::span<const double>> steps = {{data.data(), 2}, {data.data() + 2, 2}, {data.data() + 4, 2}};
    const Expression expr("V1", data, steps, "V");
    // act/assert
    ASSERT_EQ(expr.step_count(), 3);
}

TEST(ExpressionChecks, step_count_returns_correct_count_for_complex_spans) {
    // arrange
    std::vector<std::complex<double>> data = {1, 2, 3, 4, 5, 6};
    std::vector<std::span<const std::complex<double>>> steps = {{data.data(), 2}, {data.data() + 2, 2}, {data.data() + 4, 2}};
    const Expression expr("V1", data, steps, "V");
    // act/assert
    ASSERT_EQ(expr.step_count(), 3);
}

// ========================================================================================
// View
// ========================================================================================

TEST(ExpressionChecks, view_index_operator_respects_stride) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view = make_view(buffer, 3, 2);
    // act/assert
    ASSERT_EQ(view[0], 1);
    ASSERT_EQ(view[1], 2);
    ASSERT_EQ(view[2], 3);
}

TEST(ExpressionChecks, view_size_returns_constructor_size) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view = make_view(buffer, 3, 2);
    // act/assert
    ASSERT_EQ(view.size(), 3);
}

// ========================================================================================
// data
// ========================================================================================

TEST(ExpressionChecks, data_returns_real_span_for_real_view_steps) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    std::vector<View<double>> steps;
    steps.reserve(2);
    steps.emplace_back(make_view(buffer, 3, 2));
    steps.emplace_back(make_view(buffer, 3, 2, 1));
    Expression expr("V1", steps, "V");
    // act
    const auto data = expr.data();
    // assert
    ASSERT_TRUE(std::holds_alternative<std::span<const double>>(data));
    const auto values = std::get<std::span<const double>>(data);
    ASSERT_EQ(values.size(), 6);
    ASSERT_EQ(values[0], 1);
    ASSERT_EQ(values[1], 2);
    ASSERT_EQ(values[2], 3);
    ASSERT_EQ(values[3], 10);
    ASSERT_EQ(values[4], 20);
    ASSERT_EQ(values[5], 30);
}

TEST(ExpressionChecks, data_returns_complex_span_for_complex_view_steps) {
    // arrange
    using Complex = std::complex<double>;
    const std::vector<Complex> buffer = {Complex(1, 1), Complex(10, 10), Complex(2, 2), Complex(20, 20)};
    std::vector<View<Complex>> steps;
    steps.reserve(2);
    steps.emplace_back(make_view(buffer, 2, 2));
    steps.emplace_back(make_view(buffer, 2, 2, 1));
    Expression expr("I1", steps, "A");
    // act
    const auto data = expr.data();
    // assert
    ASSERT_TRUE(std::holds_alternative<std::span<const Complex>>(data));
    const auto values = std::get<std::span<const Complex>>(data);
    ASSERT_EQ(values.size(), 4);
    ASSERT_EQ(values[0], Complex(1, 1));
    ASSERT_EQ(values[1], Complex(2, 2));
    ASSERT_EQ(values[2], Complex(10, 10));
    ASSERT_EQ(values[3], Complex(20, 20));
}

TEST(ExpressionChecks, data_returns_real_span_for_real_span_steps) {
    // arrange
    std::vector<double> data_buffer = {1, 2, 3, 4, 5, 6};
    std::vector<std::span<const double>> steps = {{data_buffer.data(), 3}, {data_buffer.data() + 3, 3}};
    Expression expr("V1", data_buffer, steps, "V");
    // act
    const auto data = expr.data();
    // assert
    ASSERT_TRUE(std::holds_alternative<std::span<const double>>(data));
    const auto values = std::get<std::span<const double>>(data);
    ASSERT_EQ(values.size(), 6);
    ASSERT_EQ(values[0], 1);
    ASSERT_EQ(values[5], 6);
}

TEST(ExpressionChecks, data_returns_complex_span_for_complex_span_steps) {
    // arrange
    using Complex = std::complex<double>;
    std::vector<Complex> data_buffer = {Complex(1, 1), Complex(2, 2), Complex(3, 3)};
    std::vector<std::span<const Complex>> steps = {{data_buffer.data(), 1}, {data_buffer.data() + 1, 2}};
    Expression expr("I1", data_buffer, steps, "A");
    // act
    const auto data = expr.data();
    // assert
    ASSERT_TRUE(std::holds_alternative<std::span<const Complex>>(data));
    const auto values = std::get<std::span<const Complex>>(data);
    ASSERT_EQ(values.size(), 3);
    ASSERT_EQ(values[0], Complex(1, 1));
    ASSERT_EQ(values[2], Complex(3, 3));
}

// ========================================================================================
// step_data
// ========================================================================================

TEST(ExpressionChecks, step_data_returns_real_step_from_real_view_steps) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    std::vector<View<double>> steps;
    steps.reserve(2);
    steps.emplace_back(make_view(buffer, 3, 2));
    steps.emplace_back(make_view(buffer, 3, 2, 1));
    Expression expr("V1", steps, "V");
    // act
    const auto step = expr.step_data(1);
    // assert
    ASSERT_TRUE(std::holds_alternative<std::span<const double>>(step));
    const auto values = std::get<std::span<const double>>(step);
    ASSERT_EQ(values.size(), 3);
    ASSERT_EQ(values[0], 10);
    ASSERT_EQ(values[1], 20);
    ASSERT_EQ(values[2], 30);
}

TEST(ExpressionChecks, step_data_returns_complex_step_from_complex_view_steps) {
    // arrange
    using Complex = std::complex<double>;
    const std::vector<Complex> buffer = {Complex(1, 1), Complex(10, 10), Complex(2, 2), Complex(20, 20)};
    std::vector<View<Complex>> steps;
    steps.reserve(2);
    steps.emplace_back(make_view(buffer, 2, 2));
    steps.emplace_back(make_view(buffer, 2, 2, 1));
    Expression expr("I1", steps, "A");
    // act
    const auto step = expr.step_data(0);
    // assert
    ASSERT_TRUE(std::holds_alternative<std::span<const Complex>>(step));
    const auto values = std::get<std::span<const Complex>>(step);
    ASSERT_EQ(values.size(), 2);
    ASSERT_EQ(values[0], Complex(1, 1));
    ASSERT_EQ(values[1], Complex(2, 2));
}

TEST(ExpressionChecks, step_data_returns_real_step_from_real_span_steps) {
    // arrange
    std::vector<double> data_buffer = {1, 2, 3, 4};
    std::vector<std::span<const double>> steps = {{data_buffer.data(), 2}, {data_buffer.data() + 2, 2}};
    Expression expr("V1", data_buffer, steps, "V");
    // act
    const auto step = expr.step_data(0);
    // assert
    ASSERT_TRUE(std::holds_alternative<std::span<const double>>(step));
    const auto values = std::get<std::span<const double>>(step);
    ASSERT_EQ(values.size(), 2);
    ASSERT_EQ(values[0], 1);
    ASSERT_EQ(values[1], 2);
}

TEST(ExpressionChecks, step_data_returns_complex_step_from_complex_span_steps) {
    // arrange
    using Complex = std::complex<double>;
    std::vector<Complex> data_buffer = {Complex(1, 1), Complex(2, 2), Complex(3, 3)};
    std::vector<std::span<const Complex>> steps = {{data_buffer.data(), 1}, {data_buffer.data() + 1, 2}};
    Expression expr("I1", data_buffer, steps, "A");
    // act
    const auto step = expr.step_data(1);
    // assert
    ASSERT_TRUE(std::holds_alternative<std::span<const Complex>>(step));
    const auto values = std::get<std::span<const Complex>>(step);
    ASSERT_EQ(values.size(), 2);
    ASSERT_EQ(values[0], Complex(2, 2));
    ASSERT_EQ(values[1], Complex(3, 3));
}

TEST(ExpressionChecks, step_data_throws_out_of_range_for_invalid_index) {
    // arrange
    std::vector<double> data_buffer = {1, 2};
    std::vector<std::span<const double>> steps = {{data_buffer.data(), 2}};
    Expression expr("V1", data_buffer, steps, "V");
    // act/assert
    ASSERT_THROW((void)expr.step_data(1), std::out_of_range);
}

// ========================================================================================
// step_indices
// ========================================================================================

TEST(ExpressionChecks, step_indices_returns_expected_ranges_for_view_steps) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    std::vector<View<double>> steps;
    steps.reserve(2);
    steps.emplace_back(make_view(buffer, 3, 2));
    steps.emplace_back(make_view(buffer, 3, 2, 1));
    const Expression expr("V1", steps, "V");
    // act
    const auto indices = expr.step_indices();
    // assert
    ASSERT_EQ(indices.size(), 2);
    ASSERT_EQ(indices[0], (std::pair<size_t, size_t>{0, 3}));
    ASSERT_EQ(indices[1], (std::pair<size_t, size_t>{3, 6}));
}

TEST(ExpressionChecks, step_indices_returns_expected_ranges_for_span_steps) {
    // arrange
    std::vector<double> data_buffer = {1, 2, 3, 4, 5, 6};
    std::vector<std::span<const double>> steps = {{data_buffer.data(), 2}, {data_buffer.data() + 2, 4}};
    Expression expr("V1", data_buffer, steps, "V");
    // act
    const auto indices = expr.step_indices();
    // assert
    ASSERT_EQ(indices.size(), 2);
    ASSERT_EQ(indices[0], (std::pair<size_t, size_t>{0, 2}));
    ASSERT_EQ(indices[1], (std::pair<size_t, size_t>{2, 6}));
}

// ========================================================================================
// transform
// ========================================================================================

TEST(ExpressionChecks, transform_updates_metadata_and_values_for_real_expression) {
    // arrange
    std::vector<double> data_buffer = {1, 2, 3, 4};
    std::vector<std::span<const double>> steps = {{data_buffer.data(), 1}, {data_buffer.data() + 1, 3}};
    Expression expr("V1", data_buffer, steps, "V", "R1", "node");
    std::string transformed_name = "V1_scaled";
    std::string transformed_unit = "mV";
    std::string transformed_variable_type = "derived";
    // act
    Expression transformed = expr.transform<double, double>(transformed_name, transformed_unit, transformed_variable_type, [](double v) { return v * 1000.0; });
    const auto data = transformed.data();
    const auto step = transformed.step_data(1);
    // assert
    ASSERT_EQ(transformed.name(), "V1_scaled");
    ASSERT_EQ(transformed.unit(), "mV");
    ASSERT_FALSE(transformed.is_complex());
    ASSERT_EQ(transformed.step_count(), 2);
    ASSERT_TRUE(std::holds_alternative<std::span<const double>>(data));
    const auto values = std::get<std::span<const double>>(data);
    ASSERT_EQ(values.size(), 4);
    ASSERT_EQ(values[0], 1000.0);
    ASSERT_EQ(values[1], 2000.0);
    ASSERT_EQ(values[2], 3000.0);
    ASSERT_EQ(values[3], 4000.0);
    ASSERT_TRUE(std::holds_alternative<std::span<const double>>(step));
    const auto step_values = std::get<std::span<const double>>(step);
    ASSERT_EQ(step_values.size(), 3);
    ASSERT_EQ(step_values[0], 2000.0);
    ASSERT_EQ(step_values[1], 3000.0);
    ASSERT_EQ(step_values[2], 4000.0);
}

TEST(ExpressionChecks, transform_real_output_from_complex_expression) {
    // arrange
    using Complex = std::complex<double>;
    std::vector<Complex> data_buffer = {Complex(3, 4), Complex(5, 12), Complex(8, 15)};
    std::vector<std::span<const Complex>> steps = {{data_buffer.data(), 2}, {data_buffer.data() + 2, 1}};
    Expression expr("I1", data_buffer, steps, "A");
    std::string transformed_name = "I1_mag";
    std::string transformed_unit = "V";
    std::string transformed_variable_type = "magnitude";
    // act
    Expression transformed = expr.transform<Complex, double>(transformed_name, transformed_unit, transformed_variable_type, [](const Complex& v) { return std::abs(v); });
    const auto data = transformed.data();
    const auto step = transformed.step_data(0);
    // assert
    ASSERT_FALSE(transformed.is_complex());
    ASSERT_TRUE(std::holds_alternative<std::span<const double>>(data));
    const auto values = std::get<std::span<const double>>(data);
    ASSERT_EQ(values.size(), 3);
    ASSERT_EQ(values[0], 5);
    ASSERT_EQ(values[1], 13);
    ASSERT_EQ(values[2], 17);
    ASSERT_TRUE(std::holds_alternative<std::span<const double>>(step));
    const auto step_values = std::get<std::span<const double>>(step);
    ASSERT_EQ(step_values.size(), 2);
    ASSERT_EQ(step_values[0], 5);
    ASSERT_EQ(step_values[1], 13);
}

TEST(ExpressionChecks, transform_throws_when_input_type_does_not_match_expression_data) {
    // arrange
    std::vector<double> data_buffer = {1, 2};
    std::vector<std::span<const double>> steps = {{data_buffer.data(), 2}};
    Expression expr("V1", data_buffer, steps, "V");
    std::string transformed_name = "V1_bad";
    std::string transformed_unit = "V";
    std::string transformed_variable_type = "bad";
    using Complex = std::complex<double>;
    // act/assert
    const auto run_mismatched_transform = [&expr, &transformed_name, &transformed_unit, &transformed_variable_type]() { (void)expr.transform<Complex, double>(transformed_name, transformed_unit, transformed_variable_type, [](const Complex& v) { return std::abs(v); }); };
    ASSERT_THROW(run_mismatched_transform(), std::runtime_error);
}

TEST(ExpressionChecks, transform_initializes_cached_data_from_view_steps) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    std::vector<View<double>> steps;
    steps.reserve(2);
    steps.emplace_back(make_view(buffer, 3, 2));
    steps.emplace_back(make_view(buffer, 3, 2, 1));
    Expression expr("V1", steps, "V");
    std::string transformed_name = "V1_half";
    std::string transformed_unit = "V";
    std::string transformed_variable_type = "derived";
    // act
    Expression transformed = expr.transform<double, double>(transformed_name, transformed_unit, transformed_variable_type, [](double v) { return v * 0.5; });
    const auto data = transformed.data();
    // assert
    ASSERT_TRUE(std::holds_alternative<std::span<const double>>(data));
    const auto values = std::get<std::span<const double>>(data);
    ASSERT_EQ(values.size(), 6);
    ASSERT_EQ(values[0], 0.5);
    ASSERT_EQ(values[1], 1.0);
    ASSERT_EQ(values[2], 1.5);
    ASSERT_EQ(values[3], 5.0);
    ASSERT_EQ(values[4], 10.0);
    ASSERT_EQ(values[5], 15.0);
}
