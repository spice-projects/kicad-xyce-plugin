#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/step_parameters.h"
#include "ui/simulation_parameters/step_parameters_panel.h"

namespace
{
    // parent frame fixture for wxPanel-based tests
    class StepParametersPanelTest : public ::testing::Test
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

TEST_F(StepParametersPanelTest, constructor_creates_panel) {
    // arrange / act
    StepParametersPanel panel(m_parent);
    // assert
    SUCCEED();
}

// ========================================================================================
// build_step_parameters — default state
// ========================================================================================

TEST_F(StepParametersPanelTest, build_returns_disabled_defaults) {
    // arrange / act
    StepParametersPanel panel(m_parent);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_FALSE(result.enabled);
    EXPECT_EQ(result.sweep_mode, "LIN");
    EXPECT_TRUE(result.variable.empty());
    EXPECT_TRUE(result.start.empty());
    EXPECT_TRUE(result.stop.empty());
    EXPECT_TRUE(result.step.empty());
    EXPECT_TRUE(result.points.empty());
    EXPECT_TRUE(result.list_values.empty());
    EXPECT_TRUE(result.data_table_name.empty());
}

// ========================================================================================
// build / apply round-trip — LIN sweep
// ========================================================================================

TEST_F(StepParametersPanelTest, build_after_apply_returns_same_values_lin) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("LIN", "TEMP", "0", "100", "1", "", {}, "", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_TRUE(result.enabled);
    EXPECT_EQ(result.sweep_mode, "LIN");
    EXPECT_EQ(result.variable, "TEMP");
    EXPECT_EQ(result.start, "0");
    EXPECT_EQ(result.stop, "100");
    EXPECT_EQ(result.step, "1");
    EXPECT_TRUE(result.points.empty());
    EXPECT_TRUE(result.list_values.empty());
    EXPECT_TRUE(result.data_table_name.empty());
}

// ========================================================================================
// build / apply round-trip — DEC sweep
// ========================================================================================

TEST_F(StepParametersPanelTest, build_after_apply_returns_same_values_dec) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("DEC", "R1", "100", "1MEG", "", "10", {}, "", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_TRUE(result.enabled);
    EXPECT_EQ(result.sweep_mode, "DEC");
    EXPECT_EQ(result.variable, "R1");
    EXPECT_EQ(result.start, "100");
    EXPECT_EQ(result.stop, "1MEG");
    EXPECT_TRUE(result.step.empty());
    EXPECT_EQ(result.points, "10");
    EXPECT_TRUE(result.list_values.empty());
    EXPECT_TRUE(result.data_table_name.empty());
}

// ========================================================================================
// build / apply round-trip — OCT sweep
// ========================================================================================

TEST_F(StepParametersPanelTest, build_after_apply_returns_same_values_oct) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("OCT", "C1", "1n", "1u", "", "5", {}, "", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_TRUE(result.enabled);
    EXPECT_EQ(result.sweep_mode, "OCT");
    EXPECT_EQ(result.variable, "C1");
    EXPECT_EQ(result.start, "1n");
    EXPECT_EQ(result.stop, "1u");
    EXPECT_TRUE(result.step.empty());
    EXPECT_EQ(result.points, "5");
    EXPECT_TRUE(result.list_values.empty());
    EXPECT_TRUE(result.data_table_name.empty());
}

// ========================================================================================
// build / apply round-trip — LIST sweep
// ========================================================================================

TEST_F(StepParametersPanelTest, build_after_apply_returns_same_values_list) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("LIST", "R1", "", "", "", "", {"1k", "2k", "5k"}, "", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_TRUE(result.enabled);
    EXPECT_EQ(result.sweep_mode, "LIST");
    EXPECT_EQ(result.variable, "R1");
    ASSERT_EQ(result.list_values.size(), 3);
    EXPECT_EQ(result.list_values[0], "1k");
    EXPECT_EQ(result.list_values[1], "2k");
    EXPECT_EQ(result.list_values[2], "5k");
    EXPECT_TRUE(result.data_table_name.empty());
}

// ========================================================================================
// build / apply round-trip — DATA sweep
// ========================================================================================

TEST_F(StepParametersPanelTest, build_after_apply_returns_same_values_data) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("DATA", "", "", "", "", "", {}, "myTable", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_TRUE(result.enabled);
    EXPECT_EQ(result.sweep_mode, "DATA");
    EXPECT_EQ(result.data_table_name, "myTable");
    EXPECT_TRUE(result.variable.empty());
    EXPECT_TRUE(result.list_values.empty());
}

// ========================================================================================
// build — enabled / disabled state through apply
// ========================================================================================

TEST_F(StepParametersPanelTest, build_returns_enabled_when_checkbox_on) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("LIN", "TEMP", "0", "100", "1", "", {}, "", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_TRUE(result.enabled);
}

TEST_F(StepParametersPanelTest, build_returns_disabled_when_checkbox_off) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("LIN", "TEMP", "0", "100", "1", "", {}, "", false);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_FALSE(result.enabled);
}

