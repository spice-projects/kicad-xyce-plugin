#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/arrstr.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../wx_util.h"
#include "global_settings_panel.h"
#include "lin_parameters_panel.h"
#include "print_section_panel.h"
#include "simulation_card.h"
#include "simulation_parameters/lin_simulation_parameters.h"

namespace
{
    // available format choices for touchstone output
    static const std::vector<wxString> FORMAT_CHOICES = {"TOUCHSTONE2", "TOUCHSTONE1", "CITIFILE", "TSI"};
    // available lintype choices for s-parameter type
    static const std::vector<wxString> LINTYPE_CHOICES = {"S", "Y", "Z", "G", "H"};
    // available data format choices
    static const std::vector<wxString> DATAFORMAT_CHOICES = {"RI", "MA", "DB"};
    // available sweep choices
    static const std::vector<wxString> SWEEP_CHOICES = {"LIN", "DEC", "OCT", "DATA"};
} // namespace

LinParametersPanel::LinParametersPanel(wxWindow* parent) :
    wxPanel(parent) {
    // outer vertical sizer for the whole panel
    auto* outer_sizer = new wxBoxSizer(wxVERTICAL);

    // simulation card wrapping all controls
    m_card = new SimulationCard(this, "Linear Analysis (.LIN)");
    auto* content = m_card->get_content();
    auto* content_sizer = new wxBoxSizer(wxVERTICAL);

    // print section with AC print type and both BJT/FET leads available
    m_print_section = new PrintSectionPanel(content, "AC", {"AC"}, true, true, true);
    content_sizer->Add(m_print_section, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- s-parameter options ---
    auto* lin_label = new wxStaticText(content, wxID_ANY, "S-parameter options");
    wxFont lin_font = lin_label->GetFont();
    lin_font.SetWeight(wxFONTWEIGHT_BOLD);
    lin_label->SetFont(lin_font);
    content_sizer->Add(lin_label, 0, wxBOTTOM, FromDIP(4));

    // field grid for s-parameter options: 2 columns (label | control)
    auto* lin_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    lin_grid->AddGrowableCol(1, 1);

    // SPARCALC checkbox row
    auto* sparcalc_label = new wxStaticText(content, wxID_ANY, "SPARCALC");
    lin_grid->Add(sparcalc_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_sparcalc_checkbox = new wxCheckBox(content, wxID_ANY, "Enable SPARCALC");
    m_sparcalc_checkbox->SetValue(true);
    lin_grid->Add(m_sparcalc_checkbox, 0, wxEXPAND, 0);

    // output format choice row
    auto* format_label = new wxStaticText(content, wxID_ANY, "Format");
    lin_grid->Add(format_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_format_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, wx_util::to_wx_array_string(FORMAT_CHOICES));
    m_format_choice->SetSelection(0);
    lin_grid->Add(m_format_choice, 0, wxEXPAND, 0);

    // s-parameter type choice row
    auto* lintype_label = new wxStaticText(content, wxID_ANY, "Type");
    lin_grid->Add(lintype_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_lintype_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, wx_util::to_wx_array_string(LINTYPE_CHOICES));
    m_lintype_choice->SetSelection(0);
    lin_grid->Add(m_lintype_choice, 0, wxEXPAND, 0);

    // data format choice row
    auto* dataformat_label = new wxStaticText(content, wxID_ANY, "Data format");
    lin_grid->Add(dataformat_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_dataformat_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, wx_util::to_wx_array_string(DATAFORMAT_CHOICES));
    m_dataformat_choice->SetSelection(0);
    lin_grid->Add(m_dataformat_choice, 0, wxEXPAND, 0);

    // output file path row
    auto* file_label = new wxStaticText(content, wxID_ANY, "Output file");
    lin_grid->Add(file_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_file_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    lin_grid->Add(m_file_text, 0, wxEXPAND, 0);

    // width in characters row
    auto* width_label = new wxStaticText(content, wxID_ANY, "Width");
    lin_grid->Add(width_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_width_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    lin_grid->Add(m_width_text, 0, wxEXPAND, 0);

    // decimal precision row
    auto* precision_label = new wxStaticText(content, wxID_ANY, "Precision");
    lin_grid->Add(precision_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_precision_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    lin_grid->Add(m_precision_text, 0, wxEXPAND, 0);

    content_sizer->Add(lin_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // --- ac sweep ---
    auto* ac_label = new wxStaticText(content, wxID_ANY, "AC Sweep");
    wxFont ac_font = ac_label->GetFont();
    ac_font.SetWeight(wxFONTWEIGHT_BOLD);
    ac_label->SetFont(ac_font);
    content_sizer->Add(ac_label, 0, wxBOTTOM, FromDIP(4));

    // field grid for AC sweep: 2 columns (label | control)
    auto* ac_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
    ac_grid->AddGrowableCol(1, 1);

    // sweep mode choice row
    auto* sweep_mode_label = new wxStaticText(content, wxID_ANY, "Sweep mode");
    ac_grid->Add(sweep_mode_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_sweep_mode_choice = new wxChoice(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, wx_util::to_wx_array_string(SWEEP_CHOICES));
    m_sweep_mode_choice->SetSelection(0);
    ac_grid->Add(m_sweep_mode_choice, 0, wxEXPAND, 0);

    // number of points row
    auto* points_label = new wxStaticText(content, wxID_ANY, "Points");
    ac_grid->Add(points_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_points_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    ac_grid->Add(m_points_text, 0, wxEXPAND, 0);

    // start frequency row
    auto* start_label = new wxStaticText(content, wxID_ANY, "Start");
    ac_grid->Add(start_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_start_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    ac_grid->Add(m_start_text, 0, wxEXPAND, 0);

    // end frequency row
    auto* end_label = new wxStaticText(content, wxID_ANY, "End");
    ac_grid->Add(end_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_end_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    ac_grid->Add(m_end_text, 0, wxEXPAND, 0);

    // data table name row
    auto* data_table_label = new wxStaticText(content, wxID_ANY, "Data table");
    ac_grid->Add(data_table_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_data_table_text = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    ac_grid->Add(m_data_table_text, 0, wxEXPAND, 0);

    content_sizer->Add(ac_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    // global settings panel with replace-ground checkbox
    m_global_settings = new GlobalSettingsPanel(content);
    content_sizer->Add(m_global_settings, 0, wxEXPAND, 0);

    // attach content sizer and card to outer layout
    content->SetSizer(content_sizer);
    outer_sizer->Add(m_card, 1, wxEXPAND, 0);
    SetSizer(outer_sizer);
}

LinSimulationParameters LinParametersPanel::build_lin_parameters() const {
    // read s-parameter options
    bool sparcalc = m_sparcalc_checkbox->GetValue();
    std::string format = wx_util::get_string_selection(*m_format_choice);
    std::string lintype = wx_util::get_string_selection(*m_lintype_choice);
    std::string dataformat = wx_util::get_string_selection(*m_dataformat_choice);
    std::string file = wx_util::get_text(*m_file_text);
    std::string width = wx_util::get_text(*m_width_text);
    std::string precision = wx_util::get_text(*m_precision_text);

    // read AC sweep parameters
    std::string sweep_mode = wx_util::get_string_selection(*m_sweep_mode_choice);
    std::string points = wx_util::get_text(*m_points_text);
    std::string start = wx_util::get_text(*m_start_text);
    std::string end = wx_util::get_text(*m_end_text);
    std::string data_table_name = wx_util::get_text(*m_data_table_text);

    // read print parameters from print section
    std::optional<PrintParameters> print_params = m_print_section->build_print_parameters();

    return LinSimulationParameters(sparcalc, std::move(format), std::move(lintype), std::move(dataformat), std::move(file), std::move(width), std::move(precision), std::move(sweep_mode), std::move(points), std::move(start), std::move(end), std::move(data_table_name), std::move(print_params));
}

void LinParametersPanel::apply(const LinSimulationParameters& params) {
    // restore SPARCALC checkbox
    m_sparcalc_checkbox->SetValue(params.sparcalc);

    // restore format choice, fall back to first if not found
    wx_util::set_choice_by_string(*m_format_choice, params.format);

    // restore lintype choice, fall back to first if not found
    wx_util::set_choice_by_string(*m_lintype_choice, params.lintype);

    // restore dataformat choice, fall back to first if not found
    wx_util::set_choice_by_string(*m_dataformat_choice, params.dataformat);

    // restore s-parameter text fields
    wx_util::set_text(*m_file_text, params.file);
    wx_util::set_text(*m_width_text, params.width);
    wx_util::set_text(*m_precision_text, params.precision);

    // restore sweep mode choice, fall back to first if not found
    wx_util::set_choice_by_string(*m_sweep_mode_choice, params.sweep_mode);

    // restore AC sweep text fields
    wx_util::set_text(*m_points_text, params.points);
    wx_util::set_text(*m_start_text, params.start);
    wx_util::set_text(*m_end_text, params.end);
    wx_util::set_text(*m_data_table_text, params.data_table_name);

    // restore print parameters (BJT and FET leads both always relevant for LIN)
    m_print_section->apply(params.print_parameters ? &*params.print_parameters : nullptr, true, true);
}

GlobalSettingsPanel* LinParametersPanel::get_global_settings() const { return m_global_settings; }

PrintSectionPanel* LinParametersPanel::get_print_section() const { return m_print_section; }
