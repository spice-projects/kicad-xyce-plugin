#include <filesystem>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "expression/expression.h"
#include "expression/expression_manager.h"
#include "file/xyce_output_file.h"
#include "step_information.h"

// ========================================================================================
// type traits
// ========================================================================================

static_assert(!std::is_copy_constructible_v<XyceOutputFile>);
static_assert(!std::is_copy_assignable_v<XyceOutputFile>);

// ========================================================================================
// constructor / filename accessor
// ========================================================================================

TEST(XyceOutputFileChecks, constructor_stores_filename) {
    // arrange
    const std::filesystem::path expected_filename = "/tmp/test.raw";
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output(expected_filename, "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_EQ(output.filename(), expected_filename);
}

TEST(XyceOutputFileChecks, filename_accessor_returns_const_reference) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // act
    const auto& const_output = output;
    const auto& filename = const_output.filename();
    // assert
    ASSERT_EQ(filename, "/tmp/test.raw");
}

// ========================================================================================
// title accessor
// ========================================================================================

TEST(XyceOutputFileChecks, constructor_stores_title) {
    // arrange
    const std::string expected_title = "My Simulation";
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", expected_title, false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_EQ(output.title(), expected_title);
}

// ========================================================================================
// is_complex accessor
// ========================================================================================

TEST(XyceOutputFileChecks, is_complex_returns_false_when_set_false) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_FALSE(output.is_complex());
}

TEST(XyceOutputFileChecks, is_complex_returns_true_when_set_true) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", true, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_TRUE(output.is_complex());
}

// ========================================================================================
// step_information accessor
// ========================================================================================

TEST(XyceOutputFileChecks, step_information_returns_stored_step_information) {
    // arrange
    const std::vector<std::string> expected_keys = {"R1", "TEMP"};
    const std::vector<std::vector<double>> expected_values = {{1000.0, 27.0}, {2000.0, 85.0}};
    const std::vector<std::pair<double, double>> expected_ranges = {{0.0, 10.0}, {0.0, 10.0}};
    StepInformation step_info(expected_keys, expected_values, expected_ranges);
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_EQ(output.step_information().keys(), expected_keys);
    ASSERT_EQ(output.step_information().values(), expected_values);
    ASSERT_EQ(output.step_information().length(), 2);
}

TEST(XyceOutputFileChecks, step_information_accessor_is_const) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // act
    const auto& const_output = output;
    const auto& info = const_output.step_information();
    // assert
    ASSERT_EQ(info.length(), 1);
}

// ========================================================================================
// abscissa accessor
// ========================================================================================

TEST(XyceOutputFileChecks, abscissa_returns_stored_expression) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_EQ(output.abscissa().name(), "time");
    ASSERT_EQ(output.abscissa().unit(), "s");
}

TEST(XyceOutputFileChecks, abscissa_returns_mutable_reference) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // act
    auto& abscissa_ref = output.abscissa();
    // assert
    ASSERT_EQ(abscissa_ref.name(), "time");
}

// ========================================================================================
// abscissa_scale accessor
// ========================================================================================

TEST(XyceOutputFileChecks, abscissa_scale_returns_linear) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_EQ(output.abscissa_scale(), AbscissaScale::LINEAR);
}

TEST(XyceOutputFileChecks, abscissa_scale_returns_decade) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::DECADE, std::move(manager), nullptr);
    // assert
    ASSERT_EQ(output.abscissa_scale(), AbscissaScale::DECADE);
}

TEST(XyceOutputFileChecks, abscissa_scale_returns_octave) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::OCTAVE, std::move(manager), nullptr);
    // assert
    ASSERT_EQ(output.abscissa_scale(), AbscissaScale::OCTAVE);
}

// ========================================================================================
// plot_type accessor
// ========================================================================================

TEST(XyceOutputFileChecks, plot_type_returns_stored_value) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::AC, AbscissaScale::DECADE, std::move(manager), nullptr);
    // assert
    ASSERT_EQ(output.plot_type(), PlotType::AC);
}

TEST(XyceOutputFileChecks, plot_type_returns_unknown_when_stored_unknown) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_EQ(output.plot_type(), PlotType::UNKNOWN);
}

// ========================================================================================
// expression_manager accessor
// ========================================================================================

TEST(XyceOutputFileChecks, expression_manager_returns_stored_expressions) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_EQ(output.expression_manager().expressions().size(), 1);
}

TEST(XyceOutputFileChecks, expression_manager_returns_mutable_reference) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // act
    auto& manager_ref = output.expression_manager();
    auto* expr = manager_ref.evaluate("time");
    // assert
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(std::get<Expression<double>>(*expr).unit(), "s");
}

// ========================================================================================
// metadata accessor
// ========================================================================================

TEST(XyceOutputFileChecks, metadata_returns_empty_when_defaulted) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_TRUE(output.metadata().empty());
}

TEST(XyceOutputFileChecks, metadata_returns_stored_values) {
    // arrange
    const std::unordered_map<std::string, std::string> expected_metadata{{"Window", "HANN"}, {"Normalized", "true"}};
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr, {}, expected_metadata);
    // assert
    ASSERT_EQ(output.metadata(), expected_metadata);
}

