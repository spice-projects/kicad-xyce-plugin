#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/hb_simulation_parameters.h"
#include "ui/simulation_parameters/hb_parameters_panel.h"

namespace
{
    class HbParametersPanelTest : public ::testing::Test
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

TEST_F(HbParametersPanelTest, constructor_creates_panel) {
    // arrange / act
    HbParametersPanel panel(m_parent);
    // assert
    ASSERT_NE(panel.get_global_settings(), nullptr);
    ASSERT_NE(panel.get_print_section(), nullptr);
}

// ========================================================================================
// build_hb_parameters — default state
// ========================================================================================

TEST_F(HbParametersPanelTest, build_returns_defaults_by_default) {
    // arrange / act
    HbParametersPanel panel(m_parent);
    auto result = panel.build_hb_parameters();
    // assert
    EXPECT_TRUE(result.frequencies.empty());
    EXPECT_TRUE(result.harmonics.empty());
    EXPECT_FALSE(result.tahb.has_value());
    EXPECT_FALSE(result.selectharms.has_value());
    EXPECT_FALSE(result.startup_periods.has_value());
    EXPECT_FALSE(result.replace_ground);
    EXPECT_FALSE(result.print_parameters.has_value());
    EXPECT_TRUE(result.nonlin_options.empty());
    EXPECT_TRUE(result.linsol_options.empty());
}

// ========================================================================================
// build with field values via apply/round-trip
// ========================================================================================

TEST_F(HbParametersPanelTest, build_after_apply_returns_same_values) {
    // arrange
    HbParametersPanel panel(m_parent);
    std::map<std::string, std::string> nonlin_opts;
    nonlin_opts["MAXITER"] = "50";
    nonlin_opts["ABSTOL"] = "1e-12";
    std::map<std::string, std::string> linsol_opts;
    linsol_opts["METHOD"] = "KINSOL";
    auto print_params = PrintParameters("HB_FD", "CSV", "hb.csv", {"V(1)", "I(V1)"}, {});
    HbSimulationParameters input({"1e6", "2e6"}, {5, 7, 9}, 100, "1,3,5", 10, true, print_params, nonlin_opts, linsol_opts);
    // act
    panel.apply(input);
    auto result = panel.build_hb_parameters();
    // assert
    ASSERT_EQ(result.frequencies.size(), 2);
    EXPECT_EQ(result.frequencies[0], "1e6");
    EXPECT_EQ(result.frequencies[1], "2e6");
    ASSERT_EQ(result.harmonics.size(), 3);
    EXPECT_EQ(result.harmonics[0], 5);
    EXPECT_EQ(result.harmonics[1], 7);
    EXPECT_EQ(result.harmonics[2], 9);
    ASSERT_TRUE(result.tahb.has_value());
    EXPECT_EQ(*result.tahb, 100);
    ASSERT_TRUE(result.selectharms.has_value());
    EXPECT_EQ(*result.selectharms, "1,3,5");
    ASSERT_TRUE(result.startup_periods.has_value());
    EXPECT_EQ(*result.startup_periods, 10);
    EXPECT_TRUE(result.replace_ground);
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "HB_FD");
    EXPECT_EQ(result.print_parameters->print_format, "CSV");
    EXPECT_EQ(result.print_parameters->print_file, "hb.csv");
    ASSERT_EQ(result.nonlin_options.size(), 2);
    EXPECT_EQ(result.nonlin_options.at("MAXITER"), "50");
    EXPECT_EQ(result.nonlin_options.at("ABSTOL"), "1e-12");
    ASSERT_EQ(result.linsol_options.size(), 1);
    EXPECT_EQ(result.linsol_options.at("METHOD"), "KINSOL");
}

// ========================================================================================
// build with replace_ground
// ========================================================================================

TEST_F(HbParametersPanelTest, build_with_replace_ground) {
    // arrange
    HbParametersPanel panel(m_parent);
    HbSimulationParameters input({}, {}, std::nullopt, std::nullopt, std::nullopt, true, std::nullopt, {}, {});
    // act
    panel.apply(input);
    auto result = panel.build_hb_parameters();
    // assert
    EXPECT_TRUE(result.replace_ground);
}

// ========================================================================================
// build with print section
// ========================================================================================

TEST_F(HbParametersPanelTest, build_with_print_section_enabled) {
    // arrange
    HbParametersPanel panel(m_parent);
    auto print_params = PrintParameters("HB", "RAW", "out.raw", {"V(*)"}, {});
    HbSimulationParameters input({}, {}, std::nullopt, std::nullopt, std::nullopt, false, print_params, {}, {});
    // act
    panel.apply(input);
    auto result = panel.build_hb_parameters();
    // assert
    ASSERT_TRUE(result.print_parameters.has_value());
    EXPECT_EQ(result.print_parameters->print_type, "HB");
    EXPECT_EQ(result.print_parameters->print_format, "RAW");
    EXPECT_EQ(result.print_parameters->print_file, "out.raw");
}

TEST_F(HbParametersPanelTest, build_without_print_section) {
    // arrange
    HbParametersPanel panel(m_parent);
    HbSimulationParameters input({}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
    // act
    panel.apply(input);
    auto result = panel.build_hb_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// apply — disabling print section from a previously enabled state
// ========================================================================================

TEST_F(HbParametersPanelTest, apply_without_print_params_disables_print_section) {
    // arrange
    HbParametersPanel panel(m_parent);
    auto print_params = PrintParameters("HB", "RAW", "out.raw", {}, {});
    HbSimulationParameters with_print({}, {}, std::nullopt, std::nullopt, std::nullopt, false, print_params, {}, {});
    panel.apply(with_print);
    ASSERT_TRUE(panel.build_hb_parameters().print_parameters.has_value());
    // act — apply without print parameters
    HbSimulationParameters without_print({}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {});
    panel.apply(without_print);
    auto result = panel.build_hb_parameters();
    // assert
    EXPECT_FALSE(result.print_parameters.has_value());
}

// ========================================================================================
// build with nonlin options
// ========================================================================================

TEST_F(HbParametersPanelTest, build_with_nonlin_options) {
    // arrange
    HbParametersPanel panel(m_parent);
    std::map<std::string, std::string> nonlin_opts;
    nonlin_opts["MAXITER"] = "100";
    nonlin_opts["RELAXTYPE"] = "LINEAR";
    HbSimulationParameters input({}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, nonlin_opts, {});
    // act
    panel.apply(input);
    auto result = panel.build_hb_parameters();
    // assert
    ASSERT_EQ(result.nonlin_options.size(), 2);
    EXPECT_EQ(result.nonlin_options.at("MAXITER"), "100");
    EXPECT_EQ(result.nonlin_options.at("RELAXTYPE"), "LINEAR");
}
