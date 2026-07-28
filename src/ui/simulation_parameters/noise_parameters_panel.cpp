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

#include "global_settings_panel.h"
#include "noise_parameters_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"
#include "simulation_parameters/noise_simulation_parameters.h"

namespace
{
    // parse device noise operators from newline-separated text (type node source)
    [[nodiscard]] std::vector<DeviceNoiseOperator> parse_device_noise_text(const wxString& text) {
        std::vector<DeviceNoiseOperator> operators;
        std::istringstream stream(std::string(text.ToUTF8()));
        std::string line;
        while (std::getline(stream, line)) {
            // trim leading and trailing whitespace
            size_t start = line.find_first_not_of(" \t\r");
            if (start == std::string::npos) {
                continue;
            }
            size_t end = line.find_last_not_of(" \t\r");
            line = line.substr(start, end - start + 1);
            // skip empty lines
            if (line.empty()) {
                continue;
            }
            // split line into whitespace-delimited tokens
            std::vector<std::string> tokens;
            std::istringstream line_stream(line);
            std::string token;
            while (line_stream >> token) {
                tokens.push_back(token);
            }
            // require at least 3 tokens: type, node, source
            if (tokens.size() >= 3) {
                operators.emplace_back(tokens[0], tokens[1], tokens[2]);
            }
        }
        return operators;
    }

    // format device noise operators as newline-separated text
    [[nodiscard]] wxString format_device_noise_operators(const std::vector<DeviceNoiseOperator>& operators) {
        if (operators.empty()) {
            return wxEmptyString;
        }
        wxString result;
        for (size_t i = 0; i < operators.size(); ++i) {
            // add newline separator between entries
            if (i > 0) {
                result += "\n";
            }
            result += wxString::FromUTF8(operators[i].type) + " " + wxString::FromUTF8(operators[i].node) + " " + wxString::FromUTF8(operators[i].source);
        }
        return result;
    }
} // namespace

NoiseParametersPanel::NoiseParametersPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole panel
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // simulation card wrapping all controls
    m_card = new SimulationCard(this, "Noise Analysis (.NOISE)");
    auto* content = m_card->get_content();
    auto* content_sizer = new wxBoxSizer(wxVERTICAL);

    // print section with NOISE print type and BJT leads hidden, FET leads available
    m_print_section = new PrintSectionPanel(content, "NOISE", {"NOISE"}, false, true, true);
    content_sizer->Add(m_print_section, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- noise configuration ---
    // field grid for noise parameters: 2 columns (label | control)
    auto* field_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    field_grid->AddGrowableCol(1, 1);

    // output node row
    auto* output_label = new wxStaticText(content, wxID_ANY, "Output node");
    field_grid->Add(output_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_output_node_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_output_node_text, 0, wxEXPAND, 0);

    // reference node row
    auto* ref_label = new wxStaticText(content, wxID_ANY, "Reference node");
    field_grid->Add(ref_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_ref_node_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_ref_node_text, 0, wxEXPAND, 0);

    // source name row
    auto* source_label = new wxStaticText(content, wxID_ANY, "Source name");
    field_grid->Add(source_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_source_name_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_source_name_text, 0, wxEXPAND, 0);

    // start frequency row
    auto* start_freq_label = new wxStaticText(content, wxID_ANY, "Start frequency");
    field_grid->Add(start_freq_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_start_freq_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_start_freq_text, 0, wxEXPAND, 0);

    // end frequency row
    auto* end_freq_label = new wxStaticText(content, wxID_ANY, "End frequency");
    field_grid->Add(end_freq_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_end_freq_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_end_freq_text, 0, wxEXPAND, 0);

    // number of points row
    auto* num_points_label = new wxStaticText(content, wxID_ANY, "Number of points");
    field_grid->Add(num_points_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_num_points_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_num_points_text, 0, wxEXPAND, 0);

    // sweep type choice row
    auto* sweep_type_label = new wxStaticText(content, wxID_ANY, "Sweep type");
    field_grid->Add(sweep_type_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    wxArrayString sweep_choices;
    sweep_choices.Add("LIN");
    sweep_choices.Add("DEC");
    sweep_choices.Add("OCT");
    sweep_choices.Add("DATA");
    m_sweep_type_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, sweep_choices);
    m_sweep_type_choice->SetSelection(0);
    field_grid->Add(m_sweep_type_choice, 0, wxEXPAND, 0);

    // data table name row
    auto* data_table_label = new wxStaticText(content, wxID_ANY, "Data table");
    field_grid->Add(data_table_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_data_table_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_data_table_text, 0, wxEXPAND, 0);

    content_sizer->Add(field_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- device noise operators ---
    // device noise operator label and multiline text
    auto* dno_label = new wxStaticText(content, wxID_ANY, "Device noise operators (one per line: DNI|DNO node source)");
    content_sizer->Add(dno_label, 0, wxBOTTOM, FromDIP(4));
    m_device_noise_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(300), FromDIP(60)), wxTE_MULTILINE);
    content_sizer->Add(m_device_noise_text, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // global settings panel with replace-ground checkbox
    m_global_settings = new GlobalSettingsPanel(content);
    content_sizer->Add(m_global_settings, 0, wxEXPAND, 0);

    // attach content sizer and card to outer layout
    content->SetSizer(content_sizer);
    outer_sizer->Add(m_card, 1, wxEXPAND, 0);
    SetSizer(outer_sizer);
}

