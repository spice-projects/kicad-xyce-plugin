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
        ".OPTIONS DEVICE TEMP=25 GMIN=1e-12", ".OPTIONS TIMEINT RELTOL=1e-3 ABSTOL=1e-12", ".OPTIONS NONLIN MAXSTEP=10", ".OPTIONS LINSOL TYPE=AZTECOO", ".OPTIONS FFT FFT_ACCURATE=1 FFTOUT=1 FFT_MODE=0",
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
}

TEST(OptionParametersChecks, parse_fft_options) {
    // arrange
    const std::vector<std::string> directives = {
        ".OPTIONS FFT FFT_ACCURATE=0 FFTOUT=1 FFT_MODE=1",
    };
    // act
    const auto params = OptionParameters::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(params.device.size(), 0);
    ASSERT_EQ(params.fft.size(), 3);
    ASSERT_EQ(params.fft.at("FFT_ACCURATE"), "0");
    ASSERT_EQ(params.fft.at("FFTOUT"), "1");
    ASSERT_EQ(params.fft.at("FFT_MODE"), "1");
}

TEST(OptionParametersChecks, parse_fft_options_with_mixed_case) {
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
