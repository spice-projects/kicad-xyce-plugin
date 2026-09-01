#pragma once

#include <array>
#include <map>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <implot.h>

#include "../core/step_information.h"
#include "../expression/expression.h"
#include "../expression/expression_manager.h"
#include "../io/xyce_output_file.h"

using OrdinateSeries = std::tuple<AnyExpression*, int, std::unordered_map<size_t, std::pair<View<double>, View<double>>>, double, double, ImVec4>;

using Series = std::map<std::string, OrdinateSeries>;

// default series color palette (Apple / Cupertino inspired)
inline const std::vector<ImVec4> SERIES_COLOR_PALETTE = {
    ImVec4(0.00f, 0.48f, 1.00f, 1.0f), // #007AFF Vibrant Blue
    ImVec4(1.00f, 0.58f, 0.00f, 1.0f), // #FF9500 Vibrant Orange
    ImVec4(0.20f, 0.78f, 0.35f, 1.0f), // #34C759 Vibrant Green
    ImVec4(0.69f, 0.32f, 0.87f, 1.0f), // #AF52DE Vibrant Purple
    ImVec4(1.00f, 0.18f, 0.33f, 1.0f), // #FF2D55 Vibrant Pink
    ImVec4(0.19f, 0.69f, 0.78f, 1.0f), // #30B0C7 Vibrant Teal
    ImVec4(0.35f, 0.34f, 0.84f, 1.0f), // #5856D6 Vibrant Indigo
    ImVec4(1.00f, 0.80f, 0.00f, 1.0f), // #FFCC00 Vibrant Yellow
    ImVec4(0.00f, 0.78f, 0.75f, 1.0f), // #00C7BE Vibrant Mint
    ImVec4(1.00f, 0.23f, 0.19f, 1.0f), // #FF3B30 Vibrant Coral Red
};

struct AxisInformation
{
    int axis;
    double plot_min_value;
    double plot_max_value;
    std::string unit;
    int plots;
    double min_value;
    double max_value;
};

class Chart
{
public:
    Chart() = delete;

    Chart(ExpressionManager* expression_manager, StepInformation const* step_information, AbscissaScale abscissa_scale, size_t decimate_target);

    Chart(const Chart&) = delete;

    Chart(Chart&&) noexcept = default;

    Chart& operator=(const Chart&) = delete;

    Chart& operator=(Chart&&) noexcept = default;

    const std::set<size_t>& selected_steps();

    void set_selected_steps(const std::set<size_t>& steps);

    std::vector<AnyExpression*> selected_expressions();

    void render();

    void plot_series(const std::set<AnyExpression*>& expressions);

    void auto_range();

    void clear();

    void reset_zoom_window(bool horizontal, bool vertical);

    void update_zoom_window(double x_left_ratio, double x_right_ratio, double y_top_ratio, double y_bottom_ratio);

    void update(ExpressionManager* expression_manager, const StepInformation* step_information, AbscissaScale abscissa_scale);

    void set_decimate_target(size_t decimate_target);

    [[nodiscard]] const std::tuple<float, float, float, float>& get_plot_rect() const;

    [[nodiscard]] const std::tuple<double, double, double, double>& zoom_window() const { return m_zoom_window; }

    [[nodiscard]] double ratio_to_abscissa_value(double x_ratio) const;

    [[nodiscard]] double plot_ratio_to_abscissa_value(double x_ratio) const;

    [[nodiscard]] std::string hovered_series_text(double abscissa_value) const;

    static std::string format_metric(double value, std::string_view unit);

private:
    ExpressionManager* m_expression_manager;
    const StepInformation* m_step_information;
    AbscissaScale m_abscissa_scale;
    size_t m_decimate_target;
    std::string m_abscissa_name;
    std::string m_abscissa_unit;
    std::tuple<float, float, float, float> m_plot_rect = {-1.0f, -1.0f, -1.0f, -1.0f};

    Series m_series;
    std::set<size_t> m_selected_steps = {0};
    double m_abscissa_left_value = 0.0;
    double m_abscissa_right_value = 1.0;
    size_t m_next_color_index = 0;
    std::tuple<double, double, double, double> m_zoom_window = {-1, -1, -1, -1};
    std::array<AxisInformation, 3> m_axes = {{{ImAxis_Y1, 0.0, 1.0, "", 0, 0.0, 1.0}, {ImAxis_Y2, 0.0, 1.0, "", 0, 0.0, 1.0}, {ImAxis_Y3, 0.0, 1.0, "", 0, 0.0, 1.0}}};

    std::tuple<bool, View<double>, View<double>, double, double> plot_step(Expression<double>& ordinate_variant, size_t step, double min_value, double max_value, double x_right_ratio, double x_left_ratio) const;

    int get_y_axis(const std::string& unit);

    bool release_y_axis(int axis);

    [[nodiscard]] std::pair<size_t, size_t> find_abscissa_indexes(const std::span<const double>& abscissa, double left_value, double right_value) const;

    void redraw_all_series();
};
