#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../../simulation_parameters/dc_simulation_parameters.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"

class DcParametersPanel : public wxPanel
{
public:
    explicit DcParametersPanel(wxWindow* parent);

    [[nodiscard]] DCSimulationParameters build_dc_parameters() const;

    void apply(const DCSimulationParameters& params);

    [[nodiscard]] GlobalSettingsPanel* get_global_settings() const;
    [[nodiscard]] PrintSectionPanel* get_print_section() const;

private:
    SimulationCard* m_card = nullptr;
    GlobalSettingsPanel* m_global_settings = nullptr;
    PrintSectionPanel* m_print_section = nullptr;

    wxChoice* m_sweep_mode_choice = nullptr;
    wxTextCtrl* m_primary_variable_text = nullptr;
    wxTextCtrl* m_start_text = nullptr;
    wxTextCtrl* m_stop_text = nullptr;
    wxTextCtrl* m_step_text = nullptr;
    wxTextCtrl* m_points_text = nullptr;
    wxTextCtrl* m_list_values_text = nullptr;
    wxTextCtrl* m_data_table_text = nullptr;
    wxTextCtrl* m_secondary_variable_text = nullptr;
    wxTextCtrl* m_secondary_start_text = nullptr;
    wxTextCtrl* m_secondary_stop_text = nullptr;
    wxTextCtrl* m_secondary_step_text = nullptr;
    wxTextCtrl* m_secondary_points_text = nullptr;

    wxTextCtrl* m_measure_text = nullptr;
};
