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
#endif

#include "../../simulation_parameters/transient_simulation_parameters.h"
#include "global_settings_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"
#include "transient_parameters_panel.h"

namespace
{
    // OP keyword display labels mapped to model values (index 0 is the empty/none option)
    static const std::vector<wxString> OP_KEYWORD_LABELS = {"(None)", "NOOP", "UIC"};

    // OP keyword model values corresponding to each display label
    static const std::vector<wxString> OP_KEYWORD_VALUES = {"", "NOOP", "UIC"};

    // resolve the combo index for a given OP keyword string; returns 0 when not found
    [[nodiscard]] int op_keyword_index_for_string(const wxString& keyword) {
        auto it = std::find(OP_KEYWORD_VALUES.begin(), OP_KEYWORD_VALUES.end(), keyword.Upper());
        if (it == OP_KEYWORD_VALUES.end()) {
            return 0;
        }
        return static_cast<int>(std::distance(OP_KEYWORD_VALUES.begin(), it));
    }

    // parse a multi-line schedule text into schedule points;
    // each non-empty line is split on the first comma into (time, max_step)
    [[nodiscard]] std::vector<TransientSchedulePoint> parse_schedule_text(const wxString& text) {
        std::vector<TransientSchedulePoint> points;
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
            if (line.empty()) {
                continue;
            }
            // split on the first comma
            auto comma_pos = line.find(',');
            if (comma_pos == std::string::npos) {
                continue;
            }
            std::string time_part = line.substr(0, comma_pos);
            std::string step_part = line.substr(comma_pos + 1);
            // trim each part
            auto trim = [](std::string& s) {
                size_t b = s.find_first_not_of(" \t\r");
                size_t e = s.find_last_not_of(" \t\r");
                if (b == std::string::npos) {
                    s.clear();
                }
                else {
                    s = s.substr(b, e - b + 1);
                }
            };
            trim(time_part);
            trim(step_part);
            if (!time_part.empty() && !step_part.empty()) {
                points.emplace_back(time_part, step_part);
            }
        }
        return points;
    }

    // format schedule points as multi-line text, one "time, max_step" per line
    [[nodiscard]] wxString format_schedule_text(const std::vector<TransientSchedulePoint>& points) {
        if (points.empty()) {
            return wxEmptyString;
        }
        wxString result;
        for (size_t i = 0; i < points.size(); ++i) {
            if (i > 0) {
                result += "\n";
            }
            result += wxString::FromUTF8(points[i].time_value) + ", " + wxString::FromUTF8(points[i].max_time_step_value);
        }
        return result;
    }
} // namespace

