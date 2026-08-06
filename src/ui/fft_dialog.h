#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/radiobut.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../expression/expression.h"
#include "../expression/expression_manager.h"
#include "../fft/fft.h"
#include "expression_selector_panel.h"

class FftDialog : public wxDialog
{
public:
    FftDialog(wxWindow* parent, ExpressionManager* expressions_manager, std::vector<AnyExpression*> selected_expressions, double min_abscissa_value, double max_abscissa_value, double min_abscissa_value_zoomed, double max_abscissa_value_zoomed, double default_max_frequency);

    [[nodiscard]] std::vector<AnyExpression*> selected_expressions() const;

    [[nodiscard]] double from_index() const { return m_from_index; }

    [[nodiscard]] double to_index() const { return m_to_index; }

    [[nodiscard]] fft::WindowFunction window_function() const { return m_window_function; }

    [[nodiscard]] double max_frequency() const { return m_max_frequency; }

    [[nodiscard]] bool normalize() const { return m_normalize; }

    [[nodiscard]] bool keep_dc() const { return m_keep_dc; }

    [[nodiscard]] fft::FftOutput output() const { return m_output; }

private:
    friend class FftDialogTest;

    void on_range_changed();
    void on_ok(wxCommandEvent& event);

    ExpressionSelectorPanel* m_expression_selector{nullptr};

    wxRadioButton* m_range_mode_all{nullptr};
    wxRadioButton* m_range_mode_zoom{nullptr};
    wxRadioButton* m_range_mode_custom{nullptr};

    wxTextCtrl* m_custom_from_input{nullptr};
    wxTextCtrl* m_custom_to_input{nullptr};

    wxChoice* m_window_choice{nullptr};
    wxChoice* m_output_choice{nullptr};
    wxChoice* m_frequency_choice{nullptr};
    wxTextCtrl* m_custom_frequency_input{nullptr};

    wxCheckBox* m_normalize_checkbox{nullptr};
    wxCheckBox* m_keep_dc_checkbox{nullptr};

    double m_min_abscissa_value{0.0};
    double m_max_abscissa_value{1.0};
    double m_min_abscissa_value_zoomed{0.0};
    double m_max_abscissa_value_zoomed{1.0};

    double m_from_index{0.0};
    double m_to_index{1.0};
    fft::WindowFunction m_window_function{fft::WindowFunction::HANNING};
    double m_max_frequency{1e5};
    bool m_normalize{true};
    bool m_keep_dc{false};
    fft::FftOutput m_output{fft::FftOutput::MAGNITUDE};
};
