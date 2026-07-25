#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <utility>

#include <wx/event.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/dcclient.h>
#include <wx/graphics.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#endif

#include "../expression/expression.h"
#include "add_plot_dialog.h"

namespace
{
    // type indicator colors matching QML getIndicatorColor
    const wxColour VOLTAGE_COLOR(0x5b, 0x9b, 0xd5);
    const wxColour CURRENT_COLOR(0x7c, 0xb3, 0x42);
    const wxColour FREQ_COLOR(0xe5, 0x73, 0x73);
    const wxColour TIME_COLOR(0xba, 0x68, 0xc8);
    const wxColour POWER_COLOR(0xff, 0xb7, 0x4d);
    const wxColour MISC_COLOR(0x3a, 0x3d, 0x4a);

    std::string to_lower(std::string s) {
        // convert string to lower case
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        // exit
        return s;
    }

    class ChipPanel : public wxPanel
    {
    public:
        ChipPanel(wxWindow* parent, const wxColour& color, const std::string& label, bool selected, int radius = 5) :
            wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 28)), m_radius(radius) {
            // prevent background flickering in Windows
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            // set background color based on selection state
            SetBackgroundColour(selected ? wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT) : wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
            // create sizer
            auto sizer = new wxBoxSizer(wxHORIZONTAL);
            // create color dot
            auto dot = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(6, 6));
            dot->SetBackgroundColour(color);
            // create label
            auto text_label = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
            wxFont font = text_label->GetFont();
            font.SetPointSize(12);
            text_label->SetFont(font);
            // add to sizer
            sizer->Add(dot, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
            sizer->Add(text_label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
            // set sizer
            SetSizer(sizer);
            // add event handler
            Bind(wxEVT_PAINT, &ChipPanel::on_paint, this);
        }

    private:
        int m_radius = 0;

        void on_paint(wxPaintEvent& event) {
            // create a paint DC to handle the paint event
            wxPaintDC dc(this);
            // graphics context for anti-aliased drawing
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            if (gc) {
                // get the size of the panel
                wxSize size = GetClientSize();
                // set the brush to a light gray color for the background
                gc->SetBrush(wxBrush(wxColour(245, 245, 245)));
                // set the pen to a slightly darker gray for the border
                gc->SetPen(wxPen(wxColour(200, 200, 200), 1));
                // draw a rounded rectangle with the specified radius
                gc->DrawRoundedRectangle(1, 1, size.GetWidth() - 2, size.GetHeight() - 2, m_radius);
            }
            // skip event
            event.Skip();
        }
    };
} // namespace

