// #include <gtest/gtest.h>

// #include "simulation_parameters/noise_simulation_parameters.h"

// // ========================================================================================
// // DeviceNoiseOperator
// // ========================================================================================

// TEST(DeviceNoiseOperatorChecks, create_device_noise_operator) {
//     // arrange
//     const DeviceNoiseOperator operator1("DNI", "R1", "");
//     // act/assert
//     ASSERT_EQ(operator1.type, "DNI");
//     ASSERT_EQ(operator1.node, "R1");
//     ASSERT_EQ(operator1.source, "");
// }

// TEST(DeviceNoiseOperatorChecks, create_device_noise_operator_with_noise_source) {
//     // arrange
//     const DeviceNoiseOperator operator1("DNO", "Q2", "FLICKER");
//     // act/assert
//     ASSERT_EQ(operator1.type, "DNO");
//     ASSERT_EQ(operator1.node, "Q2");
//     ASSERT_EQ(operator1.source, "FLICKER");
// }

// TEST(DeviceNoiseOperatorChecks, equality_operator_equal) {
//     // arrange
//     const DeviceNoiseOperator op1("DNI", "R1", "");
//     const DeviceNoiseOperator op2("DNI", "R1", "");
//     // act
//     const bool result = op1 == op2;
//     // assert
//     ASSERT_TRUE(result);
// }

// TEST(DeviceNoiseOperatorChecks, equality_operator_different_type) {
//     // arrange
//     const DeviceNoiseOperator op1("DNI", "R1", "");
//     const DeviceNoiseOperator op2("DNO", "R1", "");
//     // act
//     const bool result = op1 == op2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(DeviceNoiseOperatorChecks, equality_operator_different_node) {
//     // arrange
//     const DeviceNoiseOperator op1("DNI", "R1", "");
//     const DeviceNoiseOperator op2("DNI", "Q2", "");
//     // act
//     const bool result = op1 == op2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(DeviceNoiseOperatorChecks, equality_operator_different_source) {
//     // arrange
//     const DeviceNoiseOperator op1("DNI", "R1", "");
//     const DeviceNoiseOperator op2("DNI", "R1", "FLICKER");
//     // act
//     const bool result = op1 == op2;
//     // assert
//     ASSERT_FALSE(result);
// }

// // ========================================================================================
// // from_xyce_directives
// // ========================================================================================

// TEST(NoiseSimulationParametersChecks, parse_lin_sweep) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 100 1 1MEG"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->output_node, "5");
//     ASSERT_EQ(result->ref_node, "");
//     ASSERT_EQ(result->source_name, "V1");
//     ASSERT_EQ(result->sweep_type, "LIN");
//     ASSERT_EQ(result->num_points_value, "100");
//     ASSERT_EQ(result->start_freq_value_freq_value, "1");
//     ASSERT_EQ(result->end_freq_value_freq_value, "1MEG");
// }

// TEST(NoiseSimulationParametersChecks, parse_output_with_ref_node) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5,3) V1 LIN 100 1 1MEG"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->output_node, "5");
//     ASSERT_EQ(result->ref_node, "3");
// }

// TEST(NoiseSimulationParametersChecks, parse_dec_sweep) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(out) Vin DEC 10 1k 100MEG"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->sweep_type, "DEC");
//     ASSERT_EQ(result->num_points_value, "10");
//     ASSERT_EQ(result->start_freq_value_freq_value, "1k");
//     ASSERT_EQ(result->end_freq_value_freq_value, "100MEG");
// }

// TEST(NoiseSimulationParametersChecks, parse_data_sweep) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(out) Vin DATA=myTable"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->sweep_type, "DATA");
//     ASSERT_EQ(result->data_table_name, "myTable");
// }

// TEST(NoiseSimulationParametersChecks, parse_replaceground_true) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".PREPROCESS REPLACEGROUND TRUE"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->replace_ground, true);
// }

// TEST(NoiseSimulationParametersChecks, no_noise_directive_returns_none) {
//     // arrange
//     const std::vector<std::string> directives = {".TRAN 1u 1m"};
//     // act
//     const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_FALSE(result.has_value());
// }

// // ========================================================================================
// // to_xyce_directives
// // ========================================================================================

// TEST(NoiseSimulationParametersChecks, generates_lin_directive) {
//     // arrange
//     const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 1);
//     ASSERT_EQ(directives[0], ".NOISE V(5) V1 LIN 100 1 1MEG");
// }

// TEST(NoiseSimulationParametersChecks, generates_with_ref_node) {
//     // arrange
//     const NoiseSimulationParameters params("5", "3", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 1);
//     ASSERT_EQ(directives[0], ".NOISE V(5,3) V1 LIN 100 1 1MEG");
// }

