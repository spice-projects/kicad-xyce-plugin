#include <map>
#include <sstream>
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

#include "../../simulation_parameters/hb_simulation_parameters.h"
#include "global_settings_panel.h"
#include "hb_parameters_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"

namespace
{
    // parse newline-separated key=value options into map
    [[nodiscard]] std::map<std::string, std::string> parse_options_text(const wxString& text) {
        std::map<std::string, std::string> options;
        wxStringTokenizer tokenizer(text, "\n");
        while (tokenizer.HasMoreTokens()) {
            wxString line = tokenizer.GetNextToken().Trim(true).Trim(false);
            // skip empty lines
            if (line.IsEmpty()) {
                continue;
            }
            int eq_pos = line.Find('=');
            // skip lines without key=value separator
            if (eq_pos == wxNOT_FOUND) {
                continue;
            }
            wxString key = line.Mid(0, eq_pos).Trim(true).Trim(false).Upper();
            wxString value = line.Mid(eq_pos + 1).Trim(true).Trim(false);
            // store key-value pair if key is non-empty
            if (!key.IsEmpty()) {
                options[std::string(key.ToUTF8())] = std::string(value.ToUTF8());
            }
        }
        return options;
    }

    // format key=value options map as newline-separated text
    [[nodiscard]] wxString format_options_text(const std::map<std::string, std::string>& options) {
        if (options.empty()) {
            return wxEmptyString;
        }
        wxString result;
        for (auto it = options.begin(); it != options.end(); ++it) {
            // add newline separator between entries
            if (it != options.begin()) {
                result += "\n";
            }
            result += wxString::FromUTF8(it->first) + "=" + wxString::FromUTF8(it->second);
        }
        return result;
    }
} // namespace

