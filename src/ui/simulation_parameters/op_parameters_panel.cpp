#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/arrstr.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>
#endif

#include "../../simulation_parameters/op_simulation_parameters.h"
#include "global_settings_panel.h"
#include "op_parameters_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"

namespace
{
    // parse space-separated V(node)=voltage tokens into NodesetEntry objects
    [[nodiscard]] std::vector<NodesetEntry> parse_nodeset_text(const wxString& text) {
        std::vector<NodesetEntry> entries;
        wxStringTokenizer tokenizer(text, " \t\r\n");
        while (tokenizer.HasMoreTokens()) {
            wxString token = tokenizer.GetNextToken();
            int eq_pos = token.Find('=');
            if (eq_pos == wxNOT_FOUND) {
                continue;
            }
            wxString node_part = token.Mid(0, eq_pos);
            wxString voltage = token.Mid(eq_pos + 1);
            // validate V(node) format
            if (node_part.StartsWith("V(") && node_part.EndsWith(")")) {
                wxString node = node_part.Mid(2, node_part.Length() - 3);
                if (!node.IsEmpty() && !voltage.IsEmpty()) {
                    entries.emplace_back(std::string(node.ToUTF8()), std::string(voltage.ToUTF8()));
                }
            }
        }
        return entries;
    }

    // format NodesetEntry objects as space-separated V(node)=voltage tokens
    [[nodiscard]] wxString format_nodeset_entries(const std::vector<NodesetEntry>& entries) {
        if (entries.empty()) {
            return wxEmptyString;
        }
        wxString result;
        for (size_t i = 0; i < entries.size(); ++i) {
            if (i > 0) {
                result += " ";
            }
            result += "V(" + wxString::FromUTF8(entries[i].node) + ")=" + wxString::FromUTF8(entries[i].voltage);
        }
        return result;
    }

    // parse IC text into IcEntry objects;
    // accepts V(node)=voltage, node=voltage, and bare node voltage pair formats
    [[nodiscard]] std::vector<IcEntry> parse_ic_text(const wxString& text) {
        std::vector<IcEntry> entries;
        wxArrayString tokens = wxStringTokenize(text, " \t\r\n");
        for (size_t i = 0; i < tokens.size();) {
            const wxString& token = tokens[i];
            int eq_pos = token.Find('=');
            if (eq_pos != wxNOT_FOUND) {
                // V(node)=val or node=val form
                wxString lhs = token.Mid(0, eq_pos);
                wxString voltage = token.Mid(eq_pos + 1);
                wxString node;
                if (lhs.StartsWith("V(") && lhs.EndsWith(")")) {
                    node = lhs.Mid(2, lhs.Length() - 3);
                }
                else {
                    node = lhs;
                }
                if (!node.IsEmpty() && !voltage.IsEmpty()) {
                    entries.emplace_back(std::string(node.ToUTF8()), std::string(voltage.ToUTF8()));
                }
                ++i;
            }
            else if (i + 1 < tokens.size()) {
                // bare node voltage pair form
                entries.emplace_back(std::string(token.ToUTF8()), std::string(tokens[i + 1].ToUTF8()));
                i += 2;
            }
            else {
                ++i;
            }
        }
        return entries;
    }

    // format IcEntry objects as space-separated V(node)=voltage tokens
    [[nodiscard]] wxString format_ic_entries(const std::vector<IcEntry>& entries) {
        if (entries.empty()) {
            return wxEmptyString;
        }
        wxString result;
        for (size_t i = 0; i < entries.size(); ++i) {
            if (i > 0) {
                result += " ";
            }
            result += "V(" + wxString::FromUTF8(entries[i].node) + ")=" + wxString::FromUTF8(entries[i].voltage);
        }
        return result;
    }
} // namespace

