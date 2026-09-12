#include <gtest/gtest.h>

#include "simulation/step_parameters.h"

// ========================================================================================
// from_single_directive
// ========================================================================================

TEST(StepParametersChecks, parse_linear_step_implicit) {
    // arrange
    const std::string directive = ".STEP R1 1k 10k 1k";
    // act
    const auto result = StepParameters::from_single_directive(directive);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->enabled);
    ASSERT_EQ(result->sweep_mode, "LIN");
    ASSERT_EQ(result->variable, "R1");
    ASSERT_EQ(result->start, "1k");
    ASSERT_EQ(result->stop, "10k");
    ASSERT_EQ(result->step, "1k");
}

TEST(StepParametersChecks, parse_linear_step_explicit) {
    // arrange
    const std::string directive = ".STEP LIN R1 1k 10k 1k";
    // act
    const auto result = StepParameters::from_single_directive(directive);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->enabled);
    ASSERT_EQ(result->sweep_mode, "LIN");
    ASSERT_EQ(result->variable, "R1");
    ASSERT_EQ(result->start, "1k");
    ASSERT_EQ(result->stop, "10k");
    ASSERT_EQ(result->step, "1k");
}

TEST(StepParametersChecks, parse_dec_step) {
    // arrange
    const std::string directive = ".STEP DEC R1 100 1MEG 10";
    // act
    const auto result = StepParameters::from_single_directive(directive);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->enabled);
    ASSERT_EQ(result->sweep_mode, "DEC");
    ASSERT_EQ(result->variable, "R1");
    ASSERT_EQ(result->start, "100");
    ASSERT_EQ(result->stop, "1MEG");
    ASSERT_EQ(result->points, "10");
}

TEST(StepParametersChecks, parse_list_step) {
    // arrange
    const std::string directive = ".STEP R1 LIST 1k 2k 5k";
    // act
    const auto result = StepParameters::from_single_directive(directive);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->enabled);
    ASSERT_EQ(result->sweep_mode, "LIST");
    ASSERT_EQ(result->variable, "R1");
    ASSERT_EQ(result->list_values.size(), 3);
    ASSERT_EQ(result->list_values[0], "1k");
    ASSERT_EQ(result->list_values[1], "2k");
    ASSERT_EQ(result->list_values[2], "5k");
}

TEST(StepParametersChecks, parse_data_step) {
    // arrange
    const std::string directive = ".STEP DATA=myTable";
    // act
    const auto result = StepParameters::from_single_directive(directive);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->enabled);
    ASSERT_EQ(result->sweep_mode, "DATA");
    ASSERT_EQ(result->data_table_name, "myTable");
}

TEST(StepParametersChecks, parse_data_step_with_spaces) {
    // arrange
    const std::string directive = ".STEP DATA = myTable";
    // act
    const auto result = StepParameters::from_single_directive(directive);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->enabled);
    ASSERT_EQ(result->sweep_mode, "DATA");
    ASSERT_EQ(result->data_table_name, "myTable");
}

TEST(StepParametersChecks, parsing_returns_disabled_when_no_directive) {
    // arrange
    const std::string directive = ".TRAN 1u 1m";
    // act
    const auto result = StepParameters::from_single_directive(directive);
    // assert
    ASSERT_FALSE(result.has_value());
}

TEST(StepParametersChecks, from_xyce_directives_returns_multiple_steps) {
    // arrange - nested loops: inner first, outer last
    const std::vector<std::string> directives = {
        ".STEP R1 1k 10k 1k",
        ".STEP C1 1n 100n 10",
    };
    // act
    const auto result = StepParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->enabled);
    ASSERT_EQ(result->sweep_mode, "LIN");
    ASSERT_EQ(result->variable, "R1");
    ASSERT_EQ(result->start, "1k");
    ASSERT_EQ(result->stop, "10k");
    ASSERT_EQ(result->step, "1k");
}

TEST(StepParametersChecks, from_xyce_directives_empty_returns_disabled) {
    // arrange
    const std::vector<std::string> directives = {};
    // act
    const auto result = StepParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->enabled);
}

