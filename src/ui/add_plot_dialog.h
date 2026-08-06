#pragma once

#include <functional>
#include <set>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/dialog.h>
#include <wx/stattext.h>
#endif

#include "../expression/expression_manager.h"
#include "expression_selector_panel.h"

// add plot dialog class
class AddPlotDialog : public wxDialog
{
public:
    AddPlotDialog(wxWindow* parent, ExpressionManager* expressions_manager, std::vector<AnyExpression*> selected_expressions, bool allow_custom_expressions = true, std::function<bool(const AnyExpression*)> expression_filter = [](const AnyExpression*) { return true; });

    [[nodiscard]] std::set<AnyExpression*> selected_expressions() const;

private:
    friend class AddPlotDialogTest;

    void on_ok(wxCommandEvent& event);

    ExpressionSelectorPanel* m_expression_selector{nullptr};
};
