#include <set>
#include <sstream>
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

#include "../../simulation_parameters/print_parameters.h"
#include "print_section_panel.h"

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

    // join string tokens with a space separator
    [[nodiscard]] wxString join_tokens(const std::vector<std::string>& tokens) {
        if (tokens.empty()) {
            return wxEmptyString;
        }
        std::ostringstream oss;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) {
                oss << ' ';
            }
            oss << tokens[i];
        }
        return wxString(oss.str());
    }
} // namespace

PrintSectionPanel::PrintSectionPanel(wxWindow* parent, const wxString& analysis_prefix, std::vector<wxString> print_types, bool show_power, bool show_bjt_fet, bool show_print_type_combo) :
    wxPanel(parent), m_analysis_prefix(analysis_prefix), m_print_types(std::move(print_types)), m_show_power(show_power), m_show_bjt_fet(show_bjt_fet), m_show_print_type_combo(show_print_type_combo) {
    // outer vertical sizer for all controls
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // enable/disable checkbox at the top
    m_enable_checkbox = new wxCheckBox(this, wxID_ANY, "Enable .PRINT output");
    wxFont enable_font = m_enable_checkbox->GetFont();
    enable_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_enable_checkbox->SetFont(enable_font);
    outer_sizer->Add(m_enable_checkbox, 0, wxALL, 0);

    // container for all controls gated by the enable checkbox
    auto* body = new wxPanel(this, wxID_ANY);
    auto* body_sizer = new wxBoxSizer(wxVERTICAL);

    // print type combo row (conditional)
    if (m_show_print_type_combo && !m_print_types.empty()) {
        auto* type_row = new wxBoxSizer(wxHORIZONTAL);
        auto* type_label = new wxStaticText(body, wxID_ANY, "Print type");
        type_row->Add(type_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
        // populate the choice with the provided print types
        wxArrayString type_choices;
        for (const auto& pt : m_print_types) {
            type_choices.Add(pt);
        }
        m_print_type_choice = new wxChoice(body, wxID_ANY, wxDefaultPosition, wxDefaultSize, type_choices);
        m_print_type_choice->SetSelection(0);
        type_row->Add(m_print_type_choice, 0, wxALL, 0);
        body_sizer->Add(type_row, 0, wxLEFT | wxTOP, FromDIP(12));
    }

    // wildcard checkbox row
    auto* wildcard_row = new wxBoxSizer(wxHORIZONTAL);
    m_all_nodes_checkbox = new wxCheckBox(body, wxID_ANY, "All voltages V(*)");
    wildcard_row->Add(m_all_nodes_checkbox, 0, wxRIGHT, FromDIP(16));
    m_all_currents_checkbox = new wxCheckBox(body, wxID_ANY, "All currents I(*)");
    wildcard_row->Add(m_all_currents_checkbox, 0, wxRIGHT, FromDIP(16));
    if (m_show_power) {
        m_power_checkbox = new wxCheckBox(body, wxID_ANY, "Power P(*)");
        wildcard_row->Add(m_power_checkbox, 0, wxRIGHT, FromDIP(16));
    }
    body_sizer->Add(wildcard_row, 0, wxLEFT | wxTOP, FromDIP(12));

    // BJT/FET lead checkbox row (conditional)
    if (m_show_bjt_fet) {
        auto* lead_row = new wxBoxSizer(wxHORIZONTAL);
        m_bjt_leads_checkbox = new wxCheckBox(body, wxID_ANY, "BJT leads");
        lead_row->Add(m_bjt_leads_checkbox, 0, wxRIGHT, FromDIP(16));
        m_fet_leads_checkbox = new wxCheckBox(body, wxID_ANY, "FET leads");
        lead_row->Add(m_fet_leads_checkbox, 0, wxRIGHT, FromDIP(16));
        body_sizer->Add(lead_row, 0, wxLEFT | wxTOP, FromDIP(12));
    }

    // detail grid: additional variables, format, output file
    auto* detail_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    detail_grid->AddGrowableCol(1, 1);

    // additional variables row
    auto* vars_label = new wxStaticText(body, wxID_ANY, "Additional variables");
    detail_grid->Add(vars_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_specific_vars_text = new wxTextCtrl(body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    detail_grid->Add(m_specific_vars_text, 0, wxEXPAND, 0);

    // format combo row
    auto* format_label = new wxStaticText(body, wxID_ANY, "Format");
    detail_grid->Add(format_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    wxArrayString format_choices;
    for (const auto& fmt : FORMAT_MODEL) {
        format_choices.Add(fmt);
    }
    m_format_choice = new wxChoice(body, wxID_ANY, wxDefaultPosition, wxDefaultSize, format_choices);
    m_format_choice->SetSelection(0);
    detail_grid->Add(m_format_choice, 0, wxEXPAND, 0);

    // output file row
    auto* file_label = new wxStaticText(body, wxID_ANY, "Output file");
    detail_grid->Add(file_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_output_file_text = new wxTextCtrl(body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    detail_grid->Add(m_output_file_text, 0, wxEXPAND, 0);

    body_sizer->Add(detail_grid, 0, wxEXPAND | wxLEFT | wxTOP, FromDIP(12));
    body->SetSizer(body_sizer);

    // add body to outer sizer and indent it to align under the checkbox
    outer_sizer->Add(body, 1, wxEXPAND | wxLEFT, FromDIP(28));

    SetSizer(outer_sizer);
}

std::optional<PrintParameters> PrintSectionPanel::build_print_parameters() const {
    // return none when the print section is disabled
    if (!m_enable_checkbox->GetValue()) {
        return std::nullopt;
    }

    // collect output variable tokens from all enabled checkboxes
    std::vector<std::string> output_vars;
    if (m_all_nodes_checkbox->GetValue()) {
        output_vars.push_back("V(*)");
    }
    if (m_all_currents_checkbox->GetValue()) {
        output_vars.push_back("I(*)");
    }
    if (m_show_power && m_power_checkbox && m_power_checkbox->GetValue()) {
        output_vars.push_back("P(*)");
    }

    // append BJT lead wildcards when the BJT checkbox is checked
    if (m_show_bjt_fet && m_bjt_leads_checkbox && m_bjt_leads_checkbox->GetValue()) {
        for (const auto& wc : BJT_WILDCARDS) {
            if (std::find(output_vars.begin(), output_vars.end(), wc) == output_vars.end()) {
                output_vars.push_back(wc);
            }
        }
    }

    // append FET lead wildcards deduplicating tokens shared with the BJT group
    if (m_show_bjt_fet && m_fet_leads_checkbox && m_fet_leads_checkbox->GetValue()) {
        for (const auto& wc : FET_WILDCARDS) {
            if (std::find(output_vars.begin(), output_vars.end(), wc) == output_vars.end()) {
                output_vars.push_back(wc);
            }
        }
    }

    // append any explicitly listed specific variables
    wxString specific_text = m_specific_vars_text->GetValue().Trim(true).Trim(false);
    if (!specific_text.IsEmpty()) {
        std::istringstream stream(std::string(specific_text.ToUTF8()));
        std::string token;
        while (stream >> token) {
            if (!token.empty()) {
                output_vars.push_back(std::move(token));
            }
        }
    }

    // resolve the print type
    wxString print_type;
    if (m_show_print_type_combo && m_print_type_choice) {
        int sel = m_print_type_choice->GetSelection();
        if (sel >= 0 && sel < static_cast<int>(m_print_types.size())) {
            print_type = m_print_types[sel];
        }
        else {
            print_type = m_analysis_prefix;
        }
    }
    else {
        print_type = m_analysis_prefix;
    }

    // resolve the output format (index 0 is the default/empty value)
    wxString print_format;
    int fmt_sel = m_format_choice->GetSelection();
    if (fmt_sel > 0 && fmt_sel < static_cast<int>(FORMAT_VALUES.size())) {
        print_format = FORMAT_VALUES[fmt_sel];
    }

    // read the output file path
    wxString print_file = m_output_file_text->GetValue().Trim(true).Trim(false);

    return PrintParameters(std::string(print_type.ToUTF8()), std::string(print_format.ToUTF8()), std::string(print_file.ToUTF8()), std::move(output_vars), {});
}

void PrintSectionPanel::apply(const PrintParameters* params, bool has_bjt, bool has_fet) {
    if (!params) {
        // disable the print section and reset controls to defaults
        m_enable_checkbox->SetValue(false);

        m_all_nodes_checkbox->SetValue(false);
        m_all_currents_checkbox->SetValue(false);
        if (m_show_power && m_power_checkbox) {
            m_power_checkbox->SetValue(false);
        }
        if (m_show_bjt_fet) {
            if (m_bjt_leads_checkbox) {
                m_bjt_leads_checkbox->SetValue(false);
                m_bjt_leads_checkbox->Show(has_bjt);
            }
            if (m_fet_leads_checkbox) {
                m_fet_leads_checkbox->SetValue(false);
                m_fet_leads_checkbox->Show(has_fet);
            }
        }
        m_specific_vars_text->SetValue(wxEmptyString);
        m_format_choice->SetSelection(0);
        m_output_file_text->SetValue(wxEmptyString);
        if (m_print_type_choice) {
            m_print_type_choice->SetSelection(0);
        }
        return;
    }

    // enable the print section and restore from saved parameters
    m_enable_checkbox->SetValue(true);

    // index saved output variables for quick wildcard lookup
    auto var_set = make_variable_set(params->output_variables);

    // wildcard checkboxes
    m_all_nodes_checkbox->SetValue(var_set.count("V(*)") > 0);
    m_all_currents_checkbox->SetValue(var_set.count("I(*)") > 0);
    if (m_show_power && m_power_checkbox) {
        m_power_checkbox->SetValue(var_set.count("P(*)") > 0);
    }

    // lead current checkboxes and visibility
    if (m_show_bjt_fet) {
        if (m_bjt_leads_checkbox) {
            m_bjt_leads_checkbox->Show(has_bjt);
            m_bjt_leads_checkbox->SetValue(has_any_wildcard(var_set, {"IC(*)", "IE(*)"}));
        }
        if (m_fet_leads_checkbox) {
            m_fet_leads_checkbox->Show(has_fet);
            m_fet_leads_checkbox->SetValue(has_any_wildcard(var_set, {"ID(*)", "IG(*)"}));
        }
    }

    // specific vars: only saved non-wildcard tokens
    std::vector<std::string> specific_tokens;
    for (const auto& v : params->output_variables) {
        if (WILDCARD_TOKENS.count(v) == 0) {
            specific_tokens.push_back(v);
        }
    }
    m_specific_vars_text->SetValue(join_tokens(specific_tokens));

    // restore format combo (match format string, default to 0)
    wxString fmt_str = wxString::FromUTF8(params->print_format);
    m_format_choice->SetSelection(format_index_for_string(fmt_str));

    // restore output file path
    m_output_file_text->SetValue(wxString::FromUTF8(params->print_file));

    // restore print type combo (match type string, default to 0)
    if (m_print_type_choice) {
        wxString type_str = wxString::FromUTF8(params->print_type);
        int type_index = 0;
        for (size_t i = 0; i < m_print_types.size(); ++i) {
            if (m_print_types[i].Upper() == type_str.Upper()) {
                type_index = static_cast<int>(i);
                break;
            }
        }
        m_print_type_choice->SetSelection(type_index);
    }

    // re-layout since control visibility may have changed
    Layout();
}
