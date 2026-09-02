#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "netlist/netlist.h"
#include "simulation/option_parameters.h"

// ========================================================================================
// from_xyce_directives
// ========================================================================================

TEST(OptionParametersChecks, parse_option_directives) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS DEVICE TEMP=25 GMIN=1e-12", ".OPTIONS TIMEINT RELTOL=1e-3 ABSTOL=1e-12", ".OPTIONS NONLIN MAXSTEP=10", ".OPTIONS LINSOL TYPE=AZTECOO", ".OPTIONS FFT FFT_ACCURATE=1 FFTOUT=1 FFT_MODE=0", ".OPTIONS DIAGNOSTIC DEBUGLEVEL=3",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.device.size(), 2);
    ASSERT_EQ(params.device.at("TEMP"), "25");
    ASSERT_EQ(params.device.at("GMIN"), "1e-12");
    ASSERT_EQ(params.timeint.size(), 2);
    ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
    ASSERT_EQ(params.timeint.at("ABSTOL"), "1e-12");
    ASSERT_EQ(params.nonlin.size(), 1);
    ASSERT_EQ(params.nonlin.at("MAXSTEP"), "10");
    ASSERT_EQ(params.linsol.size(), 1);
    ASSERT_EQ(params.linsol.at("TYPE"), "AZTECOO");
    ASSERT_EQ(params.fft.size(), 3);
    ASSERT_EQ(params.fft.at("FFT_ACCURATE"), "1");
    ASSERT_EQ(params.fft.at("FFTOUT"), "1");
    ASSERT_EQ(params.fft.at("FFT_MODE"), "0");
    ASSERT_EQ(params.diagnostic.size(), 1);
    ASSERT_EQ(params.diagnostic.at("DEBUGLEVEL"), "3");
}

TEST(OptionParametersChecks, parse_diagnostic_flag_style_option) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS DIAGNOSTIC DEBUGLEVEL=2",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.device.size(), 0);
    ASSERT_EQ(params.diagnostic.size(), 1);
    ASSERT_EQ(params.diagnostic.at("DEBUGLEVEL"), "2");
}

TEST(OptionParametersChecks, parse_fft_options) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS fft ffT_ACCURATE=0 fFtOut=0",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.fft.size(), 2);
    ASSERT_EQ(params.fft.at("FFT_ACCURATE"), "0");
    ASSERT_EQ(params.fft.at("FFTOUT"), "0");
}

TEST(OptionParametersChecks, parse_fft_flag_style_option) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS FFT FFTOUT",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.fft.size(), 1);
    ASSERT_EQ(params.fft.at("FFTOUT"), "");
}

TEST(OptionParametersChecks, parse_single_option_directive) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS DEVICE TEMP=25",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.device.size(), 1);
    ASSERT_EQ(params.device.at("TEMP"), "25");
}

TEST(OptionParametersChecks, parse_empty_directives) {
    // arrange
    const std::vector<std::string> directives = {};
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.device.size(), 0);
    ASSERT_EQ(params.timeint.size(), 0);
    ASSERT_EQ(params.nonlin.size(), 0);
    ASSERT_EQ(params.linsol.size(), 0);
    ASSERT_EQ(params.fft.size(), 0);
}

TEST(OptionParametersChecks, parse_non_option_directive) {
    // arrange
    const std::vector<std::string> directives = {
        ".TRAN 1u 1m",
        ".OPTIONS FFT FFTOUT=1",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.device.size(), 0);
    ASSERT_EQ(params.fft.size(), 1);
    ASSERT_EQ(params.fft.at("FFTOUT"), "1");
}

TEST(OptionParametersChecks, parse_parser_package_options) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS PARSER MODEL_BINNING=0 SCALE=2.5",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.parser.size(), 2);
    ASSERT_EQ(params.parser.at("MODEL_BINNING"), "0");
    ASSERT_EQ(params.parser.at("SCALE"), "2.5");
}

TEST(OptionParametersChecks, parse_parser_package_options_case_insensitive) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS Parser model_binning=TRUE",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.parser.size(), 1);
    ASSERT_EQ(params.parser.at("MODEL_BINNING"), "TRUE");
}

TEST(OptionParametersChecks, parse_linsol_ac_options) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS LINSOL-AC TYPE=KLU",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.linsol_ac.size(), 1);
    ASSERT_EQ(params.linsol_ac.at("TYPE"), "KLU");
    // the AC-scoped package must not leak into the generic LINSOL package
    ASSERT_EQ(params.linsol.size(), 0);
}

TEST(OptionParametersChecks, parse_loca_options) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS LOCA Max_Num_Starts=4 Min_Start=0.1",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.loca.size(), 2);
    ASSERT_EQ(params.loca.at("MAX_NUM_STARTS"), "4");
    ASSERT_EQ(params.loca.at("MIN_START"), "0.1");
}

TEST(OptionParametersChecks, parse_dist_strategy_option) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS DIST STRATEGY=2",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.dist.size(), 1);
    ASSERT_EQ(params.dist.at("STRATEGY"), "2");
}

