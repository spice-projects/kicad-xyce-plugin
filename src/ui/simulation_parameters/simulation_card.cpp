#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#endif

#include "simulation_card.h"

SimulationCard::SimulationCard(wxWindow* parent, const wxString& title, const wxString& badge) :
    wxPanel(parent) {
    // create the static box that provides the native card border
    auto* box = new wxStaticBox(this, wxID_ANY, wxEmptyString);
    // wrap the box in a vertical sizer
    auto* box_sizer = new wxStaticBoxSizer(box, wxVERTICAL);
    // header panel for the title row and optional badge
    auto* header = new wxPanel(box, wxID_ANY);
    // horizontal sizer for header contents
    auto* header_sizer = new wxBoxSizer(wxHORIZONTAL);
    // title text rendered in bold
    m_title_text = new wxStaticText(header, wxID_ANY, title);
    wxFont title_font = m_title_text->GetFont();
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_title_text->SetFont(title_font);
    // title fills remaining header space, vertically centered
    header_sizer->Add(m_title_text, 1, wxALIGN_CENTER_VERTICAL);
    // optional badge shown on the right side of the header
    if (!badge.IsEmpty()) {
        // create badge text with bold font
        m_badge_text = new wxStaticText(header, wxID_ANY, badge);
        wxFont badge_font = m_badge_text->GetFont();
        badge_font.SetWeight(wxFONTWEIGHT_BOLD);
        m_badge_text->SetFont(badge_font);
        // badge right-aligned with left margin from the title
        header_sizer->Add(m_badge_text, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(12));
    }
    // attach sizer to header and add to box sizer with padding
    header->SetSizer(header_sizer);
    box_sizer->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
    // content panel where callers add child controls
    m_content = new wxPanel(box, wxID_ANY);
    box_sizer->Add(m_content, 1, wxEXPAND | wxALL, FromDIP(16));
    // attach the box sizer to this panel
    SetSizer(box_sizer);
}

wxPanel* SimulationCard::get_content() const { return m_content; }
