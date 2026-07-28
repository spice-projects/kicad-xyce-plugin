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

#include "../../simulation_parameters/ac_simulation_parameters.h"
#include "ac_parameters_panel.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"

AcParametersPanel::AcParametersPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole panel
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // simulation card wrapping all controls
    m_card = new SimulationCard(this, "AC Analysis (.AC)");
    auto* content = m_card->get_content();
    auto* content_sizer = new wxBoxSizer(wxVERTICAL);

    // global settings panel with replace-ground checkbox
    m_global_settings = new GlobalSettingsPanel(content);
    content_sizer->Add(m_global_settings, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- sweep configuration ---
    // AC field grid: 2 columns (label | control)
    auto* field_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    field_grid->AddGrowableCol(1, 1);

    // sweep mode row
    auto* sweep_mode_label = new wxStaticText(content, wxID_ANY, "Sweep mode");
    field_grid->Add(sweep_mode_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    wxArrayString sweep_choices;
    sweep_choices.Add("LIN");
    sweep_choices.Add("DEC");
    sweep_choices.Add("OCT");
    sweep_choices.Add("DATA");
    m_sweep_mode_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, sweep_choices);
    m_sweep_mode_choice->SetSelection(0);
    field_grid->Add(m_sweep_mode_choice, 0, wxEXPAND, 0);

    // points row
    auto* points_label = new wxStaticText(content, wxID_ANY, "Points");
    field_grid->Add(points_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_points_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_points_text, 0, wxEXPAND, 0);

    // start frequency row
    auto* start_label = new wxStaticText(content, wxID_ANY, "Start");
    field_grid->Add(start_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_start_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_start_text, 0, wxEXPAND, 0);

    // end frequency row
    auto* end_label = new wxStaticText(content, wxID_ANY, "End");
    field_grid->Add(end_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_end_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_end_text, 0, wxEXPAND, 0);

    // data table name row
    auto* data_table_label = new wxStaticText(content, wxID_ANY, "Data table");
    field_grid->Add(data_table_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_data_table_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_data_table_text, 0, wxEXPAND, 0);

    content_sizer->Add(field_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- print section ---
    // print section with AC print types and both BJT/FET leads available
    m_print_section = new PrintSectionPanel(content, "AC", {"AC", "AC_IC"}, true, true, true);
    content_sizer->Add(m_print_section, 0, wxEXPAND, 0);

    // attach content sizer and card to outer layout
    content->SetSizer(content_sizer);
    outer_sizer->Add(m_card, 1, wxEXPAND, 0);
    SetSizer(outer_sizer);
}

AcSimulationParameters AcParametersPanel::build_ac_parameters() const {
    // read text fields
    std::string points = std::string(m_points_text->GetValue().ToUTF8());
    std::string start = std::string(m_start_text->GetValue().ToUTF8());
    std::string end = std::string(m_end_text->GetValue().ToUTF8());

    // read sweep mode from combo
    std::string sweep_mode;
    int sel = m_sweep_mode_choice->GetSelection();
    if (sel != wxNOT_FOUND) {
        sweep_mode = std::string(m_sweep_mode_choice->GetString(sel).ToUTF8());
    }

    // read data table name
    std::string data_table = std::string(m_data_table_text->GetValue().ToUTF8());

    // read replace ground from global settings
    bool replace_ground = m_global_settings->get_replace_ground();

    // read print parameters from print section
    std::optional<PrintParameters> print_params = m_print_section->build_print_parameters();

    return AcSimulationParameters(std::move(sweep_mode), std::move(points), std::move(start), std::move(end), std::move(data_table), replace_ground, std::move(print_params), {}, std::nullopt);
}

void AcParametersPanel::apply(const AcSimulationParameters& params) {
    // restore sweep mode
    int mode_index = m_sweep_mode_choice->FindString(wxString::FromUTF8(params.sweep_mode));
    // select stored mode or default to first choice
    if (mode_index != wxNOT_FOUND) {
        m_sweep_mode_choice->SetSelection(mode_index);
    }
    else {
        m_sweep_mode_choice->SetSelection(0);
    }

    // restore text fields
    m_points_text->SetValue(wxString::FromUTF8(params.points));
    m_start_text->SetValue(wxString::FromUTF8(params.start));
    m_end_text->SetValue(wxString::FromUTF8(params.end));
    m_data_table_text->SetValue(wxString::FromUTF8(params.data_table_name));

    // restore replace ground
    m_global_settings->set_replace_ground(params.replace_ground);

    // restore print parameters (BJT and FET leads both always relevant for AC)
    m_print_section->apply(params.print_parameters ? &*params.print_parameters : nullptr, true, true);
}

GlobalSettingsPanel* AcParametersPanel::get_global_settings() const { return m_global_settings; }

PrintSectionPanel* AcParametersPanel::get_print_section() const { return m_print_section; }