// ========================================================================================
// to_xyce_directives
// ========================================================================================

TEST(OptionParametersChecks, generate_directives) {
    // arrange
    const OptionParameters params({{"TEMP", "25"}}, {{"RELTOL", "1e-3"}}, {{"MAXSTEP", "10"}}, {{"TYPE", "AZTECOO"}}, {{"FFTOUT", "1"}});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 5);
    ASSERT_EQ(directives[0], ".OPTIONS DEVICE TEMP=25");
    ASSERT_EQ(directives[1], ".OPTIONS TIMEINT RELTOL=1e-3");
    ASSERT_EQ(directives[2], ".OPTIONS NONLIN MAXSTEP=10");
    ASSERT_EQ(directives[3], ".OPTIONS LINSOL TYPE=AZTECOO");
    ASSERT_EQ(directives[4], ".OPTIONS FFT FFTOUT=1");
}

TEST(OptionParametersChecks, generate_fft_directive) {
    // arrange
    const OptionParameters params({}, {}, {}, {}, {{"FFT_ACCURATE", "0"}, {"FFTOUT", "1"}, {"FFT_MODE", "1"}});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    // keys are emitted in sorted map order (FFTOUT < FFT_ACCURATE < FFT_MODE)
    ASSERT_EQ(directives[0], ".OPTIONS FFT FFTOUT=1 FFT_ACCURATE=0 FFT_MODE=1");
}

TEST(OptionParametersChecks, generate_flag_style_option) {
    // arrange
    const OptionParameters params({}, {}, {}, {}, {{"FFTOUT", ""}});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".OPTIONS FFT FFTOUT");
}

TEST(OptionParametersChecks, generate_diagnostic_directive) {
    // arrange
    const OptionParameters params({}, {}, {}, {}, {}, {{"DEBUGLEVEL", "2"}});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".OPTIONS DIAGNOSTIC DEBUGLEVEL=2");
}

TEST(OptionParametersChecks, generate_parser_directive) {
    // arrange
    const OptionParameters params({}, {}, {}, {}, {}, {}, {{"MODEL_BINNING", "0"}, {"SCALE", "2.5"}});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".OPTIONS PARSER MODEL_BINNING=0 SCALE=2.5");
}

TEST(OptionParametersChecks, generate_linsol_ac_directive) {
    // arrange
    const OptionParameters params({}, {}, {}, {}, {}, {}, {}, {{"TYPE", "KLU"}});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".OPTIONS LINSOL-AC TYPE=KLU");
}

TEST(OptionParametersChecks, generate_loca_directive) {
    // arrange
    const OptionParameters params({}, {}, {}, {}, {}, {}, {}, {}, {{"MAX_NUM_STARTS", "4"}});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".OPTIONS LOCA MAX_NUM_STARTS=4");
}

TEST(OptionParametersChecks, generate_dist_directive) {
    // arrange
    const OptionParameters params({}, {}, {}, {}, {}, {}, {}, {}, {}, {{"STRATEGY", "2"}});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".OPTIONS DIST STRATEGY=2");
}

TEST(OptionParametersChecks, generate_new_packages_in_deterministic_order) {
    // arrange
    const OptionParameters params({}, {}, {}, {}, {}, {}, {{"MODEL_BINNING", "0"}}, {{"TYPE", "KLU"}}, {{"MAX_NUM_STARTS", "4"}}, {{"STRATEGY", "1"}});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 4);
    ASSERT_EQ(directives[0], ".OPTIONS LINSOL-AC TYPE=KLU");
    ASSERT_EQ(directives[1], ".OPTIONS PARSER MODEL_BINNING=0");
    ASSERT_EQ(directives[2], ".OPTIONS LOCA MAX_NUM_STARTS=4");
    ASSERT_EQ(directives[3], ".OPTIONS DIST STRATEGY=1");
}

TEST(OptionParametersChecks, generate_empty_directives) {
    // arrange
    const OptionParameters params({}, {}, {}, {}, {});
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 0);
}

// ========================================================================================
// round trip
// ========================================================================================

TEST(OptionParametersChecks, round_trip_directives) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS DEVICE TEMP=25",
        ".OPTIONS NONLIN MAXSTEP=10",
        ".OPTIONS FFT FFTOUT=1",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    const auto round_trip = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(round_trip.size(), 3);
    ASSERT_EQ(round_trip[0], ".OPTIONS DEVICE TEMP=25");
    ASSERT_EQ(round_trip[1], ".OPTIONS NONLIN MAXSTEP=10");
    ASSERT_EQ(round_trip[2], ".OPTIONS FFT FFTOUT=1");
}

