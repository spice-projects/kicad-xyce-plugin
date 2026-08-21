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
    Chart chart(&expression_manager, &step_information, AbscissaScale::LINEAR, 1000);
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
    Chart chart(&expression_manager, &step_information, AbscissaScale::DECADE, 1000);
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
    Chart chart(&expression_manager, &step_information, AbscissaScale::DECADE, 1000);
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
    Chart chart(&expression_manager, &step_information, AbscissaScale::DECADE, 1000);
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
    Chart chart(&expression_manager, &step_information, AbscissaScale::OCTAVE, 1000);
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
    Chart chart(&expression_manager, &step_information, AbscissaScale::DECADE, 1000);
    // act
    const double value = chart.ratio_to_abscissa_value(0.5);
    // assert
    ASSERT_TRUE(std::isfinite(value));
    ASSERT_NEAR(value, 4.5, 1e-9);
}

TEST(ChartRatioTest, decade_plot_ratio_interpolates_geometrically_over_visible_range) {
    // arrange
    std::vector<double> abscissa_data = {1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0};
    std::vector<std::pair<size_t, size_t>> step_slices = {{0, 6}};
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(Expression<double>("sweep", std::move(abscissa_data), step_slices, "V"));
    ExpressionManager expression_manager(expressions, step_slices);
    StepInformation step_information({"sweep"}, {{}}, {{1.0, 100000.0}});
    Chart chart(&expression_manager, &step_information, AbscissaScale::DECADE, 1000);
    // set the visible abscissa range as when a chart is first created
    chart.plot_series({});
    // act
    const double value_at_10 = chart.plot_ratio_to_abscissa_value(0.2);
    const double value_at_100 = chart.plot_ratio_to_abscissa_value(0.4);
    // assert
    ASSERT_NEAR(value_at_10, 10.0, 1e-9);
    ASSERT_NEAR(value_at_100, 100.0, 1e-9);
}

TEST(ChartRatioTest, linear_plot_ratio_interpolates_linearly_over_visible_range) {
    // arrange
    std::vector<double> abscissa_data = {0.0, 2.0, 4.0, 6.0, 8.0, 10.0};
    std::vector<std::pair<size_t, size_t>> step_slices = {{0, 6}};
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), step_slices, "s"));
    ExpressionManager expression_manager(expressions, step_slices);
    StepInformation step_information({"time"}, {{}}, {{0.0, 10.0}});
    Chart chart(&expression_manager, &step_information, AbscissaScale::LINEAR, 1000);
    // set the visible abscissa range as when a chart is first created
    chart.plot_series({});
    // act
    const double value = chart.plot_ratio_to_abscissa_value(0.5);
    // assert
    ASSERT_NEAR(value, 5.0, 1e-9);
}

TEST(ChartRatioTest, hovered_series_text_avoids_scientific_prefix_overflow) {
    // arrange
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0, 3.0};
    // series is 1 mA, slightly below the milli divider so the mantissa would overflow to 1e+03uA
    std::vector<double> current_data = {0.0009999, 0.0009999, 0.0009999, 0.0009999};
    std::vector<std::pair<size_t, size_t>> step_slices = {{0, 4}};
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), step_slices, "s"));
    expressions.emplace_back(Expression<double>("I(R1)", std::move(current_data), step_slices, "A"));
    ExpressionManager expression_manager(expressions, step_slices);
    StepInformation step_information({"time"}, {{}}, {{0.0, 3.0}});
    Chart chart(&expression_manager, &step_information, AbscissaScale::LINEAR, 1000);
    // plot the current series
    chart.plot_series({expression_manager.expressions()[1]});
    // act
    const std::string text = chart.hovered_series_text(1.5);
    // assert
    EXPECT_NE(text.find("I(R1)=1 mA"), std::string::npos);
    EXPECT_EQ(text.find("1e+03"), std::string::npos);
}

TEST(ChartRatioTest, hovered_series_text_after_zoom) {
    // arrange
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<double> current_data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    std::vector<std::pair<size_t, size_t>> step_slices = {{0, 6}};
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), step_slices, "s"));
    expressions.emplace_back(Expression<double>("V(out)", std::move(current_data), step_slices, "V"));
    ExpressionManager expression_manager(expressions, step_slices);
    StepInformation step_information({"time"}, {{}}, {{0.0, 5.0}});
    Chart chart(&expression_manager, &step_information, AbscissaScale::LINEAR, 1000);
    chart.plot_series({expression_manager.expressions()[1]});
    // act
    // zoom from 1.2s to 1.8s (ratios 0.24 to 0.36)
    chart.update_zoom_window(0.24, 0.36, 0.0, 1.0);
    const double hovered_x = chart.plot_ratio_to_abscissa_value(0.5);
    const std::string text = chart.hovered_series_text(hovered_x);
    // assert
    EXPECT_NE(text.find("V(out)="), std::string::npos);
}

TEST(ChartFormatTest, metric_places_space_before_unit) {
    // si format: value + space + prefix + unit
    EXPECT_EQ(Chart::format_metric(0.001, "A"), "1 mA");
    EXPECT_EQ(Chart::format_metric(1.5, "A"), "1.5 A");
    EXPECT_EQ(Chart::format_metric(12.345, "A"), "12.3 A");
    EXPECT_EQ(Chart::format_metric(123.4, "A"), "123 A");
    EXPECT_EQ(Chart::format_metric(1000.0, "A"), "1 kA");
}

TEST(ChartFormatTest, metric_uses_three_significant_digits) {
    EXPECT_EQ(Chart::format_metric(1.2333, "A"), "1.23 A");
    EXPECT_EQ(Chart::format_metric(1.678, "A"), "1.68 A");
}

TEST(ChartFormatTest, metric_bumps_mantissa_to_next_prefix) {
    EXPECT_EQ(Chart::format_metric(0.0009995, "A"), "1 mA");
    EXPECT_EQ(Chart::format_metric(0.0009999, "A"), "1 mA");
    EXPECT_EQ(Chart::format_metric(0.9999999, "V"), "1 V");
    // no scientific notation overflow like 1e+03uA for a mA value
    EXPECT_EQ(Chart::format_metric(0.0009999, "A").find("e+"), std::string::npos);
}

TEST(ChartFormatTest, metric_zero_threshold_drops_prefix_and_separator) {
    // values below the 1e-12 threshold render as bare zero
    EXPECT_EQ(Chart::format_metric(1e-13, "V"), "0V");
    EXPECT_EQ(Chart::format_metric(1e-15, "V"), "0V");
    // the threshold itself still gets the pico prefix
    EXPECT_EQ(Chart::format_metric(1e-12, "V"), "1 pV");
}

TEST(ChartFormatTest, metric_uses_micro_sign) {
    EXPECT_EQ(Chart::format_metric(1e-6, "A"), "1 µA");
    // the former ASCII 'u' prefix is no longer emitted
    EXPECT_EQ(Chart::format_metric(1e-6, "A").find("uA"), std::string::npos);
}
