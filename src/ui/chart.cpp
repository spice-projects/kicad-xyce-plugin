#include <format>
#include <limits>
#include <ranges>
#include <set>
#include <span>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "implot.h"
#include "spdlog/spdlog.h"

#include "chart.h"
#include "decimate.h"

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

static int metric_formatter(const double value, char* buff, const int size, const void* data) {
    // unit
    const auto unit = static_cast<const char*>(data);
    // dividers
    static double v[] = {1000000000, 1000000, 1000, 1, 0.001, 0.000001, 0.000000001};
    // prefixes
    static const char* p[] = {"G", "M", "k", "", "m", "u", "n"};
    // zero
    if (value == 0) {
        return snprintf(buff, size, "0 %s", unit);
    }
    // loop scales
    for (int i = 0; i < 7; ++i) {
        // check we should format value with this scale
        if (fabs(value) >= v[i])
            return snprintf(buff, size, "%g %s%s", value / v[i], p[i], unit);
    }
    return snprintf(buff, size, "%g %s%s", value / v[6], p[6], unit);
}

Chart::Chart(wxEvtHandler* parent, ExpressionManager* expression_manager, const StepInformation* step_information, std::string abscissa_label, const AbscissaScale abscissa_scale, const size_t decimate_target)
    : m_parent(parent), m_expression_manager(expression_manager), m_step_information(step_information), m_abscissa_label(std::move(abscissa_label)), m_abscissa_scale(abscissa_scale), m_decimate_target(decimate_target) {
    // abscissa unit
    m_abscissa_unit = expression_manager->abscissa().unit();
}

const std::set<size_t>& Chart::selected_steps() {
    return m_selected_steps;
}