// TEST(NoiseSimulationParametersChecks, generates_dec_directive) {
//     // arrange
//     const NoiseSimulationParameters params("out", "", "Vin", "1k", "100MEG", "10", "DEC", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 1);
//     ASSERT_EQ(directives[0], ".NOISE V(out) Vin DEC 10 1k 100MEG");
// }

// TEST(NoiseSimulationParametersChecks, generates_data_directive) {
//     // arrange
//     const NoiseSimulationParameters params("out", "", "Vin", "1k", "100MEG", "10", "DATA", std::vector<DeviceNoiseOperator>{}, "myTable", false, std::nullopt);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 1);
//     ASSERT_EQ(directives[0], ".NOISE V(out) Vin DATA=myTable");
// }

// TEST(NoiseSimulationParametersChecks, generates_with_replace_ground) {
//     // arrange
//     const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "10", "LIN", std::vector<DeviceNoiseOperator>{}, "", true, std::nullopt);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".PREPROCESS REPLACEGROUND TRUE");
//     ASSERT_EQ(directives[1], ".NOISE V(5) V1 LIN 10 1 1MEG");
// }

// TEST(NoiseSimulationParametersChecks, generates_with_print_noise_directive) {
//     // arrange
//     const PrintParameters print_params("NOISE", "", "", {"V(OUT)"}, {});
//     const NoiseSimulationParameters params("OUT", "", "V1", "1", "1MEG", "10", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, print_params);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".NOISE V(OUT) V1 LIN 10 1 1MEG");
//     ASSERT_EQ(directives[1], ".PRINT NOISE V(OUT)");
// }

// TEST(NoiseSimulationParametersChecks, parses_print_noise_directive) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT NOISE V(OUT)"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_TRUE(result->print_parameters.has_value());
//     ASSERT_EQ(result->print_parameters->print_type, "NOISE");
//     ASSERT_EQ(result->print_parameters->output_variables.size(), 1);
//     ASSERT_EQ(result->print_parameters->output_variables[0], "V(OUT)");
// }

// TEST(NoiseSimulationParametersChecks, ignores_non_noise_print_directive) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT TRAN V(OUT)"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_FALSE(result->print_parameters.has_value());
// }

// TEST(NoiseSimulationParametersChecks, parses_empty_directives_returns_none) {
//     // arrange
//     const std::vector<std::string> directives = {};
//     // act
//     const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_FALSE(result.has_value());
// }

// TEST(NoiseSimulationParametersChecks, parses_non_noise_directives_return_none) {
//     // arrange
//     const std::vector<std::string> directives = {".TRAN 1ns 1ms"};
//     // act
//     const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_FALSE(result.has_value());
// }

// TEST(NoiseSimulationParametersChecks, generates_with_device_noise_operators) {
//     // arrange
//     const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{DeviceNoiseOperator("DNI", "R1", "")}, "", false, std::nullopt);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 1);
//     ASSERT_EQ(directives[0], ".NOISE V(5) V1 LIN 100 1 1MEG");
// }

// TEST(NoiseSimulationParametersChecks, generates_with_print_parameters) {
//     // arrange
//     const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, PrintParameters("TRAN", "", "", {"V(OUT)"}, {}));
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".NOISE V(5) V1 LIN 100 1 1MEG");
//     ASSERT_EQ(directives[1], ".PRINT TRAN V(OUT)");
// }

// // ========================================================================================
// // equality operator
// // ========================================================================================

// TEST(NoiseSimulationParametersChecks, equality_operator_equal_params) {
//     // arrange
//     const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     const NoiseSimulationParameters params2("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_TRUE(result);
// }

// TEST(NoiseSimulationParametersChecks, equality_operator_different_output_node) {
//     // arrange
//     const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     const NoiseSimulationParameters params2("6", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(NoiseSimulationParametersChecks, equality_operator_different_ref_node) {
//     // arrange
//     const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     const NoiseSimulationParameters params2("5", "3", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(NoiseSimulationParametersChecks, equality_operator_different_source_name) {
//     // arrange
//     const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     const NoiseSimulationParameters params2("5", "", "V2", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(NoiseSimulationParametersChecks, equality_operator_different_sweep_type) {
//     // arrange
//     const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     const NoiseSimulationParameters params2("5", "", "V1", "1", "1MEG", "100", "DEC", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(NoiseSimulationParametersChecks, equality_operator_different_replace_ground) {
//     // arrange
//     const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     const NoiseSimulationParameters params2("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", true, std::nullopt);
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// TEST(NoiseSimulationParametersChecks, equality_operator_different_print_parameters) {
//     // arrange
//     const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, PrintParameters("TRAN", "", "", {"V(OUT)"}, {}));
//     const NoiseSimulationParameters params2("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, PrintParameters("TRAN", "", "", {"I(V1)"}, {}));
//     // act
//     const bool result = params1 == params2;
//     // assert
//     ASSERT_FALSE(result);
// }

