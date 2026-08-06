#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#endif

#include "global_settings_panel.h"

GlobalSettingsPanel::GlobalSettingsPanel(wxWindow* parent) :
    wxPanel(parent) {
    // vertical layout for checkbox and explanation
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    // checkbox for enabling ground replacement preprocessing
    m_replace_ground_checkbox = new wxCheckBox(this, wxID_ANY, "Replace ground (GND) with 0");
    sizer->Add(m_replace_ground_checkbox, 0, wxALL, 0);
    // explanatory label describing why ground replacement is needed
    m_explanation = new wxStaticText(this, wxID_ANY, "When enabled, all ground (GND) references in the netlist are replaced with the node name '0' before simulation. This is required by Xyce which uses '0' as the global ground node.");
    // use italic font for explanatory text to visually distinguish it
    wxFont label_font = m_explanation->GetFont();
    label_font.SetStyle(wxFONTSTYLE_ITALIC);
    m_explanation->SetFont(label_font);
    // use system text colour for readability on any theme
    m_explanation->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    sizer->Add(m_explanation, 0, wxEXPAND | wxTOP, FromDIP(4));
    // attach sizer to this panel
    SetSizer(sizer);
    // wrap the explanation to a bounded width so it never forces the panel
    // (and the containing dialog) wider than the available viewport
    m_explanation->Wrap(FromDIP(520));
    // re-wrap the explanation whenever the panel is resized so the text
    // always fits the available width
    Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
        event.Skip();
        const int width = GetClientSize().GetWidth();
        if (width > 0) {
            m_explanation->Wrap(width);
        }
    });
}

bool GlobalSettingsPanel::get_replace_ground() const { return m_replace_ground_checkbox->GetValue(); }

void GlobalSettingsPanel::set_replace_ground(bool replace) { m_replace_ground_checkbox->SetValue(replace); }