OpParametersPanel::OpParametersPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole panel
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // simulation card wrapping all controls
    m_card = new SimulationCard(this, "Operating Point Analysis (.OP)");
    auto* content = m_card->get_content();
    auto* content_sizer = new wxBoxSizer(wxVERTICAL);

    // print section with DC print type and both BJT/FET leads available
    m_print_section = new PrintSectionPanel(content, "DC", {"DC"}, true, true, false);
    content_sizer->Add(m_print_section, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- save section ---
    m_save_enable_cb = new wxCheckBox(content, wxID_ANY, "Enable .SAVE operating point");
    m_save_enable_cb->SetFont(m_save_enable_cb->GetFont().MakeBold());
    content_sizer->Add(m_save_enable_cb, 0, wxBOTTOM, FromDIP(4));

    auto* save_body = new wxPanel(content);
    auto* save_body_sizer = new wxBoxSizer(wxVERTICAL);
    // radio buttons for save type
    auto* save_type_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_save_nodeset_rb = new wxRadioButton(save_body, wxID_ANY, "Save as .NODESET", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_save_nodeset_rb->SetValue(true);
    save_type_sizer->Add(m_save_nodeset_rb, 0, wxRIGHT, FromDIP(16));
    m_save_ic_rb = new wxRadioButton(save_body, wxID_ANY, "Save as .IC");
    save_type_sizer->Add(m_save_ic_rb, 0, 0, 0);
    save_body_sizer->Add(save_type_sizer, 0, wxBOTTOM, FromDIP(8));
    // save file row
    auto* save_file_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* save_file_label = new wxStaticText(save_body, wxID_ANY, "Save file");
    save_file_label->SetForegroundColour(wxColour("#6B6B66"));
    save_file_sizer->Add(save_file_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
    m_save_file_text = new wxTextCtrl(save_body, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_save_file_text->SetHint("optional output file path");
    save_file_sizer->Add(m_save_file_text, 1, wxEXPAND, 0);
    save_body_sizer->Add(save_file_sizer, 0, wxEXPAND, 0);
    save_body->SetSizer(save_body_sizer);
    content_sizer->Add(save_body, 0, wxEXPAND | wxLEFT, FromDIP(28));
    content_sizer->Add(0, FromDIP(12), 0, 0);

    // --- nodeset section ---
    auto* nodeset_label = new wxStaticText(content, wxID_ANY, "Convergence Hints (.NODESET)");
    nodeset_label->SetFont(nodeset_label->GetFont().MakeBold());
    content_sizer->Add(nodeset_label, 0, wxBOTTOM, FromDIP(2));
    auto* nodeset_desc = new wxStaticText(content, wxID_ANY, "Initial voltage/current guesses to help DC convergence");
    nodeset_desc->SetForegroundColour(wxColour("#6B6B66"));
    nodeset_desc->SetFont(nodeset_desc->GetFont().Smaller());
    content_sizer->Add(nodeset_desc, 0, wxBOTTOM, FromDIP(4));
    m_nodeset_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    wxString nodeset_hint = "e.g. V(1)=5.0 V(2)=3.3";
    m_nodeset_text->SetHint(nodeset_hint);
    content_sizer->Add(m_nodeset_text, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- initial conditions section ---
    auto* ic_label = new wxStaticText(content, wxID_ANY, "Initial Conditions (.IC / .DCVOLT)");
    ic_label->SetFont(ic_label->GetFont().MakeBold());
    content_sizer->Add(ic_label, 0, wxBOTTOM, FromDIP(2));
    auto* ic_desc = new wxStaticText(content, wxID_ANY, "Initial conditions for operating-point analysis");
    ic_desc->SetForegroundColour(wxColour("#6B6B66"));
    ic_desc->SetFont(ic_desc->GetFont().Smaller());
    content_sizer->Add(ic_desc, 0, wxBOTTOM, FromDIP(4));
    m_ic_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_ic_text->SetHint("e.g. V(out)=1.0 V(in)=0");
    content_sizer->Add(m_ic_text, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // global settings panel with replace-ground checkbox
    m_global_settings = new GlobalSettingsPanel(content);
    content_sizer->Add(m_global_settings, 0, wxEXPAND, 0);

    // attach content sizer and card to outer layout
    content->SetSizer(content_sizer);
    outer_sizer->Add(m_card, 1, wxEXPAND, 0);
    SetSizer(outer_sizer);
}

OpSimulationParameters OpParametersPanel::build_op_parameters() const {
    // read print parameters from print section
    std::optional<PrintParameters> print_params = m_print_section->build_print_parameters();

    // read save section
    bool save_enabled = m_save_enable_cb->GetValue();
    std::string save_type;
    if (m_save_ic_rb->GetValue()) {
        save_type = "IC";
    }
    else {
        save_type = "NODESET";
    }
    std::string save_file = std::string(m_save_file_text->GetValue().ToUTF8());

    // parse nodeset and IC entries from text controls
    auto nodeset_entries = parse_nodeset_text(m_nodeset_text->GetValue());
    auto ic_entries = parse_ic_text(m_ic_text->GetValue());

    // read replace ground from global settings
    bool replace_ground = m_global_settings->get_replace_ground();

    // legacy DC print fields are left at defaults since print_parameters takes priority
    return OpSimulationParameters(print_params.has_value(), // print_dc_enabled
                                  false, // print_dc_all_nodes (legacy)
                                  false, // print_dc_all_currents (legacy)
                                  {}, // print_dc_specific_variables (legacy)
                                  "", // print_dc_format (legacy)
                                  "", // print_dc_file (legacy)
                                  save_enabled, save_type, save_file, std::move(nodeset_entries), std::move(ic_entries), replace_ground, std::move(print_params));
}

void OpParametersPanel::apply(const OpSimulationParameters& params) {
    // restore print parameters (BJT and FET leads both always relevant for OP)
    m_print_section->apply(params.print_parameters ? &*params.print_parameters : nullptr, true, true);

    // restore save section
    m_save_enable_cb->SetValue(params.save_enabled);
    wxString save_type_upper = wxString::FromUTF8(params.save_type).Upper();
    m_save_ic_rb->SetValue(save_type_upper == "IC");
    m_save_nodeset_rb->SetValue(save_type_upper != "IC");
    m_save_file_text->SetValue(wxString::FromUTF8(params.save_file));

    // restore nodeset and IC entries
    m_nodeset_text->SetValue(format_nodeset_entries(params.nodeset_entries));
    m_ic_text->SetValue(format_ic_entries(params.ic_entries));

    // restore replace ground
    m_global_settings->set_replace_ground(params.replace_ground);
}

GlobalSettingsPanel* OpParametersPanel::get_global_settings() const { return m_global_settings; }

PrintSectionPanel* OpParametersPanel::get_print_section() const { return m_print_section; }
