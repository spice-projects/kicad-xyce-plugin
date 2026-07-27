// #include <gtest/gtest.h>

// #include "simulation_parameters/option_parameters.h"

// // ========================================================================================
// // from_xyce_directives
// // ========================================================================================

// TEST(OptionParametersChecks, parse_option_directives) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25 GMIN=1e-12",
//         ".OPTIONS TIMEINT RELTOL=1e-3 ABSTOL=1e-12",
//         ".OPTIONS NONLIN MAXSTEP=10",
//         ".OPTIONS LINSOL TYPE=AZTECOO",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 2);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.device.at("GMIN"), "1e-12");
//     ASSERT_EQ(params.timeint.size(), 2);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
//     ASSERT_EQ(params.timeint.at("ABSTOL"), "1e-12");
//     ASSERT_EQ(params.nonlin.size(), 1);
//     ASSERT_EQ(params.nonlin.at("MAXSTEP"), "10");
//     ASSERT_EQ(params.linsol.size(), 1);
//     ASSERT_EQ(params.linsol.at("TYPE"), "AZTECOO");
// }

// TEST(OptionParametersChecks, parse_single_option_directive) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
// }

// TEST(OptionParametersChecks, parse_empty_directives) {
//     // arrange
//     const std::vector<std::string> directives = {};
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 0);
//     ASSERT_EQ(params.timeint.size(), 0);
//     ASSERT_EQ(params.nonlin.size(), 0);
//     ASSERT_EQ(params.linsol.size(), 0);
// }

// // ========================================================================================
// // to_xyce_directives
// // ========================================================================================

// TEST(OptionParametersChecks, generate_directives) {
//     // arrange
//     const OptionParameters params({{"TEMP", "25"}}, {{"RELTOL", "1e-3"}}, {{"MAXSTEP", "10"}}, {{"TYPE", "AZTECOO"}});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 4);
//     ASSERT_EQ(directives[0], ".OPTIONS DEVICE TEMP=25");
//     ASSERT_EQ(directives[1], ".OPTIONS TIMEINT RELTOL=1e-3");
//     ASSERT_EQ(directives[2], ".OPTIONS NONLIN MAXSTEP=10");
//     ASSERT_EQ(directives[3], ".OPTIONS LINSOL TYPE=AZTECOO");
// }

// TEST(OptionParametersChecks, generate_empty_directives) {
//     // arrange
//     const OptionParameters params({}, {}, {}, {});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 0);
// }

// // ========================================================================================
// // round_trip
// // ========================================================================================

// TEST(OptionParametersChecks, round_trip_directives) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25",
//         ".OPTIONS NONLIN MAXSTEP=10",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     const auto round_trip = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(round_trip.size(), 2);
//     ASSERT_EQ(round_trip[0], ".OPTIONS DEVICE TEMP=25");
//     ASSERT_EQ(round_trip[1], ".OPTIONS NONLIN MAXSTEP=10");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_spaces) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE  TEMP=25  GMIN=1e-12",
//         ".OPTIONS TIMEINT  RELTOL=1e-3  ABSTOL=1e-12",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 2);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.device.at("GMIN"), "1e-12");
//     ASSERT_EQ(params.timeint.size(), 2);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
//     ASSERT_EQ(params.timeint.at("ABSTOL"), "1e-12");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_mixed_case) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".options device temp=25 gmin=1e-12",
//         ".OPTIONS TimeInt reltol=1e-3 abstol=1e-12",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 2);
//     ASSERT_EQ(params.device.at("temp"), "25");
//     ASSERT_EQ(params.device.at("gmin"), "1e-12");
//     ASSERT_EQ(params.timeint.size(), 2);
//     ASSERT_EQ(params.timeint.at("reltol"), "1e-3");
//     ASSERT_EQ(params.timeint.at("abstol"), "1e-12");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_special_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25.5 GMIN=1E-12",
//         ".OPTIONS TIMEINT RELTOL=0.001 ABSTOL=1.0E-12",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 2);
//     ASSERT_EQ(params.device.at("TEMP"), "25.5");
//     ASSERT_EQ(params.device.at("GMIN"), "1E-12");
//     ASSERT_EQ(params.timeint.size(), 2);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "0.001");
//     ASSERT_EQ(params.timeint.at("ABSTOL"), "1.0E-12");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_multiple_sections) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25",
//         ".OPTIONS TIMEINT RELTOL=1e-3",
//         ".OPTIONS NONLIN MAXSTEP=10",
//         ".OPTIONS LINSOL TYPE=AZTECOO",
//         ".OPTIONS OUTPUT PROBE=1",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
//     ASSERT_EQ(params.nonlin.size(), 1);
//     ASSERT_EQ(params.nonlin.at("MAXSTEP"), "10");
//     ASSERT_EQ(params.linsol.size(), 1);
//     ASSERT_EQ(params.linsol.at("TYPE"), "AZTECOO");
//     ASSERT_EQ(params.output.size(), 1);
//     ASSERT_EQ(params.output.at("PROBE"), "1");
// }

