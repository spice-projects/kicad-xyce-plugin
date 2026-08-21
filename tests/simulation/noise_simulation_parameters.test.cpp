#include <gtest/gtest.h>

#include "netlist/netlist.h"
#include "simulation/noise_simulation_parameters.h"

// ========================================================================================
// DeviceNoiseOperator
// ========================================================================================

TEST(DeviceNoiseOperatorChecks, create_device_noise_operator) {
    // arrange
    const DeviceNoiseOperator operator1("DNI", "R1", "");
    // act/assert
    ASSERT_EQ(operator1.type, "DNI");
    ASSERT_EQ(operator1.node, "R1");
    ASSERT_EQ(operator1.source, "");
}

TEST(DeviceNoiseOperatorChecks, create_device_noise_operator_with_noise_source) {
    // arrange
    const DeviceNoiseOperator operator1("DNO", "Q2", "FLICKER");
    // act/assert
    ASSERT_EQ(operator1.type, "DNO");
    ASSERT_EQ(operator1.node, "Q2");
    ASSERT_EQ(operator1.source, "FLICKER");
}

TEST(DeviceNoiseOperatorChecks, equality_operator_equal) {
    // arrange
    const DeviceNoiseOperator op1("DNI", "R1", "");
    const DeviceNoiseOperator op2("DNI", "R1", "");
    // act
    const bool result = op1 == op2;
    // assert
    ASSERT_TRUE(result);
}

TEST(DeviceNoiseOperatorChecks, equality_operator_different_type) {
    // arrange
    const DeviceNoiseOperator op1("DNI", "R1", "");
    const DeviceNoiseOperator op2("DNO", "R1", "");
    // act
    const bool result = op1 == op2;
    // assert
    ASSERT_FALSE(result);
}

TEST(DeviceNoiseOperatorChecks, equality_operator_different_node) {
    // arrange
    const DeviceNoiseOperator op1("DNI", "R1", "");
    const DeviceNoiseOperator op2("DNI", "Q2", "");
    // act
    const bool result = op1 == op2;
    // assert
    ASSERT_FALSE(result);
}

TEST(DeviceNoiseOperatorChecks, equality_operator_different_source) {
    // arrange
    const DeviceNoiseOperator op1("DNI", "R1", "");
    const DeviceNoiseOperator op2("DNI", "R1", "FLICKER");
    // act
    const bool result = op1 == op2;
    // assert
    ASSERT_FALSE(result);
}

// ========================================================================================
// from_xyce_directives
// ========================================================================================

TEST(NoiseSimulationParametersChecks, parse_lin_sweep) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 100 1 1MEG"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_node, "5");
    ASSERT_EQ(result->ref_node, "");
    ASSERT_EQ(result->source_name, "V1");
    ASSERT_EQ(result->sweep_type, "LIN");
    ASSERT_EQ(result->num_points_value, "100");
    ASSERT_EQ(result->start_freq_value, "1");
    ASSERT_EQ(result->end_freq_value, "1MEG");
}

TEST(NoiseSimulationParametersChecks, parse_output_with_ref_node) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5,3) V1 LIN 100 1 1MEG"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_node, "5");
    ASSERT_EQ(result->ref_node, "3");
}

TEST(NoiseSimulationParametersChecks, parse_dec_sweep) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(out) Vin DEC 10 1k 100MEG"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->sweep_type, "DEC");
    ASSERT_EQ(result->num_points_value, "10");
    ASSERT_EQ(result->start_freq_value, "1k");
    ASSERT_EQ(result->end_freq_value, "100MEG");
}

TEST(NoiseSimulationParametersChecks, parse_data_sweep) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(out) Vin DATA=myTable"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->sweep_type, "DATA");
    ASSERT_EQ(result->data_table_name, "myTable");
}

TEST(NoiseSimulationParametersChecks, no_noise_directive_returns_none) {
    // arrange
    const std::vector<std::string> directives = {".TRAN 1u 1m"};
    // act
    const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_FALSE(result.has_value());
}

// ========================================================================================
// to_xyce_directives
// ========================================================================================

TEST(NoiseSimulationParametersChecks, generates_lin_directive) {
    // arrange
    const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".NOISE V(5) V1 LIN 100 1 1MEG");
}

