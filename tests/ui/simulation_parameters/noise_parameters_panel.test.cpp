#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/noise_simulation_parameters.h"
#include "ui/simulation_parameters/noise_parameters_panel.h"

namespace
{
    class NoiseParametersPanelTest : public ::testing::Test
    {
    protected:
        void SetUp() override { m_parent = new wxFrame(nullptr, wxID_ANY, "test"); }

        void TearDown() override { delete m_parent; }

        wxFrame* m_parent = nullptr;
    };
} // namespace

// ========================================================================================
// constructor
// ========================================================================================

TEST_F(NoiseParametersPanelTest, constructor_creates_panel) {
    // arrange / act
    NoiseParametersPanel panel(m_parent);
    // assert
    ASSERT_NE(panel.get_global_settings(), nullptr);
    ASSERT_NE(panel.get_print_section(), nullptr);
}

// ========================================================================================
// build_noise_parameters — default state
// ========================================================================================

TEST_F(NoiseParametersPanelTest, build_returns_defaults_by_default) {
    // arrange / act
    NoiseParametersPanel panel(m_parent);
    auto result = panel.build_noise_parameters();
    // assert
    EXPECT_EQ(result.output_node, "");
    EXPECT_EQ(result.ref_node, "");
    EXPECT_EQ(result.source_name, "");
    EXPECT_EQ(result.start_freq_value, "");
    EXPECT_EQ(result.end_freq_value, "");
    EXPECT_EQ(result.num_points_value, "");
    EXPECT_EQ(result.sweep_type, "LIN");
    EXPECT_EQ(result.data_table_name, "");
    EXPECT_TRUE(result.device_noise_operators.empty());
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with field values via apply/round-trip
// ========================================================================================

TEST_F(NoiseParametersPanelTest, build_after_apply_returns_same_values) {
    // arrange
    NoiseParametersPanel panel(m_parent);
    auto print_params = PrintParameters("NOISE", "RAW", "noise.raw", {"V(*)"}, {});
    std::vector<DeviceNoiseOperator> dno;
    dno.emplace_back("DNI", "OUT", "V1");
    dno.emplace_back("DNO", "OUT", "V1");
    NoiseSimulationParameters input("OUT", "0", "V1", "10", "100k", "100", "DEC", dno, "", print_params);
    // act
    panel.apply(input);
    auto result = panel.build_noise_parameters();
    // assert
    EXPECT_EQ(result.output_node, "OUT");
    EXPECT_EQ(result.ref_node, "0");
    EXPECT_EQ(result.source_name, "V1");
    EXPECT_EQ(result.start_freq_value, "10");
    EXPECT_EQ(result.end_freq_value, "100k");
    EXPECT_EQ(result.num_points_value, "100");
    EXPECT_EQ(result.sweep_type, "DEC");
    EXPECT_EQ(result.data_table_name, "");
    ASSERT_EQ(result.device_noise_operators.size(), 2);
    EXPECT_EQ(result.device_noise_operators[0].type, "DNI");
    EXPECT_EQ(result.device_noise_operators[0].node, "OUT");
    EXPECT_EQ(result.device_noise_operators[0].source, "V1");
    EXPECT_EQ(result.device_noise_operators[1].type, "DNO");
    EXPECT_EQ(result.device_noise_operators[1].node, "OUT");
    EXPECT_EQ(result.device_noise_operators[1].source, "V1");
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "NOISE");
    EXPECT_EQ(result.print_parameters->print_format, "RAW");
    EXPECT_EQ(result.print_parameters->print_file, "noise.raw");
}

// ========================================================================================
// build with print section
// ========================================================================================

TEST_F(NoiseParametersPanelTest, build_with_print_section_enabled) {
    // arrange
    NoiseParametersPanel panel(m_parent);
    auto print_params = PrintParameters("NOISE", "CSV", "data.csv", {"V(1)", "V(2)"}, {});
    NoiseSimulationParameters input("", "", "", "", "", "", "LIN", {}, "", print_params);
    // act
    panel.apply(input);
    auto result = panel.build_noise_parameters();
    // assert
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "NOISE");
    EXPECT_EQ(result.print_parameters->print_format, "CSV");
    EXPECT_EQ(result.print_parameters->print_file, "data.csv");
}

TEST_F(NoiseParametersPanelTest, build_without_print_section) {
    // arrange
    NoiseParametersPanel panel(m_parent);
    NoiseSimulationParameters input("", "", "", "", "", "", "LIN", {}, "", std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_noise_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// apply — disabling print section from a previously enabled state
// ========================================================================================

TEST_F(NoiseParametersPanelTest, apply_without_print_params_disables_print_section) {
    // arrange
    NoiseParametersPanel panel(m_parent);
    auto print_params = PrintParameters("NOISE", "RAW", "noise.raw", {}, {});
    NoiseSimulationParameters with_print("", "", "", "", "", "", "LIN", {}, "", print_params);
    panel.apply(with_print);
    ASSERT_TRUE(panel.build_noise_parameters().print_parameters.has_value());
    // act — apply without print parameters
    NoiseSimulationParameters without_print("", "", "", "", "", "", "LIN", {}, "", std::nullopt);
    panel.apply(without_print);
    auto result = panel.build_noise_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with device noise operators
// ========================================================================================

TEST_F(NoiseParametersPanelTest, build_with_device_noise_operators) {
    // arrange
    NoiseParametersPanel panel(m_parent);
    std::vector<DeviceNoiseOperator> dno;
    dno.emplace_back("DNI", "OUT", "V1");
    NoiseSimulationParameters input("OUT", "0", "V1", "10", "100k", "100", "LIN", dno, "", std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_noise_parameters();
    // assert
    ASSERT_EQ(result.device_noise_operators.size(), 1);
    EXPECT_EQ(result.device_noise_operators[0].type, "DNI");
    EXPECT_EQ(result.device_noise_operators[0].node, "OUT");
    EXPECT_EQ(result.device_noise_operators[0].source, "V1");
}
