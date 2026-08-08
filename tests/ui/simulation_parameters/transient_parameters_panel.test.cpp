#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/transient_simulation_parameters.h"
#include "ui/simulation_parameters/transient_parameters_panel.h"

namespace
{
    // parent frame fixture for wxPanel-based tests
    class UiTransientParametersPanelTest : public ::testing::Test
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

TEST_F(UiTransientParametersPanelTest, constructor_creates_panel) {
    // arrange / act
    TransientParametersPanel panel(m_parent);
    // assert
    ASSERT_NE(panel.get_global_settings(), nullptr);
    ASSERT_NE(panel.get_print_section(), nullptr);
}

// ========================================================================================
// build_transient_parameters — default state
// ========================================================================================

TEST_F(UiTransientParametersPanelTest, build_returns_empty_fields_by_default) {
    // arrange / act
    TransientParametersPanel panel(m_parent);
    auto result = panel.build_transient_parameters();
    // assert
    EXPECT_EQ(result.initial_step_value, "");
    EXPECT_EQ(result.final_time_value, "");
    EXPECT_EQ(result.start_time_value, "");
    EXPECT_EQ(result.step_ceiling_value, "");
    EXPECT_EQ(result.op_keyword, "");
    EXPECT_TRUE(result.schedule_points.empty());
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with field values via apply/round-trip
// ========================================================================================

TEST_F(UiTransientParametersPanelTest, build_after_apply_returns_same_values) {
    // arrange
    TransientParametersPanel panel(m_parent);
    auto schedule = std::vector<TransientSchedulePoint>{TransientSchedulePoint("1u", "10n"), TransientSchedulePoint("10u", "100n")};
    auto print_params = PrintParameters("TRANADJOINT", "RAW", "out.raw", {"V(*)", "IC(*)"}, {});
    TransientSimulationParameters input("1u", "1m", "0", "5u", "NOOP", schedule, print_params, {}, {}, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_transient_parameters();
    // assert
    EXPECT_EQ(result.initial_step_value, "1u");
    EXPECT_EQ(result.final_time_value, "1m");
    EXPECT_EQ(result.start_time_value, "0");
    EXPECT_EQ(result.step_ceiling_value, "5u");
    EXPECT_EQ(result.op_keyword, "NOOP");
    ASSERT_EQ(result.schedule_points.size(), 2);
    EXPECT_EQ(result.schedule_points[0].time_value, "1u");
    EXPECT_EQ(result.schedule_points[0].max_time_step_value, "10n");
    EXPECT_EQ(result.schedule_points[1].time_value, "10u");
    EXPECT_EQ(result.schedule_points[1].max_time_step_value, "100n");
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "TRANADJOINT");
    EXPECT_EQ(result.print_parameters->print_format, "RAW");
    EXPECT_EQ(result.print_parameters->print_file, "out.raw");
    // the print section expands IB(*) into all four BJT lead wildcards
    ASSERT_EQ(result.print_parameters->output_variables.size(), 5);
    EXPECT_EQ(result.print_parameters->output_variables[0], "V(*)");
    EXPECT_EQ(result.print_parameters->output_variables[1], "IB(*)");
    EXPECT_EQ(result.print_parameters->output_variables[2], "IC(*)");
    EXPECT_EQ(result.print_parameters->output_variables[3], "IE(*)");
    EXPECT_EQ(result.print_parameters->output_variables[4], "IS(*)");
}

TEST_F(UiTransientParametersPanelTest, build_with_noop_keyword) {
    // arrange
    TransientParametersPanel panel(m_parent);
    TransientSimulationParameters input("1u", "1m", "", "", "NOOP", {}, std::nullopt, {}, {}, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_transient_parameters();
    // assert
    EXPECT_EQ(result.op_keyword, "NOOP");
}

TEST_F(UiTransientParametersPanelTest, build_with_uic_keyword) {
    // arrange
    TransientParametersPanel panel(m_parent);
    TransientSimulationParameters input("1u", "1m", "", "", "UIC", {}, std::nullopt, {}, {}, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_transient_parameters();
    // assert
    EXPECT_EQ(result.op_keyword, "UIC");
}

// ========================================================================================
// build with print section
// ========================================================================================

TEST_F(UiTransientParametersPanelTest, build_with_print_section_enabled) {
    // arrange
    TransientParametersPanel panel(m_parent);
    auto print_params = PrintParameters("TRAN", "CSV", "data.csv", {"V(1)", "V(2)"}, {});
    TransientSimulationParameters input("1u", "1m", "", "", "", {}, print_params, {}, {}, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_transient_parameters();
    // assert
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "TRAN");
    EXPECT_EQ(result.print_parameters->print_format, "CSV");
    EXPECT_EQ(result.print_parameters->print_file, "data.csv");
}

TEST_F(UiTransientParametersPanelTest, build_without_print_section) {
    // arrange
    TransientParametersPanel panel(m_parent);
    TransientSimulationParameters input("1u", "1m", "", "", "", {}, std::nullopt, {}, {}, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_transient_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// apply — disabling print section from a previously enabled state
// ========================================================================================

TEST_F(UiTransientParametersPanelTest, apply_without_print_params_disables_print_section) {
    // arrange
    TransientParametersPanel panel(m_parent);
    auto print_params = PrintParameters("TRAN", "RAW", "out.raw", {}, {});
    TransientSimulationParameters with_print("1u", "1m", "", "", "", {}, print_params, {}, {}, {}, std::nullopt);
    panel.apply(with_print);
    ASSERT_TRUE(panel.build_transient_parameters().print_parameters.has_value());
    // act — apply without print parameters
    TransientSimulationParameters without_print("1u", "1m", "", "", "", {}, std::nullopt, {}, {}, {}, std::nullopt);
    panel.apply(without_print);
    auto result = panel.build_transient_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}
