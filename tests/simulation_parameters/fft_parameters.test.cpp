#include <gtest/gtest.h>

#include "simulation_parameters/fft_parameters.h"

// ========================================================================================
// from_xyce_statement
// ========================================================================================

TEST(FftParametersChecks, minimal_fft_statement) {
    // arrange
    const std::string statement = ".FFT V(OUT)";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "V(OUT)");
    ASSERT_EQ(result->to_xyce_statement(), ".FFT V(OUT)");
}

TEST(FftParametersChecks, full_fft_statement) {
    // arrange
    const std::string statement = ".FFT V(OUT) NP=1024 WINDOW=HANN ALFA=1.0 FORMAT=NORM START=0 STOP=10m FREQ=1k FMIN=0 FMAX=10k";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "V(OUT)");
    ASSERT_EQ(result->np, "1024");
    ASSERT_EQ(result->window, "HANN");
    ASSERT_EQ(result->alfa, "1.0");
    ASSERT_EQ(result->fft_format, "NORM");
    ASSERT_EQ(result->start, "0");
    ASSERT_EQ(result->stop, "10m");
    ASSERT_EQ(result->freq, "1k");
    ASSERT_EQ(result->fmin, "0");
    ASSERT_EQ(result->fmax, "10k");
}

TEST(FftParametersChecks, synonyms_from_to) {
    // arrange
    const std::string statement = ".FFT V(OUT) FROM=1m TO=5m";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->start, "1m");
    ASSERT_EQ(result->stop, "5m");
    ASSERT_EQ(result->to_xyce_statement(), ".FFT V(OUT) START=1m STOP=5m");
}

TEST(FftParametersChecks, synonym_triangular) {
    // arrange
    const std::string statement = ".FFT V(OUT) WINDOW=TRIANGULAR";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->window, "TRIANGULAR");
    ASSERT_EQ(result->to_xyce_statement(), ".FFT V(OUT) WINDOW=TRIANGULAR");
}

TEST(FftParametersChecks, invalid_fft_statement) {
    // arrange
    const std::string statement = ".TRAN 1u 1m";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_FALSE(result.has_value());
}

TEST(FftParametersChecks, handles_complex_output_variable) {
    // arrange
    const std::string statement = ".FFT {V(OUT)*I(R1)} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)*I(R1)}");
}

// ========================================================================================
// to_xyce_statement
// ========================================================================================

TEST(FftParametersChecks, to_xyce_statement_only_required_field) {
    // arrange
    const FftParameters params("V(OUT)", "", "", "", "", "", "", "", "", "");
    // act
    const std::string result = params.to_xyce_statement();
    // assert
    ASSERT_EQ(result, ".FFT V(OUT)");
}

TEST(FftParametersChecks, to_xyce_statement_all_fields) {
    // arrange
    const FftParameters params("V(OUT)", "1024", "HANN", "1.0", "NORM", "0", "10m", "1k", "0", "10k");
    // act
    const std::string result = params.to_xyce_statement();
    // assert
    ASSERT_EQ(result, ".FFT V(OUT) NP=1024 WINDOW=HANN ALFA=1.0 FORMAT=NORM START=0 STOP=10m FREQ=1k FMIN=0 FMAX=10k");
}

TEST(FftParametersChecks, to_xyce_statement_preserves_case_of_valid_window) {
    // arrange
    const FftParameters params("V(OUT)", "", "BLACKMAN", "", "", "", "", "", "", "");
    // act
    const std::string result = params.to_xyce_statement();
    // assert
    ASSERT_EQ(result, ".FFT V(OUT) WINDOW=BLACKMAN");
}

TEST(FftParametersChecks, to_xyce_statement_preserves_case_of_valid_format) {
    // arrange
    const FftParameters params("V(OUT)", "", "", "", "UNORM", "", "", "", "", "");
    // act
    const std::string result = params.to_xyce_statement();
    // assert
    ASSERT_EQ(result, ".FFT V(OUT) FORMAT=UNORM");
}
