#include <format>
#include <limits>

#include <implot.h>
#include <spdlog/spdlog.h>

#include "chart.h"
#include "decimate.h"

static size_t binary_search(const std::span<const double>& data, const double value, bool ascending, int side) {
    // initialize lo and hi
    size_t lo = 0;
    size_t hi = data.size();
    // check data is ascending
    if (ascending) {
        // loop
        while (lo < hi) {
            // middle index
            size_t middle = (lo + hi) / 2;
            // compare middle value with target value
            if ((side == 1 && data[middle] < value) || (side == -1 && data[middle] <= value)) {
                // move lo up
                lo = middle + 1;
            }
            else {
                // move hi down
                hi = middle;
            }
        }
        // exit
        return lo;
    }
    // data is descending, loop
    while (lo < hi) {
        // middle index
        size_t middle = (lo + hi) / 2;
        // compare middle value with targe value
        if ((side == 1 && data[middle] > value) || (side == -1 and data[middle] >= value)) {
            // move lo up
            lo = middle + 1;
        }
        else {
            // move hi down
            hi = middle;
        }
    }
    // exit
    return lo;
}

Chart::Chart(ExpressionManager& expression_manager, const StepInformation& step_information, Expression<double>& abscissa, const std::string& abscissa_label, const AbscissaScale abscissa_scale, const size_t decimate_target)
    : m_expression_manager(expression_manager), m_step_information(step_information), m_abscissa(abscissa), m_abscissa_label(abscissa_label), m_abscissa_scale(abscissa_scale), m_decimate_target(decimate_target) {
}

const std::set<size_t>& Chart::selected_steps() {
    return m_selected_steps;
}

void Chart::plot_series(const std::set<AnyExpression*>& expressions) {
    // loop existing series to find those that need to be removed (those whose expression is not in the new expressions list)
    for (auto it = m_series.begin(); it != m_series.end();) {
        // check expression should be removed
        if (auto& [expression, ordinate_series] = it->second; !expressions.contains(expression)) {
            // loop ordinate series
            for (auto& value : ordinate_series | std::views::values) {
                // release axis
                release_y_axis(std::get<0>(value));
            }
            // ordinate name
            auto name = std::visit([](auto& e) { return e.name(); }, *expression);
            // log information
            spdlog::debug("Removing series for expression '{}' from chart", name);
            // remove it from series
            it = m_series.erase(it);
            // next
            continue;
        }
        // next
        ++it;
    }
    // current zoom window in abscissa values, None if not set
    double x_left_ratio = -1;
    double x_right_ratio = -1;
    // x0 and x1
    m_abscissa_left_value = x_left_ratio != -1 ? ratio_to_abscissa_value(x_left_ratio) : m_step_information.abscissa_left_value();
    m_abscissa_right_value = x_right_ratio != -1 ? ratio_to_abscissa_value(x_right_ratio) : m_step_information.abscissa_right_value();
    // loop expressions that need to be rendered
    for (AnyExpression* ordinate : expressions) {
        // ordinate name
        auto name = std::visit([](auto& e) { return e.name(); }, *ordinate);
        // lookup ordinate in series, create default if it does not exist
        auto [it, inserted] = m_series.try_emplace(name, OrdinateSeries(ordinate, {}));
        // ordinate series
        OrdinateSeries& ordinate_series = it->second;
        // process ordinate and find expressions to plot
        for (auto ordinate_variant : get_expressions_to_plot(ordinate)) {
            // lookup ordinate variant in series
            auto [it1, inserted1] = std::get<1>(ordinate_series).try_emplace(ordinate_variant, OrdinateVariantSeriesSteps());
            // lookup ordinate variant in series
            auto& [y_axis, rendered_series, min_value, max_value, color] = (it1->second);

            // remove steps that should not be rendered
            std::erase_if(rendered_series, [this](auto& pair) { return !m_selected_steps.contains(pair.first); });

            // process axis as needed
            if (y_axis == 0) {
                // find an axis for this unit
                y_axis = get_y_axis(ordinate_variant->unit());
                // no axis is available
                if (y_axis < 0) {
                    // log information
                    spdlog::warn("Cannot add series '{}' of measurement type '{}' to chart — maximum number of Y axes reached", ordinate_variant->name(), ordinate_variant->unit());
                    // remove ordinate variant from map since we are not plot it
                    std::get<1>(ordinate_series).erase(it1);
                    // exit loop
                    break;
                }
                // update axis
                std::get<0>(it1->second) = y_axis;
            }
            // loop steps to render
            for (auto step : m_selected_steps) {
                // skip step if already rendered
                // if (rendered_series.contains(step))
                //     continue;
                // plot step
                if (auto [ok, x, y, y_min_value, y_max_value] = plot_step(ordinate_variant, color, step, min_value, max_value, x_right_ratio, x_left_ratio); ok) {
                    // update min & max values
                    min_value = y_min_value;
                    max_value = y_max_value;
                    // append to rendered series
                    rendered_series.emplace(step, std::pair(x, y));
                }
            }
            // update min & max values
            std::get<2>(it1->second) = min_value;
            std::get<3>(it1->second) = max_value;
        }
    }
    // auto range axes
    auto_range();
}

