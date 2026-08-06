#include <algorithm>
#include <cmath>
#include <set>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/msgdlg.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include <spdlog/spdlog.h>

#include "fft_dialog.h"

namespace
{
    struct MaxFrequencyOption
    {
        const char* label;
        double value;
    };

    const std::vector<MaxFrequencyOption> MAX_FREQUENCY_OPTIONS = {{"100 Hz", 100.0}, {"1 kHz", 1e3}, {"10 kHz", 1e4}, {"100 kHz", 1e5}, {"1 MHz", 1e6}, {"10 MHz", 1e7}, {"100 MHz", 1e8}, {"1 GHz", 1e9}, {"10 GHz", 1e10}, {"100 GHz", 1e11}};

    const int CUSTOM_FREQUENCY_INDEX = static_cast<int>(MAX_FREQUENCY_OPTIONS.size());

    int find_closest_frequency_index(double target) {
        // search for the option closest to the target
        return static_cast<int>(std::distance(MAX_FREQUENCY_OPTIONS.begin(), std::min_element(MAX_FREQUENCY_OPTIONS.begin(), MAX_FREQUENCY_OPTIONS.end(), [target](const MaxFrequencyOption& a, const MaxFrequencyOption& b) { return std::abs(a.value - target) < std::abs(b.value - target); })));
    }
} // namespace

