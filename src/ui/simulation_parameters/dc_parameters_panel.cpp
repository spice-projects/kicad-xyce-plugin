#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/arrstr.h>
#include <wx/choice.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../wx_util.h"
#include "dc_parameters_panel.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"
#include "simulation_parameters/dc_simulation_parameters.h"

DcParametersPanel::DcParametersPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole panel
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // simulation card wrapping all controls
    m_card = new SimulationCard(this, "DC Sweep (.DC)");
    auto* content = m_card->get_content();
    auto* content_sizer = new wxBoxSizer(wxVERTICAL);

    // print section with DC print types and both BJT/FET leads available
    m_print_section = new PrintSectionPanel(content, "DC", {"DC", "HOMOTOPY"}, true, true, true);
    content_sizer->Add(m_print_section, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- sweep mode ---
    auto* mode_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* mode_label = new wxStaticText(content, wxID_ANY, "Sweep mode");
    mode_sizer->Add(mode_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
    m_sweep_mode_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, wx_util::to_wx_array_string({"LIN", "DEC", "OCT", "LIST", "DATA"}));
    m_sweep_mode_choice->SetSelection(0);
    m_sweep_mode_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { on_sweep_mode_changed(); });
    mode_sizer->Add(m_sweep_mode_choice, 0, wxALL, 0);
    content_sizer->Add(mode_sizer, 0, wxBOTTOM, FromDIP(12));

    // --- primary sweep ---
    auto* primary_label = new wxStaticText(content, wxID_ANY, "Primary sweep");
    wxFont primary_font = primary_label->GetFont();
    primary_font.SetWeight(wxFONTWEIGHT_BOLD);
    primary_label->SetFont(primary_font);
    content_sizer->Add(primary_label, 0, wxBOTTOM, FromDIP(4));

    // field grid for primary sweep parameters: 2 columns (label | control)
    auto* field_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    field_grid->AddGrowableCol(1, 1);

    // primary variable row
    auto* primary_var_label = new wxStaticText(content, wxID_ANY, "Primary variable");
    field_grid->Add(primary_var_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_primary_variable_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_primary_variable_text, 0, wxEXPAND, 0);

    // start value row
    auto* start_label = new wxStaticText(content, wxID_ANY, "Start");
    field_grid->Add(start_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_start_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_start_text, 0, wxEXPAND, 0);

    // stop value row
    auto* stop_label = new wxStaticText(content, wxID_ANY, "Stop");
    field_grid->Add(stop_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_stop_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_stop_text, 0, wxEXPAND, 0);

    // step size row
    auto* step_label = new wxStaticText(content, wxID_ANY, "Step");
    field_grid->Add(step_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_step_text, 0, wxEXPAND, 0);

    // points for log sweep row
    auto* points_label = new wxStaticText(content, wxID_ANY, "Points");
    field_grid->Add(points_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_points_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_points_text, 0, wxEXPAND, 0);

    // list values row
    auto* list_values_label = new wxStaticText(content, wxID_ANY, "List values");
    field_grid->Add(list_values_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_list_values_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_list_values_text, 0, wxEXPAND, 0);

    // data table name row
    auto* data_table_label = new wxStaticText(content, wxID_ANY, "Data table");
    field_grid->Add(data_table_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_data_table_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_data_table_text, 0, wxEXPAND, 0);

    content_sizer->Add(field_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- secondary sweep ---
    auto* secondary_label = new wxStaticText(content, wxID_ANY, "Secondary sweep");
    wxFont secondary_font = secondary_label->GetFont();
    secondary_font.SetWeight(wxFONTWEIGHT_BOLD);
    secondary_label->SetFont(secondary_font);
    content_sizer->Add(secondary_label, 0, wxBOTTOM, FromDIP(4));

    // field grid for secondary sweep parameters: 2 columns (label | control)
    auto* secondary_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    secondary_grid->AddGrowableCol(1, 1);

    // secondary variable row
    auto* secondary_var_label = new wxStaticText(content, wxID_ANY, "Secondary variable");
    secondary_grid->Add(secondary_var_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_secondary_variable_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    secondary_grid->Add(m_secondary_variable_text, 0, wxEXPAND, 0);

    // secondary start value row
    auto* secondary_start_label = new wxStaticText(content, wxID_ANY, "Secondary start");
    secondary_grid->Add(secondary_start_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_secondary_start_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    secondary_grid->Add(m_secondary_start_text, 0, wxEXPAND, 0);

    // secondary stop value row
    auto* secondary_stop_label = new wxStaticText(content, wxID_ANY, "Secondary stop");
    secondary_grid->Add(secondary_stop_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_secondary_stop_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    secondary_grid->Add(m_secondary_stop_text, 0, wxEXPAND, 0);

    // secondary step size row
    auto* secondary_step_label = new wxStaticText(content, wxID_ANY, "Secondary step");
    secondary_grid->Add(secondary_step_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_secondary_step_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    secondary_grid->Add(m_secondary_step_text, 0, wxEXPAND, 0);

    // secondary points for log sweep row
    auto* secondary_points_label = new wxStaticText(content, wxID_ANY, "Secondary points");
    secondary_grid->Add(secondary_points_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_secondary_points_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    secondary_grid->Add(m_secondary_points_text, 0, wxEXPAND, 0);

    content_sizer->Add(secondary_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // global settings panel with replace-ground checkbox
    m_global_settings = new GlobalSettingsPanel(content);
    content_sizer->Add(m_global_settings, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // .MEASURE multi-line text
    auto* measure_label = new wxStaticText(content, wxID_ANY, ".MEASURE directives (one per line)");
    content_sizer->Add(measure_label, 0, wxBOTTOM, FromDIP(4));
    m_measure_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(300), FromDIP(60)), wxTE_MULTILINE);
    content_sizer->Add(m_measure_text, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // attach content sizer and card to outer layout
    content->SetSizer(content_sizer);
    outer_sizer->Add(m_card, 1, wxEXPAND, 0);
    SetSizer(outer_sizer);
}

DCSimulationParameters DcParametersPanel::build_dc_parameters() const {
    // read sweep mode and primary sweep text fields
    std::string sweep_mode = wx_util::get_string_selection(*m_sweep_mode_choice);
    std::string primary_variable = wx_util::get_text(*m_primary_variable_text);
    std::string start = wx_util::get_text(*m_start_text);
    std::string stop = wx_util::get_text(*m_stop_text);

    // read sweep-type-dependent primary fields
    std::string step;
    std::string points;
    std::vector<std::string> list_values;
    std::string data_table_name;
    // use step for LIN sweeps
    if (sweep_mode == "LIN") {
        step = wx_util::get_text(*m_step_text);
        // carry over the points value when switching from DEC/OCT and step is unset
        if (step.empty()) {
            step = wx_util::get_text(*m_points_text);
        }
    }
    // use points for DEC/OCT log sweeps
    else if (sweep_mode == "DEC" || sweep_mode == "OCT") {
        points = wx_util::get_text(*m_points_text);
        // carry over the step value when switching from LIN and points is unset
        if (points.empty()) {
            points = wx_util::get_text(*m_step_text);
        }
    }
    // use list values for LIST mode
    else if (sweep_mode == "LIST") {
        list_values = wx_util::split_strings(m_list_values_text->GetValue());
    }
    // use data table for DATA mode
    else if (sweep_mode == "DATA") {
        data_table_name = wx_util::get_text(*m_data_table_text);
    }

    // read secondary sweep text fields
    std::string secondary_variable = wx_util::get_text(*m_secondary_variable_text);
    std::string secondary_start = wx_util::get_text(*m_secondary_start_text);
    std::string secondary_stop = wx_util::get_text(*m_secondary_stop_text);

    // read sweep-type-dependent secondary fields
    std::string secondary_step;
    std::string secondary_points;
    // use step for LIN sweeps
    if (sweep_mode == "LIN") {
        secondary_step = wx_util::get_text(*m_secondary_step_text);
        // carry over the points value when switching from DEC/OCT and step is unset
        if (secondary_step.empty()) {
            secondary_step = wx_util::get_text(*m_secondary_points_text);
        }
    }
    // use points for DEC/OCT log sweeps
    else if (sweep_mode == "DEC" || sweep_mode == "OCT") {
        secondary_points = wx_util::get_text(*m_secondary_points_text);
        // carry over the step value when switching from LIN and points is unset
        if (secondary_points.empty()) {
            secondary_points = wx_util::get_text(*m_secondary_step_text);
        }
    }

    // read print parameters from print section
    std::optional<PrintParameters> print_params = m_print_section->build_print_parameters();

    // parse .MEASURE directives (one per line)
    std::vector<MeasureEntry> measure_params;
    {
        auto lines = wx_util::split_lines(m_measure_text->GetValue());
        for (const auto& line : lines) {
            auto parsed = MeasureEntry::from_xyce_statement(line);
            if (parsed) {
                measure_params.push_back(std::move(*parsed));
            }
        }
    }

    return DCSimulationParameters(std::move(sweep_mode), std::move(primary_variable), std::move(start), std::move(stop), std::move(step), std::move(points), std::move(list_values), std::move(data_table_name), std::move(secondary_variable), std::move(secondary_start), std::move(secondary_stop), std::move(secondary_step), std::move(secondary_points), std::move(print_params), std::move(measure_params), std::nullopt);
}

void DcParametersPanel::apply(const DCSimulationParameters& params) {
    // restore sweep mode
    wx_util::set_choice_by_string(*m_sweep_mode_choice, params.sweep_mode);

    // restore primary sweep text fields
    wx_util::set_text(*m_primary_variable_text, params.primary_variable);
    wx_util::set_text(*m_start_text, params.start);
    wx_util::set_text(*m_stop_text, params.stop);
    wx_util::set_text(*m_step_text, params.step);
    wx_util::set_text(*m_points_text, params.points);
    m_list_values_text->SetValue(wx_util::join_strings(params.list_values, " "));
    wx_util::set_text(*m_data_table_text, params.data_table_name);

    // restore secondary sweep text fields
    wx_util::set_text(*m_secondary_variable_text, params.secondary_variable);
    wx_util::set_text(*m_secondary_start_text, params.secondary_start);
    wx_util::set_text(*m_secondary_stop_text, params.secondary_stop);
    wx_util::set_text(*m_secondary_step_text, params.secondary_step);
    wx_util::set_text(*m_secondary_points_text, params.secondary_points);

    // restore print parameters (BJT and FET leads both always relevant for DC)
    m_print_section->apply(params.print_parameters ? &*params.print_parameters : nullptr, true, true);

    // restore .MEASURE directives
    m_measure_text->SetValue(wx_util::join_strings(params.measure_parameters, "\n", &MeasureEntry::to_xyce_statement));
}

GlobalSettingsPanel* DcParametersPanel::get_global_settings() const { return m_global_settings; }

PrintSectionPanel* DcParametersPanel::get_print_section() const { return m_print_section; }

void DcParametersPanel::on_sweep_mode_changed() {
    // carry the numeric value between the step and points fields when switching modes
    // so the parameter is visible and editable in the newly selected mode
    const wxString mode = m_sweep_mode_choice->GetStringSelection();
    if (mode == "DEC" || mode == "OCT") {
        // carry the LIN step value into the points field when points is unset
        if (m_points_text->GetValue().IsEmpty() && !m_step_text->GetValue().IsEmpty()) {
            m_points_text->SetValue(m_step_text->GetValue());
        }
        // carry the secondary LIN step value into the secondary points field
        if (m_secondary_points_text->GetValue().IsEmpty() && !m_secondary_step_text->GetValue().IsEmpty()) {
            m_secondary_points_text->SetValue(m_secondary_step_text->GetValue());
        }
    }
    else if (mode == "LIN") {
        // carry the log points value into the step field when step is unset
        if (m_step_text->GetValue().IsEmpty() && !m_points_text->GetValue().IsEmpty()) {
            m_step_text->SetValue(m_points_text->GetValue());
        }
        // carry the secondary log points value into the secondary step field
        if (m_secondary_step_text->GetValue().IsEmpty() && !m_secondary_points_text->GetValue().IsEmpty()) {
            m_secondary_step_text->SetValue(m_secondary_points_text->GetValue());
        }
    }
}
