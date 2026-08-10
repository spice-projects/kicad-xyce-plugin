#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <ranges>
#include <set>
#include <span>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <imgui.h>
#include <implot.h>
#include <spdlog/spdlog.h>

#include "chart.h"
#include "decimate.h"

namespace
{
    // default series color palette
    const std::vector SERIES_COLOR_PALETTE = {
        ImVec4(247.0f / 255.0f, 127.0f / 255.0f, 0.0f / 255.0f, 1.0f), // #f77f00
        ImVec4(58.0f / 255.0f, 134.0f / 255.0f, 1.0f, 1.0f), // #3a86ff
        ImVec4(1.0f, 221.0f / 255.0f, 0.0f / 255.0f, 1.0f), // #ffdd00
        ImVec4(155.0f / 255.0f, 93.0f / 255.0f, 229.0f / 255.0f, 1.0f), // #9b5de5
        ImVec4(0.0f / 255.0f, 180.0f / 255.0f, 216.0f / 255.0f, 1.0f), // #00b4d8
        ImVec4(1.0f, 143.0f / 255.0f, 163.0f / 255.0f, 1.0f), // #ff8fa3
        ImVec4(128.0f / 255.0f, 1.0f, 114.0f / 255.0f, 1.0f), // #80ff72
        ImVec4(224.0f / 255.0f, 64.0f / 255.0f, 251.0f / 255.0f, 1.0f), // #e040fb
        ImVec4(1.0f, 67.0f / 255.0f, 101.0f / 255.0f, 1.0f), // #ff4365
        ImVec4(0.0f / 255.0f, 245.0f / 255.0f, 212.0f / 255.0f, 1.0f), // #00f5d4
        ImVec4(244.0f / 255.0f, 162.0f / 255.0f, 97.0f / 255.0f, 1.0f), // #f4a261
        ImVec4(138.0f / 255.0f, 201.0f / 255.0f, 38.0f / 255.0f, 1.0f), // #8ac926
        ImVec4(76.0f / 255.0f, 201.0f / 255.0f, 240.0f / 255.0f, 1.0f), // #4cc9f0
        ImVec4(187.0f / 255.0f, 222.0f / 255.0f, 251.0f / 255.0f, 1.0f), // #bbdefb
    };

    constexpr ImPlotFlags PLOT_FLAGS = (ImPlotFlags_CanvasOnly ^ ImPlotFlags_NoLegend) | ImPlotFlags_NoInputs | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect;

    // ImPlot forward transform for base-2 logarithmic axes (mirrors ImPlot's Log10 transform clamping)
    static double log2_forward_transform(const double value, void*) {
        // clamp non-positive values to the smallest positive double
        return std::log2(value <= 0.0 ? (std::numeric_limits<double>::min)() : value);
    }

    // ImPlot inverse transform for base-2 logarithmic axes
    static double exp2_inverse_transform(const double value, void*) {
        // convert plot space value to abscissa value
        return std::exp2(value);
    }

    // compute power-of-two major tick values for a log2-scaled axis
    static std::vector<double> compute_log2_major_ticks(const double x_left, const double x_right, const int max_ticks) {
        // exit with empty vector if range is invalid
        if (x_left <= 0.0 || x_right <= 0.0)
            return {};
        // compute exponent range covering the visible window
        const double log_min = std::log2(x_left);
        const double log_max = std::log2(x_right);
        const int exp_min = static_cast<int>(std::floor(log_min));
        const int exp_max = static_cast<int>(std::ceil(log_max));
        const int num_octaves = exp_max - exp_min;
        // step: number of octaves between major ticks, adapt to available pixel budget
        int exp_step = 1;
        if (num_octaves > max_ticks)
            exp_step = static_cast<int>(std::ceil(static_cast<double>(num_octaves) / max_ticks));
        // build major tick list
        std::vector<double> result;
        for (int e = exp_min; e <= exp_max; e += exp_step) {
            const double tick_value = std::exp2(e);
            if (tick_value >= x_left - 1e-15 && tick_value <= x_right + 1e-15)
                result.push_back(tick_value);
        }
        return result;
    }

