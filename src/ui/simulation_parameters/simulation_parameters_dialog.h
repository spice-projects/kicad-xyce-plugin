#pragma once

#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/simplebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>
#endif

#include "../../simulation_parameters/data_block.h"
#include "../../simulation_parameters/simulation_config.h"
#include "../../simulation_parameters/step_parameters.h"
#include "ac_parameters_panel.h"
#include "dc_parameters_panel.h"
#include "hb_parameters_panel.h"
#include "lin_parameters_panel.h"
#include "noise_parameters_panel.h"
#include "op_parameters_panel.h"
#include "sensitivity_section_panel.h"
#include "transient_parameters_panel.h"

class TabbedPanel;

class SimulationParametersDialog : public wxDialog
{
public:
    SimulationParametersDialog(wxWindow* parent, const SimulationConfig& config);

    [[nodiscard]] SimulationConfig get_config() const;

private:
    wxPanel* create_transient_parameters_panel(wxWindow*);
    wxPanel* create_op_parameters_panel(wxWindow*);
    wxPanel* create_dc_parameters_panel(wxWindow*);
    wxPanel* create_ac_parameters_panel(wxWindow*);
    wxPanel* create_noise_parameters_panel(wxWindow*);
    wxPanel* create_hb_parameters_panel(wxWindow*);
    wxPanel* create_lin_parameters_panel(wxWindow*);

    void on_page_changed(wxCommandEvent& event);

    void apply_config(const SimulationConfig& config);

    [[nodiscard]] SimulationConfig build_preview_config() const;

    [[nodiscard]] StepParameters read_step_parameters() const;

    void apply_step_parameters(const StepParameters& params);

    // scrollable container for the tabbed panel, sensitivity section, and step
    // parameters so their content is always reachable even when the dialog is
    // too small to show everything at once
    wxScrolledWindow* m_scroll_window = nullptr;

    // tabbed panel with sidebar toggle buttons and pages
    TabbedPanel* m_tabbed_panel = nullptr;

    // pages
    OpParametersPanel* m_op_panel = nullptr;
    TransientParametersPanel* m_tran_panel = nullptr;
    DcParametersPanel* m_dc_panel = nullptr;
    AcParametersPanel* m_ac_panel = nullptr;
    NoiseParametersPanel* m_noise_panel = nullptr;
    HbParametersPanel* m_hb_panel = nullptr;
    LinParametersPanel* m_lin_panel = nullptr;

    // per-tab sensitivity sections (only TRAN, DC, AC)
    SensitivitySectionPanel* m_tran_sensitivity = nullptr;
    SensitivitySectionPanel* m_dc_sensitivity = nullptr;
    SensitivitySectionPanel* m_ac_sensitivity = nullptr;

    // step parameters section
    wxCheckBox* m_step_enable_cb = nullptr;
    wxChoice* m_step_sweep_mode_choice = nullptr;
    wxTextCtrl* m_step_variable_text = nullptr;
    wxTextCtrl* m_step_start_text = nullptr;
    wxTextCtrl* m_step_stop_text = nullptr;
    wxTextCtrl* m_step_step_text = nullptr;
    wxTextCtrl* m_step_points_text = nullptr;
    wxTextCtrl* m_step_list_values_text = nullptr;
    wxTextCtrl* m_step_data_table_text = nullptr;

    // footer
    wxStaticText* m_error_label = nullptr;

    // data blocks
    std::vector<DataBlock> m_data_blocks;

    // current analysis type string
    std::string m_analysis_type;
};