TEST(XyceOutputFileChecks, metadata_accessor_returns_const_reference) {
    // arrange
    const std::unordered_map<std::string, std::string> expected_metadata{{"Window", "HANN"}};
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr, {}, expected_metadata);
    // act
    const auto& const_output = output;
    const auto& metadata = const_output.metadata();
    // assert
    ASSERT_EQ(metadata, expected_metadata);
}

// ========================================================================================
// suggested_plots accessor
// ========================================================================================

TEST(XyceOutputFileChecks, suggested_plots_returns_empty_when_defaulted) {
    // arrange
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    // assert
    ASSERT_TRUE(output.suggested_plots().empty());
}

TEST(XyceOutputFileChecks, suggested_plots_returns_stored_expression_names) {
    // arrange
    const std::vector<std::vector<std::string>> expected_plots = {{"V(out)", "I(R1)"}, {"V(in)"}};
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr, expected_plots);
    // assert
    ASSERT_EQ(output.suggested_plots(), expected_plots);
}

TEST(XyceOutputFileChecks, suggested_plots_accessor_returns_const_reference) {
    // arrange
    const std::vector<std::vector<std::string>> expected_plots = {{"V(out)"}};
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr, expected_plots);
    // act
    const auto& const_output = output;
    const auto& plots = const_output.suggested_plots();
    // assert
    ASSERT_EQ(plots, expected_plots);
}

TEST(XyceOutputFileChecks, constructor_suggested_plots_precedes_metadata) {
    // arrange
    const std::vector<std::vector<std::string>> expected_plots = {{"V(out)"}, {"I(R1)", "I(R2)"}};
    const std::unordered_map<std::string, std::string> expected_metadata{{"Window", "HANN"}};
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    // act
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr, expected_plots, expected_metadata);
    // assert
    ASSERT_EQ(output.suggested_plots(), expected_plots);
    ASSERT_EQ(output.metadata(), expected_metadata);
}

// ========================================================================================
// move semantics
// ========================================================================================

TEST(XyceOutputFileChecks, move_constructor_transfers_suggested_plots_and_metadata) {
    // arrange
    const std::vector<std::vector<std::string>> expected_plots = {{"V(out)", "I(R1)"}, {"V(in)"}};
    const std::unordered_map<std::string, std::string> expected_metadata{{"Window", "HANN"}};
    StepInformation step_info({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions;
    std::vector<double> abscissa_data = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
    expressions.emplace_back(Expression<double>("time", std::move(abscissa_data), slices, "s"));
    ExpressionManager manager(expressions, slices);
    XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr, expected_plots, expected_metadata);
    // act
    XyceOutputFile moved(std::move(output));
    // assert
    ASSERT_EQ(moved.suggested_plots(), expected_plots);
    ASSERT_EQ(moved.metadata(), expected_metadata);
}

TEST(XyceOutputFileChecks, move_assignment_transfers_suggested_plots_and_metadata) {
    // arrange
    const std::vector<std::vector<std::string>> expected_plots = {{"V(out)"}, {"V(in)"}};
    const std::unordered_map<std::string, std::string> expected_metadata{{"Normalized", "true"}};
    StepInformation step_info_a({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions_a;
    std::vector<double> abscissa_data_a = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices_a = {{0, 3}};
    expressions_a.emplace_back(Expression<double>("time", std::move(abscissa_data_a), slices_a, "s"));
    ExpressionManager manager_a(expressions_a, slices_a);
    XyceOutputFile source("/tmp/a.raw", "A", false, std::move(step_info_a), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager_a), nullptr, expected_plots, expected_metadata);
    StepInformation step_info_b({"R1"}, {{1000.0}}, {{0.0, 10.0}});
    std::vector<AnyExpression> expressions_b;
    std::vector<double> abscissa_data_b = {0.0, 1.0, 2.0};
    std::vector<std::pair<size_t, size_t>> slices_b = {{0, 3}};
    expressions_b.emplace_back(Expression<double>("time", std::move(abscissa_data_b), slices_b, "s"));
    ExpressionManager manager_b(expressions_b, slices_b);
    XyceOutputFile target("/tmp/b.raw", "B", false, std::move(step_info_b), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager_b), nullptr);
    // act
    target = std::move(source);
    // assert
    ASSERT_EQ(target.suggested_plots(), expected_plots);
    ASSERT_EQ(target.metadata(), expected_metadata);
}

// ========================================================================================
// copy prevention
// ========================================================================================

TEST(XyceOutputFileChecks, copy_constructor_is_deleted) {
    // arrange
    // act
    static_assert(!std::is_copy_constructible_v<XyceOutputFile>);
    // assert
    SUCCEED();
}

TEST(XyceOutputFileChecks, copy_assignment_is_deleted) {
    // arrange
    // act
    static_assert(!std::is_copy_assignable_v<XyceOutputFile>);
    // assert
    SUCCEED();
}
