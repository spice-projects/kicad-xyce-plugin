#include <algorithm>
#include <chrono>
#include <cmath>
#include <set>

#include <imgui.h>
#include <implot.h>
#include <spdlog/spdlog.h>

#include "charts_renderer.h"

ChartsContextScope::ChartsContextScope(const ChartsRenderer& charts_renderer) {
    // preserve the active contexts while this renderer owns backend calls
    m_imgui_context = ImGui::GetCurrentContext();
    m_implot_context = ImPlot::GetCurrentContext();
    // activate this renderer's isolated contexts
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(charts_renderer.m_imgui_context));
    ImPlot::SetCurrentContext(static_cast<ImPlotContext*>(charts_renderer.m_implot_context));
}

ChartsContextScope::~ChartsContextScope() {
    // restore the contexts active before this scope
    ImPlot::SetCurrentContext(static_cast<ImPlotContext*>(m_implot_context));
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imgui_context));
}

ChartsRenderer::~ChartsRenderer() {
    // teardown the backend and release the isolated contexts
    terminate();
    // detach the native view
    detach();
}

void ChartsRenderer::initialize_contexts() {
    // contexts are created exactly once for the renderer lifetime
    if (m_imgui_context != nullptr)
        return;
    // preserve the context that was active before this renderer initialized
    auto* previous_imgui_context = ImGui::GetCurrentContext();
    auto* previous_implot_context = ImPlot::GetCurrentContext();
    // create and retain the ImGui context
    IMGUI_CHECKVERSION();
    m_imgui_context = ImGui::CreateContext();
    // create and retain the matching ImPlot context
    m_implot_context = ImPlot::CreateContext();
    // restore the prior active contexts
    ImPlot::SetCurrentContext(previous_implot_context);
    ImGui::SetCurrentContext(previous_imgui_context);
}

void ChartsRenderer::terminate_contexts() {
    // contexts have already been released
    if (m_imgui_context == nullptr)
        return;
    // preserve the active contexts while releasing this renderer's contexts
    auto* previous_imgui_context = ImGui::GetCurrentContext();
    auto* previous_implot_context = ImPlot::GetCurrentContext();
    // retain the context addresses while clearing the members below
    auto* imgui_context = static_cast<ImGuiContext*>(m_imgui_context);
    auto* implot_context = static_cast<ImPlotContext*>(m_implot_context);
    // activate this renderer's isolated contexts
    ImGui::SetCurrentContext(imgui_context);
    ImPlot::SetCurrentContext(implot_context);
    // destroy the plot context before its ImGui dependency
    ImPlot::DestroyContext(implot_context);
    m_implot_context = nullptr;
    // destroy the ImGui context
    ImGui::DestroyContext(imgui_context);
    m_imgui_context = nullptr;
    // never restore a context that was released above
    ImPlot::SetCurrentContext(previous_implot_context == implot_context ? nullptr : previous_implot_context);
    ImGui::SetCurrentContext(previous_imgui_context == imgui_context ? nullptr : previous_imgui_context);
}

void ChartsRenderer::update_delta_time() {
    // read the monotonic clock once for this renderer's frame
    const auto current_time = std::chrono::steady_clock::now();
    // use a conventional initial duration before a prior frame exists
    if (m_last_frame_time.time_since_epoch().count() == 0)
        ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
    else
        ImGui::GetIO().DeltaTime = std::chrono::duration<float>(current_time - m_last_frame_time).count();
    // retain the timestamp for the next frame
    m_last_frame_time = current_time;
}

