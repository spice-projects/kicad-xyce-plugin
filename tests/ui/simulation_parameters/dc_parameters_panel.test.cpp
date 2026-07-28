#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/dc_simulation_parameters.h"
#include "ui/simulation_parameters/dc_parameters_panel.h"

namespace
{
    class DcParametersPanelTest : public ::testing::Test
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

TEST_F(DcParametersPanelTest, constructor_creates_panel) {
    // arrange / act
    DcParametersPanel panel(m_parent);
    // assert
    ASSERT_NE(panel.get_global_settings(), nullptr);
    ASSERT_NE(panel.get_print_section(), nullptr);
}

// ========================================================================================
// build_dc_parameters — default state
// ========================================================================================

TEST_F(DcParametersPanelTest, build_returns_defaults_by_default) {
    // arrange / act
    DcParametersPanel panel(m_parent);
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "LIN");
    EXPECT_EQ(result.primary_variable, "");
    EXPECT_EQ(result.start, "");
    EXPECT_EQ(result.stop, "");
    EXPECT_EQ(result.step, "");
    EXPECT_EQ(result.points, "");
    EXPECT_TRUE(result.list_values.empty());
    EXPECT_EQ(result.data_table_name, "");
    EXPECT_EQ(result.secondary_variable, "");
    EXPECT_EQ(result.secondary_start, "");
    EXPECT_EQ(result.secondary_stop, "");
    EXPECT_EQ(result.secondary_step, "");
    EXPECT_EQ(result.secondary_points, "");
    EXPECT_FALSE(result.replace_ground);
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with field values via apply/round-trip
// ========================================================================================

TEST_F(DcParametersPanelTest, build_after_apply_returns_same_values) {
    // arrange
    DcParametersPanel panel(m_parent);
    auto print_params = PrintParameters("DC", "RAW", "out.raw", {"V(*)", "IC(*)"}, {});
    DCSimulationParameters input("LIN", "V1", "0", "5", "0.1", "", {}, "", "V2", "0", "10", "1", "", true, print_params, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "LIN");
    EXPECT_EQ(result.primary_variable, "V1");
    EXPECT_EQ(result.start, "0");
    EXPECT_EQ(result.stop, "5");
    EXPECT_EQ(result.step, "0.1");
    EXPECT_TRUE(result.points.empty());
    EXPECT_TRUE(result.list_values.empty());
    EXPECT_TRUE(result.data_table_name.empty());
    EXPECT_EQ(result.secondary_variable, "V2");
    EXPECT_EQ(result.secondary_start, "0");
    EXPECT_EQ(result.secondary_stop, "10");
    EXPECT_EQ(result.secondary_step, "1");
    EXPECT_TRUE(result.secondary_points.empty());
    EXPECT_TRUE(result.replace_ground);
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "DC");
    EXPECT_EQ(result.print_parameters->print_format, "RAW");
    EXPECT_EQ(result.print_parameters->print_file, "out.raw");
    ASSERT_EQ(result.print_parameters->output_variables.size(), 5);
    EXPECT_EQ(result.print_parameters->output_variables[0], "V(*)");
    EXPECT_EQ(result.print_parameters->output_variables[1], "IB(*)");
    EXPECT_EQ(result.print_parameters->output_variables[2], "IC(*)");
    EXPECT_EQ(result.print_parameters->output_variables[3], "IE(*)");
    EXPECT_EQ(result.print_parameters->output_variables[4], "IS(*)");
}

// ========================================================================================
// build with replace_ground
// ========================================================================================

TEST_F(DcParametersPanelTest, build_with_replace_ground) {
    // arrange
    DcParametersPanel panel(m_parent);
    DCSimulationParameters input("LIN", "", "", "", "", "", {}, "", "", "", "", "", "", true, std::nullopt, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_TRUE(result.replace_ground);
}

// ========================================================================================
// build with print section
// ========================================================================================

TEST_F(DcParametersPanelTest, build_with_print_section_enabled) {
    // arrange
    DcParametersPanel panel(m_parent);
    auto print_params = PrintParameters("DC", "CSV", "data.csv", {"V(1)", "V(2)"}, {});
    DCSimulationParameters input("LIN", "", "", "", "", "", {}, "", "", "", "", "", "", false, print_params, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_dc_parameters();
    // assert
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "DC");
    EXPECT_EQ(result.print_parameters->print_format, "CSV");
    EXPECT_EQ(result.print_parameters->print_file, "data.csv");
}

TEST_F(DcParametersPanelTest, build_without_print_section) {
    // arrange
    DcParametersPanel panel(m_parent);
    DCSimulationParameters input("LIN", "", "", "", "", "", {}, "", "", "", "", "", "", false, std::nullopt, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// apply — disabling print section from a previously enabled state
// ========================================================================================

TEST_F(DcParametersPanelTest, apply_without_print_params_disables_print_section) {
    // arrange
    DcParametersPanel panel(m_parent);
    auto print_params = PrintParameters("DC", "RAW", "out.raw", {}, {});
    DCSimulationParameters with_print("LIN", "", "", "", "", "", {}, "", "", "", "", "", "", false, print_params, {}, std::nullopt);
    panel.apply(with_print);
    ASSERT_TRUE(panel.build_dc_parameters().print_parameters.has_value());
    // act — apply without print parameters
    DCSimulationParameters without_print("LIN", "", "", "", "", "", {}, "", "", "", "", "", "", false, std::nullopt, {}, std::nullopt);
    panel.apply(without_print);
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with LIST sweep
// ========================================================================================

TEST_F(DcParametersPanelTest, build_with_list_sweep) {
    // arrange
    DcParametersPanel panel(m_parent);
    auto list_vals = std::vector<std::string>{"0.5", "1.0", "1.5", "2.0"};
    DCSimulationParameters input("LIST", "V1", "", "", "", "", list_vals, "", "", "", "", "", "", false, std::nullopt, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "LIST");
    EXPECT_EQ(result.primary_variable, "V1");
    ASSERT_EQ(result.list_values.size(), 4);
    EXPECT_EQ(result.list_values[0], "0.5");
    EXPECT_EQ(result.list_values[1], "1.0");
    EXPECT_EQ(result.list_values[2], "1.5");
    EXPECT_EQ(result.list_values[3], "2.0");
}

// ========================================================================================
// build with DATA sweep
// ========================================================================================

TEST_F(DcParametersPanelTest, build_with_data_sweep) {
    // arrange
    DcParametersPanel panel(m_parent);
    DCSimulationParameters input("DATA", "", "", "", "", "", {}, "my_data_table", "", "", "", "", "", false, std::nullopt, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "DATA");
    EXPECT_EQ(result.data_table_name, "my_data_table");
}

// ========================================================================================
// build with DEC sweep
// ========================================================================================

TEST_F(DcParametersPanelTest, build_with_dec_sweep) {
    // arrange
    DcParametersPanel panel(m_parent);
    DCSimulationParameters input("DEC", "V1", "1", "10k", "", "10", {}, "", "", "", "", "", "", false, std::nullopt, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "DEC");
    EXPECT_EQ(result.primary_variable, "V1");
    EXPECT_EQ(result.start, "1");
    EXPECT_EQ(result.stop, "10k");
    EXPECT_EQ(result.points, "10");
    EXPECT_TRUE(result.step.empty());
}
