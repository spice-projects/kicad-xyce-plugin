#pragma once

#include <implot.h>
#include <set>
#include <unordered_map>

#include "../step_information.h"
#include "../expression/expression.h"
#include "../expression/expression_manager.h"
#include "../file/xyce_output_file.h"

using OrdinateVariantSeriesSteps = std::tuple<
    int,
    std::unordered_map<
        size_t,
        std::pair<
            std::span<const double>,
            std::span<const double>
        >
    >,
    double, double, ImVec4>;

using OrdinateVariantSeries = std::unordered_map<
    Expression<double>*,
    OrdinateVariantSeriesSteps
>;

using OrdinateSeries = std::tuple<AnyExpression*, OrdinateVariantSeries>;

using Series = std::unordered_map<std::string, OrdinateSeries>;

struct AxisInformation
{
    const int axis;
    std::string unit;
    int plots;
    double min_value;
    double max_value;
    double plot_min_value;
    double plot_max_value;
};

class Chart
{
public:
    Chart(ExpressionManager& expression_manager, const StepInformation& step_information, Expression<double>& abscissa, const std::string& abscissa_label, AbscissaScale abscissa_scale, size_t decimate_target);

    const std::set<size_t>& selected_steps();

    void plot_series(const std::set<AnyExpression*>& expressions);

    void auto_range();

    void render();

private:
    ExpressionManager& m_expression_manager;
    StepInformation const& m_step_information;
    Expression<double>& m_abscissa;
    std::string m_abscissa_label;
    AbscissaScale m_abscissa_scale;
    size_t m_decimate_target;

    Series m_series;
    std::set<size_t> m_selected_steps = {0};
    double m_abscissa_left_value = 0.0;
    double m_abscissa_right_value = 0.0;
    size_t m_next_color_index = 0;
    std::tuple<double, double, double, double> m_zoom_window = {-1, -1, -1, -1};
    AxisInformation m_axes[3] = {{ImAxis_Y1}, {ImAxis_Y2}, {ImAxis_Y3}};

    std::vector<Expression<double>*> get_expressions_to_plot(AnyExpression*) const;

    std::tuple<bool, std::span<const double>, std::span<const double>, double, double> plot_step(Expression<double>* ordinate_variant, size_t step, double min_value, double max_value, double x_right_ratio, double x_left_ratio) const;

    int get_y_axis(const std::string& unit);

    bool release_y_axis(const int axis);

    double ratio_to_abscissa_value(double x_ratio) const;

    void update_zoom_window(double x_left_ratio, double x_right_ratio, double y_top_ratio, double y_bottom_ratio);

    std::pair<size_t, size_t> find_abscissa_indexes(const std::span<const double>& abscissa, double left_value, double right_value) const;

    void redraw_all_series();
};
