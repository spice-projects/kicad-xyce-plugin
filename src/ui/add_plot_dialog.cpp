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
#include "../util.h"
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

    class ColorDot : public wxPanel
    {
    public:
        ColorDot(wxWindow* parent, const wxColour& color) :
            wxPanel(parent, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(8, 8))) {
            // prevent background flickering in Windows
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            m_color = color;
            // paint handler for drawing the filled circle
            Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
                wxPaintDC dc(this);
                // graphics context for anti-aliased drawing
                std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
                if (gc) {
                    wxSize s = GetClientSize();
                    gc->SetBrush(wxBrush(m_color));
                    gc->SetPen(*wxTRANSPARENT_PEN);
                    gc->DrawEllipse(0, 0, s.GetWidth(), s.GetHeight());
                }
            });
        }
    private:
        wxColour m_color;
    };

    class ChipPanel : public wxPanel
    {
    public:
        ChipPanel(wxWindow* parent, const wxColour& color, const std::string& label, bool selected, int radius = 5) :
            wxPanel(parent, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(-1, 28))), m_selected(selected), m_radius(FromDIP(radius)) {
            // prevent background flickering in Windows
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            // create sizer
            auto sizer = new wxBoxSizer(wxHORIZONTAL);
            // create color dot
            auto dot = new ColorDot(this, color);
            // create label
            auto text_label = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
            wxFont font = text_label->GetFont();
            font.SetPointSize(12);
            text_label->SetFont(font);
            // set text color based on selection state so it remains readable in both dark and light appearances
            text_label->SetForegroundColour(wxSystemSettings::GetColour(m_selected ? wxSYS_COLOUR_HIGHLIGHTTEXT : wxSYS_COLOUR_LISTBOXTEXT));
            // add to sizer
            sizer->Add(dot, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(6));
            sizer->Add(text_label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
            // set sizer
            SetSizer(sizer);
            // add paint event handler
            Bind(wxEVT_PAINT, &ChipPanel::on_paint, this);
            // hover event handlers for visual feedback
            Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { m_hovered = true; Refresh(); });
            Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_hovered = false; Refresh(); });
        }

    private:
        bool m_selected = false;
        bool m_hovered = false;
        int m_radius = 0;

        void on_paint(wxPaintEvent& event) {
            // create a paint DC to handle the paint event
            wxPaintDC dc(this);
            // graphics context for anti-aliased drawing
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            if (gc) {
                // get the size of the panel
                wxSize size = GetClientSize();
                if (m_selected) {
                    // use highlight colors for selected chips
                    gc->SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT)));
                    gc->SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT), 1));
                }
                else if (m_hovered) {
                    // use window background with highlight border for hover feedback
                    gc->SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
                    gc->SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT), FromDIP(2)));
                }
                else {
                    // use default colors for unselected chips, with a subtle border for better visibility
                    gc->SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
                    gc->SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW), 1));
                }
                // draw a rounded rectangle with the specified radius
                gc->DrawRoundedRectangle(1, 1, size.GetWidth() - 2, size.GetHeight() - 2, m_radius);
            }
            // skip event
            event.Skip();
        }
    };

    struct LegendItem
    {
        std::string label;
        wxColour color;
    };

    LegendItem LEGEND_ITEMS[] = {{"Voltage", VOLTAGE_COLOR}, {"Current", CURRENT_COLOR}, {"Freq", FREQ_COLOR}, {"Time", TIME_COLOR}, {"Power", POWER_COLOR}, {"Misc", MISC_COLOR}};
} // namespace