// ========================================================================================
// build — unknown sweep mode falls back to first mode
// ========================================================================================

TEST_F(StepParametersPanelTest, build_falls_back_to_lin_for_unknown_sweep_mode) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("INVALID", "X", "1", "2", "3", "", {}, "", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "LIN");
}

// ========================================================================================
// list values round-trip — edge cases
// ========================================================================================

TEST_F(StepParametersPanelTest, build_after_apply_with_single_list_value) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("LIST", "R1", "", "", "", "", {"1k"}, "", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    ASSERT_EQ(result.list_values.size(), 1);
    EXPECT_EQ(result.list_values[0], "1k");
}

TEST_F(StepParametersPanelTest, build_after_apply_with_empty_list_values) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("LIST", "R1", "", "", "", "", {}, "", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_TRUE(result.list_values.empty());
}

// ========================================================================================
// apply — replacing previous values
// ========================================================================================

TEST_F(StepParametersPanelTest, apply_replaces_previous_values) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters first("LIN", "TEMP", "0", "100", "1", "", {}, "", true);
    panel.apply(first);
    // act
    StepParameters second("DEC", "R1", "10", "1k", "", "5", {}, "", false);
    panel.apply(second);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_FALSE(result.enabled);
    EXPECT_EQ(result.sweep_mode, "DEC");
    EXPECT_EQ(result.variable, "R1");
    EXPECT_EQ(result.start, "10");
    EXPECT_EQ(result.stop, "1k");
    EXPECT_TRUE(result.step.empty());
    EXPECT_EQ(result.points, "5");
    EXPECT_TRUE(result.list_values.empty());
    EXPECT_TRUE(result.data_table_name.empty());
}

// ========================================================================================
// list values — round-trip with large set
// ========================================================================================

TEST_F(StepParametersPanelTest, build_after_apply_with_many_list_values) {
    // arrange
    StepParametersPanel panel(m_parent);
    std::vector<std::string> many_vals = {"1k", "2k", "3k", "4k", "5k", "10k", "20k", "50k", "100k", "1MEG"};
    StepParameters input("LIST", "R1", "", "", "", "", many_vals, "", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    ASSERT_EQ(result.list_values.size(), 10);
    EXPECT_EQ(result.list_values[0], "1k");
    EXPECT_EQ(result.list_values[4], "5k");
    EXPECT_EQ(result.list_values[9], "1MEG");
}

// ========================================================================================
// build / apply round-trip — DATA sweep with empty table name
// ========================================================================================

TEST_F(StepParametersPanelTest, build_after_apply_with_empty_data_table_name) {
    // arrange
    StepParametersPanel panel(m_parent);
    StepParameters input("DATA", "", "", "", "", "", {}, "", true);
    // act
    panel.apply(input);
    auto result = panel.build_step_parameters();
    // assert
    EXPECT_TRUE(result.enabled);
    EXPECT_EQ(result.sweep_mode, "DATA");
    EXPECT_TRUE(result.data_table_name.empty());
}