// // ========================================================================================
// // Additional tests for replace_ground
// // ========================================================================================

// TEST(NoiseSimulationParametersChecks, generates_with_replace_ground_false) {
//     // arrange
//     const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 1);
//     ASSERT_EQ(directives[0], ".NOISE V(5) V1 LIN 100 1 1MEG");
// }

// TEST(NoiseSimulationParametersChecks, generates_with_replace_ground_true) {
//     // arrange
//     const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", true, std::nullopt);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".PREPROCESS REPLACEGROUND TRUE");
//     ASSERT_EQ(directives[1], ".NOISE V(5) V1 LIN 100 1 1MEG");
// }

// // ========================================================================================
// // Tests for from_xyce_directives edge cases
// // ========================================================================================

// TEST(NoiseSimulationParametersChecks, empty_directives_returns_none) {
//     // arrange
//     const std::vector<std::string> directives = {};
//     // act
//     const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_FALSE(result.has_value());
// }

// TEST(NoiseSimulationParametersChecks, non_noise_directives_return_none) {
//     // arrange
//     const std::vector<std::string> directives = {".TRAN 1ns 1ms"};
//     // act
//     const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_FALSE(result.has_value());
// }

// TEST(NoiseSimulationParametersChecks, serializes_print_noise_directive) {
//     // arrange
//     const PrintParameters print_params("NOISE", "", "", {"V(OUT)"}, {});
//     const NoiseSimulationParameters params("OUT", "V1", "LIN", "10", "1", "1MEG", "100", std::vector<DeviceNoiseOperator>{}, "", false, print_params);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[0], ".NOISE V(OUT) V1 LIN 100 1 1MEG");
//     ASSERT_EQ(directives[1], ".PRINT NOISE V(OUT)");
// }

// TEST(NoiseSimulationParametersChecks, ignores_empty_directive) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({"", ".NOISE V(5) V1 LIN 10 1 1MEG"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->output_node, "5");
// }

// TEST(NoiseSimulationParametersChecks, parses_bare_output_node) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE 5 V1 LIN 10 1 1MEG"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->output_node, "5");
//     ASSERT_EQ(result->ref_node, "");
// }

// TEST(NoiseSimulationParametersChecks, parses_noise_directive_alone) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->output_node, "");
// }

// TEST(NoiseSimulationParametersChecks, parses_noise_without_sweep_type) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->output_node, "5");
//     ASSERT_EQ(result->source_name, "V1");
//     ASSERT_EQ(result->sweep_type, "LIN");
// }

// TEST(NoiseSimulationParametersChecks, parses_implicit_lin_sweep) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 100 1 1MEG"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->sweep_type, "LIN");
//     ASSERT_EQ(result->num_points_value, "100");
//     ASSERT_EQ(result->start_freq_value, "1");
//     ASSERT_EQ(result->end_freq_value, "1MEG");
// }

// TEST(NoiseSimulationParametersChecks, parses_device_noise_operators) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT NOISE INOISE ONOISE DNI(R1) DNO(R2)"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->device_noise_operators.size(), 2);
//     ASSERT_EQ(result->device_noise_operators[0].device_name, "R1");
//     ASSERT_EQ(result->device_noise_operators[0].type, "DNI");
//     ASSERT_EQ(result->device_noise_operators[1].device_name, "R2");
//     ASSERT_EQ(result->device_noise_operators[1].type, "DNO");
// }

// TEST(NoiseSimulationParametersChecks, parses_device_noise_operators_with_noise_source) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT NOISE DNI(Q2,FLICKER) DNO(Q2,THERMAL)"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->device_noise_operators.size(), 2);
//     ASSERT_EQ(result->device_noise_operators[0].device_name, "Q2");
//     ASSERT_EQ(result->device_noise_operators[0].type, "DNI");
//     ASSERT_EQ(result->device_noise_operators[0].source, "FLICKER");
//     ASSERT_EQ(result->device_noise_operators[1].device_name, "Q2");
//     ASSERT_EQ(result->device_noise_operators[1].type, "DNO");
//     ASSERT_EQ(result->device_noise_operators[1].source, "THERMAL");
// }