// TEST(OptionParametersChecks, generate_directives_with_multiple_sections) {
//     // arrange
//     const OptionParameters params({{"TEMP", "25"}}, {{"RELTOL", "1e-3"}}, {{"MAXSTEP", "10"}}, {{"TYPE", "AZTECOO"}}, {{"PROBE", "1"}});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 5);
//     ASSERT_NE(std::find(directives.begin(), directives.end(), ".OPTIONS DEVICE TEMP=25"), directives.end());
//     ASSERT_NE(std::find(directives.begin(), directives.end(), ".OPTIONS TIMEINT RELTOL=1e-3"), directives.end());
//     ASSERT_NE(std::find(directives.begin(), directives.end(), ".OPTIONS NONLIN MAXSTEP=10"), directives.end());
//     ASSERT_NE(std::find(directives.begin(), directives.end(), ".OPTIONS LINSOL TYPE=AZTECOO"), directives.end());
//     ASSERT_NE(std::find(directives.begin(), directives.end(), ".OPTIONS OUTPUT PROBE=1"), directives.end());
// }

// TEST(OptionParametersChecks, round_trip_directives_with_multiple_sections) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25",
//         ".OPTIONS TIMEINT RELTOL=1e-3",
//         ".OPTIONS NONLIN MAXSTEP=10",
//         ".OPTIONS LINSOL TYPE=AZTECOO",
//         ".OPTIONS OUTPUT PROBE=1",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     const auto round_trip = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(round_trip.size(), 5);
//     ASSERT_NE(std::find(round_trip.begin(), round_trip.end(), ".OPTIONS DEVICE TEMP=25"), round_trip.end());
//     ASSERT_NE(std::find(round_trip.begin(), round_trip.end(), ".OPTIONS TIMEINT RELTOL=1e-3"), round_trip.end());
//     ASSERT_NE(std::find(round_trip.begin(), round_trip.end(), ".OPTIONS NONLIN MAXSTEP=10"), round_trip.end());
//     ASSERT_NE(std::find(round_trip.begin(), round_trip.end(), ".OPTIONS LINSOL TYPE=AZTECOO"), round_trip.end());
//     ASSERT_NE(std::find(round_trip.begin(), round_trip.end(), ".OPTIONS OUTPUT PROBE=1"), round_trip.end());
// }

