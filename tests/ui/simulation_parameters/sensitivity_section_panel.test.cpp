#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/sens_parameter.h"
#include "ui/simulation_parameters/sensitivity_section_panel.h"

namespace
{
    // parent frame fixture for wxPanel-based tests
    class SensitivitySectionPanelTest : public ::testing::Test
    {
    protected:
        void SetUp() override { m_parent = new wxFrame(nullptr, wxID_ANY, "test"); }

        void TearDown() override { delete m_parent; }

        wxFrame* m_parent = nullptr;
    };

    // helper — recursively find a wxCheckBox whose label contains text
    wxCheckBox* find_cb_by_label(wxWindow& parent, const wxString& fragment) {
        for (auto* child : parent.GetChildren()) {
            auto* cb = dynamic_cast<wxCheckBox*>(child);
            if (cb && cb->GetLabel().Contains(fragment)) {
                return cb;
            }
            auto* found = find_cb_by_label(*child, fragment);
            if (found) {
                return found;
            }
        }
        return nullptr;
    }

    // helper — recursively find a wxChoice whose first string matches
    wxChoice* find_choice_by_string(wxWindow& parent, const wxString& item) {
        for (auto* child : parent.GetChildren()) {
            auto* ch = dynamic_cast<wxChoice*>(child);
            if (ch && ch->GetString(0) == item) {
                return ch;
            }
            auto* found = find_choice_by_string(*child, item);
            if (found) {
                return found;
            }
        }
        return nullptr;
    }

    // helper — recursively find first wxTextCtrl
    wxTextCtrl* find_text_ctrl(wxWindow& parent) {
        for (auto* child : parent.GetChildren()) {
            auto* tc = dynamic_cast<wxTextCtrl*>(child);
            if (tc) {
                return tc;
            }
            auto* found = find_text_ctrl(*child);
            if (found) {
                return found;
            }
        }
        return nullptr;
    }
    // helper — collect all wxTextCtrl descendants into a vector
    void collect_text_ctrls(wxWindow& parent, std::vector<wxTextCtrl*>& out) {
        for (auto* child : parent.GetChildren()) {
            auto* tc = dynamic_cast<wxTextCtrl*>(child);
            if (tc) {
                out.push_back(tc);
            }
            collect_text_ctrls(*child, out);
        }
    }
} // namespace

// ========================================================================================
// constructor
// ========================================================================================

TEST_F(SensitivitySectionPanelTest, default_state_is_disabled) {
    // arrange / act
    SensitivitySectionPanel panel(m_parent);
    // assert
    ASSERT_FALSE(panel.build_sens_parameter("AC").has_value());
}

TEST_F(SensitivitySectionPanelTest, contains_enable_checkbox) {
    // arrange / act
    SensitivitySectionPanel panel(m_parent);
    // assert
    ASSERT_NE(find_cb_by_label(panel, "Enable"), nullptr);
}

TEST_F(SensitivitySectionPanelTest, contains_objective_mode_choice) {
    // arrange / act
    SensitivitySectionPanel panel(m_parent);
    // assert
    ASSERT_NE(find_choice_by_string(panel, "objfunc"), nullptr);
}

TEST_F(SensitivitySectionPanelTest, contains_direct_checkbox) {
    // arrange / act
    SensitivitySectionPanel panel(m_parent);
    // assert
    ASSERT_NE(find_cb_by_label(panel, "Direct"), nullptr);
}

TEST_F(SensitivitySectionPanelTest, contains_adjoint_checkbox) {
    // arrange / act
    SensitivitySectionPanel panel(m_parent);
    // assert
    ASSERT_NE(find_cb_by_label(panel, "Adjoint"), nullptr);
}

// ========================================================================================
// build_sens_parameter — disabled
// ========================================================================================

TEST_F(SensitivitySectionPanelTest, build_returns_nullopt_when_disabled) {
    // arrange / act
    SensitivitySectionPanel panel(m_parent);
    // assert — the enable checkbox starts unchecked
    ASSERT_FALSE(panel.build_sens_parameter("AC").has_value());
}

// ========================================================================================
// build_sens_parameter — enabled
// ========================================================================================

TEST_F(SensitivitySectionPanelTest, build_returns_params_with_analysis_type) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto* enable_cb = find_cb_by_label(panel, "Enable");
    ASSERT_NE(enable_cb, nullptr);
    enable_cb->SetValue(true);
    // act
    auto result = panel.build_sens_parameter("AC");
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->analysis_context, "AC");
}