TEST(OptionParametersChecks, round_trip_fft_options) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS FFT FFT_ACCURATE=0 FFTOUT=1 FFT_MODE=1",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    const auto round_trip = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(round_trip.size(), 1);
    // keys are emitted in sorted map order (FFTOUT < FFT_ACCURATE < FFT_MODE)
    ASSERT_EQ(round_trip[0], ".OPTIONS FFT FFTOUT=1 FFT_ACCURATE=0 FFT_MODE=1");
}

TEST(OptionParametersChecks, round_trip_diagnostic_options) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS DIAGNOSTIC DEBUGLEVEL=2",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    const auto round_trip = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(round_trip.size(), 1);
    ASSERT_EQ(round_trip[0], ".OPTIONS DIAGNOSTIC DEBUGLEVEL=2");
}

TEST(OptionParametersChecks, round_trip_new_package_options) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS PARSER MODEL_BINNING=0 SCALE=2.5",
        ".OPTIONS LINSOL-AC TYPE=KLU",
        ".OPTIONS LOCA MAX_NUM_STARTS=4 MIN_START=0.1",
        ".OPTIONS DIST STRATEGY=2",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    const auto round_trip = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(round_trip.size(), 4);
    ASSERT_EQ(round_trip[0], ".OPTIONS LINSOL-AC TYPE=KLU");
    ASSERT_EQ(round_trip[1], ".OPTIONS PARSER MODEL_BINNING=0 SCALE=2.5");
    ASSERT_EQ(round_trip[2], ".OPTIONS LOCA MAX_NUM_STARTS=4 MIN_START=0.1");
    ASSERT_EQ(round_trip[3], ".OPTIONS DIST STRATEGY=2");
}

// ========================================================================================
// equality
// ========================================================================================

TEST(OptionParametersChecks, equal_instances_compare_equal) {
    // arrange
    const OptionParameters a({{"TEMP", "25"}}, {}, {}, {}, {{"FFTOUT", "1"}});
    const OptionParameters b({{"TEMP", "25"}}, {}, {}, {}, {{"FFTOUT", "1"}});
    // act / assert
    ASSERT_TRUE(a == b);
}

TEST(OptionParametersChecks, differing_fft_options_compare_unequal) {
    // arrange
    const OptionParameters a({}, {}, {}, {}, {{"FFTOUT", "1"}});
    const OptionParameters b({}, {}, {}, {}, {{"FFTOUT", "0"}});
    // act / assert
    ASSERT_FALSE(a == b);
}

TEST(OptionParametersChecks, differing_diagnostic_options_compare_unequal) {
    // arrange
    const OptionParameters a({}, {}, {}, {}, {}, {{"DEBUGLEVEL", "1"}});
    const OptionParameters b({}, {}, {}, {}, {}, {{"DEBUGLEVEL", "2"}});
    // act / assert
    ASSERT_FALSE(a == b);
}

TEST(OptionParametersChecks, equal_instances_with_new_packages_compare_equal) {
    // arrange
    const OptionParameters a({}, {}, {}, {}, {}, {}, {{"MODEL_BINNING", "0"}}, {{"TYPE", "KLU"}}, {{"MAX_NUM_STARTS", "4"}}, {{"STRATEGY", "1"}});
    const OptionParameters b({}, {}, {}, {}, {}, {}, {{"MODEL_BINNING", "0"}}, {{"TYPE", "KLU"}}, {{"MAX_NUM_STARTS", "4"}}, {{"STRATEGY", "1"}});
    // act / assert
    ASSERT_TRUE(a == b);
}

TEST(OptionParametersChecks, differing_parser_options_compare_unequal) {
    // arrange
    const OptionParameters a({}, {}, {}, {}, {}, {}, {{"SCALE", "1.0"}});
    const OptionParameters b({}, {}, {}, {}, {}, {}, {{"SCALE", "2.0"}});
    // act / assert
    ASSERT_FALSE(a == b);
}

TEST(OptionParametersChecks, differing_linsol_ac_options_compare_unequal) {
    // arrange
    const OptionParameters a({}, {}, {}, {}, {}, {}, {}, {{"TYPE", "KLU"}});
    const OptionParameters b({}, {}, {}, {}, {}, {}, {}, {{"TYPE", "AZTECOO"}});
    // act / assert
    ASSERT_FALSE(a == b);
}

TEST(OptionParametersChecks, differing_loca_options_compare_unequal) {
    // arrange
    const OptionParameters a({}, {}, {}, {}, {}, {}, {}, {}, {{"MAX_NUM_STARTS", "4"}});
    const OptionParameters b({}, {}, {}, {}, {}, {}, {}, {}, {{"MAX_NUM_STARTS", "5"}});
    // act / assert
    ASSERT_FALSE(a == b);
}

TEST(OptionParametersChecks, differing_dist_options_compare_unequal) {
    // arrange
    const OptionParameters a({}, {}, {}, {}, {}, {}, {}, {}, {}, {{"STRATEGY", "0"}});
    const OptionParameters b({}, {}, {}, {}, {}, {}, {}, {}, {}, {{"STRATEGY", "1"}});
    // act / assert
    ASSERT_FALSE(a == b);
}
