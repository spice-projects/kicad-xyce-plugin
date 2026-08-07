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
    FftDialog(wxWindow* parent, ExpressionManager* expressions_manager, std::vector<AnyExpression*> selected_expressions, double min_abscissa_value, double max_abscissa_value, double min_abscissa_value_zoomed, double max_abscissa_value_zoomed);

    [[nodiscard]] std::vector<AnyExpression*> selected_expressions() const;

    [[nodiscard]] double from_index() const { return m_from_index; }

    [[nodiscard]] double to_index() const { return m_to_index; }

    [[nodiscard]] fft::FftParameters parameters() const;

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
    wxChoice* m_np_choice{nullptr};
    wxTextCtrl* m_custom_np_input{nullptr};

    wxChoice* m_format_choice{nullptr};
    wxCheckBox* m_keep_dc_checkbox{nullptr};

    double m_min_abscissa_value{0.0};
    double m_max_abscissa_value{1.0};
    double m_min_abscissa_value_zoomed{0.0};
    double m_max_abscissa_value_zoomed{1.0};

    double m_from_index{0.0};
    double m_to_index{1.0};
    fft::FftParameters m_parameters{};
};
