#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../../simulation_parameters/noise_simulation_parameters.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"

class NoiseParametersPanel : public wxPanel
{
public:
    explicit NoiseParametersPanel(wxWindow* parent);

    [[nodiscard]] NoiseSimulationParameters build_noise_parameters() const;

    void apply(const NoiseSimulationParameters& params);

    [[nodiscard]] GlobalSettingsPanel* get_global_settings() const;
    [[nodiscard]] PrintSectionPanel* get_print_section() const;

private:
    SimulationCard* m_card = nullptr;
    GlobalSettingsPanel* m_global_settings = nullptr;
    PrintSectionPanel* m_print_section = nullptr;

    wxTextCtrl* m_output_node_text = nullptr;
    wxTextCtrl* m_ref_node_text = nullptr;
    wxTextCtrl* m_source_name_text = nullptr;
    wxTextCtrl* m_start_freq_text = nullptr;
    wxTextCtrl* m_end_freq_text = nullptr;
    wxTextCtrl* m_num_points_text = nullptr;
    wxChoice* m_sweep_type_choice = nullptr;
    wxTextCtrl* m_data_table_text = nullptr;
    wxTextCtrl* m_device_noise_text = nullptr;
};
