#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/lin_simulation_parameters.h"
#include "ui/simulation_parameters/lin_parameters_panel.h"

namespace
{
    class UiLinParametersPanelTest : public ::testing::Test
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

TEST_F(UiLinParametersPanelTest, constructor_creates_panel) {
    // arrange / act
    LinParametersPanel panel(m_parent);
    // assert
    ASSERT_NE(panel.get_global_settings(), nullptr);
    ASSERT_NE(panel.get_print_section(), nullptr);
}

// ========================================================================================
// build_lin_parameters — default state
// ========================================================================================

TEST_F(UiLinParametersPanelTest, build_returns_defaults_by_default) {
    // arrange / act
    LinParametersPanel panel(m_parent);
    auto result = panel.build_lin_parameters();
    // assert
    EXPECT_TRUE(result.sparcalc);
    EXPECT_EQ(result.format, "TOUCHSTONE2");
    EXPECT_EQ(result.lintype, "S");
    EXPECT_EQ(result.dataformat, "RI");
    EXPECT_EQ(result.file, "");
    EXPECT_EQ(result.width, "");
    EXPECT_EQ(result.precision, "");
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

TEST_F(UiLinParametersPanelTest, build_after_apply_returns_same_values) {
    // arrange
    LinParametersPanel panel(m_parent);
    auto print_params = PrintParameters("AC", "RAW", "lin.raw", {"V(*)", "IC(*)"}, {});
    LinSimulationParameters input(true, "CITIFILE", "Z", "DB", "out.s2p", "10", "6", "DEC", "100", "1k", "1M", "", print_params);
    // act
    panel.apply(input);
    auto result = panel.build_lin_parameters();
    // assert
    EXPECT_TRUE(result.sparcalc);
    EXPECT_EQ(result.format, "CITIFILE");
    EXPECT_EQ(result.lintype, "Z");
    EXPECT_EQ(result.dataformat, "DB");
    EXPECT_EQ(result.file, "out.s2p");
    EXPECT_EQ(result.width, "10");
    EXPECT_EQ(result.precision, "6");
    EXPECT_EQ(result.sweep_mode, "DEC");
    EXPECT_EQ(result.points, "100");
    EXPECT_EQ(result.start, "1k");
    EXPECT_EQ(result.end, "1M");
    EXPECT_EQ(result.data_table_name, "");
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "AC");
    EXPECT_EQ(result.print_parameters->print_format, "RAW");
    EXPECT_EQ(result.print_parameters->print_file, "lin.raw");
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

TEST_F(UiLinParametersPanelTest, build_with_print_section_enabled) {
    // arrange
    LinParametersPanel panel(m_parent);
    auto print_params = PrintParameters("AC", "CSV", "data.csv", {"V(1)", "V(2)"}, {});
    LinSimulationParameters input(true, "TOUCHSTONE2", "S", "RI", "", "", "", "LIN", "", "", "", "", print_params);
    // act
    panel.apply(input);
    auto result = panel.build_lin_parameters();
    // assert
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "AC");
    EXPECT_EQ(result.print_parameters->print_format, "CSV");
    EXPECT_EQ(result.print_parameters->print_file, "data.csv");
}

TEST_F(UiLinParametersPanelTest, build_without_print_section) {
    // arrange
    LinParametersPanel panel(m_parent);
    LinSimulationParameters input(true, "TOUCHSTONE2", "S", "RI", "", "", "", "LIN", "", "", "", "", std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_lin_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// apply — disabling print section from a previously enabled state
// ========================================================================================

TEST_F(UiLinParametersPanelTest, apply_without_print_params_disables_print_section) {
    // arrange
    LinParametersPanel panel(m_parent);
    auto print_params = PrintParameters("AC", "RAW", "lin.raw", {}, {});
    LinSimulationParameters with_print(true, "TOUCHSTONE2", "S", "RI", "", "", "", "LIN", "", "", "", "", print_params);
    panel.apply(with_print);
    ASSERT_TRUE(panel.build_lin_parameters().print_parameters.has_value());
    // act — apply without print parameters
    LinSimulationParameters without_print(true, "TOUCHSTONE2", "S", "RI", "", "", "", "LIN", "", "", "", "", std::nullopt);
    panel.apply(without_print);
    auto result = panel.build_lin_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with DATA sweep
// ========================================================================================

TEST_F(UiLinParametersPanelTest, build_with_data_sweep) {
    // arrange
    LinParametersPanel panel(m_parent);
    LinSimulationParameters input(true, "TOUCHSTONE2", "S", "RI", "", "", "", "DATA", "", "", "", "my_table", std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_lin_parameters();
    // assert
    EXPECT_EQ(result.sweep_mode, "DATA");
    EXPECT_EQ(result.data_table_name, "my_table");
}