void Chart::auto_range() {
    // skip if no series to render
    if (m_series.empty())
        return;
    // loop axes
    for (int i = 0; i < 3; i++) {
        // current
        auto& current = m_axes[i];
        // reset min & max values for axes
        current.min_value = std::numeric_limits<double>::max();
        current.max_value = std::numeric_limits<double>::min();
    }
    // loop rendered series
    for (auto& v : m_series | std::views::values) {
        // process series
        for (auto& v1 : std::get<1>(v) | std::views::values) {
            // extract axis, min and max values
            const int y_axis = std::get<0>(v1);
            const double min_value = std::get<2>(v1);
            const double max_value = std::get<3>(v1);
            // loop axes
            for (int i = 0; i < 3; i++) {
                // current
                auto& current = m_axes[i];
                // check this is the axis
                if (current.axis == y_axis) {
                    // update min & max values for axis
                    current.min_value = std::min(current.min_value, min_value);
                    current.max_value = std::max(current.max_value, max_value);
                    // exit
                    break;
                }
            }
        }
    }
}

std::tuple<bool, std::span<const double>, std::span<const double>, double, double> Chart::plot_step(Expression<double>* ordinate_variant, int color, size_t step, const double min_value, const double max_value, const double x_right_ratio, const double x_left_ratio) const {
    // step abscissa & ordinate values
    auto abscissa_values = m_abscissa.step_data(step);
    auto ordinate_values = ordinate_variant->step_data(step);
    // check we have a zoom to apply
    if (x_left_ratio >= 0 && x_right_ratio >= 0) {
        // find indexes for the new zoom window
        const auto& [first, last] = find_abscissa_indexes(abscissa_values, m_abscissa_left_value, m_abscissa_right_value);
        // abscissa values
        abscissa_values = abscissa_values | std::views::drop(first) | std::views::take(last - first);
        // ordinate values
        ordinate_values = ordinate_values | std::views::drop(first) | std::views::take(last - first);
    }
    // check vector length
    if (abscissa_values.empty())
        return {};
    auto [x_np, y_np] = decimate_xy(abscissa_values, ordinate_values, m_decimate_target, 1);
    // TODO: remove Inf values
    // check all values were non-finite after filtering
    if (x_np.empty() || y_np.empty())
        return {};
    // TODO: stroke style for current step
    // exit
    return {true, x_np, y_np, std::min(min_value, *std::ranges::min_element(y_np)), std::max(max_value, *std::ranges::max_element(y_np))};
}

void Chart::render() {
    // initialize plot, full area
    if (ImPlot::BeginPlot("My First Plot", ImVec2(-1, -1))) {
        // x axis
        ImPlot::SetupAxis(ImAxis_X1);
        // min and max values
        ImPlot::SetupAxisLimits(ImAxis_X1, m_abscissa_left_value, m_abscissa_right_value);
        // loop axis information
        for (int i = 0; i < 3; i++) {
            // axis info at i
            if (const AxisInformation& axis_info = m_axes[i]; axis_info.plots > 0) {
                spdlog::info("configuring axis: {}, min: {}, max: {}", axis_info.axis, axis_info.min_value, axis_info.max_value);
                // setup axis
                ImPlot::SetupAxis(axis_info.axis, nullptr, axis_info.axis != ImAxis_Y1 ? ImPlotAxisFlags_Opposite : ImPlotAxisFlags_None);
                // min and max values
                ImPlot::SetupAxisLimits(axis_info.axis, axis_info.min_value, axis_info.max_value);
            }
        }
        // finish setup
        ImPlot::SetupFinish();
        // loop series to render
        for (const auto& v : m_series | std::views::values) {
            // expression name
            auto name = std::visit([](auto& e) { return e.name(); }, *std::get<0>(v));
            // process series
            for (const auto& v1 : std::get<1>(v) | std::views::values) {
                // extract axis, steps and color
                const int y_axis = std::get<0>(v1);
                const auto& steps = std::get<1>(v1);
                // TODO: const int color = std::get<4>(v1);
                // set current y axis
                ImPlot::SetAxis(y_axis);
                // loop steps
                for (const auto& [x, y] : steps | std::views::values) {
                    // Draw the line chart
                    ImPlot::PlotLine(name.c_str(), x.data(), y.data(), x.size());
                }
            }
        }
        // Finalize the plot block
        ImPlot::EndPlot();
    }
}

