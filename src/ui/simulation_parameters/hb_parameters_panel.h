#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../../simulation_parameters/hb_simulation_parameters.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"

class HbParametersPanel : public wxPanel
{
public:
    explicit HbParametersPanel(wxWindow* parent);

    [[nodiscard]] HbSimulationParameters build_hb_parameters() const;

    void apply(const HbSimulationParameters& params);

    [[nodiscard]] GlobalSettingsPanel* get_global_settings() const;
    [[nodiscard]] PrintSectionPanel* get_print_section() const;

private:
    SimulationCard* m_card = nullptr;
    GlobalSettingsPanel* m_global_settings = nullptr;
    PrintSectionPanel* m_print_section = nullptr;

    wxTextCtrl* m_frequencies_text = nullptr;
    wxTextCtrl* m_harmonics_text = nullptr;
    wxTextCtrl* m_tahb_text = nullptr;
    wxTextCtrl* m_selectharms_text = nullptr;
    wxTextCtrl* m_startup_periods_text = nullptr;
    wxTextCtrl* m_nonlin_options_text = nullptr;
    wxTextCtrl* m_linsol_options_text = nullptr;
};