TransientParametersPanel::TransientParametersPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole panel
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // simulation card wrapping all controls
    m_card = new SimulationCard(this, "Transient Analysis");
    auto* content = m_card->get_content();
    auto* content_sizer = new wxBoxSizer(wxVERTICAL);

    // global settings panel with replace-ground checkbox
    m_global_settings = new GlobalSettingsPanel(content);
    content_sizer->Add(m_global_settings, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // transient fields grid: 2 columns (label | control)
    auto* field_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    field_grid->AddGrowableCol(1, 1);

    // initial step row
    auto* initial_step_label = new wxStaticText(content, wxID_ANY, "Initial step");
    field_grid->Add(initial_step_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_initial_step_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_initial_step_text, 0, wxEXPAND, 0);

    // final time row
    auto* final_time_label = new wxStaticText(content, wxID_ANY, "Final time");
    field_grid->Add(final_time_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_final_time_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_final_time_text, 0, wxEXPAND, 0);

    // start time row
    auto* start_time_label = new wxStaticText(content, wxID_ANY, "Start time");
    field_grid->Add(start_time_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_start_time_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_start_time_text, 0, wxEXPAND, 0);

    // step ceiling row
    auto* step_ceiling_label = new wxStaticText(content, wxID_ANY, "Max time step");
    field_grid->Add(step_ceiling_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_ceiling_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    field_grid->Add(m_step_ceiling_text, 0, wxEXPAND, 0);

    // OP keyword row
    auto* op_keyword_label = new wxStaticText(content, wxID_ANY, "OP keyword");
    field_grid->Add(op_keyword_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    wxArrayString op_choices;
    for (const auto& label : OP_KEYWORD_LABELS) {
        op_choices.Add(label);
    }
    m_op_keyword_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, op_choices);
    m_op_keyword_choice->SetSelection(0);
    field_grid->Add(m_op_keyword_choice, 0, wxEXPAND, 0);

    content_sizer->Add(field_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // schedule points multi-line text
    auto* schedule_label = new wxStaticText(content, wxID_ANY, "Schedule points (one per line: time, max_step)");
    content_sizer->Add(schedule_label, 0, wxBOTTOM, FromDIP(4));
    m_schedule_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(300), FromDIP(60)), wxTE_MULTILINE);
    content_sizer->Add(m_schedule_text, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // print section with transient print types and both BJT/FET leads available
    m_print_section = new PrintSectionPanel(content, "TRAN", {"TRAN", "TRANADJOINT"}, true, true, true);
    content_sizer->Add(m_print_section, 0, wxEXPAND, 0);

    // attach content sizer and card to outer layout
    content->SetSizer(content_sizer);
    outer_sizer->Add(m_card, 1, wxEXPAND, 0);
    SetSizer(outer_sizer);
}

TransientSimulationParameters TransientParametersPanel::build_transient_parameters() const {
    // read text fields
    std::string initial_step = std::string(m_initial_step_text->GetValue().ToUTF8());
    std::string final_time = std::string(m_final_time_text->GetValue().ToUTF8());
    std::string start_time = std::string(m_start_time_text->GetValue().ToUTF8());
    std::string step_ceiling = std::string(m_step_ceiling_text->GetValue().ToUTF8());

    // resolve OP keyword from combo selection
    std::string op_keyword;
    int op_sel = m_op_keyword_choice->GetSelection();
    if (op_sel > 0 && op_sel < static_cast<int>(OP_KEYWORD_VALUES.size())) {
        op_keyword = std::string(OP_KEYWORD_VALUES[op_sel].ToUTF8());
    }

    // parse schedule points from multi-line text
    auto schedule_points = parse_schedule_text(m_schedule_text->GetValue());

    // read replace ground from global settings
    bool replace_ground = m_global_settings->get_replace_ground();

    // read print parameters from print section
    std::optional<PrintParameters> print_params = m_print_section->build_print_parameters();

    return TransientSimulationParameters(std::move(initial_step), std::move(final_time), std::move(start_time), std::move(step_ceiling), std::move(op_keyword), std::move(schedule_points), replace_ground, std::move(print_params), {}, // fft_parameters — empty (not managed by this panel)
                                         {}, // four_parameters — empty
                                         {}, // measure_parameters — empty
                                         std::nullopt // sensitivity — not managed
    );
}

void TransientParametersPanel::apply(const TransientSimulationParameters& params) {
    // populate text fields
    m_initial_step_text->SetValue(wxString::FromUTF8(params.initial_step_value));
    m_final_time_text->SetValue(wxString::FromUTF8(params.final_time_value));
    m_start_time_text->SetValue(wxString::FromUTF8(params.start_time_value));
    m_step_ceiling_text->SetValue(wxString::FromUTF8(params.step_ceiling_value));

    // restore OP keyword
    m_op_keyword_choice->SetSelection(op_keyword_index_for_string(wxString::FromUTF8(params.op_keyword)));

    // restore schedule points
    m_schedule_text->SetValue(format_schedule_text(params.schedule_points));

    // restore replace ground
    m_global_settings->set_replace_ground(params.replace_ground);

    // restore print parameters (BJT and FET leads both always relevant for TRAN)
    m_print_section->apply(params.print_parameters ? &*params.print_parameters : nullptr, true, true);
}

GlobalSettingsPanel* TransientParametersPanel::get_global_settings() const { return m_global_settings; }

PrintSectionPanel* TransientParametersPanel::get_print_section() const { return m_print_section; }
