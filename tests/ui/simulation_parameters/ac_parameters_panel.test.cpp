#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/ac_simulation_parameters.h"
#include "ui/simulation_parameters/ac_parameters_panel.h"

namespace
{
    // parent frame fixture for wxPanel-based tests
    class UiAcParametersPanelTest : public ::testing::Test
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

TEST_F(UiAcParametersPanelTest, constructor_creates_panel) {
    // arrange / act
    AcParametersPanel panel(m_parent);
    // assert
    ASSERT_NE(panel.get_global_settings(), nullptr);
    ASSERT_NE(panel.get_print_section(), nullptr);
}

// ========================================================================================
// build_ac_parameters — default state
// ========================================================================================

TEST_F(UiAcParametersPanelTest, build_returns_defaults_by_default) {
    // arrange / act
    AcParametersPanel panel(m_parent);
    auto result = panel.build_ac_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "LIN");
    EXPECT_EQ(result.points, "");
    EXPECT_EQ(result.start, "");
    EXPECT_EQ(result.end, "");
    EXPECT_EQ(result.data_table_name, "");
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with field values via apply/round-trip
// ========================================================================================

TEST_F(UiAcParametersPanelTest, build_after_apply_returns_same_values) {
    // arrange
    AcParametersPanel panel(m_parent);
    auto print_params = PrintParameters("AC", "RAW", "ac.raw", {"V(*)", "IC(*)"}, {});
    AcSimulationParameters input("LIN", "100", "1", "1k", "", print_params, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_ac_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "LIN");
    EXPECT_EQ(result.points, "100");
    EXPECT_EQ(result.start, "1");
    EXPECT_EQ(result.end, "1k");
    EXPECT_EQ(result.data_table_name, "");
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "AC");
    EXPECT_EQ(result.print_parameters->print_format, "RAW");
    EXPECT_EQ(result.print_parameters->print_file, "ac.raw");
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

TEST_F(UiAcParametersPanelTest, build_with_print_section_enabled) {
    // arrange
    AcParametersPanel panel(m_parent);
    auto print_params = PrintParameters("AC", "CSV", "data.csv", {"V(1)", "V(2)"}, {});
    AcSimulationParameters input("LIN", "", "", "", "", print_params, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_ac_parameters();
    // assert
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "AC");
    EXPECT_EQ(result.print_parameters->print_format, "CSV");
    EXPECT_EQ(result.print_parameters->print_file, "data.csv");
}

TEST_F(UiAcParametersPanelTest, build_without_print_section) {
    // arrange
    AcParametersPanel panel(m_parent);
    AcSimulationParameters input("LIN", "", "", "", "", std::nullopt, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_ac_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// apply — disabling print section from a previously enabled state
// ========================================================================================

TEST_F(UiAcParametersPanelTest, apply_without_print_params_disables_print_section) {
    // arrange
    AcParametersPanel panel(m_parent);
    auto print_params = PrintParameters("AC", "RAW", "ac.raw", {}, {});
    AcSimulationParameters with_print("LIN", "", "", "", "", print_params, {}, std::nullopt);
    panel.apply(with_print);
    ASSERT_TRUE(panel.build_ac_parameters().print_parameters.has_value());
    // act — apply without print parameters
    AcSimulationParameters without_print("LIN", "", "", "", "", std::nullopt, {}, std::nullopt);
    panel.apply(without_print);
    auto result = panel.build_ac_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with DATA sweep
// ========================================================================================

TEST_F(UiAcParametersPanelTest, build_with_data_sweep) {
    // arrange
    AcParametersPanel panel(m_parent);
    AcSimulationParameters input("DATA", "", "", "", "mytable", std::nullopt, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_ac_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "DATA");
    EXPECT_EQ(result.data_table_name, "mytable");
}

// ========================================================================================
// build with DEC sweep
// ========================================================================================

TEST_F(UiAcParametersPanelTest, build_with_dec_sweep) {
    // arrange
    AcParametersPanel panel(m_parent);
    AcSimulationParameters input("DEC", "10", "1", "100k", "", std::nullopt, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_ac_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "DEC");
    EXPECT_EQ(result.points, "10");
    EXPECT_EQ(result.start, "1");
    EXPECT_EQ(result.end, "100k");
}
