#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../../simulation_parameters/op_simulation_parameters.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"

// panel for editing operating point analysis parameters inside a SimulationCard
class OpParametersPanel : public wxPanel
{
public:
    explicit OpParametersPanel(wxWindow* parent);

    // read the current control state and build an OpSimulationParameters model
    [[nodiscard]] OpSimulationParameters build_op_parameters() const;

    // populate controls from a saved OpSimulationParameters model
    void apply(const OpSimulationParameters& params);

    // access sub-panels for fine-grained control
    [[nodiscard]] GlobalSettingsPanel* get_global_settings() const;
    [[nodiscard]] PrintSectionPanel* get_print_section() const;

private:
    SimulationCard* m_card = nullptr;
    GlobalSettingsPanel* m_global_settings = nullptr;
    PrintSectionPanel* m_print_section = nullptr;

    // save section
    wxCheckBox* m_save_enable_cb = nullptr;
    wxRadioButton* m_save_ic_rb = nullptr;
    wxRadioButton* m_save_nodeset_rb = nullptr;
    wxTextCtrl* m_save_file_text = nullptr;

    // convergence hints and initial conditions
    wxTextCtrl* m_nodeset_text = nullptr;
    wxTextCtrl* m_ic_text = nullptr;
};