AddPlotDialog::AddPlotDialog(wxWindow* parent, ExpressionManager* expressions_manager, std::vector<AnyExpression*> selected_expressions, bool allow_custom_expressions, std::function<bool(const AnyExpression*)> expression_filter) :
    wxDialog(parent, wxID_ANY, "Select Plot Expressions", wxDefaultPosition, wxSize(560, 480), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), m_expressions_manager(expressions_manager), m_allow_custom_expressions(allow_custom_expressions), m_expression_filter(std::move(expression_filter)) {

    // loop expressions
    for (AnyExpression* expression : m_expressions_manager->expressions()) {
        // check filter
        if (m_expression_filter(expression)) {
            // get name
            std::string name = std::visit([](const auto& e) { return e.name(); }, *expression);
            // get type
            std::string type = std::visit([](const auto& e) { return e.variable_type(); }, *expression);
            if (type.empty())
                type = "Misc";
            // check initial selection
            bool is_selected = (std::find(selected_expressions.begin(), selected_expressions.end(), expression) != selected_expressions.end());
            // append item
            m_all_expressions.push_back({expression, name, type, is_selected});
        }
    }

    // create main vertical sizer
    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);

    // create title label
    auto title_text = new wxStaticText(this, wxID_ANY, "Select one or more expressions to plot:");
    wxFont title_font = title_text->GetFont();
    title_font.SetPointSize(12);
    title_text->SetFont(title_font);
    main_sizer->Add(title_text, 0, wxTOP | wxLEFT | wxRIGHT, 12);

    // create search filter input
    m_filter_input = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 28));
    m_filter_input->SetHint("Filter expressions...");
    main_sizer->Add(m_filter_input, 0, wxEXPAND | wxALL, 10);
    m_filter_input->Bind(wxEVT_TEXT, &AddPlotDialog::on_filter_text_changed, this);

    // separator
    main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

    // create scrollable window for grid
    m_grid_scroller = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_grid_scroller->SetScrollRate(0, 10);
    main_sizer->Add(m_grid_scroller, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

    // create grid container panel
    m_grid_container = new wxPanel(m_grid_scroller, wxID_ANY);

    auto scroller_sizer = new wxBoxSizer(wxVERTICAL);
    scroller_sizer->Add(m_grid_container, 0, wxEXPAND);
    m_grid_scroller->SetSizer(scroller_sizer);

    // check if custom expressions are enabled
    // if (m_allow_custom_expressions) {
    //     auto custom_panel = new wxPanel(this, wxID_ANY);

    //     auto custom_sizer = new wxBoxSizer(wxVERTICAL);
    //     auto input_row_sizer = new wxBoxSizer(wxHORIZONTAL);

    //     auto expr_label = new wxStaticText(custom_panel, wxID_ANY, "Expression:");

    //     m_custom_input = new wxTextCtrl(custom_panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 28), wxTE_PROCESS_ENTER);
    //     m_custom_input->SetHint("e.g. V(net1) / I(R1)");

    //     m_add_button = new wxButton(custom_panel, wxID_ANY, "Add", wxDefaultPosition, wxSize(52, 28));

    //     input_row_sizer->Add(expr_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    //     input_row_sizer->Add(m_custom_input, 1, wxEXPAND | wxRIGHT, 6);
    //     input_row_sizer->Add(m_add_button, 0, wxALIGN_CENTER_VERTICAL);

    //     custom_sizer->Add(input_row_sizer, 0, wxEXPAND | wxALL, 10);

    //     // create error label
    //     m_error_label = new wxStaticText(custom_panel, wxID_ANY, "");
    //     custom_sizer->Add(m_error_label, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);

    //     custom_panel->SetSizer(custom_sizer);
    //     main_sizer->Add(custom_panel, 0, wxEXPAND);

    //     m_add_button->Bind(wxEVT_BUTTON, &AddPlotDialog::on_add_custom, this);
    //     m_custom_input->Bind(wxEVT_TEXT_ENTER, &AddPlotDialog::on_add_custom, this);
    // }

    // separator
    main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

    // create bottom panel for legend and action buttons
    auto bottom_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 50));
    auto bottom_sizer = new wxBoxSizer(wxHORIZONTAL);

    // create legend horizontal sizer
    auto legend_sizer = new wxBoxSizer(wxHORIZONTAL);
    struct LegendItem
    {
        std::string label;
        wxColour color;
    };
    std::vector<LegendItem> legend_items = {{"Voltage", VOLTAGE_COLOR}, {"Current", CURRENT_COLOR}, {"Freq", FREQ_COLOR}, {"Time", TIME_COLOR}, {"Power", POWER_COLOR}, {"Misc", MISC_COLOR}};

    for (const auto& item : legend_items) {
        auto dot = new wxPanel(bottom_panel, wxID_ANY, wxDefaultPosition, wxSize(6, 6));
        dot->SetBackgroundColour(item.color);
        auto lbl = new wxStaticText(bottom_panel, wxID_ANY, item.label);
        wxFont legend_font = lbl->GetFont();
        legend_font.SetPointSize(9);
        lbl->SetFont(legend_font);

        legend_sizer->Add(dot, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        legend_sizer->Add(lbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    }

    bottom_sizer->Add(legend_sizer, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

    // create cancel and ok buttons
    auto cancel_btn = new wxButton(bottom_panel, wxID_CANCEL, "Cancel", wxDefaultPosition, wxSize(80, 28));

    auto ok_btn = new wxButton(bottom_panel, wxID_OK, "OK", wxDefaultPosition, wxSize(80, 28));

    bottom_sizer->Add(cancel_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    bottom_sizer->Add(ok_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    bottom_panel->SetSizer(bottom_sizer);
    main_sizer->Add(bottom_panel, 0, wxEXPAND);

    // perform initial filter update
    update_filter();

    // bind ok button event
    Bind(wxEVT_BUTTON, &AddPlotDialog::on_ok, this, wxID_OK);
}

wxColour AddPlotDialog::get_type_colour(const std::string& type) const {
    // get lower case type
    std::string lower = to_lower(type);
    if (lower.find("voltage") != std::string::npos || lower == "v")
        return VOLTAGE_COLOR;
    if (lower.find("current") != std::string::npos || lower == "i")
        return CURRENT_COLOR;
    if (lower.find("freq") != std::string::npos)
        return FREQ_COLOR;
    if (lower.find("time") != std::string::npos)
        return TIME_COLOR;
    if (lower.find("power") != std::string::npos)
        return POWER_COLOR;
    return MISC_COLOR;
}

void AddPlotDialog::on_filter_text_changed(wxCommandEvent&) {
    // update filter on input change
    update_filter();
}

void AddPlotDialog::update_filter() {
    // get search text
    std::string query = (m_filter_input != nullptr) ? to_lower(m_filter_input->GetValue().ToStdString()) : "";
    // remove current filtered indices
    m_filtered_indices.clear();
    // filter indices
    for (size_t i = 0; i < m_all_expressions.size(); ++i) {
        // check if query is empty or if the expression name contains the query substring
        if (query.empty() || to_lower(m_all_expressions[i].name).find(query) != std::string::npos)
            m_filtered_indices.push_back(i);
    }
    // rebuild grid layout
    rebuild_grid();
}

void AddPlotDialog::rebuild_grid() {
    // destroy existing grid children
    m_grid_container->DestroyChildren();

    // create 3-column flex grid sizer with fixed row gap & column gap (cols, vgap, hgap)
    auto flex_sizer = new wxFlexGridSizer(0, 3, 6, 6);
    // allow columns to grow proportionally
    flex_sizer->AddGrowableCol(0, 1);
    flex_sizer->AddGrowableCol(1, 1);
    flex_sizer->AddGrowableCol(2, 1);

    // loop filtered items
    for (size_t filtered_idx = 0; filtered_idx < m_filtered_indices.size(); ++filtered_idx) {
        // get real index in all expressions
        size_t real_idx = m_filtered_indices[filtered_idx];
        // get item at real index
        const auto& item = m_all_expressions[real_idx];
        // create chip panel for the expression
        auto chip = new ChipPanel(m_grid_container, get_type_colour(item.type), item.name, item.selected);
        // bind events
        chip->Bind(wxEVT_COMMAND_LEFT_CLICK, [this, real_idx](wxCommandEvent&) -> void { toggle_expression_selection(real_idx); });
        // add chip to flex grid sizer
        flex_sizer->Add(chip, 0, wxEXPAND);
    }

    m_grid_container->SetSizer(flex_sizer);
    m_grid_scroller->FitInside();
    m_grid_container->Layout();
    Layout();
}

void AddPlotDialog::toggle_expression_selection(size_t real_idx) {
    // check if index is valid
    if (real_idx < m_all_expressions.size()) {
        // toggle selection boolean
        m_all_expressions[real_idx].selected = !m_all_expressions[real_idx].selected;
        // rebuild grid to reflect selection change
        rebuild_grid();
    }
}

std::set<AnyExpression*> AddPlotDialog::selected_expressions() const {
    // update selected expressions array
    std::set<AnyExpression*> selected_expressions = {};
    // loop all expressions and collect selected ones
    for (const auto& item : m_all_expressions) {
        // check if selected and expression is not null
        if (item.selected && item.expression != nullptr)
            selected_expressions.insert(item.expression);
    }
    return selected_expressions;
}

void AddPlotDialog::on_add_custom(wxCommandEvent&) {
    // check if custom input is available
    if (m_custom_input == nullptr)
        return;
    // get input string
    wxString input_wx = m_custom_input->GetValue();
    input_wx.Trim(true).Trim(false);
    if (input_wx.IsEmpty())
        return;
    std::string text = input_wx.ToStdString();
    // evaluate expression
    AnyExpression* expression = m_expressions_manager->evaluate(text, text);
    if (expression == nullptr) {
        // show error message
        m_error_label->SetLabel("Invalid expression");
        // refresh layout
        Layout();
        // exit
        return;
    }
    // reset error message
    m_error_label->SetLabel("");
    // refresh layout
    Layout();
    // search existing item
    auto it = std::find_if(m_all_expressions.begin(), m_all_expressions.end(), [expression](const DisplayedExpressionItem& item) { return item.expression == expression; });
    if (it != m_all_expressions.end()) {
        // expression already exists, mark as selected
        it->selected = true;
    }
    else {
        // name and type
        std::string name = std::visit([](const auto& e) { return e.name(); }, *expression);
        std::string type = std::visit([](const auto& e) { return e.variable_type(); }, *expression);
        if (type.empty())
            type = "Misc";
        // append to all expressions and mark as selected
        m_all_expressions.emplace_back(expression, name, type, true);
    }
    // clear custom input and update filter
    m_custom_input->Clear();
    // update filter to include new expression
    update_filter();
}

void AddPlotDialog::on_ok(wxCommandEvent& event) {
    // skip event to allow default handling (closing the dialog)
    event.Skip();
}