TEST(NoiseSimulationParametersChecks, generates_with_ref_node) {
    // arrange
    const NoiseSimulationParameters params("5", "3", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".NOISE V(5,3) V1 LIN 100 1 1MEG");
}

TEST(NoiseSimulationParametersChecks, generates_dec_directive) {
    // arrange
    const NoiseSimulationParameters params("out", "", "Vin", "1k", "100MEG", "10", "DEC", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".NOISE V(out) Vin DEC 10 1k 100MEG");
}

TEST(NoiseSimulationParametersChecks, generates_data_directive) {
    // arrange
    const NoiseSimulationParameters params("out", "", "Vin", "1k", "100MEG", "10", "DATA", std::vector<DeviceNoiseOperator>{}, "myTable", std::nullopt);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".NOISE V(out) Vin DATA=myTable");
}

TEST(NoiseSimulationParametersChecks, generates_with_print_noise_directive) {
    // arrange
    const PrintParameters print_params("NOISE", "", "", {"V(OUT)"}, {});
    const NoiseSimulationParameters params("OUT", "", "V1", "1", "1MEG", "10", "LIN", std::vector<DeviceNoiseOperator>{}, "", print_params);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_EQ(directives[0], ".NOISE V(OUT) V1 LIN 10 1 1MEG");
    ASSERT_EQ(directives[1], ".PRINT NOISE V(OUT)");
}

TEST(NoiseSimulationParametersChecks, parses_print_noise_directive) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT NOISE V(OUT)"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->print_parameters.has_value());
    ASSERT_EQ(result->print_parameters->print_type, "NOISE");
    ASSERT_EQ(result->print_parameters->output_variables.size(), 1);
    ASSERT_EQ(result->print_parameters->output_variables[0], "V(OUT)");
}

TEST(NoiseSimulationParametersChecks, ignores_non_noise_print_directive) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT TRAN V(OUT)"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->print_parameters.has_value());
}

TEST(NoiseSimulationParametersChecks, parses_empty_directives_returns_none) {
    // arrange
    const std::vector<std::string> directives = {};
    // act
    const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_FALSE(result.has_value());
}

TEST(NoiseSimulationParametersChecks, parses_non_noise_directives_return_none) {
    // arrange
    const std::vector<std::string> directives = {".TRAN 1ns 1ms"};
    // act
    const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_FALSE(result.has_value());
}

TEST(NoiseSimulationParametersChecks, generates_with_device_noise_operators) {
    // arrange
    const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{DeviceNoiseOperator("DNI", "R1", "")}, "", std::nullopt);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".NOISE V(5) V1 LIN 100 1 1MEG");
}

TEST(NoiseSimulationParametersChecks, generates_with_print_parameters) {
    // arrange
    const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", PrintParameters("TRAN", "", "", {"V(OUT)"}, {}));
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_EQ(directives[0], ".NOISE V(5) V1 LIN 100 1 1MEG");
    ASSERT_EQ(directives[1], ".PRINT TRAN V(OUT)");
}

// ========================================================================================
// equality operator
// ========================================================================================

