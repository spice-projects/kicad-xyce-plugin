#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/arrstr.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>
#endif

#include "print_section_panel.h"
#include "sensitivity_section_panel.h"
#include "simulation_parameters/sens_parameter.h"

namespace
{
    // objective mode labels matching the combo order
    static const std::vector<wxString> OBJECTIVE_MODE_LABELS = {"objfunc", "objvars", "acobjfunc"};

    // parse comma-separated text into trimmed strings
    [[nodiscard]] std::vector<std::string> parse_comma_list(const wxString& text) {
        std::vector<std::string> values;
        wxArrayString parts = wxStringTokenize(text, ",");
        for (const auto& part : parts) {
            wxString trimmed = wxString(part).Trim(true).Trim(false);
            if (!trimmed.IsEmpty()) {
                values.push_back(std::string(trimmed.ToUTF8()));
            }
        }
        return values;
    }

    // format strings as comma-separated text
    [[nodiscard]] wxString format_comma_list(const std::vector<std::string>& items) {
        if (items.empty()) {
            return wxEmptyString;
        }
        wxString result;
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0) {
                result += ", ";
            }
            result += wxString::FromUTF8(items[i]);
        }
        return result;
    }
} // namespace

SensitivitySectionPanel::SensitivitySectionPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole section
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // enable checkbox for the sensitivity section
    m_enable_checkbox = new wxCheckBox(this, wxID_ANY, "Enable sensitivity (.SENS)");
    m_enable_checkbox->SetValue(false);
    outer_sizer->Add(m_enable_checkbox, 0, wxBOTTOM, FromDIP(8));

    // --- sensitivity fields ---
    // field grid for sensitivity parameters: 2 columns (label | control)
    auto* field_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    field_grid->AddGrowableCol(1, 1);

    // objective mode row
    auto* mode_label = new wxStaticText(this, wxID_ANY, "Objective mode");
    field_grid->Add(mode_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    wxArrayString mode_choices;
    for (const auto& label : OBJECTIVE_MODE_LABELS) {
        mode_choices.Add(label);
    }
    m_objective_mode_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, mode_choices);
    m_objective_mode_choice->SetSelection(0);
    field_grid->Add(m_objective_mode_choice, 0, wxEXPAND, 0);

    // objective values row
    auto* obj_vals_label = new wxStaticText(this, wxID_ANY, "Objective values");
    field_grid->Add(obj_vals_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_objective_values_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_objective_values_text->SetHint("comma-separated");
    field_grid->Add(m_objective_values_text, 0, wxEXPAND, 0);

    // parameters row
    auto* params_label = new wxStaticText(this, wxID_ANY, "Parameters");
    field_grid->Add(params_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_parameters_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_parameters_text->SetHint("comma-separated device parameters");
    field_grid->Add(m_parameters_text, 0, wxEXPAND, 0);

    outer_sizer->Add(field_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(8));

    // --- direct / adjoint checkboxes ---
    auto* method_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_direct_checkbox = new wxCheckBox(this, wxID_ANY, "Direct");
    method_sizer->Add(m_direct_checkbox, 0, wxRIGHT, FromDIP(12));
    m_adjoint_checkbox = new wxCheckBox(this, wxID_ANY, "Adjoint");
    method_sizer->Add(m_adjoint_checkbox, 0, 0, 0);
    outer_sizer->Add(method_sizer, 0, wxBOTTOM, FromDIP(8));

    // --- print section for .PRINT SENS ---
    m_print_section = new PrintSectionPanel(this, "SENS", {"SENS"}, false, false, false);
    outer_sizer->Add(m_print_section, 0, wxEXPAND, 0);

    SetSizer(outer_sizer);
}

std::optional<SensParameter> SensitivitySectionPanel::build_sens_parameter(const wxString& analysis_type) const {
    // return nullopt when the section is disabled
    if (!m_enable_checkbox->GetValue()) {
        return std::nullopt;
    }

    // read objective mode from combo
    std::string objective_mode;
    int mode_sel = m_objective_mode_choice->GetSelection();
    if (mode_sel != wxNOT_FOUND && mode_sel < static_cast<int>(OBJECTIVE_MODE_LABELS.size())) {
        objective_mode = std::string(OBJECTIVE_MODE_LABELS[mode_sel].ToUTF8());
    }

    // read and parse objective values as comma-separated
    auto objective_values = parse_comma_list(m_objective_values_text->GetValue());

    // read and parse parameters as comma-separated
    auto parameter_list = parse_comma_list(m_parameters_text->GetValue());

    // read method checkboxes
    bool direct = m_direct_checkbox->GetValue();
    bool adjoint = m_adjoint_checkbox->GetValue();

    // read print parameters from embedded print section
    auto print_params = m_print_section->build_print_parameters();

    return SensParameter(std::string(analysis_type.ToUTF8()), std::move(objective_mode), std::move(objective_values), std::move(parameter_list), direct, adjoint, std::move(print_params));
}

void SensitivitySectionPanel::apply(const SensParameter* params) {
    if (!params) {
        // disable section and reset to defaults
        m_enable_checkbox->SetValue(false);
        m_objective_mode_choice->SetSelection(0);
        m_objective_values_text->SetValue(wxEmptyString);
        m_parameters_text->SetValue(wxEmptyString);
        m_direct_checkbox->SetValue(false);
        m_adjoint_checkbox->SetValue(false);
        m_print_section->apply(nullptr, false, false);
        return;
    }

    // enable section and populate controls
    m_enable_checkbox->SetValue(true);

    // restore objective mode
    int mode_index = m_objective_mode_choice->FindString(wxString::FromUTF8(params->objective_mode));
    if (mode_index != wxNOT_FOUND) {
        m_objective_mode_choice->SetSelection(mode_index);
    }
    else {
        m_objective_mode_choice->SetSelection(0);
    }

    // restore objective values and parameters as comma-separated text
    m_objective_values_text->SetValue(format_comma_list(params->objective_values));
    m_parameters_text->SetValue(format_comma_list(params->parameter_list));

    // restore method checkboxes
    m_direct_checkbox->SetValue(params->direct);
    m_adjoint_checkbox->SetValue(params->adjoint);

    // restore print parameters
    m_print_section->apply(params->print_parameters ? &*params->print_parameters : nullptr, false, false);
}