    static double interpolate_y(const View<double>& x_data, const View<double>& y_data, const double x, const bool ascending) {
        const size_t n = x_data.size();
        if (n == 0)
            return 0.0;
        // clamp to range
        if (ascending) {
            // check if x is lower than the first x value
            if (x <= x_data[0])
                return y_data[0];
            // check if x is greater than the last x value
            if (x >= x_data[n - 1])
                return y_data[n - 1];
        }
        else {
            // check if x is greater than the first x value
            if (x >= x_data[0])
                return y_data[0];
            // check if x is lower than the last x value
            if (x <= x_data[n - 1])
                return y_data[n - 1];
        }
        // index-based binary search for the bracketing interval (stride 1, contiguous data)
        size_t idx;
        {
            // initialize low and high indexes for binary search
            size_t low = 1;
            size_t high = n - 1;
            // binary search loop
            while (low < high) {
                // middle element
                const size_t mid = low + (high - low) / 2;
                // ascending data: first index whose value is >= x, descending data: first index whose value is <= x
                if (ascending ? (x_data[mid] < x) : (x_data[mid] > x))
                    low = mid + 1;
                else
                    high = mid;
            }
            // the first sample at or past x (the bracketing index)
            idx = low;
        }
        // linear interpolation
        const double x0 = x_data[idx - 1];
        const double x1 = x_data[idx];
        const double y0 = y_data[idx - 1];
        const double y1 = y_data[idx];
        const double t = (x - x0) / (x1 - x0);
        return y0 + t * (y1 - y0);
    }

    static double interpolate_abscissa(const double ratio, const double left_value, const double right_value, const AbscissaScale scale) {
        // logarithmic scale: interpolate geometrically over the range
        if (scale != AbscissaScale::LINEAR && left_value > 0.0 && right_value > 0.0 && left_value != right_value)
            return left_value * std::pow(right_value / left_value, ratio);
        // linear scale: interpolate linearly over the range
        return left_value + ratio * (right_value - left_value);
    }

    static int metric_formatter(const double value, char* buff, const int size, const void* data) {
        // unit
        const auto unit = static_cast<const char*>(data);
        // shared formatter renders value, space, prefix and unit (SI)
        const std::string formatted = Chart::format_metric(value, unit);
        return snprintf(buff, size, "%s", formatted.c_str());
    }

    static std::vector<Expression<double>*> get_expressions_to_plot(ExpressionManager* expression_manager, AnyExpression* expression) {
        // nothing to do on double expressions
        if (std::holds_alternative<Expression<double>>(*expression)) {
            // exit
            return {&std::get<Expression<double>>(*expression)};
        }
        // complex expression
        const auto& complex_expression = std::get<Expression<std::complex<double>>>(*expression);
        // magnitude
        auto magnitude_expression = expression_manager->evaluate(std::format("db({})", complex_expression.name()));
        if (!magnitude_expression)
            return {};
        // phase
        auto phase_expression = expression_manager->evaluate(std::format("phase({})", complex_expression.name()));
        if (!phase_expression)
            return {};
        // exit
        return {&std::get<Expression<double>>(*magnitude_expression), &std::get<Expression<double>>(*phase_expression)};
    }
} // namespace

std::string Chart::format_metric(const double value, const std::string_view unit) {
    // dividers
    static constexpr double v[] = {1e9, 1e6, 1e3, 1.0, 1e-3, 1e-6, 1e-9, 1e-12};
    // prefixes (SI, µ for micro)
    static constexpr const char* p[] = {"G", "M", "k", "", "m", "µ", "n", "p"};
    // zero value: prefix and unit, no separator
    if (std::fabs(value) < 1e-12)
        return std::format("0{}", unit);
    // loop scales
    for (int i = 0; i < 8; ++i) {
        // check we should format value with this scale
        if (std::fabs(value) >= v[i]) {
            // scaled mantissa for this divider
            double scaled = value / v[i];
            // mantissa rounding up to the next decade belongs to the next prefix (avoids 1e+03uA for 1mA)
            if (std::fabs(scaled) >= 999.5 && i > 0) {
                scaled /= 1000.0;
                return std::format("{:.3g} {}{}", scaled, p[i - 1], unit);
            }
            return std::format("{:.3g} {}{}", scaled, p[i], unit);
        }
    }
    return std::format("{:.3g} {}{}", value / v[7], p[7], unit);
}