TEST(NoiseSimulationParametersChecks, equality_operator_equal_params) {
    // arrange
    const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    const NoiseSimulationParameters params2("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_TRUE(result);
}

TEST(NoiseSimulationParametersChecks, equality_operator_different_output_node) {
    // arrange
    const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    const NoiseSimulationParameters params2("6", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(NoiseSimulationParametersChecks, equality_operator_different_ref_node) {
    // arrange
    const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    const NoiseSimulationParameters params2("5", "3", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(NoiseSimulationParametersChecks, equality_operator_different_source_name) {
    // arrange
    const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    const NoiseSimulationParameters params2("5", "", "V2", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(NoiseSimulationParametersChecks, equality_operator_different_sweep_type) {
    // arrange
    const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    const NoiseSimulationParameters params2("5", "", "V1", "1", "1MEG", "100", "DEC", std::vector<DeviceNoiseOperator>{}, "", std::nullopt);
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(NoiseSimulationParametersChecks, equality_operator_different_print_parameters) {
    // arrange
    const NoiseSimulationParameters params1("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", PrintParameters("TRAN", "", "", {"V(OUT)"}, {}));
    const NoiseSimulationParameters params2("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", PrintParameters("TRAN", "", "", {"I(V1)"}, {}));
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

// ========================================================================================
// Tests for from_xyce_directives edge cases
// ========================================================================================

TEST(NoiseSimulationParametersChecks, empty_directives_returns_none) {
    // arrange
    const std::vector<std::string> directives = {};
    // act
    const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_FALSE(result.has_value());
}

TEST(NoiseSimulationParametersChecks, non_noise_directives_return_none) {
    // arrange
    const std::vector<std::string> directives = {".TRAN 1ns 1ms"};
    // act
    const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_FALSE(result.has_value());
}

TEST(NoiseSimulationParametersChecks, serializes_print_noise_directive) {
    // arrange
    const PrintParameters print_params("NOISE", "", "", {"V(OUT)"}, {});
    const NoiseSimulationParameters params("OUT", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", print_params);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_EQ(directives[0], ".NOISE V(OUT) V1 LIN 100 1 1MEG");
    ASSERT_EQ(directives[1], ".PRINT NOISE V(OUT)");
}

TEST(NoiseSimulationParametersChecks, ignores_empty_directive) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({"", ".NOISE V(5) V1 LIN 10 1 1MEG"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_node, "5");
}

TEST(NoiseSimulationParametersChecks, parses_bare_output_node) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE 5 V1 LIN 10 1 1MEG"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_node, "5");
    ASSERT_EQ(result->ref_node, "");
}

TEST(NoiseSimulationParametersChecks, parses_noise_directive_alone) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_node, "");
}

TEST(NoiseSimulationParametersChecks, parses_noise_without_sweep_type) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_node, "5");
    ASSERT_EQ(result->source_name, "V1");
    ASSERT_EQ(result->sweep_type, "LIN");
}

TEST(NoiseSimulationParametersChecks, parses_implicit_lin_sweep) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 100 1 1MEG"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->sweep_type, "LIN");
    ASSERT_EQ(result->num_points_value, "100");
    ASSERT_EQ(result->start_freq_value, "1");
    ASSERT_EQ(result->end_freq_value, "1MEG");
}

TEST(NoiseSimulationParametersChecks, parses_device_noise_operators) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT NOISE INOISE ONOISE DNI(R1) DNO(R2)"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->device_noise_operators.size(), 2);
    ASSERT_EQ(result->device_noise_operators[0].node, "R1");
    ASSERT_EQ(result->device_noise_operators[0].type, "DNI");
    ASSERT_EQ(result->device_noise_operators[1].node, "R2");
    ASSERT_EQ(result->device_noise_operators[1].type, "DNO");
}

TEST(NoiseSimulationParametersChecks, parses_device_noise_operators_with_noise_source) {
    // arrange / act
    const auto result = NoiseSimulationParameters::from_xyce_directives({".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT NOISE DNI(Q2,FLICKER) DNO(Q2,THERMAL)"});
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->device_noise_operators.size(), 2);
    ASSERT_EQ(result->device_noise_operators[0].node, "Q2");
    ASSERT_EQ(result->device_noise_operators[0].type, "DNI");
    ASSERT_EQ(result->device_noise_operators[0].source, "FLICKER");
    ASSERT_EQ(result->device_noise_operators[1].node, "Q2");
    ASSERT_EQ(result->device_noise_operators[1].type, "DNO");
    ASSERT_EQ(result->device_noise_operators[1].source, "THERMAL");
}

TEST(NoiseSimulationParametersChecks, serializes_device_noise_operators) {
    // arrange
    const std::vector<DeviceNoiseOperator> device_operators = {DeviceNoiseOperator("DNI", "R1", ""), DeviceNoiseOperator("DNO", "R2", "")};
    const PrintParameters print_params("NOISE", "", "", {"INOISE", "ONOISE"}, {});
    const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", device_operators, "", print_params);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_NE(directives[1].find("INOISE ONOISE DNI(R1) DNO(R2)"), std::string::npos);
}

TEST(NoiseSimulationParametersChecks, serializes_device_noise_operators_with_noise_source) {
    // arrange
    const std::vector<DeviceNoiseOperator> device_operators = {DeviceNoiseOperator("DNI", "Q2", "FLICKER")};
    const PrintParameters print_params("NOISE", "", "", {}, {});
    const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", device_operators, "", print_params);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 2);
    ASSERT_NE(directives[1].find("DNI(Q2,FLICKER)"), std::string::npos);
}

TEST(NoiseSimulationParametersChecks, omits_print_when_no_output_variables) {
    // arrange
    const PrintParameters print_params("NOISE", "", "", {}, {});
    const NoiseSimulationParameters params("5", "", "V1", "1", "1MEG", "100", "LIN", std::vector<DeviceNoiseOperator>{}, "", print_params);
    // act
    const auto directives = params.to_xyce_directives(NetlistTopology{});
    // assert
    ASSERT_EQ(directives.size(), 1);
    ASSERT_EQ(directives[0], ".NOISE V(5) V1 LIN 100 1 1MEG");
}

// ========================================================================================
// Tests for reference guide examples
// ========================================================================================

TEST(NoiseSimulationParametersChecks, reference_guide_example_lin_sweep) {
    // arrange
    const std::vector<std::string> directives = {".NOISE V(5) VIN LIN 101 100Hz 200Hz"};
    // act
    const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_node, "5");
    ASSERT_EQ(result->ref_node, "");
    ASSERT_EQ(result->source_name, "VIN");
    ASSERT_EQ(result->sweep_type, "LIN");
    ASSERT_EQ(result->num_points_value, "101");
    ASSERT_EQ(result->start_freq_value, "100Hz");
    ASSERT_EQ(result->end_freq_value, "200Hz");
    // verify the directive contains the expected noise line
    const auto generated = result->to_xyce_directives(NetlistTopology{});
    ASSERT_EQ(generated.size(), 1);
    ASSERT_EQ(generated[0], ".NOISE V(5) VIN LIN 101 100Hz 200Hz");
}

TEST(NoiseSimulationParametersChecks, reference_guide_example_oct_sweep_with_ref) {
    // arrange
    const std::vector<std::string> directives = {".NOISE V(5,3) V1 OCT 10 1kHz 16kHz"};
    // act
    const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_node, "5");
    ASSERT_EQ(result->ref_node, "3");
    ASSERT_EQ(result->source_name, "V1");
    ASSERT_EQ(result->sweep_type, "OCT");
    ASSERT_EQ(result->num_points_value, "10");
    ASSERT_EQ(result->start_freq_value, "1kHz");
    ASSERT_EQ(result->end_freq_value, "16kHz");
    // verify the directive contains the expected noise line
    const auto generated = result->to_xyce_directives(NetlistTopology{});
    ASSERT_EQ(generated.size(), 1);
    ASSERT_EQ(generated[0], ".NOISE V(5,3) V1 OCT 10 1kHz 16kHz");
}

