#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

#include <slint.h>

#include <main_window.h>

#include "../core/util.h"
#include "add_plot_dialog_view.h"

namespace add_plot_dialog_view
{
    namespace
    {
        slint::SharedString to_shared_string(std::string value) { return slint::SharedString(value); }
    } // namespace

    struct AddPlotDialogView::Impl
    {
        // the main window handle; the panel is an inline child of the window,
        // so all interaction goes through the window's properties and callbacks
        slint::ComponentHandle<main_window::MainWindow> window;

        // renderer used to look up expressions and apply the selection
        ChartsRenderer& renderer;

        // notified on both accept and cancel, after the panel is hidden;
        // the caller releases the modal state from here
        std::function<void()> on_closed;

        // expression model shown in the panel grid; the panel uses the indices
        // of this model in the expression-clicked callback
        std::shared_ptr<slint::VectorModel<main_window::ExpressionItem>> expressions;

        // all expressions known to the panel with their selection state
        std::vector<main_window::ExpressionItem> m_all_items;

        // indices into m_all_items that pass the current filter
        std::vector<size_t> m_filtered_indices;

        // chart being edited
        size_t chart_index = 0;

        Impl(slint::ComponentHandle<main_window::MainWindow> w, ChartsRenderer& r) :
            window(w), renderer(r) {
            expressions = std::make_shared<slint::VectorModel<main_window::ExpressionItem>>();
            window->set_add_plot_expressions(expressions);
            connect_callbacks();
        }

        void connect_callbacks() {
            window->on_add_plot_filter_changed([this](slint::SharedString query) { apply_filter(query); });
            window->on_add_plot_expression_clicked([this](int index) { toggle_expression(index); });
            window->on_add_plot_expression_right_clicked([this](int index) { append_expression_name(index); });
            window->on_add_plot_custom_add([this] { add_custom_expression(); });
            window->on_add_plot_accepted([this] { accept(); });
            window->on_add_plot_dismissed([this] { dismiss(); });
        }

        void populate() {
            m_all_items.clear();
            // build the initial expression list from the expression manager
            for (AnyExpression* expression : renderer.all_expressions()) {
                const auto name = expression_name(*expression);
                m_all_items.push_back(main_window::ExpressionItem{to_shared_string(name), to_shared_string(expression_type(*expression)), false});
            }
            // check the chart's current selection
            const auto selected = renderer.chart_selected_expressions(chart_index);
            // mark the currently plotted expressions as selected
            for (auto& item : m_all_items) {
                item.selected = std::any_of(selected.begin(), selected.end(), [&item](AnyExpression* expression) { return expression_name(*expression) == std::string(item.name); });
            }
            // reset the filter and error state
            m_filter_query.clear();
            window->set_add_plot_filter_text(slint::SharedString(""));
            window->set_add_plot_show_error(false);
            // rebuild the grid with the empty filter
            update_filtered();
        }

        static std::string expression_name(const AnyExpression& expression) {
            return std::visit([](const auto& e) { return e.name(); }, expression);
        }

        static std::string expression_type(const AnyExpression& expression) {
            std::string type = std::visit([](const auto& e) { return e.variable_type(); }, expression);
            return type.empty() ? "Misc" : type;
        }

        void apply_filter(const slint::SharedString& query) {
            // remember the active filter, it is reapplied when items change
            m_filter_query = std::string(query);
            update_filtered();
        }

        void update_filtered() {
            // filter the display indices by a case-insensitive substring of the name
            const std::string needle = to_lower(m_filter_query);
            m_filtered_indices.clear();
            for (size_t i = 0; i < m_all_items.size(); ++i) {
                if (needle.empty() || to_lower(std::string(m_all_items[i].name)).find(needle) != std::string::npos)
                    m_filtered_indices.push_back(i);
            }
            // rebuild the model rows shown in the grid
            expressions->clear();
            for (size_t index : m_filtered_indices)
                expressions->push_back(m_all_items[index]);
        }