AddPlotDialog::AddPlotDialog(wxWindow* parent, ExpressionManager* expressions_manager, std::vector<AnyExpression*> selected_expressions, bool allow_custom_expressions, std::function<bool(const AnyExpression*)> expression_filter) :
    wxDialog(parent, wxID_ANY, "Select Plot Expressions", wxDefaultPosition, FromDIP(wxSize(600, 550)), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), m_expressions_manager(expressions_manager), m_allow_custom_expressions(allow_custom_expressions), m_expression_filter(std::move(expression_filter)) {
    // min size
    SetMinSize(FromDIP(wxSize(600, 550)));
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
    main_sizer->Add(title_text, 0, wxTOP | wxLEFT | wxRIGHT, FromDIP(8));
    // create search filter input
    m_filter_input = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(-1, 28)));
    m_filter_input->SetHint("Filter expressions...");
    main_sizer->Add(m_filter_input, 0, wxEXPAND | wxALL, FromDIP(4));
    // separator
    main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxBOTTOM, FromDIP(2));
    // create scrollable window for grid
    m_grid_scroller = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_grid_scroller->SetScrollRate(0, FromDIP(10));
    main_sizer->Add(m_grid_scroller, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    // create grid container panel
    m_grid_container = new wxPanel(m_grid_scroller, wxID_ANY);
    // create sizer for scroller
    auto scroller_sizer = new wxBoxSizer(wxVERTICAL);
    scroller_sizer->Add(m_grid_container, 0, wxEXPAND);
    m_grid_scroller->SetSizer(scroller_sizer);
    // check if custom expressions are enabled
    if (m_allow_custom_expressions) {
        // separator
        main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxTOP, FromDIP(2));
        // create custom expression input panel
        auto custom_panel = new wxPanel(this, wxID_ANY);
        // panel sizer
        auto custom_sizer = new wxBoxSizer(wxVERTICAL);
        custom_panel->SetSizer(custom_sizer);
        // row sizer for input and button
        auto input_row_sizer = new wxBoxSizer(wxHORIZONTAL);
        // label
        auto expr_label = new wxStaticText(custom_panel, wxID_ANY, "Expression:");
        input_row_sizer->Add(expr_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
        // input
        m_custom_input = new wxTextCtrl(custom_panel, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(-1, 28)), wxTE_PROCESS_ENTER);
        m_custom_input->SetHint("e.g. V(net1) / I(R1)");
        input_row_sizer->Add(m_custom_input, 1, wxEXPAND | wxRIGHT, FromDIP(6));
        // button
        m_add_button = new wxButton(custom_panel, wxID_ANY, "Add", wxDefaultPosition, FromDIP(wxSize(52, 28)));
        input_row_sizer->Add(m_add_button, 0, wxALIGN_CENTER_VERTICAL);
        // append input row sizer to custom sizer with padding
        custom_sizer->Add(input_row_sizer, 0, wxEXPAND | wxALL, FromDIP(6));
        // create error label
        m_error_label = new wxStaticText(custom_panel, wxID_ANY, "");
        custom_sizer->Add(m_error_label, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(2));
        m_error_label->Show(false);
        custom_sizer->Show(m_error_label, false);
        // append custom panel to main sizer with padding
        main_sizer->Add(custom_panel, 0, wxEXPAND);
        // bind events for adding custom expressions
        m_add_button->Bind(wxEVT_BUTTON, &AddPlotDialog::on_add_custom, this);
        m_custom_input->Bind(wxEVT_TEXT_ENTER, &AddPlotDialog::on_add_custom, this);
    }
    // separator
    main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxTOP, FromDIP(2));
    // legend panel
    auto legend_panel = new wxPanel(this, wxID_ANY);
    main_sizer->Add(legend_panel, 0, wxEXPAND | wxLEFT, FromDIP(12));
    // legend sizer
    auto legend_sizer = new wxBoxSizer(wxHORIZONTAL);
    legend_panel->SetSizer(legend_sizer);
    // legend items
    for (const auto& item : LEGEND_ITEMS) {
        // create color dot
        auto dot = new ColorDot(legend_panel, item.color);
        legend_sizer->Add(dot, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
        // label for the legend item
        auto lbl = new wxStaticText(legend_panel, wxID_ANY, item.label);
        wxFont legend_font = lbl->GetFont();
        legend_font.SetPointSize(9);
        lbl->SetFont(legend_font);
        legend_sizer->Add(lbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
    }
    // create standard OK/Cancel button sizer using platform conventions
    main_sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, FromDIP(12));
    // perform initial filter update
    update_filter();
    // event handlers
    m_filter_input->Bind(wxEVT_TEXT, &AddPlotDialog::on_filter_text_changed, this);    
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
    // prevent repaints until layout is complete
    Freeze();
    // destroy existing grid children
    m_grid_container->DestroyChildren();
    // create 3-column flex grid sizer with fixed row gap & column gap (cols, vgap, hgap)
    auto flex_sizer = new wxFlexGridSizer(0, 3, FromDIP(6), FromDIP(6));
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
        chip->Bind(wxEVT_LEFT_DOWN, [this, real_idx](wxMouseEvent&) { toggle_expression_selection(real_idx); });
        // check custom expressions are enabled
        if (m_custom_input != nullptr)
            chip->Bind(wxEVT_RIGHT_DOWN, [this, &item](wxMouseEvent&) { m_custom_input->SetValue(m_custom_input->GetValue() + item.name); });
        // add chip to flex grid sizer
        flex_sizer->Add(chip, 0, wxEXPAND);
    }
    // render items
    m_grid_container->SetSizer(flex_sizer);
    m_grid_scroller->FitInside();
    m_grid_container->Layout();
    Layout();
    // resume repaints
    Thaw();
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
    auto* error_sizer = m_error_label->GetContainingSizer();
    if (expression == nullptr) {
        // show error message
        m_error_label->SetLabel("Invalid expression");
        m_error_label->Show(true);
        if (error_sizer != nullptr)
            error_sizer->Show(m_error_label, true);
        // refresh layout
        Layout();
        // exit
        return;
    }
    // reset error message
    m_error_label->SetLabel("");
    m_error_label->Show(false);
    if (error_sizer != nullptr)
        error_sizer->Show(m_error_label, false);
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
