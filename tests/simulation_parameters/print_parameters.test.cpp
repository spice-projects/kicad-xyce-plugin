#include <gtest/gtest.h>

#include "simulation_parameters/print_parameters.h"

// ========================================================================================
// from_xyce_statement
// ========================================================================================

TEST(PrintParametersChecks, non_print_statement_returns_none) {
    // arrange
    const std::string statement = ".TRAN 1u 1m";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_FALSE(result.has_value());
}

TEST(PrintParametersChecks, minimal_print_statement) {
    // arrange
    const std::string statement = ".PRINT TRAN";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->print_type, "TRAN");
    ASSERT_EQ(result->print_format, "");
    ASSERT_EQ(result->print_file, "");
    ASSERT_EQ(result->output_variables.size(), 0);
    ASSERT_EQ(result->extra_options.size(), 0);
}

TEST(PrintParametersChecks, parses_format_file_and_variables) {
    // arrange
    const std::string statement = ".print tran FORMAT=RAW FILE=waves.raw V(OUT) I(V1)";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->print_type, "TRAN");
    ASSERT_EQ(result->print_format, "RAW");
    ASSERT_EQ(result->print_file, "waves.raw");
    ASSERT_EQ(result->output_variables.size(), 2);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
    ASSERT_EQ(result->output_variables[1], "I(V1)");
}

TEST(PrintParametersChecks, parses_expression_with_spaces) {
    // arrange
    const std::string statement = ".PRINT TRAN FORMAT=RAW V(OUT) {V(OUT) * I(V1)}";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 2);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
    ASSERT_EQ(result->output_variables[1], "{V(OUT) * I(V1)}");
}

TEST(PrintParametersChecks, parses_generic_options) {
    // arrange
    const std::string statement = ".PRINT TRAN WIDTH=20 PRECISION=12 V(OUT)";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->extra_options.size(), 2);
    ASSERT_EQ(result->extra_options[0], "WIDTH=20");
    ASSERT_EQ(result->extra_options[1], "PRECISION=12");
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
}

TEST(PrintParametersChecks, ignores_invalid_format_value) {
    // arrange
    const std::string statement = ".PRINT TRAN FORMAT=INVALID V(OUT)";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->print_format, "");
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
}

TEST(PrintParametersChecks, ignores_sample_options_for_non_sample_print_type) {
    // arrange
    const std::string statement = ".PRINT TRAN OUTPUT_SAMPLE_STATS=TRUE OUTPUT_ALL_SAMPLES=TRUE V(OUT)";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->extra_options.size(), 0);
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
}

TEST(PrintParametersChecks, accepts_sample_options_for_es_print_type) {
    // arrange
    const std::string statement = ".PRINT ES OUTPUT_SAMPLE_STATS=TRUE OUTPUT_ALL_SAMPLES=FALSE V(OUT)";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->extra_options.size(), 2);
    ASSERT_EQ(result->extra_options[0], "OUTPUT_SAMPLE_STATS=TRUE");
    ASSERT_EQ(result->extra_options[1], "OUTPUT_ALL_SAMPLES=FALSE");
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
}

TEST(PrintParametersChecks, ignores_invalid_format_value_with_warning) {
    // arrange
    const std::string statement = ".PRINT TRAN FORMAT=INVALID V(OUT)";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->print_format, "");
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
}

TEST(PrintParametersChecks, round_trip) {
    // arrange
    const PrintParameters params("TRAN", "RAW", "waves.raw", {"V(OUT)", "ID(M1)", "{V(OUT)*I(V1)}"}, {"WIDTH=20"});
    // act
    const std::string statement = params.to_xyce_statement();
    const auto reparsed = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(reparsed.has_value());
    ASSERT_EQ(reparsed->print_type, "TRAN");
    ASSERT_EQ(reparsed->print_format, "RAW");
    ASSERT_EQ(reparsed->print_file, "waves.raw");
    ASSERT_EQ(reparsed->output_variables.size(), 3);
    ASSERT_EQ(reparsed->output_variables[0], "V(OUT)");
    ASSERT_EQ(reparsed->output_variables[1], "ID(M1)");
    ASSERT_EQ(reparsed->output_variables[2], "{V(OUT)*I(V1)}");
    ASSERT_EQ(reparsed->extra_options.size(), 1);
    ASSERT_EQ(reparsed->extra_options[0], "WIDTH=20");
}

TEST(PrintParametersChecks, serializes_statement_with_all_fields) {
    // arrange
    const PrintParameters params("TRAN", "RAW", "waves.raw", {"V(OUT)", "ID(M1)", "{V(OUT)*I(V1)}"}, {"WIDTH=20", "PRECISION=12"});
    // act
    const std::string statement = params.to_xyce_statement();
    // assert
    ASSERT_EQ(statement, ".PRINT TRAN FORMAT=RAW FILE=waves.raw WIDTH=20 PRECISION=12 V(OUT) ID(M1) {V(OUT)*I(V1)}");
}