TEST(StepParametersChecks, from_xyce_directives_non_step_returns_disabled) {
    // arrange
    const std::vector<std::string> directives = {".TRAN 1u 1m"};
    // act
    const auto result = StepParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->enabled);
}

// ========================================================================================
// all_from_xyce_directives
// ========================================================================================

TEST(StepParametersChecks, all_from_xyce_directives_returns_multiple_steps) {
    // arrange
    const std::vector<std::string> directives = {
        ".STEP R1 1k 10k 1k",
        ".STEP C1 1n 100n 10n",
    };
    // act
    const auto results = StepParameters::all_from_xyce_directives(directives);
    // assert
    ASSERT_EQ(results.size(), 2);
    ASSERT_TRUE(results[0].enabled);
    ASSERT_EQ(results[0].variable, "R1");
    ASSERT_TRUE(results[1].enabled);
    ASSERT_EQ(results[1].variable, "C1");
}

// ========================================================================================
// from_xyce_directives
// ========================================================================================

TEST(StepParametersChecks, from_xyce_directives_returns_first_step) {
    // arrange
    const std::vector<std::string> directives = {
        ".STEP R1 1k 10k 1k",
        ".STEP C1 1n 100n 10n",
    };
    // act
    const auto result = StepParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(result->enabled);
    ASSERT_EQ(result->variable, "R1");
}

TEST(StepParametersChecks, from_xyce_directives_disabled_when_no_step) {
    // arrange
    const std::vector<std::string> directives = {
        ".TRAN 1u 1m",
    };
    // act
    const auto result = StepParameters::from_xyce_directives(directives);
    // assert
    ASSERT_FALSE(result->enabled);
}

// ========================================================================================
// to_xyce_directives
// ========================================================================================

TEST(StepParametersChecks, generates_linear_step_directive) {
    // arrange
    const StepParameters params("LIN", "R1", "1k", "10k", "1k", "", {}, "", true);
    // act
    const auto directives = params.to_xyce_directives();
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".STEP LIN R1 1k 10k 1k");
}

TEST(StepParametersChecks, generates_dec_step_directive) {
    // arrange
    const StepParameters params("DEC", "R1", "100", "1MEG", "", "10", {}, "", true);
    // act
    const auto directives = params.to_xyce_directives();
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".STEP DEC R1 100 1MEG 10");
}

TEST(StepParametersChecks, generates_list_step_directive) {
    // arrange
    const StepParameters params("LIST", "R1", "", "", "", "", {"1k", "2k", "5k"}, "", true);
    // act
    const auto directives = params.to_xyce_directives();
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".STEP R1 LIST 1k 2k 5k");
}

TEST(StepParametersChecks, generates_data_step_directive) {
    // arrange
    const StepParameters params("DATA", "", "", "", "", "", std::vector<std::string>{}, "myTable", true);
    // act
    const auto directives = params.to_xyce_directives();
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".STEP DATA=myTable");
}

TEST(StepParametersChecks, generates_disabled_step_returns_empty) {
    // arrange
    const StepParameters params("", "", "", "", "", "", std::vector<std::string>{}, "", false);
    // act
    const auto directives = params.to_xyce_directives();
    // assert
    ASSERT_EQ(directives.size(), 0);
}

// ========================================================================================
// validate
// ========================================================================================

TEST(StepParametersChecks, disabled_step_validation_passes) {
    // arrange
    const StepParameters params;
    // act / assert
    EXPECT_FALSE(params.validate().has_value());
}

TEST(StepParametersChecks, data_validation_requires_table_name) {
    // arrange
    const StepParameters params("DATA", "", "", "", "", "", std::vector<std::string>{}, "", true);
    // act / assert
    const auto error = params.validate();
    ASSERT_TRUE(error.has_value());
    EXPECT_NE(error->find("table name"), std::string::npos);
}

TEST(StepParametersChecks, data_validation_passes_with_table_name) {
    // arrange
    const StepParameters params("DATA", "", "", "", "", "", std::vector<std::string>{}, "myTable", true);
    // act / assert
    EXPECT_FALSE(params.validate().has_value());
}