// TEST(NoiseSimulationParametersChecks, serializes_device_noise_operators) {
//     // arrange
//     const std::vector<DeviceNoiseOperator> device_operators = {DeviceNoiseOperator("R1", "DNI", ""), DeviceNoiseOperator("R2", "DNO", "")};
//     const PrintParameters print_params("NOISE", "", "", {"INOISE", "ONOISE"}, {});
//     const NoiseSimulationParameters params("5", "V1", "LIN", "10", "1", "1MEG", "100", device_operators, "", false, print_params);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_NE(directives[1].find("INOISE ONOISE DNI(R1) DNO(R2)"), std::string::npos);
// }

// TEST(NoiseSimulationParametersChecks, serializes_device_noise_operators_with_noise_source) {
//     // arrange
//     const std::vector<DeviceNoiseOperator> device_operators = {DeviceNoiseOperator("Q2", "DNI", "FLICKER")};
//     const PrintParameters print_params("NOISE", "", "", {}, {});
//     const NoiseSimulationParameters params("5", "V1", "LIN", "10", "1", "1MEG", "100", device_operators, "", false, print_params);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_NE(directives[1].find("DNI(Q2,FLICKER)"), std::string::npos);
// }

// TEST(NoiseSimulationParametersChecks, omits_print_when_no_output_variables) {
//     // arrange
//     const PrintParameters print_params("NOISE", "", "", {}, {});
//     const NoiseSimulationParameters params("5", "V1", "LIN", "10", "1", "1MEG", "100", std::vector<DeviceNoiseOperator>{}, "", false, print_params);
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 1);
//     ASSERT_EQ(directives[0], ".NOISE V(5) V1 LIN 100 1 1MEG");
// }

// // ========================================================================================
// // Tests for measure parameters
// // ========================================================================================

// TEST(NoiseSimulationParametersChecks, parses_single_measure_directive) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".MEASURE NOISE noise_at_1k FIND INOISE AT=1k"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->measure_parameters.size(), 1);
//     ASSERT_EQ(result->measure_parameters[0].result_name, "noise_at_1k");
//     ASSERT_EQ(result->measure_parameters[0].measure_type, "FIND");
//     ASSERT_EQ(result->measure_parameters[0].analysis_type, "NOISE");
//     ASSERT_EQ(result->measure_parameters[0].variable, "INOISE");
// }

// TEST(NoiseSimulationParametersChecks, parses_multiple_measure_directives) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".MEASURE NOISE noise_at_1k FIND INOISE AT=1k", ".MEASURE NOISE onoise_at_10k FIND ONOISE AT=10k"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->measure_parameters.size(), 2);
//     ASSERT_EQ(result->measure_parameters[0].result_name, "noise_at_1k");
//     ASSERT_EQ(result->measure_parameters[1].result_name, "onoise_at_10k");
// }

// TEST(NoiseSimulationParametersChecks, ignores_non_noise_measure_directive) {
//     // arrange / act
//     const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".MEASURE TRAN avg_out AVG V(OUT)"});
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->measure_parameters.size(), 0);
// }

// TEST(NoiseSimulationParametersChecks, emits_single_measure_directive) {
//     // arrange
//     const MeasureEntry measure("noise_at_1k", "FIND", "NOISE", "INOISE", "", "", "", "", "1k");
//     const NoiseSimulationParameters params("5", "V1", "LIN", "10", "1", "1MEG", "100", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt, {measure});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 2);
//     ASSERT_EQ(directives[1], ".MEASURE NOISE noise_at_1k FIND INOISE AT=1k");
// }

// TEST(NoiseSimulationParametersChecks, emits_multiple_measure_directives) {
//     // arrange
//     const MeasureEntry measure1("noise_at_1k", "FIND", "NOISE", "INOISE", "", "", "", "", "1k");
//     const MeasureEntry measure2("onoise_at_10k", "FIND", "NOISE", "ONOISE", "", "", "", "", "10k");
//     const NoiseSimulationParameters params("5", "V1", "LIN", "10", "1", "1MEG", "100", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt, {measure1, measure2});
//     // act
//     const auto directives = params.to_xyce_directives();
//     // assert
//     ASSERT_EQ(directives.size(), 3);
//     ASSERT_EQ(directives[1], ".MEASURE NOISE noise_at_1k FIND INOISE AT=1k");
//     ASSERT_EQ(directives[2], ".MEASURE NOISE onoise_at_10k FIND ONOISE AT=10k");
// }

