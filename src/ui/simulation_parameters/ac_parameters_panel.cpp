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
#include "ac_parameters_panel.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"
#include "simulation_parameters/ac_simulation_parameters.h"

AcParametersPanel::AcParametersPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole panel
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // simulation card wrapping all controls
    m_card = new SimulationCard(this, "AC Analysis (.AC)");
    auto* content = m_card->get_content();
    auto* content_sizer = new wxBoxSizer(wxVERTICAL);

    // --- sweep configuration ---
    // AC field grid: 2 columns (label | control)
    auto* field_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    field_grid->AddGrowableCol(1, 1);

    // sweep mode row
    auto* sweep_mode_label = new wxStaticText(content, wxID_ANY, "Sweep mode");
    field_grid->Add(sweep_mode_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_sweep_mode_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, wx_util::to_wx_array_string({"LIN", "DEC", "OCT", "DATA"}));
    m_sweep_mode_choice->SetSelection(0);
    m_sweep_mode_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { on_sweep_mode_changed(); });
    field_grid->Add(m_sweep_mode_choice, 0, wxEXPAND, 0);

    // points row
    m_points_label = new wxStaticText(content, wxID_ANY, "Points");
    field_grid->Add(m_points_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_points_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_points_text, 0, wxEXPAND, 0);

    // start frequency row
    m_start_label = new wxStaticText(content, wxID_ANY, "Start");
    field_grid->Add(m_start_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_start_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_start_text, 0, wxEXPAND, 0);

    // end frequency row
    m_end_label = new wxStaticText(content, wxID_ANY, "End");
    field_grid->Add(m_end_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_end_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_end_text, 0, wxEXPAND, 0);

    // data table name row
    m_data_table_label = new wxStaticText(content, wxID_ANY, "Data table");
    field_grid->Add(m_data_table_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_data_table_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_data_table_text, 0, wxEXPAND, 0);

    content_sizer->Add(field_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // .MEASURE multi-line text
    auto* measure_label = new wxStaticText(content, wxID_ANY, ".MEASURE directives (one per line)");
    content_sizer->Add(measure_label, 0, wxBOTTOM, FromDIP(4));
    m_measure_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(300), FromDIP(60)), wxTE_MULTILINE);
    content_sizer->Add(m_measure_text, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- print section ---
    // print section with AC print types; power is not available for AC analysis
    m_print_section = new PrintSectionPanel(content, "AC", {"AC", "AC_IC"}, false, true, true);
    content_sizer->Add(m_print_section, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // global settings panel with replace-ground checkbox
    m_global_settings = new GlobalSettingsPanel(content);
    content_sizer->Add(m_global_settings, 0, wxEXPAND, 0);

    // attach content sizer and card to outer layout
    content->SetSizer(content_sizer);
    outer_sizer->Add(m_card, 1, wxEXPAND, 0);
    SetSizer(outer_sizer);

    // sync field visibility to the default LIN sweep mode
    on_sweep_mode_changed();
}

AcSimulationParameters AcParametersPanel::build_ac_parameters() const {
    // read text fields
    std::string points = wx_util::get_text(*m_points_text);
    std::string start = wx_util::get_text(*m_start_text);
    std::string end = wx_util::get_text(*m_end_text);

    // read sweep mode from combo
    std::string sweep_mode = wx_util::get_string_selection(*m_sweep_mode_choice);

    // read data table name
    std::string data_table = wx_util::get_text(*m_data_table_text);

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

    return AcSimulationParameters(std::move(sweep_mode), std::move(points), std::move(start), std::move(end), std::move(data_table), std::move(print_params), std::move(measure_params), std::nullopt);
}

void AcParametersPanel::apply(const AcSimulationParameters& params) {
    // restore sweep mode
    wx_util::set_choice_by_string(*m_sweep_mode_choice, params.sweep_mode);

    // restore text fields
    wx_util::set_text(*m_points_text, params.points);
    wx_util::set_text(*m_start_text, params.start);
    wx_util::set_text(*m_end_text, params.end);
    wx_util::set_text(*m_data_table_text, params.data_table_name);

    // restore print parameters (BJT and FET leads both always relevant for AC)
    m_print_section->apply(params.print_parameters ? &*params.print_parameters : nullptr, true, true);

    // restore .MEASURE directives
    m_measure_text->SetValue(wx_util::join_strings(params.measure_parameters, "\n", &MeasureEntry::to_xyce_statement));

    // sync field visibility to the restored sweep mode
    on_sweep_mode_changed();
}

GlobalSettingsPanel* AcParametersPanel::get_global_settings() const { return m_global_settings; }

PrintSectionPanel* AcParametersPanel::get_print_section() const { return m_print_section; }

void AcParametersPanel::on_sweep_mode_changed() {
    // data table only applies to DATA sweeps
    const bool is_data_sweep = m_sweep_mode_choice->GetStringSelection() == "DATA";
    // show the data table row for DATA sweeps, otherwise reset and hide it
    m_data_table_label->Show(is_data_sweep);
    m_data_table_text->Show(is_data_sweep);
    // reset the data table field if it is not shown
    if (!is_data_sweep)
        m_data_table_text->SetValue(wxEmptyString);
    // show the sweep range fields for LIN, DEC and OCT sweeps
    const bool show_range_fields = !is_data_sweep;
    // show or hide the sweep range fields
    m_points_label->Show(show_range_fields);
    m_points_text->Show(show_range_fields);
    m_start_label->Show(show_range_fields);
    m_start_text->Show(show_range_fields);
    m_end_label->Show(show_range_fields);
    m_end_text->Show(show_range_fields);
    // reset the hidden range fields if they are not shown
    if (!show_range_fields) {
        // reset the hidden range fields
        m_points_text->SetValue(wxEmptyString);
        m_start_text->SetValue(wxEmptyString);
        m_end_text->SetValue(wxEmptyString);
    }
    // re-layout the panel to reflect the new field visibility
    Layout();
}
