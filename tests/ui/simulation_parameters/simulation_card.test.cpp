#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#endif

#include "ui/simulation_parameters/simulation_card.h"

namespace
{
    // parent frame fixture for wxPanel-based tests
    class SimulationCardTest : public ::testing::Test
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

TEST_F(SimulationCardTest, constructor_creates_card_with_title) {
    // arrange / act
    SimulationCard card(m_parent, "DC Analysis");
    // assert
    ASSERT_NE(card.get_content(), nullptr);
}

TEST_F(SimulationCardTest, constructor_creates_card_with_title_and_badge) {
    // arrange / act
    SimulationCard card(m_parent, "DC Analysis", "optional");
    // assert
    ASSERT_NE(card.get_content(), nullptr);
}

// ========================================================================================
// get_content
// ========================================================================================

TEST_F(SimulationCardTest, get_content_returns_valid_panel) {
    // arrange / act
    SimulationCard card(m_parent, "AC Analysis");
    // assert
    ASSERT_NE(card.get_content(), nullptr);
    ASSERT_TRUE(card.get_content()->IsKindOf(wxCLASSINFO(wxPanel)));
}

TEST_F(SimulationCardTest, get_content_panel_nests_inside_card) {
    // arrange / act
    SimulationCard card(m_parent, "AC Analysis");
    // assert — the content panel is a descendant of the card
    ASSERT_NE(card.get_content(), nullptr);
    ASSERT_EQ(card.get_content()->GetGrandParent(), &card);
}
