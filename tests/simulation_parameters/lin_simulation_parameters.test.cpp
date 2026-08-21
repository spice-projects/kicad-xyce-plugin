#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "simulation/lin_simulation_parameters.h"

namespace
{
    // locate the .LIN directive within a directive list
    auto find_lin_directive(const std::vector<std::string>& directives) -> std::string {
        const auto lin_line = std::find_if(directives.begin(), directives.end(), [](const std::string& d) { return d.rfind(".LIN", 0) == 0; });
        EXPECT_NE(lin_line, directives.end());
        return lin_line == directives.end() ? std::string{} : *lin_line;
    }
} // namespace

// ========================================================================================
// from_xyce_directives — LINTYPE keyword
// ========================================================================================

TEST(LinSimulationParametersChecks, parses_lintype_keyword) {
    // arrange / act
    const auto result = LinSimulationParameters::from_xyce_directives({".AC LIN 100 1 1MEG", ".LIN LINTYPE=Z"});
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->lintype, "Z");
}

TEST(LinSimulationParametersChecks, parses_lintype_keyword_lowercase) {
    // arrange / act
    const auto result = LinSimulationParameters::from_xyce_directives({".AC LIN 100 1 1MEG", ".LIN lintype=z"});
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->lintype, "Z");
}

TEST(LinSimulationParametersChecks, parses_type_keyword_backward_compatibility) {
    // arrange / act — TYPE= is accepted for backward compatibility
    const auto result = LinSimulationParameters::from_xyce_directives({".AC LIN 100 1 1MEG", ".LIN TYPE=Y"});
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->lintype, "Y");
}

// ========================================================================================
// from_xyce_directives — FILENAME synonym
// ========================================================================================

TEST(LinSimulationParametersChecks, parses_filename_synonym) {
    // arrange / act
    const auto result = LinSimulationParameters::from_xyce_directives({".AC LIN 100 1 1MEG", ".LIN FILENAME=foo.s2p"});
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->file, "foo.s2p");
}

TEST(LinSimulationParametersChecks, file_wins_over_filename_when_both_given) {
    // arrange / act — FILE= appears after FILENAME=
    const auto result = LinSimulationParameters::from_xyce_directives({".AC LIN 100 1 1MEG", ".LIN FILENAME=foo.s2p FILE=bar.s2p"});
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->file, "bar.s2p");
}

TEST(LinSimulationParametersChecks, file_wins_over_filename_when_both_given_reversed) {
    // arrange / act — FILE= appears before FILENAME=
    const auto result = LinSimulationParameters::from_xyce_directives({".AC LIN 100 1 1MEG", ".LIN FILE=bar.s2p FILENAME=foo.s2p"});
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->file, "bar.s2p");
}

// ========================================================================================
// to_xyce_directives — keyword emission
// ========================================================================================

TEST(LinSimulationParametersChecks, emits_lintype_keyword) {
    // arrange
    const LinSimulationParameters params(true, "TOUCHSTONE2", "Z", "RI", "", "", "", "LIN", "100", "1", "1MEG", "", std::nullopt);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    const std::string lin_line = find_lin_directive(directives);
    EXPECT_EQ(lin_line, ".LIN LINTYPE=Z");
}

TEST(LinSimulationParametersChecks, omits_lintype_keyword_for_default) {
    // arrange — default lintype "S" must not be emitted
    const LinSimulationParameters params(true, "TOUCHSTONE2", "S", "RI", "", "", "", "LIN", "100", "1", "1MEG", "", std::nullopt);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    const std::string lin_line = find_lin_directive(directives);
    EXPECT_EQ(lin_line.find("LINTYPE="), std::string::npos);
}

TEST(LinSimulationParametersChecks, emits_file_keyword) {
    // arrange
    const LinSimulationParameters params(true, "TOUCHSTONE2", "S", "RI", "out.s2p", "", "", "LIN", "100", "1", "1MEG", "", std::nullopt);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    const std::string lin_line = find_lin_directive(directives);
    EXPECT_NE(lin_line.find("FILE=out.s2p"), std::string::npos);
}

// ========================================================================================
// round-trip tests
// ========================================================================================

TEST(LinSimulationParametersChecks, lintype_round_trips_through_directives) {
    // arrange
    const LinSimulationParameters input(true, "TOUCHSTONE2", "Z", "RI", "", "", "", "LIN", "100", "1", "1MEG", "", std::nullopt);
    // act
    const auto directives = input.to_xyce_directives(NetlistTopology{});
    const auto output = LinSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(output.has_value());
    EXPECT_EQ(output->lintype, "Z");
}

TEST(LinSimulationParametersChecks, filename_round_trips_through_directives) {
    // arrange — .LIN FILENAME=foo on input, emitted as FILE=, parsed back as file
    const auto input = LinSimulationParameters::from_xyce_directives({".AC LIN 100 1 1MEG", ".LIN FILENAME=foo.s2p"});
    ASSERT_TRUE(input.has_value());
    // act
    const auto directives = input->to_xyce_directives(NetlistTopology{});
    const auto output = LinSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(output.has_value());
    EXPECT_EQ(output->file, "foo.s2p");
}

TEST(LinSimulationParametersChecks, reference_guide_lintype_round_trip) {
    // arrange — RG 2.1.17 style directive: .LIN LINTYPE=Z DATAFORMAT=MA FILENAME=foo
    const auto input = LinSimulationParameters::from_xyce_directives({".AC LIN 100 1 1MEG", ".LIN LINTYPE=Z DATAFORMAT=MA FILENAME=foo"});
    ASSERT_TRUE(input.has_value());
    // act
    const auto directives = input->to_xyce_directives(NetlistTopology{});
    const auto output = LinSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(output.has_value());
    EXPECT_EQ(output->lintype, "Z");
    EXPECT_EQ(output->dataformat, "MA");
    EXPECT_EQ(output->file, "foo");
}
