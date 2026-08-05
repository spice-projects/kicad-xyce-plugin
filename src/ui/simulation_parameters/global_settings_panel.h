#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#endif

// panel with the "Replace ground (GND) with 0" checkbox shared across all analysis panels
class GlobalSettingsPanel : public wxPanel
{
public:
    explicit GlobalSettingsPanel(wxWindow* parent);

    // access the replace-ground checkbox state
    [[nodiscard]] bool get_replace_ground() const;
    void set_replace_ground(bool replace);

private:
    wxCheckBox* m_replace_ground_checkbox = nullptr;
    wxStaticText* m_explanation = nullptr;
};
