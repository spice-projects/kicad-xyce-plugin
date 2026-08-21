#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <slint.h>

#include <fft_dialog.h>

#include "../../expression/expression.h"
#include "../../fft/fft.h"
#include "../../util.h"
#include "fft_dialog_view.h"

namespace fft_dialog_view
{
    namespace
    {
        // preset np options exposed by the dialog, defaulting to the xyce default of 1024
        const std::vector<size_t> NP_OPTIONS = {256, 512, 1024, 2048, 4096, 8192, 16384};

        // index of the "Custom..." entry in the np combo box
        const size_t CUSTOM_NP_INDEX = NP_OPTIONS.size();

        // canonicalize a requested np value to the nearest power of two, rounding up on the midpoint
        size_t resolve_np(size_t requested_np) {
            // find the exponent bounds around the requested value
            const double log2_value = std::log2(static_cast<double>(requested_np));
            // lower power of two exponent
            const size_t lower_exponent = static_cast<size_t>(std::floor(log2_value));
            // upper power of two exponent
            const size_t upper_exponent = static_cast<size_t>(std::ceil(log2_value));
            // lower power of two value
            const size_t lower = static_cast<size_t>(1) << lower_exponent;
            // upper power of two value
            const size_t upper = static_cast<size_t>(1) << upper_exponent;
            // round down unless closer to the upper bound
            return (requested_np - lower < upper - requested_np) ? lower : upper;
        }

        slint::SharedString to_shared_string(std::string value) { return slint::SharedString(value); }
    } // namespace

    struct FftDialogView::Impl
    {
        // renderer used to look up expressions and the abscissa range
        ChartsRenderer& renderer;

        // the slint dialog window, created lazily on the first use
        slint::ComponentHandle<fft_dialog::FftDialog> dialog;

        // presenter that receives the accepted expressions and parameters
        MainWindowViewDefEvents* handler = nullptr;

        // notified on both accept and cancel, after the dialog window is hidden;
        // the caller releases the modal state from here
        std::function<void()> on_closed;

        // expression model shown in the dialog grid; the dialog uses the indices
        // of this model in the expression-clicked callback
        std::shared_ptr<slint::VectorModel<fft_dialog::ExpressionItem>> expressions;

        // eligible expressions known to the dialog with their selection state,
        // mirroring the wx expression selector panel (real, non-time-domain)
        std::vector<fft_dialog::ExpressionItem> m_all_items;

        // indices into m_all_items that pass the current filter
        std::vector<size_t> m_filtered_indices;

        // chart being edited
        size_t chart_index = 0;

        // full abscissa value range of the loaded file
        double m_min_abscissa_value = 0.0;
        double m_max_abscissa_value = 1.0;

        Impl(ChartsRenderer& renderer) :
            renderer(renderer), dialog(fft_dialog::FftDialog::create()) {
            expressions = std::make_shared<slint::VectorModel<fft_dialog::ExpressionItem>>();
            dialog->set_expressions(expressions);
            connect_callbacks();
        }

        void connect_callbacks() {
            dialog->on_filter_changed([this](slint::SharedString query) { apply_filter(query); });
            dialog->on_expression_clicked([this](int index) { toggle_expression(index); });
            dialog->on_accepted([this] { accept(); });
            dialog->on_dismissed([this] { dismiss(); });
        }

