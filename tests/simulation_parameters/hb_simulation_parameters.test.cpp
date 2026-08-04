// #include <gtest/gtest.h>

// #include "simulation_parameters/hb_simulation_parameters.h"

// // ========================================================================================
// // from_xyce_directives
// // ========================================================================================

// TEST(HbSimulationParametersChecks, parses_single_frequency) {
//     // arrange / act
//     const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->frequencies.size(), 1);
//     ASSERT_EQ(result->frequencies[0], "1MEG");
// }

// TEST(HbSimulationParametersChecks, parses_multiple_frequencies) {
//     // arrange / act
//     const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG 2MEG 500K"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->frequencies.size(), 3);
//     ASSERT_EQ(result->frequencies[0], "1MEG");
//     ASSERT_EQ(result->frequencies[1], "2MEG");
//     ASSERT_EQ(result->frequencies[2], "500K");
// }

// TEST(HbSimulationParametersChecks, parses_replaceground_true) {
//     // arrange / act
//     const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".PREPROCESS REPLACEGROUND TRUE"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->replace_ground, true);
// }

// TEST(HbSimulationParametersChecks, parses_hbint_options) {
//     // arrange / act
//     const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".OPTIONS HBINT NUMFREQ=15 TAHB=2 SELECTHARMS=box STARTUPPERIODS=5"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->harmonics.size(), 1);
//     ASSERT_EQ(result->harmonics[0], 15);
//     ASSERT_EQ(result->tahb.value(), 2);
//     ASSERT_EQ(result->selectharms.value(), "box");
//     ASSERT_EQ(result->startup_periods.value(), 5);
// }

// TEST(HbSimulationParametersChecks, no_hb_directive_returns_none) {
//     // arrange
//     const std::vector<std::string> directives = {".TRAN 1u 1m"};
//     // act
//     const auto result = HbSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_FALSE(result.has_value());
// }

// // ========================================================================================
// // to_xyce_directives
// // ========================================================================================

// TEST(HbSimulationParametersChecks, generates_single_frequency_directive) {
//     // arrange
//     const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 1);
//     ASSERT_EQ(directives[0], ".HB 1MEG");
// }

// TEST(HbSimulationParametersChecks, generates_multiple_frequencies_directive) {
//     // arrange
//     const HbSimulationParameters params({"1MEG", "2MEG", "500K"}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 1);
//     ASSERT_EQ(directives[0], ".HB 1MEG 2MEG 500K");
// }

// TEST(HbSimulationParametersChecks, generates_with_replace_ground) {
//     // arrange
//     const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, true, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".PREPROCESS REPLACEGROUND TRUE");
//     ASSERT_EQ(directives[1], ".HB 1MEG");
// }

// TEST(HbSimulationParametersChecks, generates_with_harmonics_directive) {
//     // arrange
//     const HbSimulationParameters params({"1MEG", "2MEG"}, {15, 12}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".HB 1MEG 2MEG");
//     ASSERT_EQ(directives[1], ".OPTIONS HBINT NUMFREQ=15,12");
// }

// TEST(HbSimulationParametersChecks, generates_with_tahb_directive) {
//     // arrange
//     const HbSimulationParameters params({"1MEG"}, {}, 2, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".HB 1MEG");
//     ASSERT_EQ(directives[1], ".OPTIONS HBINT TAHB=2");
// }

// TEST(HbSimulationParametersChecks, generates_with_selectharms_directive) {
//     // arrange
//     const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, "box", std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".HB 1MEG");
//     ASSERT_EQ(directives[1], ".OPTIONS HBINT SELECTHARMS=box");
// }

// TEST(HbSimulationParametersChecks, generates_with_startup_periods_directive) {
//     // arrange
//     const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, 5, false, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".HB 1MEG");
//     ASSERT_EQ(directives[1], ".OPTIONS HBINT STARTUPPERIODS=5");
// }

// TEST(HbSimulationParametersChecks, generates_combined_hbint_options) {
//     // arrange
//     const HbSimulationParameters params({"1MEG"}, {10}, 1, "hybrid", 0, false, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".HB 1MEG");
//     ASSERT_EQ(directives[1], ".OPTIONS HBINT NUMFREQ=10 TAHB=1 SELECTHARMS=hybrid STARTUPPERIODS=0");
// }

// TEST(HbSimulationParametersChecks, generates_combined_hbint_options_with_multiple_harmonics) {
//     // arrange
//     const HbSimulationParameters params({"1MEG", "2MEG"}, {15, 12}, 2, "box", 5, false, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".HB 1MEG 2MEG");
//     ASSERT_EQ(directives[1], ".OPTIONS HBINT NUMFREQ=15,12 TAHB=2 SELECTHARMS=box STARTUPPERIODS=5");
// }

// TEST(HbSimulationParametersChecks, generates_with_print_parameters) {
//     // arrange
//     const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, false, PrintParameters("HB", "", "", {"V(OUT)"}, {}), {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".HB 1MEG");
//     ASSERT_EQ(directives[1], ".PRINT HB V(OUT)");
// }

// // ========================================================================================
// // equality operator
// // ========================================================================================

// TEST(HbSimulationParametersChecks, equality_operator_equal_params) {
//     // arrange
//     const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     const HbSimulationParameters params2({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_TRUE(result);
// }

// TEST(HbSimulationParametersChecks, equality_operator_different_frequencies) {
//     // arrange
//     const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     const HbSimulationParameters params2({"2MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(HbSimulationParametersChecks, equality_operator_different_harmonics) {
//     // arrange
//     const HbSimulationParameters params1({"1MEG"}, {10}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     const HbSimulationParameters params2({"1MEG"}, {15}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(HbSimulationParametersChecks, equality_operator_different_tahb) {
//     // arrange
//     const HbSimulationParameters params1({"1MEG"}, {}, 1, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     const HbSimulationParameters params2({"1MEG"}, {}, 2, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(HbSimulationParametersChecks, equality_operator_different_selectharms) {
//     // arrange
//     const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, "box", std::nullopt, false, std::nullopt, {}, {});
//     const HbSimulationParameters params2({"1MEG"}, {}, std::nullopt, "hybrid", std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(HbSimulationParametersChecks, equality_operator_different_startup_periods) {
//     // arrange
//     const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, std::nullopt, 5, false, std::nullopt, {}, {});
//     const HbSimulationParameters params2({"1MEG"}, {}, std::nullopt, std::nullopt, 10, false, std::nullopt, {}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(HbSimulationParametersChecks, equality_operator_different_replace_ground) {
//     // arrange
//     const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     const HbSimulationParameters params2({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, true, std::nullopt, {}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(HbSimulationParametersChecks, equality_operator_different_print_parameters) {
//     // arrange
//     const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, false, PrintParameters("HB", "", "", {"V(OUT)"}, {}), {}, {});
//     const HbSimulationParameters params2({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, false, PrintParameters("TRAN", "", "", {"I(V1)"}, {}), {}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// // ========================================================================================
// // Additional tests for replace_ground
// // ========================================================================================

// TEST(HbSimulationParametersChecks, generates_with_replace_ground_false) {
//     // arrange
//     const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 1);
//     ASSERT_EQ(directives[0], ".HB 1MEG");
// }

// TEST(HbSimulationParametersChecks, generates_with_replace_ground_true) {
//     // arrange
//     const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, true, std::nullopt, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".PREPROCESS REPLACEGROUND TRUE");
//     ASSERT_EQ(directives[1], ".HB 1MEG");
// }

// // ========================================================================================
// // Tests for from_xyce_directives edge cases
// // ========================================================================================

// TEST(HbSimulationParametersChecks, empty_directives_returns_none) {
//     // arrange
//     const std::vector<std::string> directives = {};
//     // act
//     const auto result = HbSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_FALSE(result.has_value());
// }

// TEST(HbSimulationParametersChecks, non_hb_directives_return_none) {
//     // arrange
//     const std::vector<std::string> directives = {".TRAN 1ns 1ms"};
//     // act
//     const auto result = HbSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_FALSE(result.has_value());
// }

// TEST(HbSimulationParametersChecks, parses_print_hb_directive) {
//     // arrange / act
//     const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".PRINT HB V(1) I(V1)"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->print_parameters.has_value(), true);
//     ASSERT_EQ(result->print_parameters->print_type, "HB");
//     ASSERT_EQ(result->print_parameters->output_variables.size(), 2);
//     ASSERT_EQ(result->print_parameters->output_variables[0], "V(1)");
//     ASSERT_EQ(result->print_parameters->output_variables[1], "I(V1)");
// }

// TEST(HbSimulationParametersChecks, ignores_non_hb_print_directive) {
//     // arrange / act
//     const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".PRINT TRAN V(1)"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->print_parameters.has_value(), false);
// }

// // ========================================================================================
// // Tests for reference guide examples
// // ========================================================================================

// TEST(HbSimulationParametersChecks, reference_guide_example_single_frequency) {
//     // arrange
//     const std::vector<std::string> directives = {".HB 1e4"};
//     // act
//     const auto result = HbSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->frequencies.size(), 1);
//     ASSERT_EQ(result->frequencies[0], "1e4");
//     // verify the directive contains the expected hb line
//     const auto generated = result->to_xyce_directives();
//     ASSERT_EQ(generated.size(), 2);
//     ASSERT_EQ(generated[0], ".PREPROCESS REPLACEGROUND TRUE");
//     ASSERT_EQ(generated[1], ".HB 1e4");
// }

// TEST(HbSimulationParametersChecks, reference_guide_example_multiple_frequencies) {
//     // arrange
//     const std::vector<std::string> directives = {".hb 1e4 2e2"};
//     // act
//     const auto result = HbSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->frequencies.size(), 2);
//     ASSERT_EQ(result->frequencies[0], "1e4");
//     ASSERT_EQ(result->frequencies[1], "2e2");
//     // verify the directive contains the expected hb line
//     const auto generated = result->to_xyce_directives();
//     ASSERT_EQ(generated.size(), 2);
//     ASSERT_EQ(generated[0], ".PREPROCESS REPLACEGROUND TRUE");
//     ASSERT_EQ(generated[1], ".HB 1e4 2e2");
// }
