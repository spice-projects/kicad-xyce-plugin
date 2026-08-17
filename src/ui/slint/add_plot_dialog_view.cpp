#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

#include <slint.h>

#include <add_plot_dialog.h>

#include "../../util.h"
#include "add_plot_dialog_view.h"

namespace add_plot_dialog_view
{
    namespace
    {
        slint::SharedString to_shared_string(std::string value) { return slint::SharedString(value); }
    } // namespace

    struct AddPlotDialogView::Impl
    {
        // renderer used to look up expressions and apply the selection
        ChartsRenderer& renderer;

        // the slint dialog window, created lazily on the first use
        slint::ComponentHandle<add_plot_dialog::AddPlotDialog> dialog;

        // notified on both accept and cancel, after the dialog window is hidden;
        // the caller releases the modal state from here
        std::function<void()> on_closed;

        // expression model shown in the dialog grid; the dialog uses the indices
        // of this model in the expression-clicked callback
        std::shared_ptr<slint::VectorModel<add_plot_dialog::ExpressionItem>> expressions;

        // all expressions known to the dialog with their selection state,
        // mirroring the wx expression selector panel
        std::vector<add_plot_dialog::ExpressionItem> m_all_items;

        // indices into m_all_items that pass the current filter
        std::vector<size_t> m_filtered_indices;

        // chart being edited
        size_t chart_index = 0;

        Impl(ChartsRenderer& renderer) :
            renderer(renderer), dialog(add_plot_dialog::AddPlotDialog::create()) {
            expressions = std::make_shared<slint::VectorModel<add_plot_dialog::ExpressionItem>>();
            dialog->set_expressions(expressions);
            connect_callbacks();
        }

        void connect_callbacks() {
            dialog->on_filter_changed([this](slint::SharedString query) { apply_filter(query); });
            dialog->on_expression_clicked([this](int index) { toggle_expression(index); });
            dialog->on_custom_add([this] { add_custom_expression(); });
            dialog->on_accepted([this] { accept(); });
            dialog->on_dismissed([this] { dismiss(); });
        }

        void populate() {
            m_all_items.clear();
            // build the initial expression list from the expression manager
            for (AnyExpression* expression : renderer.all_expressions()) {
                const auto name = expression_name(*expression);
                m_all_items.push_back(add_plot_dialog::ExpressionItem{to_shared_string(name), to_shared_string(expression_type(*expression)), false});
            }
            // check the chart's current selection
            const auto selected = renderer.chart_selected_expressions(chart_index);
            // mark the currently plotted expressions as selected
            for (auto& item : m_all_items) {
                item.selected = std::any_of(selected.begin(), selected.end(), [&item](AnyExpression* expression) { return expression_name(*expression) == std::string(item.name); });
            }
            // reset the filter and error state
            dialog->set_filter_text(slint::SharedString(""));
            dialog->set_show_error(false);
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

        void add_custom_expression() {
            // read the custom expression text
            const std::string text = std::string(dialog->get_custom_text());
            const auto trimmed_start = text.find_first_not_of(" \t");
            const auto trimmed_end = text.find_last_not_of(" \t");
            // ignore empty input
            if (trimmed_start == std::string::npos)
                return;
            const std::string trimmed = text.substr(trimmed_start, trimmed_end - trimmed_start + 1);
            // evaluate the custom expression
            AnyExpression* expression = renderer.evaluate_expression(trimmed);
            if (expression == nullptr) {
                dialog->set_show_error(true);
                return;
            }
            dialog->set_show_error(false);
            // derive name and type
            const std::string name = expression_name(*expression);
            const std::string type = expression_type(*expression);
            // search for an existing item with the same name
            const auto it = std::find_if(m_all_items.begin(), m_all_items.end(), [&name](const add_plot_dialog::ExpressionItem& item) { return std::string(item.name) == name; });
            if (it != m_all_items.end()) {
                // expression already exists, mark it as selected
                it->selected = true;
            }
            else {
                // append the new expression, selected
                m_all_items.push_back(add_plot_dialog::ExpressionItem{to_shared_string(name), to_shared_string(type), true});
            }
            // clear the custom input for the next entry
            dialog->set_custom_text(slint::SharedString(""));
            // rebuild the grid, respecting the active filter
            update_filtered();
        }

        void accept() {
            // gather the selected expressions by name
            std::set<AnyExpression*> selected;
            for (AnyExpression* expression : renderer.all_expressions()) {
                const auto name = expression_name(*expression);
                const auto it = std::find_if(m_all_items.begin(), m_all_items.end(), [&name](const add_plot_dialog::ExpressionItem& item) { return item.selected && std::string(item.name) == name; });
                if (it != m_all_items.end())
                    selected.insert(expression);
            }
            // hide the dialog before applying the selection
            dialog->hide();
            // release the modal state held by the caller
            if (on_closed)
                on_closed();
            // apply the selection to the chart
            renderer.plot_chart_expressions(chart_index, selected);
        }

        void dismiss() {
            // hide the dialog and drop the pending state
            dialog->hide();
            // release the modal state held by the caller
            if (on_closed)
                on_closed();
        }

        // active filter text, reapplied when the item list changes
        std::string m_filter_query;
    };

    AddPlotDialogView::AddPlotDialogView(ChartsRenderer& renderer) :
        m_impl(std::make_unique<Impl>(renderer)) {}

    AddPlotDialogView::~AddPlotDialogView() = default;

    slint::Window& AddPlotDialogView::window() {
        // expose the dialog window (dialog must be shown first)
        return m_impl->dialog->window();
    }

    void AddPlotDialogView::show_for_chart(float chart_position, const std::function<void()>& on_closed) {
        // remember the close notification for this show
        m_impl->on_closed = on_closed;
        // resolve the chart index from the panel position
        m_impl->chart_index = m_impl->renderer.position_to_index(chart_position);
        // rebuild the expression list for the current state
        m_impl->populate();
        // show the dialog window
        m_impl->dialog->show();
    }
} // namespace add_plot_dialog_view