TEST_F(SensitivitySectionPanelTest, build_returns_params_with_selected_objective_mode) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto* enable_cb = find_cb_by_label(panel, "Enable");
    ASSERT_NE(enable_cb, nullptr);
    enable_cb->SetValue(true);
    auto* mode_choice = find_choice_by_string(panel, "objfunc");
    ASSERT_NE(mode_choice, nullptr);
    mode_choice->SetSelection(1); // "objvars"
    // act
    auto result = panel.build_sens_parameter("DC");
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->objective_mode, "objvars");
}

TEST_F(SensitivitySectionPanelTest, build_returns_params_with_objective_values) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto* enable_cb = find_cb_by_label(panel, "Enable");
    ASSERT_NE(enable_cb, nullptr);
    enable_cb->SetValue(true);
    // set comma-separated objective values
    auto* tc = find_text_ctrl(panel);
    ASSERT_NE(tc, nullptr);
    // there are multiple text controls; set the first hint-matching one
    tc->SetValue("V(1), I(R1)");
    // act
    auto result = panel.build_sens_parameter("TRAN");
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->objective_values.size(), 2);
    EXPECT_EQ(result->objective_values[0], "V(1)");
    EXPECT_EQ(result->objective_values[1], "I(R1)");
}

TEST_F(SensitivitySectionPanelTest, build_returns_params_with_parameters) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto* enable_cb = find_cb_by_label(panel, "Enable");
    ASSERT_NE(enable_cb, nullptr);
    enable_cb->SetValue(true);
    // find all text controls and set the second one (parameters field)
    std::vector<wxTextCtrl*> text_ctrls;
    collect_text_ctrls(panel, text_ctrls);
    ASSERT_GE(text_ctrls.size(), 2);
    text_ctrls[1]->SetValue("R1, C1");
    // act
    auto result = panel.build_sens_parameter("AC");
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->parameter_list.size(), 2);
    EXPECT_EQ(result->parameter_list[0], "R1");
    EXPECT_EQ(result->parameter_list[1], "C1");
}

TEST_F(SensitivitySectionPanelTest, build_returns_params_with_direct_flag) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto* enable_cb = find_cb_by_label(panel, "Enable");
    ASSERT_NE(enable_cb, nullptr);
    enable_cb->SetValue(true);
    auto* direct_cb = find_cb_by_label(panel, "Direct");
    ASSERT_NE(direct_cb, nullptr);
    direct_cb->SetValue(true);
    // act
    auto result = panel.build_sens_parameter("DC");
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->direct);
}

TEST_F(SensitivitySectionPanelTest, build_returns_params_with_adjoint_flag) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto* enable_cb = find_cb_by_label(panel, "Enable");
    ASSERT_NE(enable_cb, nullptr);
    enable_cb->SetValue(true);
    auto* adjoint_cb = find_cb_by_label(panel, "Adjoint");
    ASSERT_NE(adjoint_cb, nullptr);
    adjoint_cb->SetValue(true);
    // act
    auto result = panel.build_sens_parameter("TRAN");
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->adjoint);
}

// ========================================================================================
// apply
// ========================================================================================

TEST_F(SensitivitySectionPanelTest, apply_nullptr_disables) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto* enable_cb = find_cb_by_label(panel, "Enable");
    ASSERT_NE(enable_cb, nullptr);
    enable_cb->SetValue(true);
    ASSERT_TRUE(panel.build_sens_parameter("AC").has_value());
    // act
    panel.apply(nullptr);
    // assert
    ASSERT_FALSE(panel.build_sens_parameter("AC").has_value());
}

TEST_F(SensitivitySectionPanelTest, apply_restores_sens_parameter) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto pp = PrintParameters("SENS", "CSV", "sens.csv", {"V(*)"}, {});
    auto sens = SensParameter("AC", "objvars", {"V(1)", "I(R1)"}, {"R1", "C1"}, true, true, std::move(pp));
    // act
    panel.apply(&sens);
    // assert
    auto result = panel.build_sens_parameter("AC");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->analysis_context, "AC");
    EXPECT_EQ(result->objective_mode, "objvars");
    ASSERT_EQ(result->objective_values.size(), 2);
    EXPECT_EQ(result->objective_values[0], "V(1)");
    EXPECT_EQ(result->objective_values[1], "I(R1)");
    ASSERT_EQ(result->parameter_list.size(), 2);
    EXPECT_EQ(result->parameter_list[0], "R1");
    EXPECT_EQ(result->parameter_list[1], "C1");
    EXPECT_TRUE(result->direct);
    EXPECT_TRUE(result->adjoint);
    ASSERT_TRUE(result->print_parameters.has_value());
    EXPECT_EQ(result->print_parameters->print_format, "CSV");
    EXPECT_EQ(result->print_parameters->print_file, "sens.csv");
}