// TEST(NoiseSimulationParametersChecks, measure_round_trip) {
//     // arrange
//     const MeasureEntry measure("noise_at_1k", "FIND", "NOISE", "INOISE", "", "", "", "", "1k");
//     const NoiseSimulationParameters params("5", "V1", "LIN", "10", "1", "1MEG", "100", std::vector<DeviceNoiseOperator>{}, "", false, std::nullopt, {measure});
//     // act
//     const auto directives = params.to_xyce_directives();
//     const auto reparsed = NoiseSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_TRUE(reparsed.has_value());
//     ASSERT_EQ(reparsed->measure_parameters.size(), 1);
//     ASSERT_EQ(reparsed->measure_parameters[0].result_name, "noise_at_1k");
//     ASSERT_EQ(reparsed->measure_parameters[0].measure_type, "FIND");
//     ASSERT_EQ(reparsed->measure_parameters[0].analysis_type, "NOISE");
//     ASSERT_EQ(reparsed->measure_parameters[0].variable, "INOISE");
//     ASSERT_EQ(reparsed->measure_parameters[0].at_val, "1k");
// }

// // ========================================================================================
// // Tests for reference guide examples
// // ========================================================================================

// TEST(NoiseSimulationParametersChecks, reference_guide_example_lin_sweep) {
//     // arrange
//     const std::vector<std::string> directives = {".NOISE V(5) VIN LIN 101 100Hz 200Hz"};
//     // act
//     const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->output_node, "5");
//     ASSERT_EQ(result->ref_node, "");
//     ASSERT_EQ(result->source_name, "VIN");
//     ASSERT_EQ(result->sweep_type, "LIN");
//     ASSERT_EQ(result->num_points_value, "101");
//     ASSERT_EQ(result->start_freq_value, "100Hz");
//     ASSERT_EQ(result->end_freq_value, "200Hz");
//     // verify the directive contains the expected noise line
//     const auto generated = result->to_xyce_directives();
//     ASSERT_EQ(generated.size(), 1);
//     ASSERT_EQ(generated[0], ".NOISE V(5) VIN LIN 101 100Hz 200Hz");
// }

// TEST(NoiseSimulationParametersChecks, reference_guide_example_oct_sweep_with_ref) {
//     // arrange
//     const std::vector<std::string> directives = {".NOISE V(5,3) V1 OCT 10 1kHz 16kHz"};
//     // act
//     const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->output_node, "5");
//     ASSERT_EQ(result->ref_node, "3");
//     ASSERT_EQ(result->source_name, "V1");
//     ASSERT_EQ(result->sweep_type, "OCT");
//     ASSERT_EQ(result->num_points_value, "10");
//     ASSERT_EQ(result->start_freq_value, "1kHz");
//     ASSERT_EQ(result->end_freq_value, "16kHz");
//     // verify the directive contains the expected noise line
//     const auto generated = result->to_xyce_directives();
//     ASSERT_EQ(generated.size(), 1);
//     ASSERT_EQ(generated[0], ".NOISE V(5,3) V1 OCT 10 1kHz 16kHz");
// }

// TEST(NoiseSimulationParametersChecks, reference_guide_example_dec_sweep) {
//     // arrange
//     const std::vector<std::string> directives = {".NOISE V(4) V2 DEC 20 1MEG 100MEG"};
//     // act
//     const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->output_node, "4");
//     ASSERT_EQ(result->ref_node, "");
//     ASSERT_EQ(result->source_name, "V2");
//     ASSERT_EQ(result->sweep_type, "DEC");
//     ASSERT_EQ(result->num_points_value, "20");
//     ASSERT_EQ(result->start_freq_value, "1MEG");
//     ASSERT_EQ(result->end_freq_value, "100MEG");
//     // verify the directive contains the expected noise line
//     const auto generated = result->to_xyce_directives();
//     ASSERT_EQ(generated.size(), 1);
//     ASSERT_EQ(generated[0], ".NOISE V(4) V2 DEC 20 1MEG 100MEG");
// }

// TEST(NoiseSimulationParametersChecks, reference_guide_example_data_sweep) {
//     // arrange
//     const std::vector<std::string> directives = {".NOISE V(4) V2 DATA=myTable"};
//     // act
//     const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
//     // assert
//     ASSERT_TRUE(result.has_value());
//     ASSERT_EQ(result->output_node, "4");
//     ASSERT_EQ(result->source_name, "V2");
//     ASSERT_EQ(result->sweep_type, "DATA");
//     ASSERT_EQ(result->data_table_name, "myTable");
//     // verify the directive contains the expected noise line
//     const auto generated = result->to_xyce_directives();
//     ASSERT_EQ(generated.size(), 1);
//     ASSERT_EQ(generated[0], ".NOISE V(4) V2 DATA=myTable");
// }
