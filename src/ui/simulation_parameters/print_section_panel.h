#pragma once

#include <optional>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../../simulation_parameters/print_parameters.h"

// reusable .PRINT section shared across all analysis types
class PrintSectionPanel : public wxPanel
{
public:
    // construct a print section panel
    //   analysis_prefix   — e.g. "TRAN", "DC", "AC" (used as fallback print type)
    //   print_types       — combo items, e.g. {"TRAN", "TRANADJOINT"}
    //   show_power        — show the P(*) checkbox
    //   show_bjt_fet      — show BJT/FET lead checkboxes
    //   show_print_type_combo — show the print-type combo
    PrintSectionPanel(wxWindow* parent, const wxString& analysis_prefix, std::vector<wxString> print_types, bool show_power, bool show_bjt_fet, bool show_print_type_combo);

    // read the current control state and build a PrintParameters model;
    // returns nullopt when the print section is disabled
    [[nodiscard]] std::optional<PrintParameters> build_print_parameters() const;

    // populate controls from a saved PrintParameters model;
    // pass nullptr to disable the section and reset to defaults
    void apply(const PrintParameters* params, bool has_bjt, bool has_fet);

private:
    void on_enable_toggle(wxCommandEvent& event);

    wxString m_analysis_prefix;
    std::vector<wxString> m_print_types;
    bool m_show_power;
    bool m_show_bjt_fet;
    bool m_show_print_type_combo;

    wxCheckBox* m_enable_checkbox = nullptr;
    wxPanel* m_body = nullptr;
    wxChoice* m_print_type_choice = nullptr;
    wxCheckBox* m_all_nodes_checkbox = nullptr;
    wxCheckBox* m_all_currents_checkbox = nullptr;
    wxCheckBox* m_power_checkbox = nullptr;
    wxCheckBox* m_bjt_leads_checkbox = nullptr;
    wxCheckBox* m_fet_leads_checkbox = nullptr;
    wxTextCtrl* m_specific_vars_text = nullptr;
    wxChoice* m_format_choice = nullptr;
    wxTextCtrl* m_output_file_text = nullptr;
};