TEST(PrintParametersChecks, serializes_statement_with_minimal_fields) {
    // arrange
    const PrintParameters params("TRAN", "", "", {}, {});
    // act
    const std::string statement = params.to_xyce_statement();
    // assert
    ASSERT_EQ(statement, ".PRINT TRAN");
}

TEST(PrintParametersChecks, parses_expression_with_complex_parentheses) {
    // arrange
    const std::string statement = ".PRINT TRAN {V(OUT) * I(V1)} {V(IN) + V(OUT)}";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 2);
    ASSERT_EQ(result->output_variables[0], "{V(OUT) * I(V1)}");
    ASSERT_EQ(result->output_variables[1], "{V(IN) + V(OUT)}");
}

TEST(PrintParametersChecks, parses_multiple_extra_options) {
    // arrange
    const std::string statement = ".PRINT TRAN WIDTH=20 PRECISION=12 HEADINGS=1 V(OUT)";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->extra_options.size(), 3);
    ASSERT_EQ(result->extra_options[0], "WIDTH=20");
    ASSERT_EQ(result->extra_options[1], "PRECISION=12");
    ASSERT_EQ(result->extra_options[2], "HEADINGS=1");
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
}

TEST(PrintParametersChecks, parses_format_lowercase) {
    // arrange
    const std::string statement = ".print tran FORMAT=raw FILE=waves.raw V(OUT)";
    // act
    const auto result = PrintParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->print_type, "TRAN");
    ASSERT_EQ(result->print_format, "raw");
    ASSERT_EQ(result->print_file, "waves.raw");
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
}

// ========================================================================================
// to_xyce_statement
// ========================================================================================

TEST(PrintParametersChecks, serializes_statement) {
    // arrange
    const PrintParameters params("TRAN", "RAW", "waves.raw", {"V(OUT)", "ID(M1)", "{V(OUT)*I(V1)}"}, {"WIDTH=20"});
    // act
    const std::string statement = params.to_xyce_statement();
    // assert
    ASSERT_EQ(statement, ".PRINT TRAN FORMAT=RAW FILE=waves.raw WIDTH=20 V(OUT) ID(M1) {V(OUT)*I(V1)}");
}

TEST(PrintParametersChecks, serializes_minimal_statement) {
    // arrange
    const PrintParameters params("TRAN", "", "", {}, {});
    // act
    const std::string statement = params.to_xyce_statement();
    // assert
    ASSERT_EQ(statement, ".PRINT TRAN");
}

TEST(PrintParametersChecks, serializes_with_all_fields) {
    // arrange
    const PrintParameters params("DC", "CSV", "output.csv", {"V(*)", "I(*)"}, {"WIDTH=20", "PRECISION=12"});
    // act
    const std::string statement = params.to_xyce_statement();
    // assert
    ASSERT_EQ(statement, ".PRINT DC FORMAT=CSV FILE=output.csv WIDTH=20 PRECISION=12 V(*) I(*)");
}

// ========================================================================================
// equality operator
// ========================================================================================

TEST(PrintParametersChecks, equality_operator_equal_params) {
    // arrange
    const PrintParameters params1("TRAN", "RAW", "file.raw", {"V(OUT)"}, {"WIDTH=20"});
    const PrintParameters params2("TRAN", "RAW", "file.raw", {"V(OUT)"}, {"WIDTH=20"});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_TRUE(result);
}

TEST(PrintParametersChecks, equality_operator_different_type) {
    // arrange
    const PrintParameters params1("TRAN", "", "", {}, {});
    const PrintParameters params2("DC", "", "", {}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(PrintParametersChecks, equality_operator_different_format) {
    // arrange
    const PrintParameters params1("TRAN", "RAW", "", {}, {});
    const PrintParameters params2("TRAN", "CSV", "", {}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(PrintParametersChecks, equality_operator_different_file) {
    // arrange
    const PrintParameters params1("TRAN", "", "file1.raw", {}, {});
    const PrintParameters params2("TRAN", "", "file2.raw", {}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(PrintParametersChecks, equality_operator_different_variables) {
    // arrange
    const PrintParameters params1("TRAN", "", "", {"V(OUT)"}, {});
    const PrintParameters params2("TRAN", "", "", {"I(V1)"}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(PrintParametersChecks, equality_operator_different_extra_options) {
    // arrange
    const PrintParameters params1("TRAN", "", "", {}, {"WIDTH=20"});
    const PrintParameters params2("TRAN", "", "", {}, {"WIDTH=40"});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}