std::vector<Expression<double>*> Chart::get_expressions_to_plot(AnyExpression* expression) const {
    // nothing to do on double expressions
    if (std::holds_alternative<Expression<double>>(*expression)) {
        // exit
        return {&std::get<Expression<double>>(*expression)};
    }
    // complex expression
    const auto& complex_expression = std::get<Expression<std::complex<double>>>(*expression);
    // magnitude
    auto magnitude_expression = m_expression_manager.evaluate(std::format("db({})", complex_expression.name()));
    if (!magnitude_expression)
        return {};
    // phase
    auto phase_expression = m_expression_manager.evaluate(std::format("phase({})", complex_expression.name()));
    if (!phase_expression)
        return {};
    // exit
    return {&std::get<Expression<double>>(*magnitude_expression), &std::get<Expression<double>>(*phase_expression)};
}

int Chart::get_y_axis(const std::string& unit) {
    // use pointer
    AxisInformation* available = nullptr;
    // loop axis information
    for (int i = 0; i < 3; i++) {
        // axis info at i
        AxisInformation& axis_info = m_axes[i];
        // check it is in use
        if (axis_info.plots > 0) {
            // check unit
            if (axis_info.unit == unit) {
                // increase ref count
                axis_info.plots += 1;
                // use it
                return axis_info.axis;
            }
            continue;
        }
        // use this axis if none exist for the unit
        if (available == nullptr)
            available = &axis_info;
    }
    // check we have an available axis
    if (available) {
        // log information
        spdlog::debug("Creating Y axis for measurement type: {}", unit.empty() ? "<no unit>" : unit);
        // use it
        available->plots = 1;
        available->unit = unit;
        available->min_value = std::numeric_limits<double>::max();
        available->max_value = std::numeric_limits<double>::min();
        // exit
        return available->axis;
    }
    return -1;
}

bool Chart::release_y_axis(const int axis) {
    // loop axis information
    for (int i = 0; i < 3; i++) {
        // check this is the axis to release
        if (AxisInformation& axis_info = m_axes[i]; axis_info.axis == axis) {
            // decrease ref counter
            axis_info.plots--;
            // check axis is no longer in use
            if (axis_info.plots == 0) {
                // log information
                spdlog::debug("Releasing Y axis for measurement type: {}", axis_info.unit.empty() ? "<no unit>" : axis_info.unit);
                // remove it from chart
                return true;
            }
            // keep it in chart
            return false;
        }
    }
    return false;
}

double Chart::ratio_to_abscissa_value(const double x_ratio) const {
    // make sure ratio is in the interval [0, 1]
    double percentage = std::max(0.0, std::min(1.0, x_ratio));
    // convert to abscissa value
    return m_step_information.abscissa_left_value() + percentage * (m_step_information.abscissa_right_value() - m_step_information.abscissa_left_value());
}

std::pair<size_t, size_t> Chart::find_abscissa_indexes(const std::span<const double>& abscissa, const double left_value, const double right_value) const {
    // ascending or descending abscissa
    const bool ascending = m_step_information.is_abscissa_ascending();
    // check abscissa is not within the zoom window
    if ((ascending && (abscissa[0] > right_value || abscissa[abscissa.size() - 1] < left_value)) || (!ascending && (abscissa[0] < right_value || abscissa[abscissa.size() - 1] > left_value)))
        return {0, 0};
    // find left and right indexes using binary search
    size_t left_index = binary_search(abscissa, left_value, ascending, 1);
    size_t right_index = binary_search(abscissa, right_value, ascending, -1);
    // return slice for values within the zoom window
    return {left_index, right_index};
}