Chart::Chart(ExpressionManager* expression_manager, const StepInformation* step_information, const AbscissaScale abscissa_scale, const size_t decimate_target) :
    m_expression_manager(expression_manager), m_step_information(step_information), m_abscissa_scale(abscissa_scale), m_decimate_target(decimate_target) {
    // abscissa
    auto& abscissa = expression_manager->abscissa();
    // abscissa name & unit
    m_abscissa_name = abscissa.name();
    m_abscissa_unit = abscissa.unit();
}

const std::set<size_t>& Chart::selected_steps() {
    // return selected steps
    return m_selected_steps;
}

std::vector<AnyExpression*> Chart::selected_expressions() {
    // result
    std::vector<AnyExpression*> result;
    // allocate vector
    result.reserve(m_series.size());
    // loop series
    for (const auto& [expression, _] : m_series | std::views::values) {
        // append expression
        result.push_back(expression);
    }
    // exit
    return result;
}

void Chart::render(const std::tuple<float, float, float, float>& selection) {
    // initialize plot, full area
    if (ImPlot::BeginPlot("My First Plot", ImVec2(-1, -1), PLOT_FLAGS)) {
        // x axis
        ImPlot::SetupAxis(ImAxis_X1);
        // format
        ImPlot::SetupAxisFormat(ImAxis_X1, reinterpret_cast<ImPlotFormatter>(metric_formatter), (void*)m_abscissa_unit.c_str());
        // abscissa scale
        if (m_abscissa_scale == AbscissaScale::DECADE) {
            // log10 abscissa axis
            ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
        }
        else if (m_abscissa_scale == AbscissaScale::OCTAVE) {
            // log2 abscissa axis
            ImPlot::SetupAxisScale(ImAxis_X1, log2_forward_transform, exp2_inverse_transform);
        }
        // abscissa limits clamped above zero for logarithmic scales
        double x_left_value = m_abscissa_left_value;
        double x_right_value = m_abscissa_right_value;
        if (m_abscissa_scale != AbscissaScale::LINEAR) {
            // clamp non-positive left limit to a fraction of the right limit
            if (x_left_value <= 0.0)
                x_left_value = x_right_value > 0.0 ? x_right_value / 1e6 : 1.0;
            // clamp non-positive right limit to a multiple of the left limit
            if (x_right_value <= 0.0)
                x_right_value = x_left_value > 0.0 ? x_left_value * 1e6 : 1.0;
            // avoid a degenerate zero-width range
            if (x_left_value == x_right_value)
                x_right_value = x_left_value * 2.0;
        }
        // min and max values
        ImPlot::SetupAxisLimits(ImAxis_X1, x_left_value, x_right_value, ImPlotCond_Always);
        // log2 custom ticks for OCTAVE scale
        if (m_abscissa_scale == AbscissaScale::OCTAVE) {
            // estimate plot width from last frame (fallback to 800 pixels)
            const float plot_width = std::get<2>(m_plot_rect) - std::get<0>(m_plot_rect);
            const int max_ticks = (std::max)(2, static_cast<int>(std::lround((plot_width > 0.0f ? plot_width : 800.0f) * 0.01f)));
            auto log2_ticks = compute_log2_major_ticks(x_left_value, x_right_value, max_ticks);
            if (!log2_ticks.empty())
                ImPlot::SetupAxisTicks(ImAxis_X1, log2_ticks.data(), static_cast<int>(log2_ticks.size()), nullptr, false);
        }
        // loop axis information
        for (const auto& axis_info : m_axes) {
            // axis info at i (Y1 is always enabled)
            if (axis_info.axis == ImAxis_Y1 || axis_info.plots > 0) {
                // setup axis
                ImPlot::SetupAxis(axis_info.axis, nullptr, axis_info.axis != ImAxis_Y1 ? ImPlotAxisFlags_Opposite : ImPlotAxisFlags_None);
                // format
                ImPlot::SetupAxisFormat(axis_info.axis, reinterpret_cast<ImPlotFormatter>(metric_formatter), (void*)axis_info.unit.c_str());
                // min and max values
                ImPlot::SetupAxisLimits(axis_info.axis, axis_info.plot_min_value, axis_info.plot_max_value, ImPlotCond_Always);
            }
        }
        ImPlot::SetupLegend(ImPlotLocation_South, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);
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
                const auto& color = std::get<4>(v1);
                // set current y axis
                ImPlot::SetAxis(y_axis);
                // style
                ImPlotSpec spec;
                spec.LineColor = color;
                spec.LineWeight = 2.0f;
                // loop steps
                for (const auto& [x, y] : steps | std::views::values) {
                    // draw the line chart
                    ImPlot::PlotLine(name.c_str(), x.data(), y.data(), static_cast<int>(x.size()), spec);
                }
            }
        }
        // current rectangle (zoom selection)
        if (const auto [x1, y1, x2, y2] = selection; x1 >= 0 && y1 >= 0 && x2 >= 0 && y2 >= 0) {
            // get the draw list
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            // draw rect
            draw_list->AddRect(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(255, 0, 0, 255), 0.0, 2.0);
        }
        // plot position and size
        ImVec2 plot_position = ImPlot::GetPlotPos();
        ImVec2 plot_size = ImPlot::GetPlotSize();
        // update plot rect
        m_plot_rect = {plot_position.x, plot_position.y, plot_position.x + plot_size.x, plot_position.y + plot_size.y};
        // finalize the plot block
        ImPlot::EndPlot();
    }
}

