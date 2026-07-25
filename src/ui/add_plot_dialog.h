#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../expression/expression_manager.h"

// Struct for displayed expression chip metadata
struct DisplayedExpressionItem
{
    AnyExpression* expression{nullptr};
    std::string name;
    std::string type;
    bool selected{false};
};

// add plot dialog class
class AddPlotDialog : public wxDialog
{
public:
    AddPlotDialog(wxWindow* parent, ExpressionManager* expressions_manager, std::vector<AnyExpression*> selected_expressions, bool allow_custom_expressions = true, std::function<bool(const AnyExpression*)> expression_filter = [](const AnyExpression*) { return true; });

    [[nodiscard]] std::set<AnyExpression*> selected_expressions() const;

private:
    friend class AddPlotDialogTest;

    void rebuild_grid();
    void update_filter();
    void on_filter_text_changed(wxCommandEvent& event);
    void on_add_custom(wxCommandEvent& event);
    void on_ok(wxCommandEvent& event);
    void toggle_expression_selection(size_t);
    wxColour get_type_colour(const std::string& type) const;

    ExpressionManager* m_expressions_manager;
    bool m_allow_custom_expressions;
    std::function<bool(const AnyExpression*)> m_expression_filter;

    std::vector<DisplayedExpressionItem> m_all_expressions;
    std::vector<size_t> m_filtered_indices;

    wxTextCtrl* m_filter_input{nullptr};
    wxScrolledWindow* m_grid_scroller{nullptr};
    wxPanel* m_grid_container{nullptr};
    wxTextCtrl* m_custom_input{nullptr};
    wxButton* m_add_button{nullptr};
    wxStaticText* m_error_label{nullptr};
};
