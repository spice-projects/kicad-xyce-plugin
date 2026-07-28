#include <sstream>
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
#include <wx/tokenzr.h>
#endif

#include "dc_parameters_panel.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"
#include "simulation_parameters/dc_simulation_parameters.h"

namespace
{
    // split multi-line text into non-empty trimmed lines
    [[nodiscard]] std::vector<std::string> parse_lines(const wxString& text) {
        std::vector<std::string> lines;
        std::istringstream stream(std::string(text.ToUTF8()));
        std::string line;
        while (std::getline(stream, line)) {
            size_t start = line.find_first_not_of(" \t\r");
            if (start == std::string::npos)
                continue;
            size_t end = line.find_last_not_of(" \t\r");
            line = line.substr(start, end - start + 1);
            if (!line.empty())
                lines.push_back(std::move(line));
        }
        return lines;
    }

    // format list values as space-separated string
    [[nodiscard]] wxString format_list_values(const std::vector<std::string>& values) {
        if (values.empty()) {
            return wxEmptyString;
        }
        wxString result;
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                result += " ";
            }
            result += wxString::FromUTF8(values[i]);
        }
        return result;
    }

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
} // namespace

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
    wxArrayString mode_choices;
    mode_choices.Add("LIN");
    mode_choices.Add("DEC");
    mode_choices.Add("OCT");
    mode_choices.Add("LIST");
    mode_choices.Add("DATA");
    m_sweep_mode_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, mode_choices);
    m_sweep_mode_choice->SetSelection(0);
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
    std::string sweep_mode = std::string(m_sweep_mode_choice->GetStringSelection().ToUTF8());
    std::string primary_variable = std::string(m_primary_variable_text->GetValue().ToUTF8());
    std::string start = std::string(m_start_text->GetValue().ToUTF8());
    std::string stop = std::string(m_stop_text->GetValue().ToUTF8());

    // read sweep-type-dependent primary fields
    std::string step;
    std::string points;
    std::vector<std::string> list_values;
    std::string data_table_name;
    // use step for LIN sweeps
    if (sweep_mode == "LIN") {
        step = std::string(m_step_text->GetValue().ToUTF8());
    }
    // use points for DEC/OCT log sweeps
    else if (sweep_mode == "DEC" || sweep_mode == "OCT") {
        points = std::string(m_points_text->GetValue().ToUTF8());
    }
    // use list values for LIST mode
    else if (sweep_mode == "LIST") {
        list_values = parse_list_values(m_list_values_text->GetValue());
    }
    // use data table for DATA mode
    else if (sweep_mode == "DATA") {
        data_table_name = std::string(m_data_table_text->GetValue().ToUTF8());
    }

    // read secondary sweep text fields
    std::string secondary_variable = std::string(m_secondary_variable_text->GetValue().ToUTF8());
    std::string secondary_start = std::string(m_secondary_start_text->GetValue().ToUTF8());
    std::string secondary_stop = std::string(m_secondary_stop_text->GetValue().ToUTF8());

    // read sweep-type-dependent secondary fields
    std::string secondary_step;
    std::string secondary_points;
    // use step for LIN sweeps
    if (sweep_mode == "LIN") {
        secondary_step = std::string(m_secondary_step_text->GetValue().ToUTF8());
    }
    // use points for DEC/OCT log sweeps
    else if (sweep_mode == "DEC" || sweep_mode == "OCT") {
        secondary_points = std::string(m_secondary_points_text->GetValue().ToUTF8());
    }

    // read replace ground from global settings
    bool replace_ground = m_global_settings->get_replace_ground();

    // read print parameters from print section
    std::optional<PrintParameters> print_params = m_print_section->build_print_parameters();

    // parse .MEASURE directives (one per line)
    std::vector<MeasureEntry> measure_params;
    {
        auto lines = parse_lines(m_measure_text->GetValue());
        for (const auto& line : lines) {
            auto parsed = MeasureEntry::from_xyce_statement(line);
            if (parsed) {
                measure_params.push_back(std::move(*parsed));
            }
        }
    }

    return DCSimulationParameters(std::move(sweep_mode), std::move(primary_variable), std::move(start), std::move(stop), std::move(step), std::move(points), std::move(list_values), std::move(data_table_name), std::move(secondary_variable), std::move(secondary_start), std::move(secondary_stop), std::move(secondary_step), std::move(secondary_points), replace_ground, std::move(print_params), std::move(measure_params), std::nullopt);
}

void DcParametersPanel::apply(const DCSimulationParameters& params) {
    // restore sweep mode
    wxString mode = wxString::FromUTF8(params.sweep_mode);
    int mode_index = m_sweep_mode_choice->FindString(mode);
    // select stored mode or default to first choice
    if (mode_index != wxNOT_FOUND) {
        m_sweep_mode_choice->SetSelection(mode_index);
    }
    else {
        m_sweep_mode_choice->SetSelection(0);
    }

    // restore primary sweep text fields
    m_primary_variable_text->SetValue(wxString::FromUTF8(params.primary_variable));
    m_start_text->SetValue(wxString::FromUTF8(params.start));
    m_stop_text->SetValue(wxString::FromUTF8(params.stop));
    m_step_text->SetValue(wxString::FromUTF8(params.step));
    m_points_text->SetValue(wxString::FromUTF8(params.points));
    m_list_values_text->SetValue(format_list_values(params.list_values));
    m_data_table_text->SetValue(wxString::FromUTF8(params.data_table_name));

    // restore secondary sweep text fields
    m_secondary_variable_text->SetValue(wxString::FromUTF8(params.secondary_variable));
    m_secondary_start_text->SetValue(wxString::FromUTF8(params.secondary_start));
    m_secondary_stop_text->SetValue(wxString::FromUTF8(params.secondary_stop));
    m_secondary_step_text->SetValue(wxString::FromUTF8(params.secondary_step));
    m_secondary_points_text->SetValue(wxString::FromUTF8(params.secondary_points));

    // restore replace ground
    m_global_settings->set_replace_ground(params.replace_ground);

    // restore print parameters (BJT and FET leads both always relevant for DC)
    m_print_section->apply(params.print_parameters ? &*params.print_parameters : nullptr, true, true);

    // restore .MEASURE directives
    {
        wxString text;
        for (size_t i = 0; i < params.measure_parameters.size(); ++i) {
            if (i > 0)
                text += "\n";
            text += wxString::FromUTF8(params.measure_parameters[i].to_xyce_statement());
        }
        m_measure_text->SetValue(text);
    }
}

GlobalSettingsPanel* DcParametersPanel::get_global_settings() const { return m_global_settings; }

PrintSectionPanel* DcParametersPanel::get_print_section() const { return m_print_section; }