void Chart::plot_series(const std::set<AnyExpression*>& expressions) {
    // loop existing series to find those that need to be removed (those whose expression is not in the new expressions list)
    for (auto it = m_series.begin(); it != m_series.end();) {
        // check expression should be removed
        if (auto& [expression, ordinate_series] = it->second; !expressions.contains(expression)) {
            // loop ordinate series
            for (auto& [key, value] : ordinate_series) {
                // log information
                spdlog::info("Removing series for expression '{}' from chart", key->name());
                // release axis
                release_y_axis(std::get<0>(value));
            }
            // remove it from series
            it = m_series.erase(it);
            // next
            continue;
        }
        // next
        ++it;
    }
    // current zoom window in abscissa values, None if not set
    const double x_left_ratio = std::get<0>(m_zoom_window);
    const double x_right_ratio = std::get<2>(m_zoom_window);
    // x0 and x1
    m_abscissa_left_value = x_left_ratio != -1 ? ratio_to_abscissa_value(x_left_ratio) : m_step_information->abscissa_left_value();
    m_abscissa_right_value = x_right_ratio != -1 ? ratio_to_abscissa_value(x_right_ratio) : m_step_information->abscissa_right_value();
    // loop expressions that need to be rendered
    for (AnyExpression* ordinate : expressions) {
        // ordinate name
        auto name = std::visit([](auto& e) { return e.name(); }, *ordinate);
        // lookup ordinate in series, create default if it does not exist
        auto [it, inserted] = m_series.try_emplace(name, OrdinateSeries(ordinate, OrdinateVariantSeries()));
        // ordinate series
        OrdinateSeries& ordinate_series = it->second;
        // process ordinate and find expressions to plot
        for (auto ordinate_variant : get_expressions_to_plot(m_expression_manager, ordinate)) {
            // lookup ordinate variant in series
            auto [it1, inserted1] = std::get<1>(ordinate_series).try_emplace(ordinate_variant, OrdinateVariantSeriesSteps());
            // lookup ordinate variant in series
            auto& [y_axis, rendered_series, min_value, max_value, color] = (it1->second);
            // loop rendered steps
            for (auto it2 = rendered_series.begin(); it2 != rendered_series.end();) {
                // check step in selected steps
                if (!m_selected_steps.contains(it2->first)) {
                    // log information
                    spdlog::info("Removing series for expression [{}] from chart, step: {}", ordinate_variant->name(), it2->first);
                    // remove it
                    it2 = rendered_series.erase(it2);
                    // next
                    continue;
                }
                // next
                ++it2;
            }
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
            // process color
            if (color.x == 0 && color.y == 0 && color.z == 0 && color.w == 0) {
                // assign next color in palette
                color = SERIES_COLOR_PALETTE[m_next_color_index % SERIES_COLOR_PALETTE.size()];
                // increment color index
                m_next_color_index++;
            }
            // loop steps to render
            for (size_t step : m_selected_steps) {
                // skip step if already rendered
                if (rendered_series.contains(step))
                    continue;
                // plot step
                if (auto [ok, x, y, y_min_value, y_max_value] = plot_step(ordinate_variant, step, min_value, max_value, x_right_ratio, x_left_ratio); ok) {
                    // update min & max values
                    min_value = y_min_value;
                    max_value = y_max_value;
                    // append to rendered series
                    rendered_series.emplace(step, std::pair(std::move(x), std::move(y)));
                }
            }
            // update min & max values
            std::get<2>(it1->second) = min_value;
            std::get<3>(it1->second) = max_value;
            // update color
            std::get<4>(it1->second) = color;
        }
    }
    // auto range axes
    auto_range();
}

