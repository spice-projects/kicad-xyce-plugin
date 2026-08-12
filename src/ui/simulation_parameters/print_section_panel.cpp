#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/arrstr.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../wx_util.h"
#include "print_section_panel.h"
#include "simulation_parameters/print_parameters.h"

namespace
{
    // well-known wildcard tokens that map to dedicated checkbox shortcuts
    static const std::set<std::string> WILDCARD_TOKENS = {"V(*)", "I(*)", "P(*)", "W(*)", "IB(*)", "IC(*)", "ID(*)", "IE(*)", "IG(*)", "IS(*)"};

    // ordered BJT lead current wildcards
    static const std::vector<std::string> BJT_WILDCARDS = {"IB(*)", "IC(*)", "IE(*)", "IS(*)"};

    // ordered FET lead current wildcards
    static const std::vector<std::string> FET_WILDCARDS = {"IB(*)", "ID(*)", "IG(*)", "IS(*)"};

    // print format model matching the combo order
    static const std::vector<wxString> FORMAT_MODEL = {"(default)", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"};

    // format strings that map to combo display values (index 0 is empty)
    static const std::vector<wxString> FORMAT_VALUES = {"", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"};

    // resolve the combo index for a given format string; returns 0 when not found
    [[nodiscard]] int format_index_for_string(const wxString& format_str) {
        auto it = std::find(FORMAT_VALUES.begin(), FORMAT_VALUES.end(), format_str.Upper());
        if (it == FORMAT_VALUES.end()) {
            return 0;
        }
        return static_cast<int>(std::distance(FORMAT_VALUES.begin(), it));
    }

    // build a set of output variable strings for quick lookup
    [[nodiscard]] std::set<std::string> make_variable_set(const std::vector<std::string>& vars) {
        std::set<std::string> result;
        for (const auto& v : vars) {
            result.insert(v);
        }
        return result;
    }

    // check whether any of the candidate tokens appear in the variable set
    [[nodiscard]] bool has_any_wildcard(const std::set<std::string>& var_set, const std::vector<std::string>& candidates) {
        for (const auto& c : candidates) {
            if (var_set.count(c)) {
                return true;
            }
        }
        return false;
    }
} // namespace

PrintSectionPanel::PrintSectionPanel(wxWindow* parent, const wxString& analysis_prefix, std::vector<wxString> print_types, bool show_power, bool show_bjt_fet, bool show_print_type_combo) :
    wxPanel(parent), m_analysis_prefix(analysis_prefix), m_print_types(std::move(print_types)), m_show_power(show_power), m_show_bjt_fet(show_bjt_fet), m_show_print_type_combo(show_print_type_combo) {
    // outer vertical sizer for all controls
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);
    // enable/disable checkbox at the top
    m_enable_checkbox = new wxCheckBox(this, wxID_ANY, "Enable .PRINT output");
    wxFont enable_font = m_enable_checkbox->GetFont();
    // make the checkbox label bold
    enable_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_enable_checkbox->SetFont(enable_font);
    outer_sizer->Add(m_enable_checkbox, 0, wxALL, 0);
    // container for all controls gated by the enable checkbox
    m_body = new wxPanel(this, wxID_ANY);
    // body sizer
    auto* body_sizer = new wxBoxSizer(wxVERTICAL);
    // print type combo row (conditional)
    if (m_show_print_type_combo && !m_print_types.empty()) {
        // horizontal sizer
        auto* type_row = new wxBoxSizer(wxHORIZONTAL);
        // print type
        auto* type_label = new wxStaticText(m_body, wxID_ANY, "Print type");
        type_row->Add(type_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
        // populate the choice with the provided print types
        wxArrayString type_choices = wx_util::to_wx_array_string(m_print_types);
        m_print_type_choice = new wxChoice(m_body, wxID_ANY, wxDefaultPosition, wxDefaultSize, type_choices);
        m_print_type_choice->SetSelection(0);
        // size the choice explicitly so the longest print type is fully visible on all platforms
        int widest_width = 0;
        for (const auto& pt : type_choices) {
            widest_width = (std::max)(widest_width, m_print_type_choice->GetTextExtent(pt).GetWidth());
        }
        widest_width += FromDIP(48);
        const int type_height = m_print_type_choice->GetBestSize().GetHeight();
        m_print_type_choice->SetSize(wxSize(widest_width, type_height));
        m_print_type_choice->SetMinSize(wxSize(widest_width, type_height));
        type_row->Add(m_print_type_choice, 0, wxALL, 0);
        // add row
        body_sizer->Add(type_row, 0, wxLEFT | wxTOP, FromDIP(12));
    }
    // wildcard checkbox row
    auto* wildcard_row = new wxBoxSizer(wxHORIZONTAL);
    m_all_nodes_checkbox = new wxCheckBox(m_body, wxID_ANY, "All voltages V(*)");
    wildcard_row->Add(m_all_nodes_checkbox, 0, wxRIGHT, FromDIP(16));
    m_all_currents_checkbox = new wxCheckBox(m_body, wxID_ANY, "All currents I(*)");
    wildcard_row->Add(m_all_currents_checkbox, 0, wxRIGHT, FromDIP(16));
    if (m_show_power) {
        m_power_checkbox = new wxCheckBox(m_body, wxID_ANY, "Power P(*)");
        wildcard_row->Add(m_power_checkbox, 0, wxRIGHT, FromDIP(16));
    }
    body_sizer->Add(wildcard_row, 0, wxLEFT | wxTOP, FromDIP(12));
    // BJT/FET lead checkbox row (conditional)
    if (m_show_bjt_fet) {
        auto* lead_row = new wxBoxSizer(wxHORIZONTAL);
        m_bjt_leads_checkbox = new wxCheckBox(m_body, wxID_ANY, "BJT leads");
        lead_row->Add(m_bjt_leads_checkbox, 0, wxRIGHT, FromDIP(16));
        m_fet_leads_checkbox = new wxCheckBox(m_body, wxID_ANY, "FET leads");
        lead_row->Add(m_fet_leads_checkbox, 0, wxRIGHT, FromDIP(16));
        body_sizer->Add(lead_row, 0, wxLEFT | wxTOP, FromDIP(12));
    }
    // detail grid: additional variables, format, output file
    auto* detail_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    detail_grid->AddGrowableCol(1, 1);
    // additional variables row
    auto* vars_label = new wxStaticText(m_body, wxID_ANY, "Additional variables");
    detail_grid->Add(vars_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_specific_vars_text = new wxTextCtrl(m_body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    detail_grid->Add(m_specific_vars_text, 0, wxEXPAND, 0);
    // format combo row
    auto* format_label = new wxStaticText(m_body, wxID_ANY, "Format");
    detail_grid->Add(format_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    wxArrayString format_choices = wx_util::to_wx_array_string(FORMAT_MODEL);
    m_format_choice = new wxChoice(m_body, wxID_ANY, wxDefaultPosition, wxDefaultSize, format_choices);
    m_format_choice->SetSelection(0);
    detail_grid->Add(m_format_choice, 0, wxEXPAND, 0);
    // output file row
    auto* file_label = new wxStaticText(m_body, wxID_ANY, "Output file");
    detail_grid->Add(file_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_output_file_text = new wxTextCtrl(m_body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    detail_grid->Add(m_output_file_text, 0, wxEXPAND, 0);
    body_sizer->Add(detail_grid, 0, wxEXPAND | wxLEFT | wxTOP, FromDIP(12));
    // set the body sizer and layout
    m_body->SetSizer(body_sizer);
    // add m_body to outer sizer and indent it to align under the checkbox
    outer_sizer->Add(m_body, 1, wxEXPAND | wxLEFT, FromDIP(28));
    // panel sizer
    SetSizer(outer_sizer);
    // disable all controls at the end to have the visual effect
    m_body->Enable(false);
    // handlers
    m_enable_checkbox->Bind(wxEVT_CHECKBOX, &PrintSectionPanel::on_enable_toggle, this);
}

void PrintSectionPanel::on_enable_toggle(wxCommandEvent& event) {
    // gate all child controls on the enable checkbox state
    m_body->Enable(event.IsChecked());
}

std::optional<PrintParameters> PrintSectionPanel::build_print_parameters() const {
    // return none when the print section is disabled
    if (!m_enable_checkbox->GetValue())
        return std::nullopt;
    // collect output variable tokens from all enabled checkboxes
    std::vector<std::string> output_vars;
    // append wildcard tokens when the corresponding checkbox is checked
    if (m_all_nodes_checkbox->GetValue()) {
        // add the all-nodes wildcard
        output_vars.push_back("V(*)");
    }
    if (m_all_currents_checkbox->GetValue()) {
        // add the all-currents wildcard
        output_vars.push_back("I(*)");
    }
    if (m_show_power && m_power_checkbox && m_power_checkbox->GetValue()) {
        // add the power wildcard
        output_vars.push_back("P(*)");
    }
    // append BJT lead wildcards when the BJT checkbox is checked
    if (m_show_bjt_fet && m_bjt_leads_checkbox && m_bjt_leads_checkbox->GetValue()) {
        // loop bjt wildcards
        for (const auto& wc : BJT_WILDCARDS) {
            // skip when the wildcard is already present
            if (std::find(output_vars.begin(), output_vars.end(), wc) == output_vars.end())
                output_vars.push_back(wc);
        }
    }
    // append FET lead wildcards deduplicating tokens shared with the BJT group
    if (m_show_bjt_fet && m_fet_leads_checkbox && m_fet_leads_checkbox->GetValue()) {
        // loop FET wildcards
        for (const auto& wc : FET_WILDCARDS) {
            // skip when the wildcard is already present
            if (std::find(output_vars.begin(), output_vars.end(), wc) == output_vars.end())
                output_vars.push_back(wc);
        }
    }
    // append any explicitly listed specific variables
    wxString specific_text = m_specific_vars_text->GetValue().Trim(true).Trim(false);
    if (!specific_text.IsEmpty()) {
        // add each token as a specific variable
        for (auto& token : wx_util::split_strings(specific_text)) {
            output_vars.push_back(std::move(token));
        }
    }
    // resolve the print type
    wxString print_type;
    if (m_show_print_type_combo && m_print_type_choice) {
        // read the selected index from the combo
        int sel = m_print_type_choice->GetSelection();
        if (sel >= 0 && sel < static_cast<int>(m_print_types.size())) {
            // use the selected print type
            print_type = m_print_types[sel];
        }
        else {
            // fall back to the analysis prefix
            print_type = m_analysis_prefix;
        }
    }
    else {
        // fall back to the analysis prefix when the combo is hidden
        print_type = m_analysis_prefix;
    }
    // resolve the output format (index 0 is the default/empty value)
    wxString print_format;
    int fmt_sel = m_format_choice->GetSelection();
    if (fmt_sel > 0 && fmt_sel < static_cast<int>(FORMAT_VALUES.size())) {
        // use the selected format
        print_format = FORMAT_VALUES[fmt_sel];
    }
    // read the output file path
    wxString print_file = m_output_file_text->GetValue().Trim(true).Trim(false);
    // create parameters
    return PrintParameters(std::string(print_type.ToUTF8()), std::string(print_format.ToUTF8()), std::string(print_file.ToUTF8()), std::move(output_vars), {});
}

void PrintSectionPanel::apply(const PrintParameters* params, bool has_bjt, bool has_fet) {
    // disable the print section and reset controls to defaults
    m_enable_checkbox->SetValue(params != nullptr);
    // disable the body controls
    m_body->Enable(params != nullptr);
    // conditional logic to either clear controls or restore from saved parameters
    if (!params) {
        // clear the all-nodes checkbox
        m_all_nodes_checkbox->SetValue(false);
        // clear the all-currents checkbox
        m_all_currents_checkbox->SetValue(false);
        if (m_show_power && m_power_checkbox) {
            // clear the power checkbox
            m_power_checkbox->SetValue(false);
        }
        if (m_show_bjt_fet) {
            if (m_bjt_leads_checkbox) {
                // clear the BJT lead checkbox
                m_bjt_leads_checkbox->SetValue(false);
                // update the BJT lead checkbox visibility
                m_bjt_leads_checkbox->Show(has_bjt);
            }
            if (m_fet_leads_checkbox) {
                // clear the FET lead checkbox
                m_fet_leads_checkbox->SetValue(false);
                // update the FET lead checkbox visibility
                m_fet_leads_checkbox->Show(has_fet);
            }
        }
        // clear the specific variables text
        m_specific_vars_text->SetValue(wxEmptyString);
        // reset the format combo to the default
        m_format_choice->SetSelection(0);
        // clear the output file path
        m_output_file_text->SetValue(wxEmptyString);
        if (m_print_type_choice) {
            // reset the print type combo to the first entry
            m_print_type_choice->SetSelection(0);
        }
        return;
    }
    // index saved output variables for quick wildcard lookup
    auto var_set = make_variable_set(params->output_variables);
    // restore the all-nodes wildcard checkbox
    m_all_nodes_checkbox->SetValue(var_set.count("V(*)") > 0);
    // restore the all-currents wildcard checkbox
    m_all_currents_checkbox->SetValue(var_set.count("I(*)") > 0);
    if (m_show_power && m_power_checkbox) {
        // restore the power wildcard checkbox
        m_power_checkbox->SetValue(var_set.count("P(*)") > 0);
    }
    // restore lead current checkboxes and visibility
    if (m_show_bjt_fet) {
        // restore the BJT checkbox visibility and state
        if (m_bjt_leads_checkbox) {
            // update the BJT lead checkbox visibility
            m_bjt_leads_checkbox->Show(has_bjt);
            // restore the BJT lead checkbox state
            m_bjt_leads_checkbox->SetValue(has_any_wildcard(var_set, {"IC(*)", "IE(*)"}));
        }
        // restore the FET checkbox visibility and state
        if (m_fet_leads_checkbox) {
            // update the FET lead checkbox visibility
            m_fet_leads_checkbox->Show(has_fet);
            // restore the FET lead checkbox state
            m_fet_leads_checkbox->SetValue(has_any_wildcard(var_set, {"ID(*)", "IG(*)"}));
        }
    }
    // collect non-wildcard tokens for the specific variables text
    std::vector<std::string> specific_tokens;
    // loop variables
    for (const auto& v : params->output_variables) {
        // skip tokens that match known wildcards
        if (WILDCARD_TOKENS.count(v) == 0) {
            // keep the non-wildcard token
            specific_tokens.push_back(v);
        }
    }
    m_specific_vars_text->SetValue(wx_util::join_strings(specific_tokens, " "));
    // restore the format combo by matching the format string
    wxString fmt_str = wxString::FromUTF8(params->print_format);
    m_format_choice->SetSelection(format_index_for_string(fmt_str));
    // restore the output file path
    m_output_file_text->SetValue(wxString::FromUTF8(params->print_file));
    // restore the print type combo by matching the type string
    if (m_print_type_choice) {
        wxString type_str = wxString::FromUTF8(params->print_type);
        int type_index = 0;
        for (size_t i = 0; i < m_print_types.size(); ++i) {
            // compare case-insensitively
            if (m_print_types[i].Upper() == type_str.Upper()) {
                // found the matching type
                type_index = static_cast<int>(i);
                break;
            }
        }
        m_print_type_choice->SetSelection(type_index);
    }
    // re-layout since control visibility may have changed
    Layout();
}
