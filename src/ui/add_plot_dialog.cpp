#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

#include <wx/button.h>
#include <wx/sizer.h>

#include "add_plot_dialog.h"

AddPlotDialog::AddPlotDialog(wxWindow* parent, ExpressionManager& expressions_manager, const std::vector<AnyExpression*>& selected_expressions, bool allow_custom_expressions, std::function<bool(const AnyExpression&)> expression_filter) :
    wxDialog(parent, wxID_ANY, "Add Plot", wxDefaultPosition, wxSize(560, 480)),
    // initialize fields
    m_expressions_manager(expressions_manager), m_selected_expressions(selected_expressions), m_allow_custom_expressions(allow_custom_expressions), m_expression_filter(std::move(expression_filter)), m_list_box(nullptr), m_custom_input(nullptr), m_add_button(nullptr), m_error_label(nullptr) {
    // create top-level sizer
    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    // create list box with multi-selection enabled
    m_list_box = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_MULTIPLE);
    // loop through expressions in manager
    for (const auto& expression : m_expressions_manager.expressions()) {
        // apply filter to expression
        if (m_expression_filter(expression)) {
            // get name
            std::string name = std::visit([](const auto& e) { return e.name(); }, expression);
            // get type
            std::string type = std::visit([](const auto& e) { return e.variable_type(); }, expression);
            // check if type is empty
            if (type.empty()) {
                // default type
                type = "Misc";
            }
            // format display string
            std::string display_str = name + " [" + type + "]";
            // append to list box
            m_list_box->Append(display_str);
            // track displayed expression pointer
            m_displayed_expressions.push_back(m_expressions_manager.evaluate(name, name));
        }
    }
    // select items that are in the initial selected_expressions list
    for (size_t i = 0; i < m_displayed_expressions.size(); ++i) {
        // search for displayed expression in selected list
        if (auto it = std::ranges::find(m_selected_expressions, m_displayed_expressions[i]); it != m_selected_expressions.end()) {
            // select in list box
            m_list_box->Select(i);
        }
    }
    // add list box to main sizer
    main_sizer->Add(m_list_box, 1, wxEXPAND | wxALL, 10);
    // check if custom expressions are allowed
    if (m_allow_custom_expressions) {
        // create custom expression horizontal sizer
        auto custom_sizer = new wxBoxSizer(wxHORIZONTAL);
        // create label
        auto label = new wxStaticText(this, wxID_ANY, "Custom Expression:");
        // add label to custom sizer
        custom_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        // create text input for custom expression
        m_custom_input = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
        // add text input to custom sizer
        custom_sizer->Add(m_custom_input, 1, wxEXPAND | wxRIGHT, 5);
        // create add button
        m_add_button = new wxButton(this, wxID_ANY, "Add");
        // add button to custom sizer
        custom_sizer->Add(m_add_button, 0, wxALIGN_CENTER_VERTICAL);
        // add custom sizer to main sizer
        main_sizer->Add(custom_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        // create error label
        m_error_label = new wxStaticText(this, wxID_ANY, wxEmptyString);
        // set error label text color to red
        m_error_label->SetForegroundColour(*wxRED);
        // add error label to main sizer
        main_sizer->Add(m_error_label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        // bind add button click event
        m_add_button->Bind(wxEVT_BUTTON, &AddPlotDialog::on_add_custom, this);
        // bind enter key press on text input
        m_custom_input->Bind(wxEVT_TEXT_ENTER, &AddPlotDialog::on_add_custom, this);
    }
    // create buttons sizer
    if (wxSizer* button_sizer = CreateButtonSizer(wxOK | wxCANCEL)) {
        // add button sizer to main sizer
        main_sizer->Add(button_sizer, 0, wxALIGN_RIGHT | wxALL, 10);
    }
    // set dialog sizer
    SetSizer(main_sizer);
    // bind ok button event
    Bind(wxEVT_BUTTON, &AddPlotDialog::on_ok, this, wxID_OK);
}

std::vector<AnyExpression*> AddPlotDialog::selected_expressions() const {
    // return selected expressions list
    return m_selected_expressions;
}

void AddPlotDialog::on_add_custom(wxCommandEvent&) {
    // get input value
    wxString input_wx = m_custom_input->GetValue();
    // trim leading and trailing spaces
    input_wx.Trim(true).Trim(false);
    // check if input is empty
    if (input_wx.IsEmpty()) {
        // return early
        return;
    }
    // convert to standard string
    std::string text = input_wx.ToStdString();
    // evaluate expression using manager
    AnyExpression* expr = m_expressions_manager.evaluate(text, text);
    // check if evaluation failed
    if (!expr) {
        // display error message
        m_error_label->SetLabel("Invalid expression");
        // perform layout update
        Layout();
        // return early
        return;
    }
    // clear error label text
    m_error_label->SetLabel(wxEmptyString);
    // perform layout update
    Layout();
    // find if expression is already displayed in list box
    auto it = std::find(m_displayed_expressions.begin(), m_displayed_expressions.end(), expr);
    // check if found
    if (it != m_displayed_expressions.end()) {
        // calculate index of existing item
        int index = static_cast<int>(std::distance(m_displayed_expressions.begin(), it));
        // check if item is not selected
        if (!m_list_box->IsSelected(index)) {
            // select the item in list box
            m_list_box->Select(index);
        }
    }
    else {
        // get name
        std::string name = std::visit([](const auto& e) { return e.name(); }, *expr);
        // get type
        std::string type = std::visit([](const auto& e) { return e.variable_type(); }, *expr);
        // check if type is empty
        if (type.empty()) {
            // default type
            type = "Misc";
        }
        // format display string
        std::string display_str = name + " [" + type + "]";
        // append display string to list box
        int new_index = m_list_box->Append(display_str);
        // add pointer to displayed expressions list
        m_displayed_expressions.push_back(expr);
        // select newly added item in list box
        m_list_box->Select(new_index);
    }
    // clear custom input text field
    m_custom_input->Clear();
}

void AddPlotDialog::on_ok(wxCommandEvent& event) {
    // clear previous selection list
    m_selected_expressions.clear();
    // create array to hold selected indices
    wxArrayInt selections;
    // get selected indices from list box
    m_list_box->GetSelections(selections);
    // loop selected indices
    for (size_t i = 0; i < selections.size(); ++i) {
        // get list box index
        const int idx = selections[i];
        // append to selected expressions list
        m_selected_expressions.push_back(m_displayed_expressions[idx]);
    }
    // skip event to allow default processing
    event.Skip();
}