TEST_F(SensitivitySectionPanelTest, apply_restores_without_print) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto sens = SensParameter("TRAN", "objfunc", {}, {}, false, false, std::nullopt);
    // act
    panel.apply(&sens);
    // assert
    auto result = panel.build_sens_parameter("TRAN");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->analysis_context, "TRAN");
    EXPECT_EQ(result->objective_mode, "objfunc");
    EXPECT_TRUE(result->objective_values.empty());
    EXPECT_TRUE(result->parameter_list.empty());
    EXPECT_FALSE(result->direct);
    EXPECT_FALSE(result->adjoint);
    EXPECT_FALSE(result->print_parameters.has_value());
}

// ========================================================================================
// enable / disable body controls
// ========================================================================================

TEST_F(SensitivitySectionPanelTest, body_controls_disabled_by_default) {
    // arrange / act
    SensitivitySectionPanel panel(m_parent);
    auto* mode_choice = find_choice_by_string(panel, "objfunc");
    ASSERT_NE(mode_choice, nullptr);
    // assert — all gated fields are disabled initially
    ASSERT_FALSE(mode_choice->IsEnabled());
}

TEST_F(SensitivitySectionPanelTest, checking_enable_checkbox_enables_body_controls) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto* enable_cb = find_cb_by_label(panel, "Enable");
    ASSERT_NE(enable_cb, nullptr);
    auto* mode_choice = find_choice_by_string(panel, "objfunc");
    ASSERT_NE(mode_choice, nullptr);
    ASSERT_FALSE(mode_choice->IsEnabled());
    // act — simulate checking the enable checkbox
    enable_cb->SetValue(true);
    wxCommandEvent evt(wxEVT_CHECKBOX, enable_cb->GetId());
    evt.SetInt(1);
    enable_cb->GetEventHandler()->ProcessEvent(evt);
    // assert
    EXPECT_TRUE(mode_choice->IsEnabled());
}

TEST_F(SensitivitySectionPanelTest, unchecking_enable_checkbox_disables_body_controls) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto* enable_cb = find_cb_by_label(panel, "Enable");
    ASSERT_NE(enable_cb, nullptr);
    auto* mode_choice = find_choice_by_string(panel, "objfunc");
    ASSERT_NE(mode_choice, nullptr);
    // enable first
    enable_cb->SetValue(true);
    wxCommandEvent evt_on(wxEVT_CHECKBOX, enable_cb->GetId());
    evt_on.SetInt(1);
    enable_cb->GetEventHandler()->ProcessEvent(evt_on);
    ASSERT_TRUE(mode_choice->IsEnabled());
    // act — simulate unchecking the enable checkbox
    enable_cb->SetValue(false);
    wxCommandEvent evt_off(wxEVT_CHECKBOX, enable_cb->GetId());
    evt_off.SetInt(0);
    enable_cb->GetEventHandler()->ProcessEvent(evt_off);
    // assert — all gated controls should be disabled
    EXPECT_FALSE(mode_choice->IsEnabled());
}

TEST_F(SensitivitySectionPanelTest, apply_enables_body_when_params_provided) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto sens = SensParameter("AC", "objfunc", {}, {}, false, false, std::nullopt);
    auto* mode_choice = find_choice_by_string(panel, "objfunc");
    ASSERT_NE(mode_choice, nullptr);
    ASSERT_FALSE(mode_choice->IsEnabled());
    // act
    panel.apply(&sens);
    // assert
    EXPECT_TRUE(mode_choice->IsEnabled());
}

TEST_F(SensitivitySectionPanelTest, apply_disables_body_when_nullptr) {
    // arrange
    SensitivitySectionPanel panel(m_parent);
    auto sens = SensParameter("AC", "objfunc", {}, {}, false, false, std::nullopt);
    panel.apply(&sens);
    auto* mode_choice = find_choice_by_string(panel, "objfunc");
    ASSERT_NE(mode_choice, nullptr);
    ASSERT_TRUE(mode_choice->IsEnabled());
    // act
    panel.apply(nullptr);
    // assert
    EXPECT_FALSE(mode_choice->IsEnabled());
}