FftDialog::FftDialog(wxWindow* parent, ExpressionManager* expressions_manager, std::vector<AnyExpression*> selected_expressions, double min_abscissa_value, double max_abscissa_value, double min_abscissa_value_zoomed, double max_abscissa_value_zoomed, double default_max_frequency) :
    wxDialog(parent, wxID_ANY, "FFT", wxDefaultPosition, FromDIP(wxSize(600, 550)), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), m_min_abscissa_value(min_abscissa_value), m_max_abscissa_value(max_abscissa_value), m_min_abscissa_value_zoomed(min_abscissa_value_zoomed), m_max_abscissa_value_zoomed(max_abscissa_value_zoomed), m_from_index(min_abscissa_value_zoomed), m_to_index(max_abscissa_value_zoomed), m_max_frequency(default_max_frequency > 0.0 ? default_max_frequency : 1e5) {
    // min size
    SetMinSize(FromDIP(wxSize(600, 550)));

    // create main vertical sizer
    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);

    // create title label
    auto title_text = new wxStaticText(this, wxID_ANY, "Select expressions and FFT parameters:");
    wxFont title_font = title_text->GetFont();
    title_font.SetPointSize(12);
    title_text->SetFont(title_font);
    main_sizer->Add(title_text, 0, wxTOP | wxLEFT | wxRIGHT, FromDIP(8));

    // FFT is only for real expressions, so we filter out non-real expressions and time-domain expressions (unit "s")
    auto skip_time_domain_expressions = [](const AnyExpression* expr) {
        // skip non-real expressions
        if (!std::holds_alternative<Expression<double>>(*expr))
            return false;
        // unit
        const auto& unit = std::get<Expression<double>>(*expr).unit();
        // skip time domain expressions
        return unit != "s";
    };

    // create expression selector panel (pre-selects all expressions like Python)
    ExpressionSelectorConfig config{.show_filter = true, .show_custom_input = false, .show_legend = true, .allow_empty_selection = false, .filter_hint = "Filter expressions...", .custom_hint = ""};
    m_expression_selector = new ExpressionSelectorPanel(this, expressions_manager, selected_expressions, config, skip_time_domain_expressions);
    main_sizer->Add(m_expression_selector, 1, wxEXPAND);

    // data range section
    auto range_label = new wxStaticText(this, wxID_ANY, "Data Range:");
    wxFont label_font = range_label->GetFont();
    label_font.SetPointSize(10);
    label_font.MakeBold();
    range_label->SetFont(label_font);
    main_sizer->Add(range_label, 0, wxTOP | wxLEFT | wxRIGHT, FromDIP(8));

    // create horizontal sizer for range radio buttons and custom inputs
    auto range_sizer = new wxBoxSizer(wxHORIZONTAL);

    // range mode radio buttons
    m_range_mode_all = new wxRadioButton(this, wxID_ANY, "All", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_range_mode_zoom = new wxRadioButton(this, wxID_ANY, "Current Zoom");
    m_range_mode_custom = new wxRadioButton(this, wxID_ANY, "Custom");

    // add radio buttons to sizer
    range_sizer->Add(m_range_mode_all, 0, wxRIGHT, FromDIP(12));
    range_sizer->Add(m_range_mode_zoom, 0, wxRIGHT, FromDIP(12));
    range_sizer->Add(m_range_mode_custom, 0, wxRIGHT, FromDIP(12));

    // custom range input fields
    m_custom_from_input = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(80, -1)));
    m_custom_from_input->SetHint("from");
    m_custom_to_input = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(80, -1)));
    m_custom_to_input->SetHint("to");

    // add custom range fields to sizer
    range_sizer->Add(new wxStaticText(this, wxID_ANY, "From:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
    range_sizer->Add(m_custom_from_input, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
    range_sizer->Add(new wxStaticText(this, wxID_ANY, "To:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
    range_sizer->Add(m_custom_to_input, 0, wxALIGN_CENTER_VERTICAL);

    main_sizer->Add(range_sizer, 0, wxLEFT | wxRIGHT, FromDIP(16));

    // default to all range
    m_range_mode_all->SetValue(true);
    m_custom_from_input->Enable(false);
    m_custom_to_input->Enable(false);

    // bind radio button events
    m_range_mode_all->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { on_range_changed(); });
    m_range_mode_zoom->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { on_range_changed(); });
    m_range_mode_custom->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) {
        on_range_changed();
        m_custom_from_input->Enable(true);
        m_custom_to_input->Enable(true);
    });

    // window function section
    auto window_label = new wxStaticText(this, wxID_ANY, "Window Function:");
    window_label->SetFont(label_font);
    main_sizer->Add(window_label, 0, wxTOP | wxLEFT | wxRIGHT, FromDIP(8));

    // create horizontal sizer for window function and normalize
    auto window_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_window_choice = new wxChoice(this, wxID_ANY);
    m_window_choice->Append("Rectangular");
    m_window_choice->Append("Hamming");
    m_window_choice->Append("Hanning");
    m_window_choice->Append("Blackman");
    // default to Hanning
    m_window_choice->SetSelection(2);
    window_sizer->Add(m_window_choice, 0, wxRIGHT, FromDIP(12));

    // normalize checkbox
    auto normalize_label = new wxStaticText(this, wxID_ANY, "Normalize:");
    window_sizer->Add(normalize_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
    m_normalize_checkbox = new wxCheckBox(this, wxID_ANY, "");
    m_normalize_checkbox->SetValue(true);
    window_sizer->Add(m_normalize_checkbox, 0, wxALIGN_CENTER_VERTICAL);

    main_sizer->Add(window_sizer, 0, wxLEFT | wxRIGHT, FromDIP(16));

    // output section
    auto output_label = new wxStaticText(this, wxID_ANY, "Output:");
    output_label->SetFont(label_font);
    main_sizer->Add(output_label, 0, wxTOP | wxLEFT | wxRIGHT, FromDIP(8));

    // create horizontal sizer for output type and keep dc
    auto output_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_output_choice = new wxChoice(this, wxID_ANY);
    m_output_choice->Append("Magnitude");
    m_output_choice->Append("Magnitude (dB)");
    m_output_choice->Append("Phase");
    // default to Magnitude
    m_output_choice->SetSelection(0);
    output_sizer->Add(m_output_choice, 0, wxRIGHT, FromDIP(12));

    // keep dc checkbox
    auto keep_dc_label = new wxStaticText(this, wxID_ANY, "Keep DC:");
    output_sizer->Add(keep_dc_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
    m_keep_dc_checkbox = new wxCheckBox(this, wxID_ANY, "");
    m_keep_dc_checkbox->SetValue(false);
    output_sizer->Add(m_keep_dc_checkbox, 0, wxALIGN_CENTER_VERTICAL);

    main_sizer->Add(output_sizer, 0, wxLEFT | wxRIGHT, FromDIP(16));

    // max frequency section
    auto freq_label = new wxStaticText(this, wxID_ANY, "Maximum Frequency:");
    freq_label->SetFont(label_font);
    main_sizer->Add(freq_label, 0, wxTOP | wxLEFT | wxRIGHT, FromDIP(8));

    // create horizontal sizer for frequency choice and custom input
    auto freq_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_frequency_choice = new wxChoice(this, wxID_ANY);
    for (const auto& opt : MAX_FREQUENCY_OPTIONS)
        m_frequency_choice->Append(opt.label);
    m_frequency_choice->Append("Custom...");
    // set selection closest to default max frequency
    m_frequency_choice->SetSelection(find_closest_frequency_index(m_max_frequency));
    freq_sizer->Add(m_frequency_choice, 0, wxRIGHT, FromDIP(8));

    // custom frequency input
    m_custom_frequency_input = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(80, -1)));
    m_custom_frequency_input->SetHint("e.g. 50000");
    m_custom_frequency_input->Enable(false);
    freq_sizer->Add(m_custom_frequency_input, 0, wxALIGN_CENTER_VERTICAL);

    main_sizer->Add(freq_sizer, 0, wxLEFT | wxRIGHT, FromDIP(16));

    // bind frequency choice event to toggle custom input
    m_frequency_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
        bool is_custom = (m_frequency_choice->GetSelection() == CUSTOM_FREQUENCY_INDEX);
        m_custom_frequency_input->Enable(is_custom);
    });

    // create standard OK/Cancel button sizer using platform conventions
    main_sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, FromDIP(12));

    // event handlers
    Bind(wxEVT_BUTTON, &FftDialog::on_ok, this, wxID_OK);
}

