#include <gtest/gtest.h>
#include <wx/checkbox.h>
#include <wx/frame.h>
#include <wx/panel.h>

#include "ui/simulation_parameters/global_settings_panel.h"

namespace
{
    // parent frame fixture for wxPanel-based tests
    class GlobalSettingsPanelTest : public ::testing::Test
    {
    protected:
        void SetUp() override {
            m_parent = new wxFrame(nullptr, wxID_ANY, "test");
        }

        void TearDown() override {
            delete m_parent;
        }

        wxFrame* m_parent = nullptr;
    };
} // namespace

// ========================================================================================
// constructor
// ========================================================================================

TEST_F(GlobalSettingsPanelTest, constructor_creates_panel) {
    // arrange / act
    GlobalSettingsPanel panel(m_parent);
    // assert
    ASSERT_FALSE(panel.get_replace_ground());
}

// ========================================================================================
// get_replace_ground / set_replace_ground
// ========================================================================================

TEST_F(GlobalSettingsPanelTest, get_replace_ground_returns_false_by_default) {
    // arrange / act
    GlobalSettingsPanel panel(m_parent);
    // assert
    ASSERT_FALSE(panel.get_replace_ground());
}

TEST_F(GlobalSettingsPanelTest, set_replace_ground_true_updates_state) {
    // arrange
    GlobalSettingsPanel panel(m_parent);
    // act
    panel.set_replace_ground(true);
    // assert
    ASSERT_TRUE(panel.get_replace_ground());
}

TEST_F(GlobalSettingsPanelTest, set_replace_ground_false_after_true_works) {
    // arrange
    GlobalSettingsPanel panel(m_parent);
    panel.set_replace_ground(true);
    // act
    panel.set_replace_ground(false);
    // assert
    ASSERT_FALSE(panel.get_replace_ground());
}
