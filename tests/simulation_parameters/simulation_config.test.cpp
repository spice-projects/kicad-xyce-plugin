#include <gtest/gtest.h>

#include "simulation_parameters/simulation_config.h"

TEST(SimulationConfigOutputPathChecks, raw_output_file_path_returns_nullopt_for_non_raw_format) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".OP", ".PRINT DC V(1)"});
    // act
    auto raw_path = config.raw_output_file_path("/tmp", "/tmp/test.cir");
    // assert
    ASSERT_FALSE(raw_path.has_value());
}

TEST(SimulationConfigOutputPathChecks, raw_output_file_path_uses_print_file) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".TRAN 1u 1m", ".PRINT TRAN FORMAT=RAW FILE=output.raw V(1)"});
    // act
    auto raw_path = config.raw_output_file_path("/tmp/work", "/tmp/work/test.cir");
    // assert
    ASSERT_TRUE(raw_path.has_value());
    ASSERT_EQ(raw_path->string(), "/tmp/work/output.raw");
}

TEST(SimulationConfigOutputPathChecks, raw_output_file_path_appends_raw_suffix) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".TRAN 1u 1m", ".PRINT TRAN FORMAT=RAW V(1)"});
    // act
    auto raw_path = config.raw_output_file_path("/tmp/work", "/tmp/work/test.cir");
    // assert
    ASSERT_TRUE(raw_path.has_value());
    ASSERT_EQ(raw_path->string(), "/tmp/work/test.cir.raw");
}

TEST(SimulationConfigOutputPathChecks, fft_output_file_path_returns_nullopt_for_op_analysis) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".OP"});
    // act
    auto fft_path = config.fft_output_file_path_pattern("/tmp/test.cir");
    // assert
    ASSERT_FALSE(fft_path.has_value());
}

TEST(SimulationConfigOutputPathChecks, fft_output_file_path_returns_pattern_for_tran_with_fft) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".TRAN 1u 1m", ".FFT V(1)"});
    // act
    auto fft_path = config.fft_output_file_path_pattern("/tmp/test.cir");
    // assert
    ASSERT_TRUE(fft_path.has_value());
    ASSERT_EQ(fft_path->string(), "/tmp/test.cir.fft*");
}

TEST(SimulationConfigOutputPathChecks, fft_output_file_path_returns_nullopt_for_tran_without_fft) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".TRAN 1u 1m"});
    // act
    auto fft_path = config.fft_output_file_path_pattern("/tmp/test.cir");
    // assert
    ASSERT_FALSE(fft_path.has_value());
}