std::vector<AnyExpression*> FftDialog::selected_expressions() const {
    // selected expressions
    auto expressions = m_expression_selector->selected_expressions();
    // return a vector, deterministic order
    return std::vector<AnyExpression*>(expressions.begin(), expressions.end());
}

void FftDialog::on_range_changed() {
    bool custom = m_range_mode_custom->GetValue();
    // update from/to index based on selected range mode
    if (m_range_mode_all->GetValue()) {
        m_from_index = m_min_abscissa_value;
        m_to_index = m_max_abscissa_value;
    }
    else if (m_range_mode_zoom->GetValue()) {
        m_from_index = m_min_abscissa_value_zoomed;
        m_to_index = m_max_abscissa_value_zoomed;
    }
    // update custom input enabled state
    m_custom_from_input->Enable(custom);
    m_custom_to_input->Enable(custom);
}

void FftDialog::on_ok(wxCommandEvent& event) {
    // validate expression selection
    if (!m_expression_selector->validate_selection()) {
        event.Skip(false);
        return;
    }

    // handle custom range input if selected
    if (m_range_mode_custom->GetValue()) {
        auto from_str = m_custom_from_input->GetValue().ToStdString();
        auto to_str = m_custom_to_input->GetValue().ToStdString();
        try {
            m_from_index = std::stod(from_str);
            m_to_index = std::stod(to_str);
        }
        catch (...) {
            // show error message for invalid input
            wxMessageBox("Invalid custom range values.", "Error", wxOK | wxICON_ERROR, this);
            // skip event to prevent dialog from closing
            event.Skip(false);
            // exit
            return;
        }
        // validate range order
        if (m_from_index >= m_to_index) {
            // show error message for invalid range
            wxMessageBox("Custom range 'from' must be less than 'to'.", "Error", wxOK | wxICON_ERROR, this);
            // skip event to prevent dialog from closing
            event.Skip(false);
            // exit
            return;
        }
    }
    else {
        // update from/to based on range mode
        on_range_changed();
    }

    // resolve window function selection
    int window_selection = m_window_choice->GetSelection();
    switch (window_selection) {
    case 0:
        m_window_function = fft::WindowFunction::RECTANGULAR;
        break;
    case 1:
        m_window_function = fft::WindowFunction::HAMMING;
        break;
    case 2:
        m_window_function = fft::WindowFunction::HANNING;
        break;
    case 3:
        m_window_function = fft::WindowFunction::BLACKMAN;
        break;
    default:
        m_window_function = fft::WindowFunction::HANNING;
        break;
    }

    // resolve output type selection
    int output_selection = m_output_choice->GetSelection();
    switch (output_selection) {
    case 0:
        m_output = fft::FftOutput::MAGNITUDE;
        break;
    case 1:
        m_output = fft::FftOutput::MAGNITUDE_DB;
        break;
    case 2:
        m_output = fft::FftOutput::PHASE;
        break;
    default:
        m_output = fft::FftOutput::MAGNITUDE;
        break;
    }

    // read checkbox values
    m_normalize = m_normalize_checkbox->GetValue();
    m_keep_dc = m_keep_dc_checkbox->GetValue();

    // handle custom frequency input if selected
    if (m_frequency_choice->GetSelection() == CUSTOM_FREQUENCY_INDEX) {
        // validate custom frequency input
        if (m_custom_frequency_input->GetValue().IsEmpty()) {
            // show error message for empty custom frequency input
            wxMessageBox("Please enter a custom frequency value.", "Error", wxOK | wxICON_ERROR, this);
            // skip event to prevent dialog from closing
            event.Skip(false);
            // exit
            return;
        }
        try {
            // parse custom frequency input
            m_max_frequency = std::stod(m_custom_frequency_input->GetValue().ToStdString());
        }
        catch (...) {
            // show error message for invalid frequency input
            wxMessageBox("Invalid frequency value.", "Error", wxOK | wxICON_ERROR, this);
            // skip event to prevent dialog from closing
            event.Skip(false);
            // exit
            return;
        }
        // validate frequency is positive
        if (m_max_frequency <= 0.0) {
            // show error message for non-positive frequency
            wxMessageBox("Maximum frequency must be positive.", "Error", wxOK | wxICON_ERROR, this);
            // skip event to prevent dialog from closing
            event.Skip(false);
            // exit
            return;
        }
    }
    else {
        // use preset frequency value
        m_max_frequency = MAX_FREQUENCY_OPTIONS[m_frequency_choice->GetSelection()].value;
    }

    // log accepted parameters
    spdlog::debug("FFT dialog accepted: {} expressions, window={}, output={}, max_freq={} Hz, range=[{}, {}], normalize={}, keep_dc={}", m_expression_selector->selected_expressions().size(), static_cast<int>(m_window_function), static_cast<int>(m_output), m_max_frequency, m_from_index, m_to_index, m_normalize, m_keep_dc);

    // skip event to allow default handling (closing the dialog)
    event.Skip();
}
