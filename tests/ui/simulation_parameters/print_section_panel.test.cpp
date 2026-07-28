#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/print_parameters.h"
#include "ui/simulation_parameters/print_section_panel.h"

namespace
{
    // parent frame fixture for wxPanel-based tests
    class PrintSectionPanelTest : public ::testing::Test
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

    // helper — enable the print section
    void enable_print(wxWindow& panel) {
        auto* cb = find_cb_by_label(panel, "Enable");
        if (cb) {
            cb->SetValue(true);
        }
    }

    // helper — recursively find first wxChoice whose string selection matches
    wxChoice* find_choice(wxWindow& parent, const wxString& selection) {
        for (auto* child : parent.GetChildren()) {
            auto* ch = dynamic_cast<wxChoice*>(child);
            if (ch && ch->GetStringSelection() == selection) {
                return ch;
            }
            auto* found = find_choice(*child, selection);
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
} // namespace

// ========================================================================================
// constructor
// ========================================================================================

TEST_F(PrintSectionPanelTest, default_state_is_disabled) {
    // arrange / act
    PrintSectionPanel panel(m_parent, "DC", {"DC"}, false, false, false);
    // assert
    ASSERT_FALSE(panel.build_print_parameters().has_value());
}

TEST_F(PrintSectionPanelTest, without_bjt_fet_no_lead_checkboxes) {
    // arrange / act
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, false, false, false);
    // assert
    ASSERT_EQ(find_cb_by_label(panel, "BJT"), nullptr);
    ASSERT_EQ(find_cb_by_label(panel, "FET"), nullptr);
}

TEST_F(PrintSectionPanelTest, without_power_no_power_checkbox) {
    // arrange / act
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, false, false, false);
    // assert
    ASSERT_EQ(find_cb_by_label(panel, "P(*)"), nullptr);
}

TEST_F(PrintSectionPanelTest, with_power_shows_power_checkbox) {
    // arrange / act
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, true, false, false);
    // assert
    ASSERT_NE(find_cb_by_label(panel, "P(*)"), nullptr);
}

TEST_F(PrintSectionPanelTest, with_bjt_fet_shows_lead_checkboxes) {
    // arrange / act
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, false, true, false);
    // assert
    ASSERT_NE(find_cb_by_label(panel, "BJT"), nullptr);
    ASSERT_NE(find_cb_by_label(panel, "FET"), nullptr);
}

TEST_F(PrintSectionPanelTest, with_print_type_combo_shows_choice) {
    // arrange / act
    PrintSectionPanel panel(m_parent, "DC", {"DC", "HOMOTOPY"}, false, false, true);
    // assert
    ASSERT_NE(find_choice(panel, "DC"), nullptr);
}

TEST_F(PrintSectionPanelTest, without_print_type_combo_no_choice) {
    // arrange / act
    PrintSectionPanel panel(m_parent, "DC", {"DC", "HOMOTOPY"}, false, false, false);
    // assert — no choice should be found with print type items
    ASSERT_EQ(find_choice(panel, "DC"), nullptr);
    ASSERT_EQ(find_choice(panel, "HOMOTOPY"), nullptr);
}

// ========================================================================================
// build_print_parameters — disabled
// ========================================================================================

TEST_F(PrintSectionPanelTest, build_returns_nullopt_when_disabled) {
    // arrange / act
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, false, false, false);
    // assert
    ASSERT_FALSE(panel.build_print_parameters().has_value());
}

// ========================================================================================
// build_print_parameters — enabled with wildcards
// ========================================================================================

TEST_F(PrintSectionPanelTest, build_returns_params_with_analysis_prefix) {
    // arrange
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, false, false, false);
    enable_print(panel);
    // act
    auto result = panel.build_print_parameters();
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->print_type, "TRAN");
    EXPECT_EQ(result->print_format, "");
    EXPECT_EQ(result->print_file, "");
    EXPECT_TRUE(result->output_variables.empty());
}

TEST_F(PrintSectionPanelTest, build_returns_params_with_wildcards) {
    // arrange
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, true, true, false);
    enable_print(panel);
    // set V(*) and I(*) wildcards
    auto* v_cb = find_cb_by_label(panel, "V(*)");
    auto* i_cb = find_cb_by_label(panel, "I(*)");
    ASSERT_NE(v_cb, nullptr);
    ASSERT_NE(i_cb, nullptr);
    v_cb->SetValue(true);
    i_cb->SetValue(true);
    // act
    auto result = panel.build_print_parameters();
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->print_type, "TRAN");
    ASSERT_GE(result->output_variables.size(), 2);
    EXPECT_EQ(result->output_variables[0], "V(*)");
    EXPECT_EQ(result->output_variables[1], "I(*)");
}

TEST_F(PrintSectionPanelTest, build_returns_params_with_specific_vars) {
    // arrange
    PrintSectionPanel panel(m_parent, "AC", {"AC", "AC_IC"}, false, false, true);
    enable_print(panel);
    // set V(*) wildcard
    auto* v_cb = find_cb_by_label(panel, "V(*)");
    ASSERT_NE(v_cb, nullptr);
    v_cb->SetValue(true);
    // fill specific vars text field
    auto* tc = find_text_ctrl(panel);
    ASSERT_NE(tc, nullptr);
    tc->SetValue("V(1) I(R1)");
    // act
    auto result = panel.build_print_parameters();
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 3);
    EXPECT_EQ(result->output_variables[0], "V(*)");
    EXPECT_EQ(result->output_variables[1], "V(1)");
    EXPECT_EQ(result->output_variables[2], "I(R1)");
}

