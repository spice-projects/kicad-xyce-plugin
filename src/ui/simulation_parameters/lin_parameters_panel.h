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

#include "../../simulation_parameters/lin_simulation_parameters.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"

class LinParametersPanel : public wxPanel
{
public:
    explicit LinParametersPanel(wxWindow* parent);

    [[nodiscard]] LinSimulationParameters build_lin_parameters() const;

    void apply(const LinSimulationParameters& params);

    [[nodiscard]] GlobalSettingsPanel* get_global_settings() const;
    [[nodiscard]] PrintSectionPanel* get_print_section() const;

private:
    SimulationCard* m_card = nullptr;
    GlobalSettingsPanel* m_global_settings = nullptr;
    PrintSectionPanel* m_print_section = nullptr;

    wxCheckBox* m_sparcalc_checkbox = nullptr;
    wxChoice* m_format_choice = nullptr;
    wxChoice* m_lintype_choice = nullptr;
    wxChoice* m_dataformat_choice = nullptr;
    wxTextCtrl* m_file_text = nullptr;
    wxTextCtrl* m_width_text = nullptr;
    wxTextCtrl* m_precision_text = nullptr;

    wxChoice* m_sweep_mode_choice = nullptr;
    wxTextCtrl* m_points_text = nullptr;
    wxTextCtrl* m_start_text = nullptr;
    wxTextCtrl* m_end_text = nullptr;
    wxTextCtrl* m_data_table_text = nullptr;
};