TEST(NoiseSimulationParametersChecks, reference_guide_example_dec_sweep) {
    // arrange
    const std::vector<std::string> directives = {".NOISE V(4) V2 DEC 20 1MEG 100MEG"};
    // act
    const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_node, "4");
    ASSERT_EQ(result->ref_node, "");
    ASSERT_EQ(result->source_name, "V2");
    ASSERT_EQ(result->sweep_type, "DEC");
    ASSERT_EQ(result->num_points_value, "20");
    ASSERT_EQ(result->start_freq_value, "1MEG");
    ASSERT_EQ(result->end_freq_value, "100MEG");
    // verify the directive contains the expected noise line
    const auto generated = result->to_xyce_directives(NetlistTopology{});
    ASSERT_EQ(generated.size(), 1);
    ASSERT_EQ(generated[0], ".NOISE V(4) V2 DEC 20 1MEG 100MEG");
}

TEST(NoiseSimulationParametersChecks, reference_guide_example_data_sweep) {
    // arrange
    const std::vector<std::string> directives = {".NOISE V(4) V2 DATA=myTable"};
    // act
    const auto result = NoiseSimulationParameters::from_xyce_directives(directives);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_node, "4");
    ASSERT_EQ(result->source_name, "V2");
    ASSERT_EQ(result->sweep_type, "DATA");
    ASSERT_EQ(result->data_table_name, "myTable");
    // verify the directive contains the expected noise line
    const auto generated = result->to_xyce_directives(NetlistTopology{});
    ASSERT_EQ(generated.size(), 1);
    ASSERT_EQ(generated[0], ".NOISE V(4) V2 DATA=myTable");
}
