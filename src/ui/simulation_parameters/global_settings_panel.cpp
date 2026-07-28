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
    auto* explanation = new wxStaticText(this, wxID_ANY, "When enabled, all ground (GND) references in the netlist are replaced with the node name '0' before simulation. This is required by Xyce which uses '0' as the global ground node.");
    // use italic font for explanatory text to visually distinguish it
    wxFont label_font = explanation->GetFont();
    label_font.SetStyle(wxFONTSTYLE_ITALIC);
    explanation->SetFont(label_font);
    // use system text colour for readability on any theme
    explanation->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    sizer->Add(explanation, 0, wxTOP, FromDIP(4));
    // attach sizer to this panel
    SetSizer(sizer);
}

bool GlobalSettingsPanel::get_replace_ground() const { return m_replace_ground_checkbox->GetValue(); }

void GlobalSettingsPanel::set_replace_ground(bool replace) { m_replace_ground_checkbox->SetValue(replace); }