        void populate() {
            m_all_items.clear();
            // full abscissa value range used for the "All" and "Current Zoom" modes
            const auto [min_abscissa, max_abscissa] = renderer.abscissa_range();
            m_min_abscissa_value = min_abscissa;
            m_max_abscissa_value = max_abscissa;
            // FFT is only for real, non-time-domain expressions, so skip the rest
            for (AnyExpression* expression : renderer.all_expressions()) {
                // skip non-real expressions
                if (!std::holds_alternative<Expression<double>>(*expression))
                    continue;
                // skip time-domain expressions (unit "s")
                if (std::get<Expression<double>>(*expression).unit() == "s")
                    continue;
                m_all_items.push_back(fft_dialog::ExpressionItem{to_shared_string(expression_name(*expression)), to_shared_string(expression_type(*expression)), false});
            }
            // check the chart's current selection
            const auto selected = renderer.chart_selected_expressions(chart_index);
            // mark the currently plotted expressions as selected
            for (auto& item : m_all_items) {
                item.selected = std::any_of(selected.begin(), selected.end(), [&item](AnyExpression* expression) { return expression_name(*expression) == std::string(item.name); });
            }
            // reset the filter and error state
            m_filter_query.clear();
            dialog->set_filter_text(slint::SharedString(""));
            dialog->set_show_error(false);
            dialog->set_error_message(slint::SharedString(""));
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

        // show a validation error inline and keep the dialog open
        void set_error(const char* message) {
            dialog->set_show_error(true);
            dialog->set_error_message(slint::SharedString(message));
        }

        // selected expressions mapped back to the expression manager pointers
        std::vector<AnyExpression*> selected_expressions() const {
            std::vector<AnyExpression*> selected;
            for (AnyExpression* expression : renderer.all_expressions()) {
                const auto name = expression_name(*expression);
                const auto it = std::find_if(m_all_items.begin(), m_all_items.end(), [&name](const fft_dialog::ExpressionItem& item) { return item.selected && std::string(item.name) == name; });
                if (it != m_all_items.end())
                    selected.push_back(expression);
            }
            return selected;
        }

        void accept() {
            // build the FFT parameters from the dialog state; validation failures
            // keep the dialog open with an inline error, mirroring the wx dialog
            fft::FftParameters params;
            // resolve the data range
            switch (dialog->get_range_mode()) {
            case 0: // all
            case 1: // current zoom (no zoom support in the slint renderer yet, use the full range)
                params.start = m_min_abscissa_value;
                params.stop = m_max_abscissa_value;
                break;
            case 2: { // custom
                const std::string from_str = std::string(dialog->get_custom_from());
                const std::string to_str = std::string(dialog->get_custom_to());
                try {
                    params.start = std::stod(from_str);
                    params.stop = std::stod(to_str);
                }
                catch (...) {
                    set_error("Invalid custom range values.");
                    return;
                }
                // validate the range order
                if (params.start >= params.stop) {
                    set_error("Custom range 'from' must be less than 'to'.");
                    return;
                }
                break;
            }
            default:
                params.start = m_min_abscissa_value;
                params.stop = m_max_abscissa_value;
                break;
            }
            // resolve the window function selection
            switch (dialog->get_window_index()) {
            case 0:
                params.window = fft::WindowFunction::RECTANGULAR;
                break;
            case 1:
                params.window = fft::WindowFunction::HAMMING;
                break;
            case 2:
                params.window = fft::WindowFunction::HANNING;
                break;
            case 3:
                params.window = fft::WindowFunction::BLACKMAN;
                break;
            default:
                params.window = fft::WindowFunction::HANNING;
                break;
            }
            // resolve the output type selection
            switch (dialog->get_output_index()) {
            case 0:
                params.output = fft::FftOutput::MAGNITUDE;
                break;
            case 1:
                params.output = fft::FftOutput::MAGNITUDE_DB;
                break;
            case 2:
                params.output = fft::FftOutput::PHASE;
                break;
            default:
                params.output = fft::FftOutput::MAGNITUDE;
                break;
            }
            // resolve the format selection
            switch (dialog->get_format_index()) {
            case 0:
                params.format = fft::FftFormat::NORM;
                break;
            case 1:
                params.format = fft::FftFormat::UNORM;
                break;
            default:
                params.format = fft::FftFormat::NORM;
                break;
            }
            // read the keep-dc value
            params.keep_dc = dialog->get_keep_dc();
            // handle the custom np input
            if (dialog->get_np_index() == static_cast<int>(CUSTOM_NP_INDEX)) {
                // validate the custom np input
                const std::string np_str = std::string(dialog->get_custom_np());
                if (np_str.empty()) {
                    set_error("Please enter a custom number of FFT points.");
                    return;
                }
                try {
                    params.np = static_cast<size_t>(std::stoll(np_str));
                }
                catch (...) {
                    set_error("Invalid number of FFT points.");
                    return;
                }
                // validate np is at least 4
                if (params.np < 4) {
                    set_error("Number of FFT points must be at least 4.");
                    return;
                }
                // canonicalize to the nearest power of two and show the resolved value
                params.np = resolve_np(params.np);
                dialog->set_custom_np(slint::SharedString(std::to_string(params.np)));
            }
            else {
                // use the preset np value
                params.np = NP_OPTIONS[static_cast<size_t>(dialog->get_np_index())];
            }
            // gather the selected expressions before hiding the dialog
            std::vector<AnyExpression*> selected = selected_expressions();
            // hide the dialog before delivering the result
            dialog->hide();
            // release the modal state held by the caller
            if (on_closed)
                on_closed();
            // deliver the accepted result to the presenter, which runs the transform
            if (handler)
                handler->on_fft_dialog_result(std::move(selected), params);
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

    FftDialogView::FftDialogView(ChartsRenderer& renderer) :
        m_impl(std::make_unique<Impl>(renderer)) {}

    FftDialogView::~FftDialogView() = default;

    slint::Window& FftDialogView::window() {
        // expose the dialog window (dialog must be shown first)
        return m_impl->dialog->window();
    }

    void FftDialogView::show_for_chart(size_t chart_index, MainWindowViewDefEvents& handler, const std::function<void()>& on_closed) {
        // remember the result delivery and close notification for this show
        m_impl->handler = &handler;
        m_impl->on_closed = on_closed;
        // remember the chart whose plotted expressions are pre-selected
        m_impl->chart_index = chart_index;
        // rebuild the expression list for the current state
        m_impl->populate();
        // show the dialog window
        m_impl->dialog->show();
    }
} // namespace fft_dialog_view