HbParametersPanel::HbParametersPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole panel
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // simulation card wrapping all controls
    m_card = new SimulationCard(this, "Harmonic Balance (.HB)");
    auto* content = m_card->get_content();
    auto* content_sizer = new wxBoxSizer(wxVERTICAL);

    // print section with HB print types and both BJT/FET leads available
    m_print_section = new PrintSectionPanel(content, "HB", {"HB", "HB_FD", "HB_TD"}, true, true, true);
    content_sizer->Add(m_print_section, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- frequency configuration ---
    // field grid for HB parameters: 2 columns (label | control)
    auto* field_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    field_grid->AddGrowableCol(1, 1);

    // fundamental frequencies row
    auto* freq_label = new wxStaticText(content, wxID_ANY, "Frequencies");
    field_grid->Add(freq_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_frequencies_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_frequencies_text->SetHint("space-separated fundamental frequencies");
    field_grid->Add(m_frequencies_text, 0, wxEXPAND, 0);

    // harmonics / NUMFREQ row
    auto* harms_label = new wxStaticText(content, wxID_ANY, "Harmonics / NUMFREQ");
    field_grid->Add(harms_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_harmonics_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_harmonics_text->SetHint("comma-separated integers");
    field_grid->Add(m_harmonics_text, 0, wxEXPAND, 0);

    // TAHB (transient analysis horizon) row
    auto* tahb_label = new wxStaticText(content, wxID_ANY, "TAHB");
    field_grid->Add(tahb_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_tahb_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_tahb_text->SetHint("transient analysis horizon");
    field_grid->Add(m_tahb_text, 0, wxEXPAND, 0);

    // SELECTHARMS row
    auto* selharms_label = new wxStaticText(content, wxID_ANY, "SELECTHARMS");
    field_grid->Add(selharms_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_selectharms_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_selectharms_text->SetHint("selective harmonics");
    field_grid->Add(m_selectharms_text, 0, wxEXPAND, 0);

    // startup periods row
    auto* startup_label = new wxStaticText(content, wxID_ANY, "Startup periods");
    field_grid->Add(startup_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_startup_periods_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_startup_periods_text->SetHint("startup periods");
    field_grid->Add(m_startup_periods_text, 0, wxEXPAND, 0);

    content_sizer->Add(field_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- solver options ---
    // nonlinear solver options label and multiline text
    auto* nonlin_label = new wxStaticText(content, wxID_ANY, "Nonlinear solver options (one key=value per line)");
    content_sizer->Add(nonlin_label, 0, wxBOTTOM, FromDIP(4));
    m_nonlin_options_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(300), FromDIP(60)), wxTE_MULTILINE);
    content_sizer->Add(m_nonlin_options_text, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // linear solver options label and multiline text
    auto* linsol_label = new wxStaticText(content, wxID_ANY, "Linear solver options (one key=value per line)");
    content_sizer->Add(linsol_label, 0, wxBOTTOM, FromDIP(4));
    m_linsol_options_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(300), FromDIP(60)), wxTE_MULTILINE);
    content_sizer->Add(m_linsol_options_text, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // global settings panel with replace-ground checkbox
    m_global_settings = new GlobalSettingsPanel(content);
    content_sizer->Add(m_global_settings, 0, wxEXPAND, 0);

    // attach content sizer and card to outer layout
    content->SetSizer(content_sizer);
    outer_sizer->Add(m_card, 1, wxEXPAND, 0);
    SetSizer(outer_sizer);
}

HbSimulationParameters HbParametersPanel::build_hb_parameters() const {
    // read and parse frequencies
    std::vector<std::string> frequencies;
    wxString freq_text = m_frequencies_text->GetValue().Trim(true).Trim(false);
    // split on whitespace when text is non-empty
    if (!freq_text.IsEmpty()) {
        wxStringTokenizer tokenizer(freq_text, " \t\r\n");
        while (tokenizer.HasMoreTokens()) {
            frequencies.push_back(std::string(tokenizer.GetNextToken().ToUTF8()));
        }
    }

    // read and parse harmonics as comma-separated integers
    std::vector<int> harmonics;
    wxString harms_text = m_harmonics_text->GetValue().Trim(true).Trim(false);
    if (!harms_text.IsEmpty()) {
        wxArrayString parts = wxStringTokenize(harms_text, ",");
        for (const auto& part : parts) {
            wxString trimmed = wxString(part).Trim(true).Trim(false);
            // convert non-empty tokens to integer
            if (!trimmed.IsEmpty()) {
                long val;
                if (trimmed.ToLong(&val)) {
                    harmonics.push_back(static_cast<int>(val));
                }
            }
        }
    }

    // read optional TAHB value
    std::optional<int> tahb;
    wxString tahb_val = m_tahb_text->GetValue().Trim(true).Trim(false);
    if (!tahb_val.IsEmpty()) {
        long val;
        if (tahb_val.ToLong(&val)) {
            tahb = static_cast<int>(val);
        }
    }

    // read optional SELECTHARMS value
    std::optional<std::string> selectharms;
    wxString sel_val = m_selectharms_text->GetValue().Trim(true).Trim(false);
    if (!sel_val.IsEmpty()) {
        selectharms = std::string(sel_val.ToUTF8());
    }

    // read optional startup periods value
    std::optional<int> startup_periods;
    wxString startup_val = m_startup_periods_text->GetValue().Trim(true).Trim(false);
    if (!startup_val.IsEmpty()) {
        long val;
        if (startup_val.ToLong(&val)) {
            startup_periods = static_cast<int>(val);
        }
    }

    // read print parameters from print section
    std::optional<PrintParameters> print_params = m_print_section->build_print_parameters();

    // read replace ground from global settings
    bool replace_ground = m_global_settings->get_replace_ground();

    // read and parse solver options
    auto nonlin_options = parse_options_text(m_nonlin_options_text->GetValue());
    auto linsol_options = parse_options_text(m_linsol_options_text->GetValue());

    return HbSimulationParameters(std::move(frequencies), std::move(harmonics), tahb, std::move(selectharms), startup_periods, replace_ground, std::move(print_params), std::move(nonlin_options), std::move(linsol_options));
}

void HbParametersPanel::apply(const HbSimulationParameters& params) {
    // restore frequencies as space-separated string
    wxString freq_text;
    for (size_t i = 0; i < params.frequencies.size(); ++i) {
        // add space separator between entries
        if (i > 0) {
            freq_text += " ";
        }
        freq_text += wxString::FromUTF8(params.frequencies[i]);
    }
    m_frequencies_text->SetValue(freq_text);

    // restore harmonics as comma-separated string
    wxString harms_text;
    for (size_t i = 0; i < params.harmonics.size(); ++i) {
        // add comma separator between entries
        if (i > 0) {
            harms_text += ",";
        }
        harms_text += wxString::Format("%d", params.harmonics[i]);
    }
    m_harmonics_text->SetValue(harms_text);

    // restore TAHB or clear if not set
    if (params.tahb.has_value()) {
        m_tahb_text->SetValue(wxString::Format("%d", *params.tahb));
    }
    else {
        m_tahb_text->SetValue(wxEmptyString);
    }

    // restore SELECTHARMS or clear if not set
    if (params.selectharms.has_value()) {
        m_selectharms_text->SetValue(wxString::FromUTF8(*params.selectharms));
    }
    else {
        m_selectharms_text->SetValue(wxEmptyString);
    }

    // restore startup periods or clear if not set
    if (params.startup_periods.has_value()) {
        m_startup_periods_text->SetValue(wxString::Format("%d", *params.startup_periods));
    }
    else {
        m_startup_periods_text->SetValue(wxEmptyString);
    }

    // restore solver options
    m_nonlin_options_text->SetValue(format_options_text(params.nonlin_options));
    m_linsol_options_text->SetValue(format_options_text(params.linsol_options));

    // restore print parameters (BJT and FET leads both always relevant for HB)
    m_print_section->apply(params.print_parameters ? &*params.print_parameters : nullptr, true, true);

    // restore replace ground
    m_global_settings->set_replace_ground(params.replace_ground);
}

GlobalSettingsPanel* HbParametersPanel::get_global_settings() const { return m_global_settings; }

PrintSectionPanel* HbParametersPanel::get_print_section() const { return m_print_section; }