TEST_F(PrintSectionPanelTest, build_uses_selected_print_type) {
    // arrange
    PrintSectionPanel panel(m_parent, "DC", {"DC", "HOMOTOPY"}, false, false, true);
    enable_print(panel);
    auto* choice = find_choice(panel, "DC");
    ASSERT_NE(choice, nullptr);
    // select HOMOTOPY (index 1)
    choice->SetSelection(1);
    // act
    auto result = panel.build_print_parameters();
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->print_type, "HOMOTOPY");
}

TEST_F(PrintSectionPanelTest, build_uses_selected_format) {
    // arrange
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, false, false, false);
    enable_print(panel);
    auto* choice = find_choice(panel, "(default)");
    ASSERT_NE(choice, nullptr);
    // select RAW (index 5)
    choice->SetSelection(5);
    // act
    auto result = panel.build_print_parameters();
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->print_format, "RAW");
}

TEST_F(PrintSectionPanelTest, build_uses_output_file) {
    // arrange — use apply to correctly target the output file field
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, false, false, false);
    auto pp = PrintParameters("TRAN", "", "output.raw", {}, {});
    // act
    panel.apply(&pp, false, false);
    auto result = panel.build_print_parameters();
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->print_file, "output.raw");
}

// ========================================================================================
// apply
// ========================================================================================

TEST_F(PrintSectionPanelTest, apply_nullptr_disables) {
    // arrange
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, false, false, false);
    enable_print(panel);
    ASSERT_TRUE(panel.build_print_parameters().has_value());
    // act
    panel.apply(nullptr, false, false);
    // assert
    ASSERT_FALSE(panel.build_print_parameters().has_value());
}

TEST_F(PrintSectionPanelTest, apply_restores_print_type_and_format) {
    // arrange
    PrintSectionPanel panel(m_parent, "DC", {"DC", "HOMOTOPY"}, false, false, true);
    auto pp = PrintParameters("HOMOTOPY", "CSV", "output.csv", {}, {});
    // act
    panel.apply(&pp, false, false);
    // assert
    auto result = panel.build_print_parameters();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->print_type, "HOMOTOPY");
    EXPECT_EQ(result->print_format, "CSV");
    EXPECT_EQ(result->print_file, "output.csv");
}

TEST_F(PrintSectionPanelTest, apply_restores_wildcards) {
    // arrange
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, true, true, false);
    auto pp = PrintParameters("TRAN", "", "", {"V(*)", "I(*)", "P(*)"}, {});
    // act
    panel.apply(&pp, false, false);
    // assert
    auto result = panel.build_print_parameters();
    ASSERT_TRUE(result.has_value());
    ASSERT_GE(result->output_variables.size(), 3);
    EXPECT_EQ(result->output_variables[0], "V(*)");
    EXPECT_EQ(result->output_variables[1], "I(*)");
    EXPECT_EQ(result->output_variables[2], "P(*)");
}

TEST_F(PrintSectionPanelTest, apply_restores_bjt_leads_and_deduplicates) {
    // arrange
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, false, true, false);
    auto pp = PrintParameters("TRAN", "", "", {"V(*)", "IC(*)", "IE(*)", "V(1)"}, {});
    // act
    panel.apply(&pp, true, false);
    // assert — BJT leads should be checked, adding IB, IC, IE, IS
    auto result = panel.build_print_parameters();
    ASSERT_TRUE(result.has_value());
    // V(*), IB(*), IC(*), IE(*), IS(*), V(1) = 6 tokens
    ASSERT_EQ(result->output_variables.size(), 6);
    EXPECT_EQ(result->output_variables[0], "V(*)");
    EXPECT_EQ(result->output_variables[1], "IB(*)");
    EXPECT_EQ(result->output_variables[2], "IC(*)");
    EXPECT_EQ(result->output_variables[3], "IE(*)");
    EXPECT_EQ(result->output_variables[4], "IS(*)");
    EXPECT_EQ(result->output_variables[5], "V(1)");
}

TEST_F(PrintSectionPanelTest, apply_bjt_visibility_by_has_bjt) {
    // arrange
    PrintSectionPanel panel(m_parent, "TRAN", {"TRAN"}, false, true, false);
    auto pp = PrintParameters("TRAN", "", "", {}, {});
    // act — apply with has_bjt=false, has_fet=true
    panel.apply(&pp, false, true);
    // assert — BJT leads should be hidden, FET leads visible
    auto* bjt_cb = find_cb_by_label(panel, "BJT");
    auto* fet_cb = find_cb_by_label(panel, "FET");
    ASSERT_NE(bjt_cb, nullptr);
    ASSERT_NE(fet_cb, nullptr);
    ASSERT_FALSE(bjt_cb->IsShown());
    ASSERT_TRUE(fet_cb->IsShown());
}