        void toggle_expression(int index) {
            // map the model row back to the full item and flip the selection
            if (index < 0 || static_cast<size_t>(index) >= m_filtered_indices.size())
                return;
            const size_t real_index = m_filtered_indices[static_cast<size_t>(index)];
            m_all_items[real_index].selected = !m_all_items[real_index].selected;
            // keep the grid in sync
            expressions->set_row_data(static_cast<size_t>(index), m_all_items[real_index]);
        }

        void append_expression_name(int index) {
            // copy the right-clicked expression name into the Expression builder,
            // only when the custom entry is shown
            if (!window->get_add_plot_allow_custom_expressions())
                return;
            if (index < 0 || static_cast<size_t>(index) >= m_filtered_indices.size())
                return;
            const size_t real_index = m_filtered_indices[static_cast<size_t>(index)];
            slint::SharedString value = window->get_add_plot_custom_text();
            value = slint::SharedString(std::string(value) + std::string(m_all_items[real_index].name));
            window->set_add_plot_custom_text(value);
        }

        void add_custom_expression() {
            // read the custom expression text
            const std::string text = std::string(window->get_add_plot_custom_text());
            const auto trimmed_start = text.find_first_not_of(" \t\r\n");
            const auto trimmed_end = text.find_last_not_of(" \t\r\n");
            // ignore empty input
            if (trimmed_start == std::string::npos)
                return;
            const std::string trimmed = text.substr(trimmed_start, trimmed_end - trimmed_start + 1);
            // evaluate the custom expression
            AnyExpression* expression = renderer.evaluate_expression(trimmed);
            if (expression == nullptr) {
                window->set_add_plot_show_error(true);
                return;
            }
            window->set_add_plot_show_error(false);
            // derive name and type
            const std::string name = expression_name(*expression);
            const std::string type = expression_type(*expression);
            // search for an existing item with the same name
            const auto it = std::find_if(m_all_items.begin(), m_all_items.end(), [&name](const main_window::ExpressionItem& item) { return std::string(item.name) == name; });
            if (it != m_all_items.end()) {
                // expression already exists, mark it as selected
                it->selected = true;
            }
            else {
                // append the new expression, selected
                m_all_items.push_back(main_window::ExpressionItem{to_shared_string(name), to_shared_string(type), true});
            }
            // clear the custom input for the next entry
            window->set_add_plot_custom_text(slint::SharedString(""));
            // rebuild the grid, respecting the active filter
            update_filtered();
        }

        void accept() {
            // gather the selected expressions by name
            std::set<AnyExpression*> selected;
            for (AnyExpression* expression : renderer.all_expressions()) {
                const auto name = expression_name(*expression);
                const auto it = std::find_if(m_all_items.begin(), m_all_items.end(), [&name](const main_window::ExpressionItem& item) { return item.selected && std::string(item.name) == name; });
                if (it != m_all_items.end())
                    selected.insert(expression);
            }
            // hide the panel before applying the selection
            window->set_add_plot_visible(false);
            // release the modal state held by the caller
            if (on_closed)
                on_closed();
            // apply the selection to the chart
            renderer.plot_chart_expressions(chart_index, selected);
        }

        void dismiss() {
            // hide the panel and drop the pending state
            window->set_add_plot_visible(false);
            // release the modal state held by the caller
            if (on_closed)
                on_closed();
        }

        // active filter text, reapplied when the item list changes
        std::string m_filter_query;
    };

    AddPlotDialogView::AddPlotDialogView(slint::ComponentHandle<main_window::MainWindow> main_window, ChartsRenderer& renderer) :
        m_impl(std::make_unique<Impl>(main_window, renderer)) {}

    AddPlotDialogView::~AddPlotDialogView() = default;

    void AddPlotDialogView::show_for_chart(float chart_position, const std::function<void()>& on_closed) {
        // remember the close notification for this show
        m_impl->on_closed = on_closed;
        // resolve the chart index from the panel position
        m_impl->chart_index = m_impl->renderer.position_to_index(chart_position);
        // rebuild the expression list for the current state
        m_impl->populate();
        // show the inline panel
        m_impl->window->set_add_plot_visible(true);
    }
} // namespace add_plot_dialog_view