// TEST(OptionParametersChecks, parse_option_directives_with_empty_directive) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS",
//         ".OPTIONS DEVICE TEMP=25",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_non_option_directive) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".TRAN 1u 1m",
//         ".OPTIONS DEVICE TEMP=25",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_duplicate_keys) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25",
//         ".OPTIONS DEVICE TEMP=50",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "50");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_special_characters) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25+5",
//         ".OPTIONS TIMEINT RELTOL=1e-3*2",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25+5");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3*2");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_very_long_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25.12345678901234567890",
//         ".OPTIONS TIMEINT RELTOL=0.00000000000000000001",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25.12345678901234567890");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "0.00000000000000000001");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_very_short_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=1e-15",
//         ".OPTIONS TIMEINT RELTOL=1e-30",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "1e-15");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-30");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_very_large_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=1e15",
//         ".OPTIONS TIMEINT RELTOL=1e30",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "1e15");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e30");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_zero_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=0",
//         ".OPTIONS TIMEINT RELTOL=0",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "0");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "0");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_negative_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=-25",
//         ".OPTIONS TIMEINT RELTOL=-1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "-25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "-1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_positive_sign) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=+25",
//         ".OPTIONS TIMEINT RELTOL=+1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "+25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "+1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_fractional_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25.5",
//         ".OPTIONS TIMEINT RELTOL=0.5",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25.5");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "0.5");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_scientific_notation) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=2.5e1",
//         ".OPTIONS TIMEINT RELTOL=5.0e-1",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "2.5e1");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "5.0e-1");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_mixed_scientific_notation) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25E1",
//         ".OPTIONS TIMEINT RELTOL=0.5E-1",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25E1");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "0.5E-1");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_underscore_in_keys) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP_25=25",
//         ".OPTIONS TIMEINT REL_TOL=1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP_25"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("REL_TOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_number_in_keys) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP1=25",
//         ".OPTIONS TIMEINT RELTOL1=1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP1"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL1"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_special_characters_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25+5i",
//         ".OPTIONS TIMEINT RELTOL=1e-3*2pi",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25+5i");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3*2pi");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_braces_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP={25+5}",
//         ".OPTIONS TIMEINT RELTOL={1e-3*2}",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "{25+5}");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "{1e-3*2}");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_parentheses_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=(25+5)",
//         ".OPTIONS TIMEINT RELTOL=(1e-3*2)",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "(25+5)");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "(1e-3*2)");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_brackets_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=[25+5]",
//         ".OPTIONS TIMEINT RELTOL=[1e-3*2]",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "[25+5]");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "[1e-3*2]");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_quotes_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\"25+5\"",
//         ".OPTIONS TIMEINT RELTOL='1e-3*2'",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "\"25+5\"");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "'1e-3*2'");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_asterisk_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25*5",
//         ".OPTIONS TIMEINT RELTOL=1e-3*2",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25*5");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3*2");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_slash_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25/5",
//         ".OPTIONS TIMEINT RELTOL=1e-3/2",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25/5");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3/2");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_percent_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25%",
//         ".OPTIONS TIMEINT RELTOL=1e-3%",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25%");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3%");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_caret_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25^5",
//         ".OPTIONS TIMEINT RELTOL=1e-3^2",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25^5");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3^2");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_ampersand_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25&5",
//         ".OPTIONS TIMEINT RELTOL=1e-3&2",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25&5");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3&2");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_pipe_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=25|5",
//         ".OPTIONS TIMEINT RELTOL=1e-3|2",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25|5");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3|2");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_tilde_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=~25",
//         ".OPTIONS TIMEINT RELTOL=~1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "~25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "~1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_exclamation_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=!25",
//         ".OPTIONS TIMEINT RELTOL=!1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "!25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "!1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_question_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=?25",
//         ".OPTIONS TIMEINT RELTOL=?1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "?25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "?1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_colon_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=:25",
//         ".OPTIONS TIMEINT RELTOL=:1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), ":25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), ":1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_semicolon_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=;25",
//         ".OPTIONS TIMEINT RELTOL=;1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), ";25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), ";1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_comma_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=,25",
//         ".OPTIONS TIMEINT RELTOL=,1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), ",25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), ",1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_dot_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=.25",
//         ".OPTIONS TIMEINT RELTOL=.1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), ".25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), ".1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_space_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP= 25",
//         ".OPTIONS TIMEINT RELTOL= 1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_tab_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\t25",
//         ".OPTIONS TIMEINT RELTOL=\t1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_newline_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\n25",
//         ".OPTIONS TIMEINT RELTOL=\n1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with carriage_return_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\r25",
//         ".OPTIONS TIMEINT RELTOL=\r1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_form_feed_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\f25",
//         ".OPTIONS TIMEINT RELTOL=\f1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_vertical_tab_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\v25",
//         ".OPTIONS TIMEINT RELTOL=\v1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_null_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\025",
//         ".OPTIONS TIMEINT RELTOL=\01e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_bell_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\a25",
//         ".OPTIONS TIMEINT RELTOL=\a1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_backspace_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\b25",
//         ".OPTIONS TIMEINT RELTOL=\b1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_escape_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\e25",
//         ".OPTIONS TIMEINT RELTOL=\e1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_feed_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\f25",
//         ".OPTIONS TIMEINT RELTOL=\f1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_shift_in_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x1025",
//         ".OPTIONS TIMEINT RELTOL=\x101e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_shift_out_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x1125",
//         ".OPTIONS TIMEINT RELTOL=\x111e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_enq_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x0525",
//         ".OPTIONS TIMEINT RELTOL=\x051e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_ack_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x0625",
//         ".OPTIONS TIMEINT RELTOL=\x061e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_bell_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x0725",
//         ".OPTIONS TIMEINT RELTOL=\x071e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_cancel_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x1825",
//         ".OPTIONS TIMEINT RELTOL=\x181e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_end_of_medium_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x1925",
//         ".OPTIONS TIMEINT RELTOL=\x191e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_substitute_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x1a25",
//         ".OPTIONS TIMEINT RELTOL=\x1a1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_escape_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x1b25",
//         ".OPTIONS TIMEINT RELTOL=\x1b1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_space_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2025",
//         ".OPTIONS TIMEINT RELTOL=\x201e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_exclamation_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2125",
//         ".OPTIONS TIMEINT RELTOL=\x211e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_quotation_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2225",
//         ".OPTIONS TIMEINT RELTOL=\x221e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_number_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2325",
//         ".OPTIONS TIMEINT RELTOL=\x231e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_dollar_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2425",
//         ".OPTIONS TIMEINT RELTOL=\x241e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_percent_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2525",
//         ".OPTIONS TIMEINT RELTOL=\x251e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_ampersand_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2625",
//         ".OPTIONS TIMEINT RELTOL=\x261e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_apostrophe_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2725",
//         ".OPTIONS TIMEINT RELTOL=\x271e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_left_parenthesis_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2825",
//         ".OPTIONS TIMEINT RELTOL=\x281e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_right_parenthesis_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2925",
//         ".OPTIONS TIMEINT RELTOL=\x291e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_asterisk_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2a25",
//         ".OPTIONS TIMEINT RELTOL=\x2a1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_plus_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2b25",
//         ".OPTIONS TIMEINT RELTOL=\x2b1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_comma_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2c25",
//         ".OPTIONS TIMEINT RELTOL=\x2c1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_minus_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2d25",
//         ".OPTIONS TIMEINT RELTOL=\x2d1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_dot_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2e25",
//         ".OPTIONS TIMEINT RELTOL=\x2e1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_slash_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x2f25",
//         ".OPTIONS TIMEINT RELTOL=\x2f1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_zero_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3025",
//         ".OPTIONS TIMEINT RELTOL=\x301e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_one_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3125",
//         ".OPTIONS TIMEINT RELTOL=\x311e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_two_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3225",
//         ".OPTIONS TIMEINT RELTOL=\x321e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_three_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3325",
//         ".OPTIONS TIMEINT RELTOL=\x331e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_four_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3425",
//         ".OPTIONS TIMEINT RELTOL=\x341e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_five_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3525",
//         ".OPTIONS TIMEINT RELTOL=\x351e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_six_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3625",
//         ".OPTIONS TIMEINT RELTOL=\x361e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_seven_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3725",
//         ".OPTIONS TIMEINT RELTOL=\x371e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_eight_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3825",
//         ".OPTIONS TIMEINT RELTOL=\x381e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_nine_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3925",
//         ".OPTIONS TIMEINT RELTOL=\x391e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_colon_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3a25",
//         ".OPTIONS TIMEINT RELTOL=\x3a1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_semicolon_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3b25",
//         ".OPTIONS TIMEINT RELTOL=\x3b1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_less_than_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3c25",
//         ".OPTIONS TIMEINT RELTOL=\x3c1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_equal_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3d25",
//         ".OPTIONS TIMEINT RELTOL=\x3d1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_greater_than_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3e25",
//         ".OPTIONS TIMEINT RELTOL=\x3e1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_question_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x3f25",
//         ".OPTIONS TIMEINT RELTOL=\x3f1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_at_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4025",
//         ".OPTIONS TIMEINT RELTOL=\x401e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_A_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4125",
//         ".OPTIONS TIMEINT RELTOL=\x411e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_B_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4225",
//         ".OPTIONS TIMEINT RELTOL=\x421e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_C_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4325",
//         ".OPTIONS TIMEINT RELTOL=\x431e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_D_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4425",
//         ".OPTIONS TIMEINT RELTOL=\x441e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_E_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4525",
//         ".OPTIONS TIMEINT RELTOL=\x451e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_F_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4625",
//         ".OPTIONS TIMEINT RELTOL=\x461e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_G_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4725",
//         ".OPTIONS TIMEINT RELTOL=\x471e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_H_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4825",
//         ".OPTIONS TIMEINT RELTOL=\x481e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_I_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4925",
//         ".OPTIONS TIMEINT RELTOL=\x491e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_J_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4a25",
//         ".OPTIONS TIMEINT RELTOL=\x4a1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_K_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4b25",
//         ".OPTIONS TIMEINT RELTOL=\x4b1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_L_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4c25",
//         ".OPTIONS TIMEINT RELTOL=\x4c1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_M_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4d25",
//         ".OPTIONS TIMEINT RELTOL=\x4d1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_N_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4e25",
//         ".OPTIONS TIMEINT RELTOL=\x4e1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_O_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x4f25",
//         ".OPTIONS TIMEINT RELTOL=\x4f1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_P_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5025",
//         ".OPTIONS TIMEINT RELTOL=\x501e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_Q_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5125",
//         ".OPTIONS TIMEINT RELTOL=\x511e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_R_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5225",
//         ".OPTIONS TIMEINT RELTOL=\x521e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_S_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5325",
//         ".OPTIONS TIMEINT RELTOL=\x531e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_T_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5425",
//         ".OPTIONS TIMEINT RELTOL=\x541e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_U_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5525",
//         ".OPTIONS TIMEINT RELTOL=\x551e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_V_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5625",
//         ".OPTIONS TIMEINT RELTOL=\x561e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_W_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5725",
//         ".OPTIONS TIMEINT RELTOL=\x571e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_X_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5825",
//         ".OPTIONS TIMEINT RELTOL=\x581e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_Y_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5925",
//         ".OPTIONS TIMEINT RELTOL=\x591e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_Z_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5a25",
//         ".OPTIONS TIMEINT RELTOL=\x5a1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_left_bracket_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5b25",
//         ".OPTIONS TIMEINT RELTOL=\x5b1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_backslash_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5c25",
//         ".OPTIONS TIMEINT RELTOL=\x5c1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_right_bracket_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5d25",
//         ".OPTIONS TIMEINT RELTOL=\x5d1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_caret_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5e25",
//         ".OPTIONS TIMEINT RELTOL=\x5e1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_underscore_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x5f25",
//         ".OPTIONS TIMEINT RELTOL=\x5f1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_backtick_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6025",
//         ".OPTIONS TIMEINT RELTOL=\x601e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_a_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6125",
//         ".OPTIONS TIMEINT RELTOL=\x611e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_b_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6225",
//         ".OPTIONS TIMEINT RELTOL=\x621e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_c_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6325",
//         ".OPTIONS TIMEINT RELTOL=\x631e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_d_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6425",
//         ".OPTIONS TIMEINT RELTOL=\x641e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_e_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6525",
//         ".OPTIONS TIMEINT RELTOL=\x651e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_f_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6625",
//         ".OPTIONS TIMEINT RELTOL=\x661e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_g_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6725",
//         ".OPTIONS TIMEINT RELTOL=\x671e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_h_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6825",
//         ".OPTIONS TIMEINT RELTOL=\x681e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_i_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6925",
//         ".OPTIONS TIMEINT RELTOL=\x691e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_j_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6a25",
//         ".OPTIONS TIMEINT RELTOL=\x6a1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_k_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6b25",
//         ".OPTIONS TIMEINT RELTOL=\x6b1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_l_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6c25",
//         ".OPTIONS TIMEINT RELTOL=\x6c1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_m_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6d25",
//         ".OPTIONS TIMEINT RELTOL=\x6d1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_n_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6e25",
//         ".OPTIONS TIMEINT RELTOL=\x6e1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_o_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x6f25",
//         ".OPTIONS TIMEINT RELTOL=\x6f1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_p_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7025",
//         ".OPTIONS TIMEINT RELTOL=\x701e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_q_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7125",
//         ".OPTIONS TIMEINT RELTOL=\x711e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_r_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7225",
//         ".OPTIONS TIMEINT RELTOL=\x721e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_s_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7325",
//         ".OPTIONS TIMEINT RELTOL=\x731e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_t_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7425",
//         ".OPTIONS TIMEINT RELTOL=\x741e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_u_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7525",
//         ".OPTIONS TIMEINT RELTOL=\x751e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_v_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7625",
//         ".OPTIONS TIMEINT RELTOL=\x761e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_w_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7725",
//         ".OPTIONS TIMEINT RELTOL=\x771e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_x_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7825",
//         ".OPTIONS TIMEINT RELTOL=\x781e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_y_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7925",
//         ".OPTIONS TIMEINT RELTOL=\x791e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_z_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7a25",
//         ".OPTIONS TIMEINT RELTOL=\x7a1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_left_brace_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7b25",
//         ".OPTIONS TIMEINT RELTOL=\x7b1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_bar_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7c25",
//         ".OPTIONS TIMEINT RELTOL=\x7c1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_right_brace_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7d25",
//         ".OPTIONS TIMEINT RELTOL=\x7d1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_tilde_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7e25",
//         ".OPTIONS TIMEINT RELTOL=\x7e1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// TEST(OptionParametersChecks, parse_option_directives_with_del_in_values) {
//     // arrange
//     const std::vector<std::string> directives = {
//         ".OPTIONS DEVICE TEMP=\x7f25",
//         ".OPTIONS TIMEINT RELTOL=\x7f1e-3",
//     };
//     // act
//     const auto params = OptionParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_EQ(params.device.size(), 1);
//     ASSERT_EQ(params.device.at("TEMP"), "25");
//     ASSERT_EQ(params.timeint.size(), 1);
//     ASSERT_EQ(params.timeint.at("RELTOL"), "1e-3");
// }

