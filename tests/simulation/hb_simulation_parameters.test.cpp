#include <map>

#include <gtest/gtest.h>

#include "netlist/netlist.h"
#include "simulation/hb_simulation_parameters.h"

// ========================================================================================
// from_xyce_directives
// ========================================================================================

TEST(HbSimulationParametersChecks, parses_single_frequency) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->frequencies.size(), 1);
    ASSERT_EQ(result->frequencies[0], "1MEG");
}

TEST(HbSimulationParametersChecks, parses_multiple_frequencies) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG 2MEG 500K"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->frequencies.size(), 3);
    ASSERT_EQ(result->frequencies[0], "1MEG");
    ASSERT_EQ(result->frequencies[1], "2MEG");
    ASSERT_EQ(result->frequencies[2], "500K");
}

TEST(HbSimulationParametersChecks, parses_hbint_options) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".OPTIONS HBINT NUMFREQ=15 TAHB=2 SELECTHARMS=box STARTUPPERIODS=5"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->harmonics.size(), 1);
    ASSERT_EQ(result->harmonics[0], 15);
    ASSERT_EQ(result->tahb.value(), 2);
    ASSERT_EQ(result->selectharms.value(), "box");
    ASSERT_EQ(result->startup_periods.value(), 5);
}

TEST(HbSimulationParametersChecks, no_hb_directive_returns_none) {
    // arrange
    const std::vector<std::string> directives = {".TRAN 1u 1m"};
    // act
    const auto result = HbSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_FALSE(result.has_value());
}

// ========================================================================================
// to_xyce_directives
// ========================================================================================

TEST(HbSimulationParametersChecks, generates_single_frequency_directive) {
    // arrange
    const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".HB 1MEG");
}

TEST(HbSimulationParametersChecks, generates_multiple_frequencies_directive) {
    // arrange
    const HbSimulationParameters params({"1MEG", "2MEG", "500K"}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".HB 1MEG 2MEG 500K");
}

TEST(HbSimulationParametersChecks, generates_with_harmonics_directive) {
    // arrange
    const HbSimulationParameters params({"1MEG", "2MEG"}, {15, 12}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_EQ(directives[0], ".HB 1MEG 2MEG");
    ASSERT_EQ(directives[1], ".OPTIONS HBINT NUMFREQ=15,12");
}

TEST(HbSimulationParametersChecks, generates_with_tahb_directive) {
    // arrange
    const HbSimulationParameters params({"1MEG"}, {}, 2, std::nullopt, std::nullopt, std::nullopt, {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_EQ(directives[0], ".HB 1MEG");
    ASSERT_EQ(directives[1], ".OPTIONS HBINT TAHB=2");
}

TEST(HbSimulationParametersChecks, generates_with_selectharms_directive) {
    // arrange
    const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, "box", std::nullopt, std::nullopt, {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_EQ(directives[0], ".HB 1MEG");
    ASSERT_EQ(directives[1], ".OPTIONS HBINT SELECTHARMS=box");
}

TEST(HbSimulationParametersChecks, generates_with_startup_periods_directive) {
    // arrange
    const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, 5, std::nullopt, {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_EQ(directives[0], ".HB 1MEG");
    ASSERT_EQ(directives[1], ".OPTIONS HBINT STARTUPPERIODS=5");
}

TEST(HbSimulationParametersChecks, generates_combined_hbint_options) {
    // arrange
    const HbSimulationParameters params({"1MEG"}, {10}, 1, "hybrid", 0, std::nullopt, {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_EQ(directives[0], ".HB 1MEG");
    ASSERT_EQ(directives[1], ".OPTIONS HBINT NUMFREQ=10 TAHB=1 SELECTHARMS=hybrid STARTUPPERIODS=0");
}

TEST(HbSimulationParametersChecks, generates_combined_hbint_options_with_multiple_harmonics) {
    // arrange
    const HbSimulationParameters params({"1MEG", "2MEG"}, {15, 12}, 2, "box", 5, std::nullopt, {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_EQ(directives[0], ".HB 1MEG 2MEG");
    ASSERT_EQ(directives[1], ".OPTIONS HBINT NUMFREQ=15,12 TAHB=2 SELECTHARMS=box STARTUPPERIODS=5");
}

TEST(HbSimulationParametersChecks, generates_with_print_parameters) {
    // arrange
    const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, PrintParameters("HB", "", "", {"V(OUT)"}, {}), {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_EQ(directives[0], ".HB 1MEG");
    ASSERT_EQ(directives[1], ".PRINT HB V(OUT)");
}

// ========================================================================================
// equality operator
// ========================================================================================

TEST(HbSimulationParametersChecks, equality_operator_equal_params) {
    // arrange
    const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {});
    const HbSimulationParameters params2({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_TRUE(result);
}

TEST(HbSimulationParametersChecks, equality_operator_different_frequencies) {
    // arrange
    const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {});
    const HbSimulationParameters params2({"2MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(HbSimulationParametersChecks, equality_operator_different_harmonics) {
    // arrange
    const HbSimulationParameters params1({"1MEG"}, {10}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {});
    const HbSimulationParameters params2({"1MEG"}, {15}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(HbSimulationParametersChecks, equality_operator_different_tahb) {
    // arrange
    const HbSimulationParameters params1({"1MEG"}, {}, 1, std::nullopt, std::nullopt, std::nullopt, {}, {});
    const HbSimulationParameters params2({"1MEG"}, {}, 2, std::nullopt, std::nullopt, std::nullopt, {}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(HbSimulationParametersChecks, equality_operator_different_selectharms) {
    // arrange
    const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, "box", std::nullopt, std::nullopt, {}, {});
    const HbSimulationParameters params2({"1MEG"}, {}, std::nullopt, "hybrid", std::nullopt, std::nullopt, {}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(HbSimulationParametersChecks, equality_operator_different_startup_periods) {
    // arrange
    const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, std::nullopt, 5, std::nullopt, {}, {});
    const HbSimulationParameters params2({"1MEG"}, {}, std::nullopt, std::nullopt, 10, std::nullopt, {}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(HbSimulationParametersChecks, equality_operator_different_print_parameters) {
    // arrange
    const HbSimulationParameters params1({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, PrintParameters("HB", "", "", {"V(OUT)"}, {}), {}, {});
    const HbSimulationParameters params2({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, PrintParameters("TRAN", "", "", {"I(V1)"}, {}), {}, {});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

// ========================================================================================
// Tests for from_xyce_directives edge cases
// ========================================================================================

TEST(HbSimulationParametersChecks, empty_directives_returns_none) {
    // arrange
    const std::vector<std::string> directives = {};
    // act
    const auto result = HbSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_FALSE(result.has_value());
}

TEST(HbSimulationParametersChecks, non_hb_directives_return_none) {
    // arrange
    const std::vector<std::string> directives = {".TRAN 1ns 1ms"};
    // act
    const auto result = HbSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_FALSE(result.has_value());
}

TEST(HbSimulationParametersChecks, parses_print_hb_directive) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".PRINT HB V(1) I(V1)"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->print_parameters.has_value(), true);
    ASSERT_EQ(result->print_parameters->print_type, "HB");
    ASSERT_EQ(result->print_parameters->output_variables.size(), 2);
    ASSERT_EQ(result->print_parameters->output_variables[0], "V(1)");
    ASSERT_EQ(result->print_parameters->output_variables[1], "I(V1)");
}

TEST(HbSimulationParametersChecks, ignores_non_hb_print_directive) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".PRINT TRAN V(1)"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->print_parameters.has_value(), false);
}

// ========================================================================================
// Tests for reference guide examples
// ========================================================================================

TEST(HbSimulationParametersChecks, reference_guide_example_single_frequency) {
    // arrange
    const std::vector<std::string> directives = {".HB 1e4"};
    // act
    const auto result = HbSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->frequencies.size(), 1);
    ASSERT_EQ(result->frequencies[0], "1e4");
    // verify the directive contains the expected hb line
    const auto generated = result->to_xyce_directives(NetlistTopology{});
    ASSERT_EQ(generated.size(), 1);
    ASSERT_EQ(generated[0], ".HB 1e4");
}

TEST(HbSimulationParametersChecks, reference_guide_example_multiple_frequencies) {
    // arrange
    const std::vector<std::string> directives = {".hb 1e4 2e2"};
    // act
    const auto result = HbSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->frequencies.size(), 2);
    ASSERT_EQ(result->frequencies[0], "1e4");
    ASSERT_EQ(result->frequencies[1], "2e2");
    // verify the directive contains the expected hb line
    const auto generated = result->to_xyce_directives(NetlistTopology{});
    ASSERT_EQ(generated.size(), 1);
    ASSERT_EQ(generated[0], ".HB 1e4 2e2");
}

// ========================================================================================
// NONLIN-HB / LINSOL-HB option packages
// ========================================================================================

TEST(HbSimulationParametersChecks, parses_nonlin_hb_options) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".OPTIONS NONLIN-HB MAXSTEP=20 ABSTOL=1e-6"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->nonlin_options.size(), 2);
    EXPECT_EQ(result->nonlin_options.at("MAXSTEP"), "20");
    EXPECT_EQ(result->nonlin_options.at("ABSTOL"), "1e-6");
}

TEST(HbSimulationParametersChecks, parses_linsol_hb_options) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".OPTIONS LINSOL-HB KTYPE=1 TOL=1e-10"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->linsol_options.size(), 2);
    EXPECT_EQ(result->linsol_options.at("KTYPE"), "1");
    EXPECT_EQ(result->linsol_options.at("TOL"), "1e-10");
}

TEST(HbSimulationParametersChecks, emits_nonlin_hb_options) {
    // arrange
    std::map<std::string, std::string> nonlin;
    nonlin["MAXSTEP"] = "20";
    const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, nonlin, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    EXPECT_EQ(directives[0], ".HB 1MEG");
    EXPECT_EQ(directives[1], ".OPTIONS NONLIN-HB MAXSTEP=20");
}

TEST(HbSimulationParametersChecks, emits_linsol_hb_options) {
    // arrange
    std::map<std::string, std::string> linsol;
    linsol["KTYPE"] = "1";
    linsol["TOL"] = "1e-10";
    const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, linsol);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    EXPECT_EQ(directives[0], ".HB 1MEG");
    EXPECT_EQ(directives[1], ".OPTIONS LINSOL-HB KTYPE=1 TOL=1e-10");
}

// ========================================================================================
// invalid HBINT values are ignored
// ========================================================================================

TEST(HbSimulationParametersChecks, invalid_numfreq_value_is_ignored) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".OPTIONS HBINT NUMFREQ=abc"});
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->harmonics.empty());
}

TEST(HbSimulationParametersChecks, invalid_tahb_value_is_ignored) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".OPTIONS HBINT TAHB=xyz"});
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->tahb.has_value());
}

TEST(HbSimulationParametersChecks, invalid_startup_periods_value_is_ignored) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".OPTIONS HBINT STARTUPPERIODS=q"});
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->startup_periods.has_value());
}

TEST(HbSimulationParametersChecks, mixed_valid_and_invalid_numfreq_values_parse_partially) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".OPTIONS HBINT NUMFREQ=5,bad"});
    // assert — the valid prefix value is kept, the invalid one dropped
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->harmonics.size(), 1);
    EXPECT_EQ(result->harmonics[0], 5);
}

// ========================================================================================
// HB_FD / HB_TD print variants
// ========================================================================================

TEST(HbSimulationParametersChecks, parses_print_hb_fd_directive) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".PRINT HB_FD V(1)"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->print_parameters.has_value());
    EXPECT_EQ(result->print_parameters->print_type, "HB_FD");
}

TEST(HbSimulationParametersChecks, parses_print_hb_td_directive) {
    // arrange / act
    const auto result = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".PRINT HB_TD V(1)"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->print_parameters.has_value());
    EXPECT_EQ(result->print_parameters->print_type, "HB_TD");
}

TEST(HbSimulationParametersChecks, emits_hb_fd_print_directive) {
    // arrange
    const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, PrintParameters("HB_FD", "", "", {"V(OUT)"}, {}), {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    EXPECT_EQ(directives[1], ".PRINT HB_FD V(OUT)");
}

TEST(HbSimulationParametersChecks, skips_non_hb_print_on_emission) {
    // arrange
    const HbSimulationParameters params({"1MEG"}, {}, std::nullopt, std::nullopt, std::nullopt, PrintParameters("TRAN", "", "", {"V(OUT)"}, {}), {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    EXPECT_EQ(directives[0], ".HB 1MEG");
}

// ========================================================================================
// full round trips with option packages
// ========================================================================================

TEST(HbSimulationParametersChecks, hbint_options_round_trip_through_directives) {
    // arrange
    const auto input = HbSimulationParameters::from_xyce_directives({".HB 1MEG 2MEG", ".OPTIONS HBINT NUMFREQ=15,12 TAHB=2 SELECTHARMS=box STARTUPPERIODS=5"});
    ASSERT_TRUE(input.has_value());
    // act
    const auto directives = input->to_xyce_directives(NetlistTopology{});
    const auto output = HbSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(output.has_value());
    EXPECT_EQ(*output, *input);
}

TEST(HbSimulationParametersChecks, option_packages_round_trip_through_directives) {
    // arrange
    const auto input = HbSimulationParameters::from_xyce_directives({".HB 1MEG", ".OPTIONS HBINT NUMFREQ=15", ".OPTIONS NONLIN-HB MAXSTEP=20", ".OPTIONS LINSOL-HB KTYPE=1"});
    ASSERT_TRUE(input.has_value());
    // act
    const auto directives = input->to_xyce_directives(NetlistTopology{});
    const auto output = HbSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(output.has_value());
    EXPECT_EQ(*output, *input);
}
