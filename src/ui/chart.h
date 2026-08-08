#pragma once

#include <array>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <implot.h>

#include "../expression/expression.h"
#include "../expression/expression_manager.h"
#include "../file/xyce_output_file.h"
#include "../step_information.h"

using OrdinateVariantSeriesSteps = std::tuple<int, std::unordered_map<size_t, std::pair<View<double>, View<double>>>, double, double, ImVec4>;

using OrdinateVariantSeries = std::unordered_map<Expression<double>*, OrdinateVariantSeriesSteps>;

using OrdinateSeries = std::tuple<AnyExpression*, OrdinateVariantSeries>;

using Series = std::unordered_map<std::string, OrdinateSeries>;

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

    Chart(ExpressionManager* expression_manager, StepInformation const* step_information, std::string abscissa_label, AbscissaScale abscissa_scale, size_t decimate_target);

    Chart(const Chart&) = delete;

    Chart(Chart&&) noexcept = default;

    Chart& operator=(const Chart&) = delete;

    Chart& operator=(Chart&&) noexcept = default;

    const std::set<size_t>& selected_steps();

    std::vector<AnyExpression*> selected_expressions();

    void render(const std::tuple<float, float, float, float>&);

    void plot_series(const std::set<AnyExpression*>& expressions);

    void auto_range();

    void clear();

    void reset_zoom_window(bool horizontal, bool vertical);

    void update_zoom_window(double x_left_ratio, double x_right_ratio, double y_top_ratio, double y_bottom_ratio);

    void update(ExpressionManager* expression_manager, const StepInformation* step_information, const std::string& abscissa_label, AbscissaScale abscissa_scale);

    void set_decimate_target(size_t decimate_target);

    [[nodiscard]] const std::tuple<float, float, float, float>& get_plot_rect() const;

    [[nodiscard]] const std::tuple<double, double, double, double>& zoom_window() const { return m_zoom_window; }

private:
    ExpressionManager* m_expression_manager;
    const StepInformation* m_step_information;
    std::string m_abscissa_label;
    AbscissaScale m_abscissa_scale;
    size_t m_decimate_target;
    std::string m_abscissa_unit;
    std::tuple<float, float, float, float> m_plot_rect = {-1, -1, -1, -1};

    Series m_series;
    std::set<size_t> m_selected_steps = {0};
    double m_abscissa_left_value = 0.0;
    double m_abscissa_right_value = 1.0;
    size_t m_next_color_index = 0;
    std::tuple<double, double, double, double> m_zoom_window = {-1, -1, -1, -1};
    std::array<AxisInformation, 3> m_axes = {{{ImAxis_Y1, 0.0, 1.0, "", 0, 0.0, 1.0}, {ImAxis_Y2, 0.0, 1.0, "", 0, 0.0, 1.0}, {ImAxis_Y3, 0.0, 1.0, "", 0, 0.0, 1.0}}};

    std::tuple<bool, View<double>, View<double>, double, double> plot_step(Expression<double>* ordinate_variant, size_t step, double min_value, double max_value, double x_right_ratio, double x_left_ratio) const;

    int get_y_axis(const std::string& unit);

    bool release_y_axis(int axis);

    [[nodiscard]] double ratio_to_abscissa_value(double x_ratio) const;

    [[nodiscard]] std::pair<size_t, size_t> find_abscissa_indexes(const std::span<const double>& abscissa, double left_value, double right_value) const;

    void redraw_all_series();
};