// // ========================================================================================
// // equality operator
// // ========================================================================================

// TEST(OptionParametersChecks, equality_operator_equal_params) {
//     // arrange
//     const OptionParameters params1({{"TEMP", "25"}}, {{"RELTOL", "1e-3"}}, {{"MAXSTEP", "10"}}, {{"TYPE", "AZTECOO"}});
//     const OptionParameters params2({{"TEMP", "25"}}, {{"RELTOL", "1e-3"}}, {{"MAXSTEP", "10"}}, {{"TYPE", "AZTECOO"}});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_TRUE(result);
// }

// TEST(OptionParametersChecks, equality_operator_different_device) {
//     // arrange
//     const OptionParameters params1({{"TEMP", "25"}}, {}, {}, {});
//     const OptionParameters params2({{"TEMP", "30"}}, {}, {}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(OptionParametersChecks, equality_operator_different_timeint) {
//     // arrange
//     const OptionParameters params1({}, {{"RELTOL", "1e-3"}}, {}, {});
//     const OptionParameters params2({}, {{"RELTOL", "1e-2"}}, {}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(OptionParametersChecks, equality_operator_different_nonlin) {
//     // arrange
//     const OptionParameters params1({}, {}, {{"MAXSTEP", "10"}}, {});
//     const OptionParameters params2({}, {}, {{"MAXSTEP", "20"}}, {});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(OptionParametersChecks, equality_operator_different_linsol) {
//     // arrange
//     const OptionParameters params1({}, {}, {}, {{"TYPE", "AZTECOO"}});
//     const OptionParameters params2({}, {}, {}, {{"TYPE", "UMFPACK"}});
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }
