#include <set>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/sizer.h>
#include <wx/stattext.h>
#endif

#include "add_plot_dialog.h"
#include "expression_selector_panel.h"

AddPlotDialog::AddPlotDialog(wxWindow* parent, ExpressionManager* expressions_manager, std::vector<AnyExpression*> selected_expressions, bool allow_custom_expressions, std::function<bool(const AnyExpression*)> expression_filter) :
    wxDialog(parent, wxID_ANY, "Select Plot Expressions", wxDefaultPosition, FromDIP(wxSize(600, 550)), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    // min size
    SetMinSize(FromDIP(wxSize(600, 550)));
    // create main vertical sizer
    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);
    // create title label
    auto title_text = new wxStaticText(this, wxID_ANY, "Select one or more expressions to plot:");
    wxFont title_font = title_text->GetFont();
    title_font.SetPointSize(12);
    title_text->SetFont(title_font);
    main_sizer->Add(title_text, 0, wxTOP | wxLEFT | wxRIGHT, FromDIP(8));
    // configure expression selector
    ExpressionSelectorConfig config{.show_filter = true, .show_custom_input = allow_custom_expressions, .show_legend = true, .allow_empty_selection = false, .filter_hint = "Filter expressions...", .custom_hint = "e.g. V(net1) / I(R1)"};
    // create expression selector panel
    m_expression_selector = new ExpressionSelectorPanel(this, expressions_manager, selected_expressions, config, expression_filter);
    main_sizer->Add(m_expression_selector, 1, wxEXPAND);
    // create standard OK/Cancel button sizer using platform conventions
    main_sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, FromDIP(12));
    // event handlers
    Bind(wxEVT_BUTTON, &AddPlotDialog::on_ok, this, wxID_OK);
}

std::set<AnyExpression*> AddPlotDialog::selected_expressions() const {
    // delegate to expression selector panel
    return m_expression_selector->selected_expressions();
}

void AddPlotDialog::on_ok(wxCommandEvent& event) {
    // skip event to allow default handling (closing the dialog)
    event.Skip();
}
