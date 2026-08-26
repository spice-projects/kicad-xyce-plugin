#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <slint.h>

#include <step_tool_dialog.h>

#include "step_tool_dialog_view.h"

namespace step_tool_dialog_view
{
    namespace
    {
        slint::SharedString to_shared_string(std::string value) { return slint::SharedString(value); }

        // format a step parameter value for display in the steps grid
        slint::SharedString format_value(double value) {
            std::ostringstream oss;
            oss << value;
            return to_shared_string(oss.str());
        }
    } // namespace

    struct StepToolDialogView::Impl
    {
        // renderer used to look up the step information and apply the selection
        ChartsRenderer& renderer;

        // the slint dialog window, created lazily on the first use
        slint::ComponentHandle<step_tool_dialog::StepToolDialog> dialog;

        // notified on both accept and cancel, after the dialog window is hidden;
        // the caller releases the modal state from here
        std::function<void()> on_closed;

        // step model shown in the dialog grid; the dialog uses the indices of
        // this model in the step-toggled callback
        std::shared_ptr<slint::VectorModel<step_tool_dialog::StepItem>> steps;

        // steps known to the dialog with their selection state
        std::vector<step_tool_dialog::StepItem> m_all_items;

        // chart being edited
        size_t chart_index = 0;

        Impl(ChartsRenderer& renderer) :
            renderer(renderer), dialog(step_tool_dialog::StepToolDialog::create()) {
            steps = std::make_shared<slint::VectorModel<step_tool_dialog::StepItem>>();
            dialog->set_steps(steps);
            connect_callbacks();
        }

        void connect_callbacks() {
            dialog->on_select_all([this] { select_all(); });
            dialog->on_clear_all([this] { clear_all(); });
            dialog->on_invert_selection([this] { invert_selection(); });
            dialog->on_step_toggled([this](int index) { toggle_step(index); });
            dialog->on_accepted([this] { accept(); });
            dialog->on_dismissed([this] { dismiss(); });
        }

        void populate() {
            // rebuild the step grid from the loaded step information
            m_all_items.clear();
            const StepInformation* step_information = renderer.step_information();
            if (step_information != nullptr) {
                // the chart's current step selection
                const std::set<size_t> selected = renderer.chart_selected_steps(chart_index);
                // one row per step: the index column plus the parameter values
                const auto& all_values = step_information->values();
                for (size_t step_index = 0; step_index < all_values.size(); ++step_index) {
                    // format the parameter values of this step
                    auto values = std::make_shared<slint::VectorModel<slint::SharedString>>();
                    for (double value : all_values[step_index])
                        values->push_back(format_value(value));
                    // build the row, pre-checking the chart's selection
                    m_all_items.push_back(step_tool_dialog::StepItem{static_cast<int>(step_index), std::move(values), selected.contains(step_index)});
                }
            }
            // expose the sweep parameter names as the column headers
            if (step_information != nullptr) {
                auto keys = std::make_shared<slint::VectorModel<slint::SharedString>>();
                for (const std::string& key : step_information->keys())
                    keys->push_back(to_shared_string(key));
                dialog->set_keys(keys);
            }
            // rebuild the grid and the selection count
            update_rows();
        }

        void update_rows() {
            // rebuild the model rows shown in the grid
            steps->clear();
            for (const auto& item : m_all_items)
                steps->push_back(item);
            // update the selection count label
            dialog->set_selection_count(slint::SharedString("Selected " + std::to_string(selected_count()) + " / " + std::to_string(m_all_items.size())));
        }

        size_t selected_count() const {
            // count the checked rows
            size_t count = 0;
            for (const auto& item : m_all_items)
                count += item.selected ? 1 : 0;
            return count;
        }

        void select_all() {
            // check every step
            for (auto& item : m_all_items)
                item.selected = true;
            update_rows();
        }

        void clear_all() {
            // uncheck every step
            for (auto& item : m_all_items)
                item.selected = false;
            update_rows();
        }

        void invert_selection() {
            // flip the selection of every step
            for (auto& item : m_all_items)
                item.selected = !item.selected;
            update_rows();
        }

        void toggle_step(int index) {
            // map the model row back to the step and flip the selection
            if (index < 0 || static_cast<size_t>(index) >= m_all_items.size())
                return;
            m_all_items[static_cast<size_t>(index)].selected = !m_all_items[static_cast<size_t>(index)].selected;
            // keep the grid in sync
            steps->set_row_data(static_cast<size_t>(index), m_all_items[static_cast<size_t>(index)]);
            // update the selection count label
            dialog->set_selection_count(slint::SharedString("Selected " + std::to_string(selected_count()) + " / " + std::to_string(m_all_items.size())));
        }

        void accept() {
            // gather the selected step indices
            std::set<size_t> selected;
            for (const auto& item : m_all_items)
                if (item.selected)
                    selected.insert(static_cast<size_t>(item.index));
            // hide the dialog before applying the selection
            dialog->hide();
            // release the modal state held by the caller
            if (on_closed)
                on_closed();
            // apply the selection to the chart
            renderer.set_chart_selected_steps(chart_index, selected);
        }

        void dismiss() {
            // hide the dialog and drop the pending state
            dialog->hide();
            // release the modal state held by the caller
            if (on_closed)
                on_closed();
        }
    };

    StepToolDialogView::StepToolDialogView(ChartsRenderer& renderer) :
        m_impl(std::make_unique<Impl>(renderer)) {}

    StepToolDialogView::~StepToolDialogView() = default;

    slint::Window& StepToolDialogView::window() {
        // expose the dialog window (dialog must be shown first)
        return m_impl->dialog->window();
    }

    void StepToolDialogView::show_for_chart(size_t chart_index, const std::function<void()>& on_closed) {
        // remember the close notification for this show
        m_impl->on_closed = on_closed;
        // remember the chart whose step selection is pre-checked
        m_impl->chart_index = chart_index;
        // rebuild the step grid for the current state
        m_impl->populate();
        // show the dialog window
        m_impl->dialog->show();
    }
} // namespace step_tool_dialog_view
