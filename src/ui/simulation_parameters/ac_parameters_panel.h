#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../../simulation_parameters/ac_simulation_parameters.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"

// panel for editing AC analysis parameters inside a SimulationCard
class AcParametersPanel : public wxPanel
{
public:
    explicit AcParametersPanel(wxWindow* parent);

    // read the current control state and build an AcSimulationParameters model
    [[nodiscard]] AcSimulationParameters build_ac_parameters() const;

    // populate controls from a saved AcSimulationParameters model
    void apply(const AcSimulationParameters& params);

    // access sub-panels for fine-grained control
    [[nodiscard]] GlobalSettingsPanel* get_global_settings() const;
    [[nodiscard]] PrintSectionPanel* get_print_section() const;

private:
    SimulationCard* m_card = nullptr;
    GlobalSettingsPanel* m_global_settings = nullptr;
    PrintSectionPanel* m_print_section = nullptr;

    wxChoice* m_sweep_mode_choice = nullptr;
    wxStaticText* m_points_label = nullptr;
    wxTextCtrl* m_points_text = nullptr;
    wxStaticText* m_start_label = nullptr;
    wxTextCtrl* m_start_text = nullptr;
    wxStaticText* m_end_label = nullptr;
    wxTextCtrl* m_end_text = nullptr;
    wxStaticText* m_data_table_label = nullptr;
    wxTextCtrl* m_data_table_text = nullptr;

    wxTextCtrl* m_measure_text = nullptr;

    // show/hide and reset the fields relevant to the selected sweep mode
    void on_sweep_mode_changed();
};
