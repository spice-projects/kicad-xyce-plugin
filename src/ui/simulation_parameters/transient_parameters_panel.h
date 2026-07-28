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

#include "../../simulation_parameters/transient_simulation_parameters.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"

// panel for editing transient analysis parameters inside a SimulationCard
class TransientParametersPanel : public wxPanel
{
public:
    explicit TransientParametersPanel(wxWindow* parent);

    // read the current control state and build a TransientSimulationParameters model
    [[nodiscard]] TransientSimulationParameters build_transient_parameters() const;

    // populate controls from a saved TransientSimulationParameters model
    void apply(const TransientSimulationParameters& params);

    // access sub-panels for fine-grained control
    [[nodiscard]] GlobalSettingsPanel* get_global_settings() const;
    [[nodiscard]] PrintSectionPanel* get_print_section() const;

private:
    SimulationCard* m_card = nullptr;
    GlobalSettingsPanel* m_global_settings = nullptr;
    PrintSectionPanel* m_print_section = nullptr;

    wxTextCtrl* m_initial_step_text = nullptr;
    wxTextCtrl* m_final_time_text = nullptr;
    wxTextCtrl* m_start_time_text = nullptr;
    wxTextCtrl* m_step_ceiling_text = nullptr;
    wxChoice* m_op_keyword_choice = nullptr;
    wxTextCtrl* m_schedule_text = nullptr;
};
