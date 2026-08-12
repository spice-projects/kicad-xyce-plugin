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
#endif

#include "../wx_util.h"
#include "step_parameters_panel.h"

namespace
{
    // sweep mode choices matching the combo order
    static const std::vector<wxString> STEP_SWEEP_MODES = {"LIN", "DEC", "OCT", "LIST", "DATA"};
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

    // container for all controls gated by the enable checkbox
    m_body = new wxPanel(content, wxID_ANY);
    auto* body_sizer = new wxBoxSizer(wxVERTICAL);

    // --- step fields ---
    // field grid for step parameters: 2 columns (label | control)
    auto* field_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(8));
    field_grid->AddGrowableCol(1, 1);

    // sweep mode row
    auto* mode_label = new wxStaticText(m_body, wxID_ANY, "Sweep mode");
    field_grid->Add(mode_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_sweep_mode_choice = new wxChoice(m_body, wxID_ANY, wxDefaultPosition, wxDefaultSize, wx_util::to_wx_array_string(STEP_SWEEP_MODES));
    m_sweep_mode_choice->SetSelection(0);
    field_grid->Add(m_sweep_mode_choice, 0, wxEXPAND, 0);

    // variable row
    auto* var_label = new wxStaticText(m_body, wxID_ANY, "Variable");
    field_grid->Add(var_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_variable_text = new wxTextCtrl(m_body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_variable_text->SetHint("e.g. TEMP");
    field_grid->Add(m_variable_text, 0, wxEXPAND, 0);

    // start row
    auto* start_label = new wxStaticText(m_body, wxID_ANY, "Start");
    field_grid->Add(start_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_start_text = new wxTextCtrl(m_body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_start_text, 0, wxEXPAND, 0);

    // stop row
    auto* stop_label = new wxStaticText(m_body, wxID_ANY, "Stop");
    field_grid->Add(stop_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_stop_text = new wxTextCtrl(m_body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_stop_text, 0, wxEXPAND, 0);

    // step row
    auto* step_label = new wxStaticText(m_body, wxID_ANY, "Step");
    field_grid->Add(step_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_text = new wxTextCtrl(m_body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_step_text, 0, wxEXPAND, 0);

    // points row
    auto* points_label = new wxStaticText(m_body, wxID_ANY, "Points");
    field_grid->Add(points_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_points_text = new wxTextCtrl(m_body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_points_text, 0, wxEXPAND, 0);

    // list values row
    auto* list_vals_label = new wxStaticText(m_body, wxID_ANY, "List values");
    field_grid->Add(list_vals_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_list_values_text = new wxTextCtrl(m_body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_list_values_text->SetHint("space-separated");
    field_grid->Add(m_list_values_text, 0, wxEXPAND, 0);

    // data table row
    auto* data_table_label = new wxStaticText(m_body, wxID_ANY, "Data table");
    field_grid->Add(data_table_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_data_table_text = new wxTextCtrl(m_body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_data_table_text, 0, wxEXPAND, 0);

    body_sizer->Add(field_grid, 0, wxEXPAND, 0);

    // attach body sizer and add body to content sizer
    m_body->SetSizer(body_sizer);
    content_sizer->Add(m_body, 1, wxEXPAND, 0);

    // attach content sizer and card to outer layout
    content->SetSizer(content_sizer);
    outer_sizer->Add(m_card, 1, wxEXPAND, 0);
    SetSizer(outer_sizer);

    // disable all gated controls initially since the checkbox starts unchecked
    m_body->Enable(false);

    // bind the enable checkbox to the toggle handler
    m_enable_cb->Bind(wxEVT_CHECKBOX, &StepParametersPanel::on_enable_toggle, this);
}

void StepParametersPanel::on_enable_toggle(wxCommandEvent& event) { m_body->Enable(event.IsChecked()); }

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
    std::string variable = wx_util::get_text(*m_variable_text);
    std::string start = wx_util::get_text(*m_start_text);
    std::string stop = wx_util::get_text(*m_stop_text);
    std::string step = wx_util::get_text(*m_step_text);
    std::string points = wx_util::get_text(*m_points_text);

    // parse list values and read data table name
    auto list_values = wx_util::split_strings(m_list_values_text->GetValue());
    std::string data_table_name = wx_util::get_text(*m_data_table_text);

    return StepParameters(std::move(sweep_mode), std::move(variable), std::move(start), std::move(stop), std::move(step), std::move(points), std::move(list_values), std::move(data_table_name), enabled);
}

void StepParametersPanel::apply(const StepParameters& params) {
    // restore enable state and sync body controls
    m_enable_cb->SetValue(params.enabled);
    m_body->Enable(params.enabled);

    // restore sweep mode
    wx_util::set_choice_by_string(*m_sweep_mode_choice, params.sweep_mode);

    // restore text fields
    wx_util::set_text(*m_variable_text, params.variable);
    wx_util::set_text(*m_start_text, params.start);
    wx_util::set_text(*m_stop_text, params.stop);
    wx_util::set_text(*m_step_text, params.step);
    wx_util::set_text(*m_points_text, params.points);

    // restore list values as space-separated text and data table name
    m_list_values_text->SetValue(wx_util::join_strings(params.list_values, " "));
    wx_util::set_text(*m_data_table_text, params.data_table_name);
}
