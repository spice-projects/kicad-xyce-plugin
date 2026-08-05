#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/arrstr.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>
#endif

#include "step_parameters_panel.h"

namespace
{
    // sweep mode choices matching the combo order
    static const std::vector<wxString> STEP_SWEEP_MODES = {"LIN", "DEC", "OCT", "LIST", "DATA"};

    // parse space-separated list values into strings
    [[nodiscard]] std::vector<std::string> parse_list_values(const wxString& text) {
        std::vector<std::string> values;
        wxString trimmed = wxString(text).Trim(true).Trim(false);
        if (trimmed.IsEmpty()) {
            return values;
        }
        wxStringTokenizer tokenizer(trimmed, " \t\r\n");
        while (tokenizer.HasMoreTokens()) {
            values.push_back(std::string(tokenizer.GetNextToken().ToUTF8()));
        }
        return values;
    }

    // format strings as space-separated text
    [[nodiscard]] wxString format_list_values(const std::vector<std::string>& items) {
        if (items.empty()) {
            return wxEmptyString;
        }
        wxString result;
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0) {
                result += " ";
            }
            result += wxString::FromUTF8(items[i]);
        }
        return result;
    }
} // namespace

StepParametersPanel::StepParametersPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole section
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);
    // simulation card wrapping all controls
    m_card = new SimulationCard(this, "Step Sweep");
    auto* content = m_card->get_content();
    auto* content_sizer = new wxBoxSizer(wxVERTICAL);

    // enable checkbox for the step section
    m_enable_cb = new wxCheckBox(content, wxID_ANY, "Enable step sweep");
    m_enable_cb->SetValue(false);
    content_sizer->Add(m_enable_cb, 0, wxBOTTOM, FromDIP(4));

    // --- step fields ---
    // field grid for step parameters: 2 columns (label | control)
    auto* field_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(8));
    field_grid->AddGrowableCol(1, 1);

    // sweep mode row
    auto* mode_label = new wxStaticText(content, wxID_ANY, "Sweep mode");
    field_grid->Add(mode_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    wxArrayString mode_choices;
    for (const auto& mode : STEP_SWEEP_MODES) {
        mode_choices.Add(mode);
    }
    m_sweep_mode_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, mode_choices);
    m_sweep_mode_choice->SetSelection(0);
    field_grid->Add(m_sweep_mode_choice, 0, wxEXPAND, 0);

    // variable row
    auto* var_label = new wxStaticText(content, wxID_ANY, "Variable");
    field_grid->Add(var_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_variable_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_variable_text->SetHint("e.g. TEMP");
    field_grid->Add(m_variable_text, 0, wxEXPAND, 0);

    // start row
    auto* start_label = new wxStaticText(content, wxID_ANY, "Start");
    field_grid->Add(start_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_start_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_start_text, 0, wxEXPAND, 0);

    // stop row
    auto* stop_label = new wxStaticText(content, wxID_ANY, "Stop");
    field_grid->Add(stop_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_stop_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_stop_text, 0, wxEXPAND, 0);

    // step row
    auto* step_label = new wxStaticText(content, wxID_ANY, "Step");
    field_grid->Add(step_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_step_text, 0, wxEXPAND, 0);

    // points row
    auto* points_label = new wxStaticText(content, wxID_ANY, "Points");
    field_grid->Add(points_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_points_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_points_text, 0, wxEXPAND, 0);

    // list values row
    auto* list_vals_label = new wxStaticText(content, wxID_ANY, "List values");
    field_grid->Add(list_vals_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_list_values_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_list_values_text->SetHint("space-separated");
    field_grid->Add(m_list_values_text, 0, wxEXPAND, 0);

    // data table row
    auto* data_table_label = new wxStaticText(content, wxID_ANY, "Data table");
    field_grid->Add(data_table_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_data_table_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_data_table_text, 0, wxEXPAND, 0);

    content_sizer->Add(field_grid, 0, wxEXPAND, 0);

    // attach content sizer and card to outer layout
    content->SetSizer(content_sizer);
    outer_sizer->Add(m_card, 1, wxEXPAND, 0);
    SetSizer(outer_sizer);
}

StepParameters StepParametersPanel::build_step_parameters() const {
    // read enable checkbox
    bool enabled = m_enable_cb->GetValue();

    // read sweep mode from combo
    std::string sweep_mode;
    int sel = m_sweep_mode_choice->GetSelection();
    if (sel != wxNOT_FOUND && sel < static_cast<int>(STEP_SWEEP_MODES.size())) {
        sweep_mode = std::string(STEP_SWEEP_MODES[sel].ToUTF8());
    }

    // read text fields
    std::string variable = std::string(m_variable_text->GetValue().ToUTF8());
    std::string start = std::string(m_start_text->GetValue().ToUTF8());
    std::string stop = std::string(m_stop_text->GetValue().ToUTF8());
    std::string step = std::string(m_step_text->GetValue().ToUTF8());
    std::string points = std::string(m_points_text->GetValue().ToUTF8());

    // parse list values and read data table name
    auto list_values = parse_list_values(m_list_values_text->GetValue());
    std::string data_table_name = std::string(m_data_table_text->GetValue().ToUTF8());

    return StepParameters(std::move(sweep_mode), std::move(variable), std::move(start), std::move(stop), std::move(step), std::move(points), std::move(list_values), std::move(data_table_name), enabled);
}

void StepParametersPanel::apply(const StepParameters& params) {
    // restore enable state
    m_enable_cb->SetValue(params.enabled);

    // restore sweep mode
    int mode_index = m_sweep_mode_choice->FindString(wxString::FromUTF8(params.sweep_mode));
    if (mode_index != wxNOT_FOUND) {
        m_sweep_mode_choice->SetSelection(mode_index);
    }
    else {
        m_sweep_mode_choice->SetSelection(0);
    }

    // restore text fields
    m_variable_text->SetValue(wxString::FromUTF8(params.variable));
    m_start_text->SetValue(wxString::FromUTF8(params.start));
    m_stop_text->SetValue(wxString::FromUTF8(params.stop));
    m_step_text->SetValue(wxString::FromUTF8(params.step));
    m_points_text->SetValue(wxString::FromUTF8(params.points));

    // restore list values as space-separated text and data table name
    m_list_values_text->SetValue(format_list_values(params.list_values));
    m_data_table_text->SetValue(wxString::FromUTF8(params.data_table_name));
}
