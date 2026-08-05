#include <algorithm>

#include <gtest/gtest.h>

#include "simulation_parameters/simulation_config.h"

TEST(SimulationConfigReplaceGroundChecks, from_xyce_directives_parses_disabled_statement) {
    // arrange / act
    const auto config = SimulationConfig::from_xyce_directives({".OP", ".PREPROCESS REPLACEGROUND FALSE"});
    // assert
    EXPECT_FALSE(config.replace_ground);
}

TEST(SimulationConfigReplaceGroundChecks, from_xyce_directives_defaults_to_true_without_statement) {
    // arrange / act
    const auto config = SimulationConfig::from_xyce_directives({".OP"});
    // assert
    EXPECT_TRUE(config.replace_ground);
}

TEST(SimulationConfigReplaceGroundChecks, to_xyce_directives_emits_disabled_statement) {
    // arrange
    const SimulationConfig config("OP", OpSimulationParameters(false, false, false, {}, "", "", false, "NODESET", "", {}, {}, std::nullopt), {}, {}, OptionParameters({}, {}, {}, {}, {}), {}, false);
    // act
    const auto directives = config.to_xyce_directives(NetlistTopology{});
    // assert
    const auto found = std::find(directives.begin(), directives.end(), ".PREPROCESS REPLACEGROUND FALSE");
    ASSERT_NE(found, directives.end());
}

TEST(SimulationConfigReplaceGroundChecks, disabled_state_round_trips_through_directives) {
    // arrange
    const SimulationConfig input("OP", OpSimulationParameters(false, false, false, {}, "", "", false, "NODESET", "", {}, {}, std::nullopt), {}, {}, OptionParameters({}, {}, {}, {}, {}), {}, false);
    // act
    const auto directives = input.to_xyce_directives(NetlistTopology{});
    const auto output = SimulationConfig::from_xyce_directives(directives);
    // assert
    EXPECT_FALSE(output.replace_ground);
}

TEST(SimulationConfigReplaceGroundChecks, enabled_state_round_trips_through_directives) {
    // arrange
    const SimulationConfig input("OP", OpSimulationParameters(false, false, false, {}, "", "", false, "NODESET", "", {}, {}, std::nullopt), {}, {}, OptionParameters({}, {}, {}, {}, {}), {}, true);
    // act
    const auto directives = input.to_xyce_directives(NetlistTopology{});
    const auto output = SimulationConfig::from_xyce_directives(directives);
    // assert
    EXPECT_TRUE(output.replace_ground);
}