void Chart::auto_range() {
    // skip if no series to render
    if (!m_series.empty()) {
        // loop axes
        for (auto& axis_info : m_axes) {
            // reset min & max values for axes
            axis_info.min_value = (std::numeric_limits<double>::max)();
            axis_info.max_value = -(std::numeric_limits<double>::max)();
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
                for (auto& axis_info : m_axes) {
                    // check this is the axis
                    if (axis_info.axis == y_axis) {
                        // update min & max values for axis
                        axis_info.min_value = (std::min)(axis_info.min_value, min_value);
                        axis_info.max_value = (std::max)(axis_info.max_value, max_value);
                        // exit
                        break;
                    }
                }
            }
        }
    }
    // loop axes
    for (auto& axis_info : m_axes) {
        // skip axis if not in use
        if (axis_info.plots == 0) {
            // ensure valid range is set in plot (Y1 is always visible even if no series are assigned to it)
            axis_info.plot_min_value = 0.0;
            axis_info.plot_max_value = 1.0;
            // next
            continue;
        }
        // range
        const double range = axis_info.max_value - axis_info.min_value;
        // delta
        const double delta = 0.03 * range;
        // update min & max values
        axis_info.plot_min_value = axis_info.min_value - delta;
        axis_info.plot_max_value = axis_info.max_value + delta;
        // log information
        spdlog::debug("Auto range for Y{} axis (unit: '{}'): min = {}, max = {}", axis_info.axis - ImAxis_Y1 + 1, axis_info.unit.empty() ? "<no unit>" : axis_info.unit, axis_info.plot_min_value, axis_info.plot_max_value);
    }
}

std::tuple<bool, View<double>, View<double>, double, double> Chart::plot_step(Expression<double>* ordinate_variant, size_t step, const double min_value, const double max_value, const double x_right_ratio, const double x_left_ratio) const {
    // abscissa
    auto& abscissa = m_expression_manager->abscissa();
    // step abscissa & ordinate values
    auto abscissa_values = abscissa.step_data(step);
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
    // decimate x and y values
    auto [x_np, y_np] = decimate_xy(abscissa_values, ordinate_values, m_decimate_target, DECIMATE_M4);
    // TODO: remove Inf values
    // log information
    spdlog::info("Adding series for expression [{}], step: {}, original size: {}, decimated size: {}", ordinate_variant->name(), step, abscissa_values.size(), x_np.size());
    // check all values were non-finite after filtering
    if (x_np.empty() || y_np.empty())
        return {};
    // exit
    return {true, std::move(x_np), std::move(y_np), (std::min)(min_value, *std::ranges::min_element(y_np)), (std::max)(max_value, *std::ranges::max_element(y_np))};
}

void Chart::clear() {
    // clear internal structures
    m_series.clear();
    // release axes
    for (auto& current : m_axes) {
        // set it as not in use
        current.plots = 0;
        // unit
        current.unit = "";
        // reset plot ranges
        current.plot_min_value = 0;
        current.plot_max_value = 1.0;
    }
}

int Chart::get_y_axis(const std::string& unit) {
    // use pointer
    AxisInformation* available = nullptr;
    // loop axis information
    for (auto& axis_info : m_axes) {
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
        spdlog::info("Creating Y{} axis for measurement type: {}", available->axis - ImAxis_Y1 + 1, unit.empty() ? "<no unit>" : unit);
        // use it
        available->plots = 1;
        available->unit = unit;
        available->min_value = (std::numeric_limits<double>::max)();
        available->max_value = -(std::numeric_limits<double>::max)();
        available->plot_min_value = 0.0;
        available->plot_max_value = 1.0;
        // exit
        return available->axis;
    }
    return -1;
}