void Chart::render(const std::tuple<float, float, float, float>& selection) {
    // set style color
    // ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4());
    // initialize plot, full area
    if (ImPlot::BeginPlot("My First Plot", ImVec2(-1, -1), PLOT_FLAGS)) {
        // x axis
        ImPlot::SetupAxis(ImAxis_X1);
        // format
        ImPlot::SetupAxisFormat(ImAxis_X1, reinterpret_cast<ImPlotFormatter>(metric_formatter), (void*)m_abscissa_unit.c_str());
        // min and max values
        ImPlot::SetupAxisLimits(ImAxis_X1, m_abscissa_left_value, m_abscissa_right_value);
        // loop axis information
        for (const auto& axis_info : m_axes) {
            // axis info at i (Y1 is always enabled)
            if (axis_info.axis == ImAxis_Y1 || axis_info.plots > 0) {
                // setup axis
                ImPlot::SetupAxis(axis_info.axis, nullptr, axis_info.axis != ImAxis_Y1 ? ImPlotAxisFlags_Opposite : ImPlotAxisFlags_None);
                // format
                ImPlot::SetupAxisFormat(axis_info.axis, reinterpret_cast<ImPlotFormatter>(metric_formatter), (void*)axis_info.unit.c_str());
                // min and max values
                ImPlot::SetupAxisLimits(axis_info.axis, axis_info.plot_min_value, axis_info.plot_max_value);
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
                spec.LineWeight = 3.0f;
                // loop steps
                for (const auto& [x, y] : steps | std::views::values) {
                    // draw the line chart
                    ImPlot::PlotLine(name.c_str(), x.data(), y.data(), static_cast<int>(x.size()), {ImPlotProp_LineWeight, 2.0});
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
    // pop style color
    // ImPlot::PopStyleColor();
}

void Chart::plot_series(const std::set<AnyExpression*>& expressions) {
    // loop existing series to find those that need to be removed (those whose expression is not in the new expressions list)
    for (auto it = m_series.begin(); it != m_series.end();) {
        // check expression should be removed
        if (auto& [expression, ordinate_series] = it->second; !expressions.contains(expression)) {
            // loop ordinate series
            for (auto& [key, value] : ordinate_series) {
                // log information
                spdlog::debug("Removing series for expression '{}' from chart", key->name());
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
        auto [it, inserted] = m_series.try_emplace(name, OrdinateSeries(ordinate, {}));
        // ordinate series
        OrdinateSeries& ordinate_series = it->second;
        // process ordinate and find expressions to plot
        for (auto ordinate_variant : get_expressions_to_plot(ordinate)) {
            // lookup ordinate variant in series
            auto [it1, inserted1] = std::get<1>(ordinate_series).try_emplace(ordinate_variant, OrdinateVariantSeriesSteps());
            // lookup ordinate variant in series
            auto& [y_axis, rendered_series, min_value, max_value, color] = (it1->second);
            // loop rendered steps
            for (auto it2 = rendered_series.begin(); it2 != rendered_series.end();) {
                // check step in selected steps
                if (!m_selected_steps.contains(it2->first)) {
                    // log information
                    spdlog::debug("Removing series for expression [{}] from chart, step: {}", ordinate_variant->name(), it2->first);
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
            for (auto step : m_selected_steps) {
                // skip step if already rendered
                if (rendered_series.contains(step))
                    continue;
                // plot step
                if (auto [ok, x, y, y_min_value, y_max_value] = plot_step(ordinate_variant, step, min_value, max_value, x_right_ratio, x_left_ratio); ok) {
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
            // update color
            std::get<4>(it1->second) = color;
        }
    }
    // auto range axes
    auto_range();
}

void Chart::auto_range() {
    // skip if no series to render
    if (m_series.empty()) {
        // y1 axis
        auto& y1 = m_axes.front();
        // ensure valid range is set in plot (Y1 is always visible even if no series are assigned to it)
        y1.plot_min_value = 0.0;
        y1.plot_max_value = 1.0;
        // exit
        return;
    }
    // loop axes
    for (auto& axis_info : m_axes) {
        // reset min & max values for axes
        axis_info.min_value = std::numeric_limits<double>::max();
        axis_info.max_value = std::numeric_limits<double>::min();
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
                    axis_info.min_value = std::min(axis_info.min_value, min_value);
                    axis_info.max_value = std::max(axis_info.max_value, max_value);
                    // exit
                    break;
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
    }
}

std::tuple<bool, std::span<const double>, std::span<const double>, double, double> Chart::plot_step(Expression<double>* ordinate_variant, size_t step, const double min_value, const double max_value, const double x_right_ratio, const double x_left_ratio) const {
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
    auto [x_np, y_np] = decimate_xy(abscissa_values, ordinate_values, m_decimate_target, 1);
    // TODO: remove Inf values
    // check all values were non-finite after filtering
    if (x_np.empty() || y_np.empty())
        return {};
    // exit
    return {true, x_np, y_np, std::min(min_value, *std::ranges::min_element(y_np)), std::max(max_value, *std::ranges::max_element(y_np))};
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

std::vector<Expression<double>*> Chart::get_expressions_to_plot(AnyExpression* expression) const {
    // nothing to do on double expressions
    if (std::holds_alternative<Expression<double>>(*expression)) {
        // exit
        return {&std::get<Expression<double>>(*expression)};
    }
    // complex expression
    const auto& complex_expression = std::get<Expression<std::complex<double>>>(*expression);
    // magnitude
    auto magnitude_expression = m_expression_manager->evaluate(std::format("db({})", complex_expression.name()));
    if (!magnitude_expression)
        return {};
    // phase
    auto phase_expression = m_expression_manager->evaluate(std::format("phase({})", complex_expression.name()));
    if (!phase_expression)
        return {};
    // exit
    return {&std::get<Expression<double>>(*magnitude_expression), &std::get<Expression<double>>(*phase_expression)};
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
        spdlog::debug("Creating Y axis for measurement type: {}", unit.empty() ? "<no unit>" : unit);
        // use it
        available->plots = 1;
        available->unit = unit;
        available->min_value = std::numeric_limits<double>::max();
        available->max_value = std::numeric_limits<double>::min();
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
                spdlog::debug("Releasing Y axis for measurement type: {}", axis_info.unit.empty() ? "<no unit>" : axis_info.unit);
                // reset plot range
                axis_info.plot_min_value = 0.0;
                axis_info.plot_max_value = 1.0;
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
    const double percentage = std::max(0.0, std::min(1.0, x_ratio));
    // convert to abscissa value
    return m_step_information->abscissa_left_value() + percentage * (m_step_information->abscissa_right_value() - m_step_information->abscissa_left_value());
}

std::pair<size_t, size_t> Chart::find_abscissa_indexes(const std::span<const double>& abscissa, const double left_value, const double right_value) const {
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
        auto& [current_x_left_ratio, current_y_top_ratio , current_x_right_ratio, current_y_bottom_ratio] = m_zoom_window;
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
        auto& [current_x_left_ratio, current_y_top_ratio , current_x_right_ratio, current_y_bottom_ratio] = m_zoom_window;
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
            axis_info.plot_min_value = visual_y_min + y_top_ratio * visual_y_range;
            axis_info.plot_max_value = visual_y_min + y_bottom_ratio * visual_y_range;
        }
    }
}

void Chart::redraw_all_series() {
    // current zoom window
    auto& [x_left_ratio, y_top_ratio , x_right_ratio, current_y_bottom_ratio] = m_zoom_window;
    // x0 and x1
    double abscissa_left_value = x_left_ratio >= 0 ? ratio_to_abscissa_value(x_left_ratio) : m_step_information->abscissa_left_value();
    double abscissa_right_value = x_right_ratio >= 0 ? ratio_to_abscissa_value(x_right_ratio) : m_step_information->abscissa_right_value();
    // loop existing series
    for (auto& [_, ordinate_series] : m_series | std::views::values) {
        // loop ordinate variant series
        for (auto& [ordinate_variant, aa] : ordinate_series) {
        }
    }
}

const std::tuple<float, float, float, float>& Chart::get_plot_rect() const {
    return m_plot_rect;
}