NoiseSimulationParameters NoiseParametersPanel::build_noise_parameters() const {
    // read noise analysis text fields
    std::string output_node = std::string(m_output_node_text->GetValue().ToUTF8());
    std::string ref_node = std::string(m_ref_node_text->GetValue().ToUTF8());
    std::string source_name = std::string(m_source_name_text->GetValue().ToUTF8());
    std::string start_freq = std::string(m_start_freq_text->GetValue().ToUTF8());
    std::string end_freq = std::string(m_end_freq_text->GetValue().ToUTF8());
    std::string num_points = std::string(m_num_points_text->GetValue().ToUTF8());

    // read sweep type from combo
    std::string sweep_type = std::string(m_sweep_type_choice->GetStringSelection().ToUTF8());

    // read data table name
    std::string data_table_name = std::string(m_data_table_text->GetValue().ToUTF8());

    // read and parse device noise operators from multiline text
    auto device_noise_operators = parse_device_noise_text(m_device_noise_text->GetValue());

    // read replace ground from global settings
    bool replace_ground = m_global_settings->get_replace_ground();

    // read print parameters from print section
    std::optional<PrintParameters> print_params = m_print_section->build_print_parameters();

    return NoiseSimulationParameters(std::move(output_node), std::move(ref_node), std::move(source_name), std::move(start_freq), std::move(end_freq), std::move(num_points), std::move(sweep_type), std::move(device_noise_operators), std::move(data_table_name), replace_ground, std::move(print_params));
}

void NoiseParametersPanel::apply(const NoiseSimulationParameters& params) {
    // restore noise analysis text fields
    m_output_node_text->SetValue(wxString::FromUTF8(params.output_node));
    m_ref_node_text->SetValue(wxString::FromUTF8(params.ref_node));
    m_source_name_text->SetValue(wxString::FromUTF8(params.source_name));
    m_start_freq_text->SetValue(wxString::FromUTF8(params.start_freq_value));
    m_end_freq_text->SetValue(wxString::FromUTF8(params.end_freq_value));
    m_num_points_text->SetValue(wxString::FromUTF8(params.num_points_value));

    // restore sweep type, fall back to first choice if not found
    int sw_idx = m_sweep_type_choice->FindString(wxString::FromUTF8(params.sweep_type));
    m_sweep_type_choice->SetSelection(sw_idx != wxNOT_FOUND ? sw_idx : 0);

    // restore data table name and device noise operators
    m_data_table_text->SetValue(wxString::FromUTF8(params.data_table_name));
    m_device_noise_text->SetValue(format_device_noise_operators(params.device_noise_operators));

    // restore replace ground
    m_global_settings->set_replace_ground(params.replace_ground);

    // restore print parameters (BJT leads hidden, FET leads relevant for NOISE)
    m_print_section->apply(params.print_parameters ? &*params.print_parameters : nullptr, true, true);
}

GlobalSettingsPanel* NoiseParametersPanel::get_global_settings() const { return m_global_settings; }

PrintSectionPanel* NoiseParametersPanel::get_print_section() const { return m_print_section; }
