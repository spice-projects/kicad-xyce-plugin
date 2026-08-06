#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "simulation_parameters/step_parameters.h"
#include "simulation_card.h"

// embedded .STEP section for each simulation tab page
class StepParametersPanel : public wxPanel
{
public:
    explicit StepParametersPanel(wxWindow* parent);

    // read the current control state and build a StepParameters model
    [[nodiscard]] StepParameters build_step_parameters() const;

    // populate controls from a saved StepParameters model
    void apply(const StepParameters& params);

private:
    void on_enable_toggle(wxCommandEvent& event);

    SimulationCard* m_card = nullptr;
    wxCheckBox* m_enable_cb = nullptr;
    wxPanel* m_body = nullptr;
    wxChoice* m_sweep_mode_choice = nullptr;
    wxTextCtrl* m_variable_text = nullptr;
    wxTextCtrl* m_start_text = nullptr;
    wxTextCtrl* m_stop_text = nullptr;
    wxTextCtrl* m_step_text = nullptr;
    wxTextCtrl* m_points_text = nullptr;
    wxTextCtrl* m_list_values_text = nullptr;
    wxTextCtrl* m_data_table_text = nullptr;
};