TEST(StepParametersChecks, list_validation_requires_variable) {
    // arrange
    const StepParameters params("LIST", "", "", "", "", "", {"1k", "2k"}, "", true);
    // act / assert
    const auto error = params.validate();
    ASSERT_TRUE(error.has_value());
    EXPECT_NE(error->find("sweep variable"), std::string::npos);
}

TEST(StepParametersChecks, list_validation_requires_at_least_one_value) {
    // arrange
    const StepParameters params("LIST", "R1", "", "", "", "", std::vector<std::string>{}, "", true);
    // act / assert
    const auto error = params.validate();
    ASSERT_TRUE(error.has_value());
    EXPECT_NE(error->find("at least one value"), std::string::npos);
}

TEST(StepParametersChecks, list_validation_passes_with_variable_and_values) {
    // arrange
    const StepParameters params("LIST", "R1", "", "", "", "", {"1k", "2k", "5k"}, "", true);
    // act / assert
    EXPECT_FALSE(params.validate().has_value());
}

TEST(StepParametersChecks, lin_validation_requires_step) {
    // arrange
    const StepParameters params("LIN", "R1", "1k", "10k", "", "", std::vector<std::string>{}, "", true);
    // act / assert
    const auto error = params.validate();
    ASSERT_TRUE(error.has_value());
    EXPECT_NE(error->find("step value is required"), std::string::npos);
}

TEST(StepParametersChecks, lin_validation_passes) {
    // arrange
    const StepParameters params("LIN", "R1", "1k", "10k", "1k", "", std::vector<std::string>{}, "", true);
    // act / assert
    EXPECT_FALSE(params.validate().has_value());
}

TEST(StepParametersChecks, descending_lin_requires_negative_step) {
    // arrange — start > stop requires step < 0
    const StepParameters params("LIN", "R1", "10k", "1k", "1k", "", std::vector<std::string>{}, "", true);
    // act / assert
    const auto error = params.validate();
    ASSERT_TRUE(error.has_value());
    EXPECT_NE(error->find("negative"), std::string::npos);
}

TEST(StepParametersChecks, descending_lin_validation_passes_with_negative_step) {
    // arrange
    const StepParameters params("LIN", "R1", "10k", "1k", "-500", "", std::vector<std::string>{}, "", true);
    // act / assert
    EXPECT_FALSE(params.validate().has_value());
}

TEST(StepParametersChecks, dec_validation_requires_positive_integer_points) {
    // arrange
    const StepParameters params("DEC", "R1", "100", "1MEG", "", "3.5", std::vector<std::string>{}, "", true);
    // act / assert
    const auto error = params.validate();
    ASSERT_TRUE(error.has_value());
    EXPECT_NE(error->find("positive integer"), std::string::npos);
}

TEST(StepParametersChecks, dec_validation_requires_start_greater_than_zero) {
    // arrange
    const StepParameters params("DEC", "R1", "0", "1MEG", "", "10", std::vector<std::string>{}, "", true);
    // act / assert
    const auto error = params.validate();
    ASSERT_TRUE(error.has_value());
    EXPECT_NE(error->find("greater than zero"), std::string::npos);
}

TEST(StepParametersChecks, dec_validation_passes_with_valid_params) {
    // arrange
    const StepParameters params("DEC", "R1", "100", "1MEG", "", "10", std::vector<std::string>{}, "", true);
    // act / assert
    EXPECT_FALSE(params.validate().has_value());
}

TEST(StepParametersChecks, oct_validation_passes_with_valid_params) {
    // arrange
    const StepParameters params("OCT", "R1", "100", "1MEG", "", "5", std::vector<std::string>{}, "", true);
    // act / assert
    EXPECT_FALSE(params.validate().has_value());
}

TEST(StepParametersChecks, lin_validation_requires_variable) {
    // arrange
    const StepParameters params("LIN", "", "1k", "10k", "1k", "", std::vector<std::string>{}, "", true);
    // act / assert
    const auto error = params.validate();
    ASSERT_TRUE(error.has_value());
    EXPECT_NE(error->find("sweep variable"), std::string::npos);
}

// ...existing code...
