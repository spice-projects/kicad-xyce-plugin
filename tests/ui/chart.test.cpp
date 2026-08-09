#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "expression/expression.h"
#include "expression/expression_manager.h"
#include "step_information.h"
#include "ui/chart.h"

TEST(ChartRatioTest, linear_ratio_interpolates_linearly) {
    // arrange
    std::vector<double> abscissa_data = {0.0, 2.0, 4.0, 6.0, 8.0, 10.0};
    std::vector<std::pair<size_t, size_t>> step_slices = {{0, 6}};
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), step_slices, "s"));
    ExpressionManager expression_manager(expressions, step_slices);
    StepInformation step_information({"time"}, {{}}, {{0.0, 10.0}});
    Chart chart(&expression_manager, &step_information, "time", AbscissaScale::LINEAR, 1000);
    // act
    const double value = chart.ratio_to_abscissa_value(0.5);
    // assert
    ASSERT_NEAR(value, 5.0, 1e-9);
}

TEST(ChartRatioTest, decade_ratio_interpolates_geometrically) {
    // arrange
    std::vector<double> abscissa_data = {1.0, 10.0, 100.0, 1000.0};
    std::vector<std::pair<size_t, size_t>> step_slices = {{0, 4}};
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(Expression<double>("sweep", std::move(abscissa_data), step_slices, "V"));
    ExpressionManager expression_manager(expressions, step_slices);
    StepInformation step_information({"sweep"}, {{}}, {{1.0, 1000.0}});
    Chart chart(&expression_manager, &step_information, "sweep", AbscissaScale::DECADE, 1000);
    // act
    const double value = chart.ratio_to_abscissa_value(0.5);
    // assert
    ASSERT_NEAR(value, std::sqrt(1000.0), 1e-9);
}

TEST(ChartRatioTest, decade_ratio_endpoints_match_range) {
    // arrange
    std::vector<double> abscissa_data = {1.0, 10.0, 100.0, 1000.0};
    std::vector<std::pair<size_t, size_t>> step_slices = {{0, 4}};
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(Expression<double>("sweep", std::move(abscissa_data), step_slices, "V"));
    ExpressionManager expression_manager(expressions, step_slices);
    StepInformation step_information({"sweep"}, {{}}, {{1.0, 1000.0}});
    Chart chart(&expression_manager, &step_information, "sweep", AbscissaScale::DECADE, 1000);
    // act
    const double left_value = chart.ratio_to_abscissa_value(0.0);
    const double right_value = chart.ratio_to_abscissa_value(1.0);
    // assert
    ASSERT_NEAR(left_value, 1.0, 1e-9);
    ASSERT_NEAR(right_value, 1000.0, 1e-9);
}

TEST(ChartRatioTest, decade_equal_ratios_produce_equal_log_ratios) {
    // arrange
    std::vector<double> abscissa_data = {1.0, 10.0, 100.0, 1000.0};
    std::vector<std::pair<size_t, size_t>> step_slices = {{0, 4}};
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(Expression<double>("sweep", std::move(abscissa_data), step_slices, "V"));
    ExpressionManager expression_manager(expressions, step_slices);
    StepInformation step_information({"sweep"}, {{}}, {{1.0, 1000.0}});
    Chart chart(&expression_manager, &step_information, "sweep", AbscissaScale::DECADE, 1000);
    // act
    const double first = chart.ratio_to_abscissa_value(0.25) / chart.ratio_to_abscissa_value(0.0);
    const double second = chart.ratio_to_abscissa_value(0.5) / chart.ratio_to_abscissa_value(0.25);
    // assert
    ASSERT_NEAR(first, second, 1e-9);
}

TEST(ChartRatioTest, octave_ratio_interpolates_geometrically) {
    // arrange
    std::vector<double> abscissa_data = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0, 256.0};
    std::vector<std::pair<size_t, size_t>> step_slices = {{0, 9}};
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(Expression<double>("sweep", std::move(abscissa_data), step_slices, "V"));
    ExpressionManager expression_manager(expressions, step_slices);
    StepInformation step_information({"sweep"}, {{}}, {{1.0, 256.0}});
    Chart chart(&expression_manager, &step_information, "sweep", AbscissaScale::OCTAVE, 1000);
    // act
    const double value = chart.ratio_to_abscissa_value(0.5);
    // assert
    ASSERT_NEAR(value, 16.0, 1e-9);
}

TEST(ChartRatioTest, logarithmic_ratio_with_non_positive_range_falls_back_to_linear) {
    // arrange
    std::vector<double> abscissa_data = {-1.0, 0.0, 1.0, 10.0};
    std::vector<std::pair<size_t, size_t>> step_slices = {{0, 4}};
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(Expression<double>("sweep", std::move(abscissa_data), step_slices, "V"));
    ExpressionManager expression_manager(expressions, step_slices);
    StepInformation step_information({"sweep"}, {{}}, {{-1.0, 10.0}});
    Chart chart(&expression_manager, &step_information, "sweep", AbscissaScale::DECADE, 1000);
    // act
    const double value = chart.ratio_to_abscissa_value(0.5);
    // assert
    ASSERT_TRUE(std::isfinite(value));
    ASSERT_NEAR(value, 4.5, 1e-9);
}
