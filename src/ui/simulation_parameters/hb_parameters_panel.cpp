#include <map>
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

#include "../wx_util.h"
#include "global_settings_panel.h"
#include "hb_parameters_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"
#include "simulation_parameters/hb_simulation_parameters.h"

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
} // namespace

HbParametersPanel::HbParametersPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole panel
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // simulation card wrapping all controls
    m_card = new SimulationCard(this, "Harmonic Balance (.HB)");
    auto* content = m_card->get_content();
    auto* content_sizer = new wxBoxSizer(wxVERTICAL);

    // print section with HB print types only
    m_print_section = new PrintSectionPanel(content, "HB", {"HB", "HB_FD", "HB_TD"}, false, false, true);
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
    m_tahb_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, wx_util::to_wx_array_string({"(None)", "0 (off)", "1 (auto)", "2", "5", "10", "20"}));
    m_tahb_choice->SetSelection(0);
    field_grid->Add(m_tahb_choice, 0, wxEXPAND, 0);

    // SELECTHARMS row
    auto* selharms_label = new wxStaticText(content, wxID_ANY, "SELECTHARMS");
    field_grid->Add(selharms_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_selectharms_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, wx_util::to_wx_array_string({"(None)", "ALL", "1", "2", "3", "5", "10"}));
    m_selectharms_choice->SetSelection(0);
    field_grid->Add(m_selectharms_choice, 0, wxEXPAND, 0);

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
    // read and parse frequencies as space-separated values
    std::vector<std::string> frequencies = wx_util::split_strings(m_frequencies_text->GetValue());

    // read and parse harmonics as comma-separated integers
    std::vector<int> harmonics;
    for (const auto& part : wx_util::split_strings(m_harmonics_text->GetValue(), ",")) {
        long val;
        if (wx_util::to_wx_string(part).ToLong(&val)) {
            harmonics.push_back(static_cast<int>(val));
        }
    }

    // read optional TAHB value from choice
    std::optional<int> tahb;
    {
        static const std::vector<std::optional<int>> TAHB_VALUES = {std::nullopt, 0, 1, 2, 5, 10, 20};
        int sel = m_tahb_choice->GetSelection();
        if (sel != wxNOT_FOUND && sel < static_cast<int>(TAHB_VALUES.size())) {
            tahb = TAHB_VALUES[sel];
        }
    }

    // read optional SELECTHARMS value from choice
    std::optional<std::string> selectharms;
    {
        static const std::vector<std::optional<std::string>> SELHARMS_VALUES = {std::nullopt, std::string("ALL"), std::string("1"), std::string("2"), std::string("3"), std::string("5"), std::string("10")};
        int sel = m_selectharms_choice->GetSelection();
        if (sel != wxNOT_FOUND && sel < static_cast<int>(SELHARMS_VALUES.size())) {
            selectharms = SELHARMS_VALUES[sel];
        }
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

    // read and parse solver options
    auto nonlin_options = parse_options_text(m_nonlin_options_text->GetValue());
    auto linsol_options = parse_options_text(m_linsol_options_text->GetValue());

    return HbSimulationParameters(std::move(frequencies), std::move(harmonics), tahb, std::move(selectharms), startup_periods, std::move(print_params), std::move(nonlin_options), std::move(linsol_options));
}

void HbParametersPanel::apply(const HbSimulationParameters& params) {
    // restore frequencies as space-separated string
    m_frequencies_text->SetValue(wx_util::join_strings(params.frequencies, " "));

    // restore harmonics as comma-separated string
    m_harmonics_text->SetValue(wx_util::join_strings(params.harmonics, ",", [](int harm) { return std::to_string(harm); }));

    // restore TAHB from choice
    if (params.tahb.has_value()) {
        int tahb_val = *params.tahb;
        int tahb_index = 0;
        if (tahb_val == 0)
            tahb_index = 1;
        else if (tahb_val == 1)
            tahb_index = 2;
        else if (tahb_val == 2)
            tahb_index = 3;
        else if (tahb_val == 5)
            tahb_index = 4;
        else if (tahb_val == 10)
            tahb_index = 5;
        else if (tahb_val == 20)
            tahb_index = 6;
        m_tahb_choice->SetSelection(tahb_index);
    }
    else {
        m_tahb_choice->SetSelection(0);
    }

    // restore SELECTHARMS from choice
    if (params.selectharms.has_value()) {
        wx_util::set_choice_by_string(*m_selectharms_choice, *params.selectharms);
    }
    else {
        m_selectharms_choice->SetSelection(0);
    }

    // restore startup periods or clear if not set
    if (params.startup_periods.has_value()) {
        m_startup_periods_text->SetValue(wxString::Format("%d", *params.startup_periods));
    }
    else {
        m_startup_periods_text->SetValue(wxEmptyString);
    }

    // restore solver options
    auto format_options = [](const auto& entry) { return entry.first + "=" + entry.second; };
    m_nonlin_options_text->SetValue(wx_util::join_strings(params.nonlin_options, "\n", format_options));
    m_linsol_options_text->SetValue(wx_util::join_strings(params.linsol_options, "\n", format_options));

    // restore print parameters (no power, no BJT/FET for HB)
    m_print_section->apply(params.print_parameters ? &*params.print_parameters : nullptr, false, false);
}

GlobalSettingsPanel* HbParametersPanel::get_global_settings() const { return m_global_settings; }

PrintSectionPanel* HbParametersPanel::get_print_section() const { return m_print_section; }
