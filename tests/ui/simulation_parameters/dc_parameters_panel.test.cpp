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

// fixture declared at global scope so it can act as a friend of DcParametersPanel
class UiDcParametersPanelTest : public ::testing::Test
{
protected:
    void SetUp() override { m_parent = new wxFrame(nullptr, wxID_ANY, "test"); }

    void TearDown() override { delete m_parent; }

    // switch the sweep mode choice, acting as a friend to simulate a UI mode change
    static void set_sweep_mode(DcParametersPanel& panel, const std::string& mode) { panel.m_sweep_mode_choice->SetSelection(panel.m_sweep_mode_choice->FindString(wxString::FromUTF8(mode))); }

    // run the mode-change handler, acting as a friend
    static void fire_sweep_mode_change(DcParametersPanel& panel) { panel.on_sweep_mode_changed(); }

    wxFrame* m_parent = nullptr;
};

// ========================================================================================
// constructor
// ========================================================================================

TEST_F(UiDcParametersPanelTest, constructor_creates_panel) {
    // arrange / act
    DcParametersPanel panel(m_parent);
    // assert
    ASSERT_NE(panel.get_global_settings(), nullptr);
    ASSERT_NE(panel.get_print_section(), nullptr);
}

// ========================================================================================
// build_dc_parameters — default state
// ========================================================================================

