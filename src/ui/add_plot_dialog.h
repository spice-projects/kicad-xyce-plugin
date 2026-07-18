#pragma once

#include <functional>
#include <vector>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "../expression/expression_manager.h"

// add plot dialog class
class AddPlotDialog : public wxDialog
{
public:
    AddPlotDialog(wxWindow* parent, ExpressionManager& expressions_manager, const std::vector<AnyExpression*>& selected_expressions, bool allow_custom_expressions = true, std::function<bool(const AnyExpression&)> expression_filter = [](const AnyExpression&) { return true; });

    [[nodiscard]] std::vector<AnyExpression*> selected_expressions() const;

private:
    friend class AddPlotDialogTest;

    void on_add_custom(wxCommandEvent& event);

    void on_ok(wxCommandEvent& event);

    ExpressionManager& m_expressions_manager;
    std::vector<AnyExpression*> m_selected_expressions;
    bool m_allow_custom_expressions;
    std::function<bool(const AnyExpression&)> m_expression_filter;
    std::vector<AnyExpression*> m_displayed_expressions;
    wxListBox* m_list_box;
    wxTextCtrl* m_custom_input;
    wxButton* m_add_button;
    wxStaticText* m_error_label;
};
