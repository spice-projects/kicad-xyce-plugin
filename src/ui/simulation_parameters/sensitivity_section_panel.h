#pragma once

#include <optional>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/event.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../../simulation_parameters/sens_parameter.h"
#include "print_section_panel.h"
#include "simulation_card.h"

// embedded .SENS section shared by AC, DC, and TRAN panels
class SensitivitySectionPanel : public wxPanel
{
public:
    explicit SensitivitySectionPanel(wxWindow* parent);

    // read the current control state and build a SensParameter model;
    // returns nullopt when the sensitivity section is disabled
    [[nodiscard]] std::optional<SensParameter> build_sens_parameter(const wxString& analysis_type) const;

    // populate controls from a saved SensParameter model;
    // pass nullptr to disable the section and reset to defaults
    void apply(const SensParameter* params);

private:
    void on_enable_toggle(wxCommandEvent& event);

    SimulationCard* m_card = nullptr;
    wxCheckBox* m_enable_checkbox = nullptr;
    wxPanel* m_body = nullptr;
    wxChoice* m_objective_mode_choice = nullptr;
    wxTextCtrl* m_objective_values_text = nullptr;
    wxTextCtrl* m_parameters_text = nullptr;
    wxCheckBox* m_direct_checkbox = nullptr;
    wxCheckBox* m_adjoint_checkbox = nullptr;
    PrintSectionPanel* m_print_section = nullptr;
};