bool Chart::release_y_axis(const int axis) {
    // loop axis information
    for (auto& axis_info : m_axes) {
        // check this is the axis to release
        if (axis_info.axis == axis) {
            // decrease ref counter
            axis_info.plots--;
            // check axis is no longer in use
            if (axis_info.plots == 0) {
                // log information
                spdlog::info("Releasing Y{} axis", axis_info.axis - ImAxis_Y1 + 1);
                // reset plot range
                axis_info.plot_min_value = 0.0;
                axis_info.plot_max_value = 1.0;
                // reset unit
                axis_info.unit = "";
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
    const double percentage = (std::max)(0.0, (std::min)(1.0, x_ratio));
    // abscissa range
    const double left_value = m_step_information->abscissa_left_value();
    const double right_value = m_step_information->abscissa_right_value();
    // scale-aware interpolation over the full abscissa range
    return interpolate_abscissa(percentage, left_value, right_value, m_abscissa_scale);
}

double Chart::plot_ratio_to_abscissa_value(const double x_ratio) const {
    // make sure ratio is in the interval [0, 1]
    const double percentage = (std::max)(0.0, (std::min)(1.0, x_ratio));
    // scale-aware interpolation over the visible (zoomed) abscissa range
    return interpolate_abscissa(percentage, m_abscissa_left_value, m_abscissa_right_value, m_abscissa_scale);
}

void Chart::reset_zoom_window(const bool horizontal, const bool vertical) {
    // check horizontal reset
    if (horizontal) {
        // update zoom window
        std::get<0>(m_zoom_window) = -1;
        std::get<2>(m_zoom_window) = -1;
        // process all series to apply the new zoom window, full redraw if horizontal zoom changed
        redraw_all_series();
    }
    // check vertical reset
    if (vertical) {
        // update zoom window
        std::get<1>(m_zoom_window) = -1;
        std::get<3>(m_zoom_window) = -1;
        // update axis ranges based on collected min and max values for each variable type
        for (auto& axis_info : m_axes) {
            // range
            const auto y_range = axis_info.max_value - axis_info.min_value;
            // delta
            const auto delta = 0.03 * y_range;
            // set y axis range
            axis_info.plot_min_value = axis_info.min_value - delta;
            axis_info.plot_max_value = axis_info.max_value + delta;
        }
    }
}

void Chart::update_zoom_window(double x_left_ratio, double x_right_ratio, double y_top_ratio, double y_bottom_ratio) {
    // check horizontal zoom ratios were provided
    if (x_left_ratio >= 0 && x_right_ratio >= 0) {
        // current zoom window
        auto& [current_x_left_ratio, current_y_top_ratio, current_x_right_ratio, current_y_bottom_ratio] = m_zoom_window;
        // use defaults
        current_x_left_ratio = current_x_left_ratio >= 0 ? current_x_left_ratio : 0.0;
        current_x_right_ratio = current_x_right_ratio >= 0 ? current_x_right_ratio : 1.0;
        // calculate new ratios based on the position of the mouse within the chart panel and the current zoom window
        x_left_ratio = current_x_left_ratio + x_left_ratio * (current_x_right_ratio - current_x_left_ratio);
        x_right_ratio = current_x_left_ratio + x_right_ratio * (current_x_right_ratio - current_x_left_ratio);
        // update zoom window
        m_zoom_window = {x_left_ratio, current_y_top_ratio, x_right_ratio, current_y_bottom_ratio};
        // process all series to apply the new zoom window, full redraw if horizontal zoom changed
        redraw_all_series();
    }
    // check vertical zoom ratios were provided
    if (y_top_ratio >= 0 && y_bottom_ratio >= 0) {
        // current zoom window
        auto& [current_x_left_ratio, current_y_top_ratio, current_x_right_ratio, current_y_bottom_ratio] = m_zoom_window;
        // use defaults
        current_y_top_ratio = current_y_top_ratio >= 0 ? current_y_top_ratio : 0.0;
        current_y_bottom_ratio = current_y_bottom_ratio >= 0 ? current_y_bottom_ratio : 1.0;
        // calculate new ratios based on the position of the mouse within the chart panel and the current zoom window
        y_top_ratio = current_y_top_ratio + y_top_ratio * (current_y_bottom_ratio - current_y_top_ratio);
        y_bottom_ratio = current_y_top_ratio + y_bottom_ratio * (current_y_bottom_ratio - current_y_top_ratio);
        // update zoom window
        m_zoom_window = {current_x_left_ratio, y_top_ratio, current_x_right_ratio, y_bottom_ratio};
        // update axis ranges based on collected min and max values for each variable type
        for (auto& axis_info : m_axes) {
            // skip axis if not in use
            if (axis_info.plots == 0) {
                // reset
                axis_info.plot_min_value = 0.0;
                axis_info.plot_max_value = 1.0;
                // next
                continue;
            }
            // range
            const double range = axis_info.max_value - axis_info.min_value;
            // delta
            const double delta = 0.03 * range;
            // actual axis min/max values (see auto_range)
            const double visual_y_min = axis_info.min_value - delta;
            const double visual_y_max = axis_info.max_value + delta;
            // calculate visual axis range
            const double visual_y_range = visual_y_max - visual_y_min;
            // update plot min & max
            axis_info.plot_min_value = visual_y_max - y_top_ratio * visual_y_range;
            axis_info.plot_max_value = visual_y_max - y_bottom_ratio * visual_y_range;
        }
    }
}

void Chart::update(ExpressionManager* expression_manager, const StepInformation* step_information, AbscissaScale abscissa_scale) {
    // update internal references
    m_expression_manager = expression_manager;
    m_step_information = step_information;
    // abscissa
    auto& abscissa = expression_manager->abscissa();
    // abscissa name & unit
    m_abscissa_name = abscissa.name();
    m_abscissa_unit = abscissa.unit();
    // scale
    m_abscissa_scale = abscissa_scale;
}

void Chart::set_decimate_target(const size_t decimate_target) {
    // store the new target; existing series are re-decimated lazily on the next redraw/zoom
    m_decimate_target = decimate_target;
}

std::pair<size_t, size_t> Chart::find_abscissa_indexes(const std::span<const double>& abscissa, double left_value, double right_value) const {
    // ascending or descending abscissa
    if (m_step_information->is_abscissa_ascending()) {
        // check abscissa is not within the zoom window
        if (abscissa[0] > right_value || abscissa[abscissa.size() - 1] < left_value)
            return {0, 0};
        // find left and right indexes using binary search
        size_t left_index = std::ranges::lower_bound(abscissa, left_value) - abscissa.begin();
        size_t right_index = std::ranges::upper_bound(abscissa, right_value) - abscissa.begin();
        // return slice for values within the zoom window
        return {left_index, right_index};
    }
    // check abscissa is not within the zoom window
    if (abscissa[0] < right_value || abscissa[abscissa.size() - 1] > left_value)
        return {0, 0};
    // find left and right indexes using binary search
    size_t left_index = std::ranges::lower_bound(abscissa, left_value, std::greater{}) - abscissa.begin();
    size_t right_index = std::ranges::upper_bound(abscissa, right_value, std::greater{}) - abscissa.begin();
    // return slice for values within the zoom window
    return {left_index, right_index};
}

void Chart::redraw_all_series() {
    // current zoom window
    auto& [x_left_ratio, y_top_ratio, x_right_ratio, current_y_bottom_ratio] = m_zoom_window;
    // x0 and x1
    m_abscissa_left_value = x_left_ratio >= 0 ? ratio_to_abscissa_value(x_left_ratio) : m_step_information->abscissa_left_value();
    m_abscissa_right_value = x_right_ratio >= 0 ? ratio_to_abscissa_value(x_right_ratio) : m_step_information->abscissa_right_value();
    // log information
    spdlog::debug("Redrawing all series for abscissa from {} to {}", m_abscissa_left_value, m_abscissa_right_value);
    // abscissa
    auto& abscissa = m_expression_manager->abscissa();
    // loop existing series
    for (auto& [_, ordinate_series] : m_series | std::views::values) {
        // loop ordinate series
        for (auto& [ordinate_variant, ordinate_variant_series] : ordinate_series) {
            // steps
            auto& rendered_series = std::get<1>(ordinate_variant_series);
            // min and max value recalculation for the new zoom window
            double min_value = (std::numeric_limits<double>::max)();
            double max_value = -(std::numeric_limits<double>::max)();
            // loop steps
            for (auto& [step, series] : rendered_series) {
                // step abscissa & ordinate values — zero copy
                auto abscissa_values = abscissa.step_data(step);
                auto ordinate_values = ordinate_variant->step_data(step);
                // check we have a zoom window to apply
                if (x_left_ratio >= 0 && x_right_ratio >= 0) {
                    // find indexes for the new zoom window
                    const auto& [first, last] = find_abscissa_indexes(abscissa_values, m_abscissa_left_value, m_abscissa_right_value);
                    // abscissa values
                    abscissa_values = abscissa_values | std::views::drop(first) | std::views::take(last - first);
                    // ordinate values
                    ordinate_values = ordinate_values | std::views::drop(first) | std::views::take(last - first);
                }
                // decimate x and y values
                auto [x, y] = decimate_xy(abscissa_values, ordinate_values, m_decimate_target, DECIMATE_M4);
                // TODO: remove Inf values
                // log information
                spdlog::debug("Updating series for expression [{}], step: {}, original size: {}, decimated size: {}", ordinate_variant->name(), step, abscissa_values.size(), x.size());
                // update min and max values
                min_value = (std::min)(min_value, *std::ranges::min_element(y));
                max_value = (std::max)(max_value, *std::ranges::max_element(y));
                // update map value
                series = std::make_pair(std::move(x), std::move(y));
            }
            // update min & max values
            std::get<2>(ordinate_variant_series) = min_value;
            std::get<3>(ordinate_variant_series) = max_value;
        }
    }
}

const std::tuple<float, float, float, float>& Chart::get_plot_rect() const { return m_plot_rect; }

std::string Chart::hovered_series_text(const double abscissa_value) const {
    // abscissa prefix
    std::string result = m_abscissa_name + "=" + format_metric(abscissa_value, m_abscissa_unit);
    // do not evaluate if no series are present
    if (m_series.empty())
        return {};
    // collect series names
    std::vector<std::string> names;
    // allocate space
    names.reserve(m_series.size());
    // append names from series map
    for (const auto& [name, _] : m_series)
        names.push_back(name);
    // sort names, deterministic order for the hover text
    std::ranges::sort(names);
    // abscissa direction
    const bool ascending = m_step_information->is_abscissa_ascending();
    // loop series in sorted order
    for (const auto& name : names) {
        // lookup ordinate series
        const auto& ordinate_series = m_series.at(name);
        const auto& variant_series = std::get<1>(ordinate_series);
        // loop ordinate variants (db/phase for complex, single for real)
        for (const auto& [ordinate_variant, variant_steps] : variant_series) {
            // steps
            const auto& rendered_series = std::get<1>(variant_steps);
            // step values at the hovered abscissa, joined in ascending step order
            std::string values;
            size_t value_count = 0;
            // collect steps and sort them, deterministic order for the hover text
            std::vector<size_t> steps;
            for (const auto& [step, _] : rendered_series)
                steps.push_back(step);
            std::ranges::sort(steps);
            // loop steps in ascending order
            for (const size_t step : steps) {
                // actual x and y values for the hovered abscissa value
                const auto& [x_view, y_view] = rendered_series.at(step);
                if (x_view.empty() || y_view.empty())
                    continue;
                // interpolate y value at the hovered abscissa value
                const double y = interpolate_y(x_view, y_view, abscissa_value, ascending);
                // append separator
                if (value_count > 0)
                    values += ", ";
                // append formatted value
                values += format_metric(y, ordinate_variant->unit());
                value_count++;
            }
            // no plotable data for any step
            if (value_count == 0)
                continue;
            // single step: plain value, multiple steps: grouped values
            if (value_count > 1)
                result += " " + ordinate_variant->name() + "=[" + values + "]";
            else
                result += " " + ordinate_variant->name() + "=" + values;
        }
    }
    return result;
}
