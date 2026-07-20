// #include <filesystem>
// #include <span>
// #include <string>
// #include <type_traits>
// #include <vector>

// #include <gtest/gtest.h>

// #include "step_information.h"
// #include "../../src/expression/expression.h"
// #include "../../src/expression/expression_manager.h"
// #include "../../src/file/xyce_output_file.h"

// namespace {

// Expression<double> make_real_expression(const std::string& name, const std::vector<double>& values, const std::string& unit = "V") {
//     std::vector<double> data(values);
//     std::vector<std::span<const double>> steps = {{data.data(), data.size()}};
//     return Expression(name, data, steps, unit);
// }

// } // namespace

// // ========================================================================================
// // type traits
// // ========================================================================================

// static_assert(!std::is_copy_constructible_v<XyceOutputFile>);
// static_assert(!std::is_copy_assignable_v<XyceOutputFile>);

// // ========================================================================================
// // constructor / filename accessor
// // ========================================================================================

// TEST(XyceOutputFileChecks, constructor_stores_filename) {
//     // arrange
//     const std::filesystem::path expected_filename = "/tmp/test.raw";
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0}, "s"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     XyceOutputFile output(expected_filename, "Test", false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // assert
//     ASSERT_EQ(output.filename(), expected_filename);
// }

// TEST(XyceOutputFileChecks, filename_accessor_returns_const_reference) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0}, "s"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // act
//     const auto& const_output = output;
//     const auto& filename = const_output.filename();
//     // assert
//     ASSERT_EQ(filename, "/tmp/test.raw");
// }

// // ========================================================================================
// // title accessor
// // ========================================================================================

// TEST(XyceOutputFileChecks, constructor_stores_title) {
//     // arrange
//     const std::string expected_title = "My Simulation";
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0}, "s"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     XyceOutputFile output("/tmp/test.raw", expected_title, false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // assert
//     ASSERT_EQ(output.title(), expected_title);
// }

// // ========================================================================================
// // is_complex accessor
// // ========================================================================================

// TEST(XyceOutputFileChecks, is_complex_returns_false_when_set_false) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0}, "s"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // assert
//     ASSERT_FALSE(output.is_complex());
// }

// TEST(XyceOutputFileChecks, is_complex_returns_true_when_set_true) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0}, "s"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     XyceOutputFile output("/tmp/test.raw", "Test", true, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // assert
//     ASSERT_TRUE(output.is_complex());
// }

// // ========================================================================================
// // step_information accessor
// // ========================================================================================

// TEST(XyceOutputFileChecks, step_information_returns_stored_step_information) {
//     // arrange
//     const std::vector<std::string> expected_keys = {"R1", "TEMP"};
//     const std::vector<std::vector<double>> expected_values = {{1000.0, 27.0}, {2000.0, 85.0}};
//     StepInformation step_info(expected_keys, expected_values, std::vector<std::pair<double, double>>{{0.0, 10.0}, {0.0, 10.0}});
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0}, "s"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // assert
//     ASSERT_EQ(output.step_information().keys(), expected_keys);
//     ASSERT_EQ(output.step_information().values(), expected_values);
//     ASSERT_EQ(output.step_information().length(), 2);
// }

// TEST(XyceOutputFileChecks, step_information_accessor_is_const) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0}, "s"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // act
//     const auto& const_output = output;
//     const auto& info = const_output.step_information();
//     // assert
//     ASSERT_EQ(info.length(), 1);
// }

// // ========================================================================================
// // abscissa accessor
// // ========================================================================================

// TEST(XyceOutputFileChecks, abscissa_returns_stored_expression) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0, 1.0, 2.0}, "s"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 3}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // assert
//     ASSERT_EQ(output.abscissa().name(), "time");
//     ASSERT_EQ(output.abscissa().unit(), "s");
// }

// TEST(XyceOutputFileChecks, abscissa_returns_mutable_reference) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0}, "s"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // act
//     auto& abscissa_ref = output.abscissa();
//     // assert
//     ASSERT_EQ(abscissa_ref.name(), "time");
// }

// // ========================================================================================
// // abscissa_scale accessor
// // ========================================================================================

// TEST(XyceOutputFileChecks, abscissa_scale_returns_linear) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0}, "s"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // assert
//     ASSERT_EQ(output.abscissa_scale(), AbscissaScale::LINEAR);
// }

// TEST(XyceOutputFileChecks, abscissa_scale_returns_decade) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("freq", {1.0}, "Hz"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::DECADE, std::move(manager), nullptr, 0);
//     // assert
//     ASSERT_EQ(output.abscissa_scale(), AbscissaScale::DECADE);
// }

// TEST(XyceOutputFileChecks, abscissa_scale_returns_octave) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("freq", {1.0}, "Hz"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::OCTAVE, std::move(manager), nullptr, 0);
//     // assert
//     ASSERT_EQ(output.abscissa_scale(), AbscissaScale::OCTAVE);
// }

// // ========================================================================================
// // expression_manager accessor
// // ========================================================================================

// TEST(XyceOutputFileChecks, expression_manager_returns_stored_expressions) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("time", {0.0}, "s"));
//     expressions.emplace_back(make_real_expression("V(out)", {1.0}, "V"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}, {1, 2}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // assert
//     ASSERT_EQ(output.expression_manager().expressions().size(), 2);
// }

// TEST(XyceOutputFileChecks, expression_manager_returns_mutable_reference) {
//     // arrange
//     std::vector<std::string> keys = {"R1"};
//     std::vector<std::vector<double>> values = {{1000.0}};
//     std::vector<std::pair<double, double>> ranges = {{0.0, 10.0}};
//     StepInformation step_info(keys, values, ranges);
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("V(out)", {1.0}, "V"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     XyceOutputFile output("/tmp/test.raw", "Test", false, std::move(step_info), AbscissaScale::LINEAR, std::move(manager), nullptr, 0);
//     // act
//     auto& manager_ref = output.expression_manager();
//     auto* expr = manager_ref.evaluate("v(out)");
//     // assert
//     ASSERT_NE(expr, nullptr);
//     ASSERT_EQ(std::get<Expression<double>>(*expr).unit(), "V");
// }

// // ========================================================================================
// // copy prevention
// // ========================================================================================

// TEST(XyceOutputFileChecks, copy_constructor_is_deleted) {
//     // arrange
//     // act
//     static_assert(!std::is_copy_constructible_v<XyceOutputFile>);
//     // assert
//     SUCCEED();
// }

// TEST(XyceOutputFileChecks, copy_assignment_is_deleted) {
//     // arrange
//     // act
//     static_assert(!std::is_copy_assignable_v<XyceOutputFile>);
//     // assert
//     SUCCEED();
// }
