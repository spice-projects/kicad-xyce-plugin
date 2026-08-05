#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/op_simulation_parameters.h"
#include "ui/simulation_parameters/op_parameters_panel.h"

namespace
{
    // parent frame fixture for wxPanel-based tests
    class OpParametersPanelTest : public ::testing::Test
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

TEST_F(OpParametersPanelTest, constructor_creates_panel) {
    // arrange / act
    OpParametersPanel panel(m_parent);
    // assert
    ASSERT_NE(panel.get_global_settings(), nullptr);
    ASSERT_NE(panel.get_print_section(), nullptr);
}

// ========================================================================================
// build_op_parameters — default state
// ========================================================================================

TEST_F(OpParametersPanelTest, build_returns_defaults_by_default) {
    // arrange / act
    OpParametersPanel panel(m_parent);
    auto result = panel.build_op_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
    EXPECT_FALSE(result.save_enabled);
    EXPECT_EQ(result.save_type, "NODESET");
    EXPECT_TRUE(result.save_file.empty());
    EXPECT_TRUE(result.nodeset_entries.empty());
    EXPECT_TRUE(result.ic_entries.empty());
}

// ========================================================================================
// build with field values via apply/round-trip
// ========================================================================================

TEST_F(OpParametersPanelTest, build_after_apply_returns_same_values) {
    // arrange
    OpParametersPanel panel(m_parent);
    auto print_params = PrintParameters("DC", "RAW", "out.raw", {"V(*)", "IC(*)"}, {});
    auto nodeset_entries = std::vector<NodesetEntry>{NodesetEntry("1", "5.0"), NodesetEntry("2", "3.3")};
    auto ic_entries = std::vector<IcEntry>{IcEntry("out", "1.0"), IcEntry("in", "0")};
    OpSimulationParameters input(true, false, false, std::vector<std::string>{}, "", "", true, "IC", "save.dat", nodeset_entries, ic_entries, print_params);
    // act
    panel.apply(input);
    auto result = panel.build_op_parameters();
    // assert
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "DC");
    EXPECT_EQ(result.print_parameters->print_format, "RAW");
    EXPECT_EQ(result.print_parameters->print_file, "out.raw");
    // IC(*) triggers BJT lead expansion: V(*) + IB, IC, IE, IS
    ASSERT_EQ(result.print_parameters->output_variables.size(), 5);
    EXPECT_EQ(result.print_parameters->output_variables[0], "V(*)");
    EXPECT_EQ(result.print_parameters->output_variables[1], "IB(*)");
    EXPECT_EQ(result.print_parameters->output_variables[2], "IC(*)");
    EXPECT_EQ(result.print_parameters->output_variables[3], "IE(*)");
    EXPECT_EQ(result.print_parameters->output_variables[4], "IS(*)");

    EXPECT_TRUE(result.save_enabled);
    EXPECT_EQ(result.save_type, "IC");
    EXPECT_EQ(result.save_file, "save.dat");

    ASSERT_EQ(result.nodeset_entries.size(), 2);
    EXPECT_EQ(result.nodeset_entries[0].node, "1");
    EXPECT_EQ(result.nodeset_entries[0].voltage, "5.0");
    EXPECT_EQ(result.nodeset_entries[1].node, "2");
    EXPECT_EQ(result.nodeset_entries[1].voltage, "3.3");

    ASSERT_EQ(result.ic_entries.size(), 2);
    EXPECT_EQ(result.ic_entries[0].node, "out");
    EXPECT_EQ(result.ic_entries[0].voltage, "1.0");
    EXPECT_EQ(result.ic_entries[1].node, "in");
    EXPECT_EQ(result.ic_entries[1].voltage, "0");
}

// ========================================================================================
// build with save section enabled
// ========================================================================================

TEST_F(OpParametersPanelTest, build_with_save_section) {
    // arrange
    OpParametersPanel panel(m_parent);
    auto nodeset = std::vector<NodesetEntry>{};
    auto ic = std::vector<IcEntry>{};
    OpSimulationParameters input(true, false, false, std::vector<std::string>{}, "", "", true, "NODESET", "op.dat", nodeset, ic, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_op_parameters();
    // assert
    EXPECT_TRUE(result.save_enabled);
    EXPECT_EQ(result.save_type, "NODESET");
    EXPECT_EQ(result.save_file, "op.dat");
}

// ========================================================================================
// build with nodeset entries
// ========================================================================================

TEST_F(OpParametersPanelTest, build_with_nodeset_entries) {
    // arrange
    OpParametersPanel panel(m_parent);
    auto nodeset = std::vector<NodesetEntry>{NodesetEntry("1", "5.0"), NodesetEntry("2", "3.3")};
    OpSimulationParameters input(false, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", nodeset, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_op_parameters();
    // assert
    ASSERT_EQ(result.nodeset_entries.size(), 2);
    EXPECT_EQ(result.nodeset_entries[0].node, "1");
    EXPECT_EQ(result.nodeset_entries[0].voltage, "5.0");
    EXPECT_EQ(result.nodeset_entries[1].node, "2");
    EXPECT_EQ(result.nodeset_entries[1].voltage, "3.3");
}

// ========================================================================================
// build with IC entries
// ========================================================================================

TEST_F(OpParametersPanelTest, build_with_ic_entries) {
    // arrange
    OpParametersPanel panel(m_parent);
    auto ic = std::vector<IcEntry>{IcEntry("out", "1.0"), IcEntry("in", "0")};
    OpSimulationParameters input(false, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", {}, ic, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_op_parameters();
    // assert
    ASSERT_EQ(result.ic_entries.size(), 2);
    EXPECT_EQ(result.ic_entries[0].node, "out");
    EXPECT_EQ(result.ic_entries[0].voltage, "1.0");
    EXPECT_EQ(result.ic_entries[1].node, "in");
    EXPECT_EQ(result.ic_entries[1].voltage, "0");
}

// ========================================================================================
// build with print section
// ========================================================================================

TEST_F(OpParametersPanelTest, build_with_print_section_enabled) {
    // arrange
    OpParametersPanel panel(m_parent);
    auto print_params = PrintParameters("DC", "CSV", "data.csv", {"V(1)", "V(2)"}, {});
    OpSimulationParameters input(true, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", {}, {}, print_params);
    // act
    panel.apply(input);
    auto result = panel.build_op_parameters();
    // assert
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "DC");
    EXPECT_EQ(result.print_parameters->print_format, "CSV");
    EXPECT_EQ(result.print_parameters->print_file, "data.csv");
}

TEST_F(OpParametersPanelTest, build_without_print_section) {
    // arrange
    OpParametersPanel panel(m_parent);
    OpSimulationParameters input(false, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", {}, {}, std::nullopt);
    // act
    panel.apply(input);
    auto result = panel.build_op_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// apply — disabling print section from a previously enabled state
// ========================================================================================

TEST_F(OpParametersPanelTest, apply_without_print_params_disables_print_section) {
    // arrange
    OpParametersPanel panel(m_parent);
    auto print_params = PrintParameters("DC", "RAW", "out.raw", {}, {});
    OpSimulationParameters with_print(true, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", {}, {}, print_params);
    panel.apply(with_print);
    ASSERT_TRUE(panel.build_op_parameters().print_parameters.has_value());
    // act — apply without print parameters
    OpSimulationParameters without_print(false, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", {}, {}, std::nullopt);
    panel.apply(without_print);
    auto result = panel.build_op_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}