void ChartsRenderer::render() {
    // render a placeholder host window; the chart UI replaces this body once the
    // data path is wired in stage 2
    render_frame([this]() -> void {
        // remove padding around the panel
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // full-area host window proving the ImGui/ImPlot pipeline
        if (ImGui::Begin("Charts Renderer", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs)) {
            // available area
            const ImVec2 total_space = ImGui::GetContentRegionAvail();
            // plot placeholder covering the panel
            if (ImPlot::BeginPlot("Host Plot", ImVec2(total_space.x, total_space.y), ImPlotFlags_NoLegend | ImPlotFlags_NoInputs | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect)) {
                // axis setup
                ImPlot::SetupAxes("x", "y");
                // sample a sine wave so the pipeline is visibly alive
                static const std::vector<float> x = []() {
                    std::vector<float> values;
                    for (int i = 0; i < 100; ++i)
                        values.push_back(static_cast<float>(i) / 10.0f);
                    return values;
                }();
                static const std::vector<float> y = []() {
                    std::vector<float> values;
                    for (int i = 0; i < 100; ++i)
                        values.push_back(std::sin(static_cast<float>(i) / 10.0f));
                    return values;
                }();
                // draw the sine wave
                ImPlot::PlotLine("sin(x)", x.data(), y.data(), static_cast<int>(x.size()));
                // finalize the plot block
                ImPlot::EndPlot();
            }
        }
        // close the host window
        ImGui::End();
        // pop style var
        ImGui::PopStyleVar();
    });
}

void ChartsRenderer::update(ExpressionManager& expression_manager, const StepInformation& step_information, const AbscissaScale abscissa_scale, const std::vector<std::vector<std::string>>& suggested_plots) {
    // recompute the decimation target from the current panel size and display scale
    update_decimation_target();
    // update fields
    m_expression_manager = &expression_manager;
    m_step_information = &step_information;
    m_abscissa_scale = abscissa_scale;
    // new simulation data, clear any hover readout from the previous file
    // check charts are present, if not add one
    if (!m_charts.empty()) {
        // loop charts
        for (const auto& chart : m_charts) {
            // update chart with new information
            chart->update(m_expression_manager, m_step_information, m_abscissa_scale);
        }
    }
    else if (!suggested_plots.empty()) {
        // create one chart per suggested plot group
        for (const auto& plot_names : suggested_plots) {
            // add a chart for the group
            auto* chart = add_chart();
            // resolved expressions for this group
            std::set<AnyExpression*> resolved_expressions;
            // resolve each expression name to its pointer
            for (const auto& name : plot_names) {
                // evaluate the expression name
                auto* expression = m_expression_manager->evaluate(name);
                // check it resolved to an existing expression
                if (expression == nullptr) {
                    // log information
                    spdlog::warn("Suggested plot expression '{}' not found in expression manager", name);
                    // skip unresolved expression
                    continue;
                }
                // append resolved expression
                resolved_expressions.insert(expression);
            }
            // plot the resolved expressions on the chart
            chart->plot_series(resolved_expressions);
        }
    }
    else {
        // add a new chart with no pre-populated expressions
        add_chart();
    }
    // refresh
    refresh_charts();
}

Chart* ChartsRenderer::add_chart() {
    // create chart and append it to vector
    m_charts.push_back(std::make_unique<Chart>(m_expression_manager, m_step_information, m_abscissa_scale, m_decimate_target));
    // chart
    auto& chart = m_charts[m_charts.size() - 1];
    // plot series (will do nothing, but will set the correct abscissa for the zoom window)
    chart->plot_series({});
    // exit
    return chart.get();
}

void ChartsRenderer::delete_all_charts() {
    // simple, clean vector
    m_charts.clear();
    // refresh
    refresh_charts();
}

size_t ChartsRenderer::compute_decimation_target() const {
    // floor to keep the decimation algorithm meaningful on very small windows
    constexpr size_t min_target = 500;
    // fallback when no usable size or scale is available (headless / early startup)
    constexpr size_t fallback_target = 9600;
    // panel width in logical pixels and the per-window display scale factor
    if (m_logical_width > 0 && m_backing_scale > 0.0)
        return (std::max)(min_target, static_cast<size_t>(std::lround(static_cast<double>(m_logical_width) * m_backing_scale)));
    // conservative fallback
    return fallback_target;
}

void ChartsRenderer::update_decimation_target() {
    // recompute the target for the current panel size and display scale
    const size_t target = compute_decimation_target();
    // check the target changed
    if (target == m_decimate_target)
        return;
    // store it
    m_decimate_target = target;
    // propagate it to existing charts so future decimations use the new target
    for (const auto& chart : m_charts)
        chart->set_decimate_target(target);
}

void ChartsRenderer::refresh_charts(int frames) { m_render_chart_frames = frames; }