TEST_F(UiDcParametersPanelTest, build_returns_defaults_by_default) {
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
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with field values via apply/round-trip
// ========================================================================================

TEST_F(UiDcParametersPanelTest, build_after_apply_returns_same_values) {
    // arrange
    DcParametersPanel panel(m_parent);
    auto print_params = PrintParameters("DC", "RAW", "out.raw", {"V(*)", "IC(*)"}, {});
    DCSimulationParameters input("LIN", "V1", "0", "5", "0.1", "", {}, "", "V2", "0", "10", "1", "", print_params, {}, std::nullopt);
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
// build with print section
// ========================================================================================

TEST_F(UiDcParametersPanelTest, build_with_print_section_enabled) {
    // arrange
    DcParametersPanel panel(m_parent);
    auto print_params = PrintParameters("DC", "CSV", "data.csv", {"V(1)", "V(2)"}, {});
    DCSimulationParameters input("LIN", "", "", "", "", "", {}, "", "", "", "", "", "", print_params, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_dc_parameters();
    // assert
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "DC");
    EXPECT_EQ(result.print_parameters->print_format, "CSV");
    EXPECT_EQ(result.print_parameters->print_file, "data.csv");
}

TEST_F(UiDcParametersPanelTest, build_without_print_section) {
    // arrange
    DcParametersPanel panel(m_parent);
    DCSimulationParameters input("LIN", "", "", "", "", "", {}, "", "", "", "", "", "", std::nullopt, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// apply — disabling print section from a previously enabled state
// ========================================================================================

TEST_F(UiDcParametersPanelTest, apply_without_print_params_disables_print_section) {
    // arrange
    DcParametersPanel panel(m_parent);
    auto print_params = PrintParameters("DC", "RAW", "out.raw", {}, {});
    DCSimulationParameters with_print("LIN", "", "", "", "", "", {}, "", "", "", "", "", "", print_params, {}, std::nullopt);
    panel.apply(with_print);
    ASSERT_TRUE(panel.build_dc_parameters().print_parameters.has_value());
    // act — apply without print parameters
    DCSimulationParameters without_print("LIN", "", "", "", "", "", {}, "", "", "", "", "", "", std::nullopt, {}, std::nullopt);
    panel.apply(without_print);
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with LIST sweep
// ========================================================================================

TEST_F(UiDcParametersPanelTest, build_with_list_sweep) {
    // arrange
    DcParametersPanel panel(m_parent);
    auto list_vals = std::vector<std::string>{"0.5", "1.0", "1.5", "2.0"};
    DCSimulationParameters input("LIST", "V1", "", "", "", "", list_vals, "", "", "", "", "", "", std::nullopt, {}, std::nullopt);
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

TEST_F(UiDcParametersPanelTest, build_with_data_sweep) {
    // arrange
    DcParametersPanel panel(m_parent);
    DCSimulationParameters input("DATA", "", "", "", "", "", {}, "my_data_table", "", "", "", "", "", std::nullopt, {}, std::nullopt);
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

TEST_F(UiDcParametersPanelTest, build_with_dec_sweep) {
    // arrange
    DcParametersPanel panel(m_parent);
    DCSimulationParameters input("DEC", "V1", "1", "10k", "", "10", {}, "", "", "", "", "", "", std::nullopt, {}, std::nullopt);
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

// ========================================================================================
// build — switching from LIN to DEC retains the sweep value as points
// ========================================================================================

TEST_F(UiDcParametersPanelTest, build_after_mode_change_lin_to_dec_keeps_value) {
    // arrange — load a LIN sweep so only the step field is populated
    DcParametersPanel panel(m_parent);
    DCSimulationParameters lin_input("LIN", "V1", "0", "5", "0.5", "", {}, "", "", "", "", "", "", std::nullopt, {}, std::nullopt);
    panel.apply(lin_input);
    // act — simulate the user changing the sweep mode to DEC
    set_sweep_mode(panel, "DEC");
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "DEC");
    EXPECT_EQ(result.primary_variable, "V1");
    EXPECT_EQ(result.start, "0");
    EXPECT_EQ(result.stop, "5");
    EXPECT_EQ(result.points, "0.5");
    EXPECT_TRUE(result.step.empty());
}

TEST_F(UiDcParametersPanelTest, build_after_mode_change_dec_to_lin_keeps_value) {
    // arrange — load a DEC sweep so only the points field is populated
    DcParametersPanel panel(m_parent);
    DCSimulationParameters dec_input("DEC", "V1", "1", "10k", "", "10", {}, "", "", "", "", "", "", std::nullopt, {}, std::nullopt);
    panel.apply(dec_input);
    // act — simulate the user changing the sweep mode to LIN
    set_sweep_mode(panel, "LIN");
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "LIN");
    EXPECT_EQ(result.primary_variable, "V1");
    EXPECT_EQ(result.start, "1");
    EXPECT_EQ(result.stop, "10k");
    EXPECT_EQ(result.step, "10");
    EXPECT_TRUE(result.points.empty());
}

TEST_F(UiDcParametersPanelTest, build_after_mode_change_lin_to_dec_keeps_secondary_value) {
    // arrange — load a LIN sweep with a secondary sweep
    DcParametersPanel panel(m_parent);
    DCSimulationParameters lin_input("LIN", "V1", "0", "5", "0.5", "", {}, "", "R1", "0", "1k", "10", "", std::nullopt, {}, std::nullopt);
    panel.apply(lin_input);
    // act — simulate the user changing the sweep mode to DEC
    set_sweep_mode(panel, "DEC");
    auto result = panel.build_dc_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "DEC");
    EXPECT_EQ(result.secondary_variable, "R1");
    EXPECT_EQ(result.secondary_start, "0");
    EXPECT_EQ(result.secondary_stop, "1k");
    EXPECT_EQ(result.secondary_points, "10");
    EXPECT_TRUE(result.secondary_step.empty());
}

// ========================================================================================
// on_sweep_mode_changed — visible fields are synced on mode change
// ========================================================================================

TEST_F(UiDcParametersPanelTest, on_sweep_mode_changed_copies_step_to_points_for_dec) {
    // arrange — load a LIN sweep so only the step field is populated
    DcParametersPanel panel(m_parent);
    DCSimulationParameters lin_input("LIN", "V1", "0", "5", "0.5", "", {}, "", "", "", "", "", "", std::nullopt, {}, std::nullopt);
    panel.apply(lin_input);
    // act — switch to DEC and run the mode-change handler
    set_sweep_mode(panel, "DEC");
    fire_sweep_mode_change(panel);
    // assert — the carried value is visible in the points field for the user to edit
    auto result = panel.build_dc_parameters();
    EXPECT_EQ(result.points, "0.5");
}

TEST_F(UiDcParametersPanelTest, on_sweep_mode_changed_copies_points_to_step_for_lin) {
    // arrange — load a DEC sweep so only the points field is populated
    DcParametersPanel panel(m_parent);
    DCSimulationParameters dec_input("DEC", "V1", "1", "10k", "", "10", {}, "", "", "", "", "", "", std::nullopt, {}, std::nullopt);
    panel.apply(dec_input);
    // act — switch to LIN and run the mode-change handler
    set_sweep_mode(panel, "LIN");
    fire_sweep_mode_change(panel);
    // assert — the carried value is visible in the step field for the user to edit
    auto result = panel.build_dc_parameters();
    EXPECT_EQ(result.step, "10");
}
