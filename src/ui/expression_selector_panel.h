#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../expression/expression_manager.h"

// configuration for expression selector behavior
struct ExpressionSelectorConfig
{
    bool show_filter{true};
    bool show_custom_input{false};
    bool show_legend{true};
    bool allow_empty_selection{false};
    std::string filter_hint{"Filter expressions..."};
    std::string custom_hint{"e.g. V(net1) / I(R1)"};
};

// struct for displayed expression chip metadata
struct ExpressionItem
{
    AnyExpression* expression{nullptr};
    std::string name;
    std::string type;
    bool selected{false};
};

// reusable expression selector panel
class ExpressionSelectorPanel : public wxPanel
{
public:
    ExpressionSelectorPanel(wxWindow* parent, ExpressionManager* expressions_manager, const std::vector<AnyExpression*>& selected_expressions, const ExpressionSelectorConfig& config = {}, std::function<bool(const AnyExpression*)> expression_filter = [](const AnyExpression*) { return true; });

    [[nodiscard]] std::set<AnyExpression*> selected_expressions() const;

    void set_selected_expressions(const std::vector<AnyExpression*>& expressions);

    [[nodiscard]] bool validate_selection(std::string* error_message = nullptr) const;

private:
    friend class ExpressionSelectorPanelTest;

    void rebuild_grid();
    void update_filter();
    void on_filter_text_changed(wxCommandEvent& event);
    void on_add_custom(wxCommandEvent& event);
    void toggle_expression_selection(size_t index);
    wxColour get_type_colour(const std::string& type) const;

    ExpressionManager* m_expressions_manager;
    ExpressionSelectorConfig m_config;
    std::function<bool(const AnyExpression*)> m_expression_filter;

    std::vector<ExpressionItem> m_all_expressions;
    std::vector<size_t> m_filtered_indices;

    wxTextCtrl* m_filter_input{nullptr};
    wxScrolledWindow* m_grid_scroller{nullptr};
    wxPanel* m_grid_container{nullptr};
    wxPanel* m_custom_panel{nullptr};
    wxTextCtrl* m_custom_input{nullptr};
    wxButton* m_add_button{nullptr};
    wxStaticText* m_error_label{nullptr};
    wxPanel* m_legend_panel{nullptr};
};
