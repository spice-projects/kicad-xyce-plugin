#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
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
    // render the charts panel UI within the native frame
    render_frame([this]() -> void {
        // remove padding around the panel
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // panel
        if (ImGui::Begin("Charts Panel", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs)) {
            // check we have charts to render
            if (!m_charts.empty()) {
                // available area
                const ImVec2 total_space = ImGui::GetContentRegionAvail();
                // chart height
                const float height = total_space.y / static_cast<float>(m_charts.size());
                // render charts within the native frame
                for (size_t i = 0; i < m_charts.size(); ++i) {
                    // area name
                    auto name = std::format("Chart {}", i);
                    // create child with given height, use the whole area in the horizontal
                    if (ImGui::BeginChild(name.c_str(), ImVec2(0, height), true)) {
                        // check current chart is selected
                        if (i == m_selected_chart_index) {
                            // render chart
                            m_charts[i]->render(m_zoom_selection);
                        }
                        else {
                            // render chart
                            m_charts[i]->render({-1, -1, -1, -1});
                        }
                        // close
                        ImGui::EndChild();
                    }
                }
            }
            // close
            ImGui::End();
        }
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

std::vector<AnyExpression*> ChartsRenderer::all_expressions() const {
    // nothing to list without an expression manager
    if (m_expression_manager == nullptr)
        return {};
    // delegate to the expression manager
    return m_expression_manager->expressions();
}

std::vector<AnyExpression*> ChartsRenderer::chart_selected_expressions(size_t chart_index) const {
    // guard against an invalid chart index
    if (chart_index >= m_charts.size())
        return {};
    // delegate to the chart
    return m_charts[chart_index]->selected_expressions();
}

AnyExpression* ChartsRenderer::evaluate_expression(const std::string& expression) {
    // nothing to evaluate without an expression manager
    if (m_expression_manager == nullptr)
        return nullptr;
    // delegate to the expression manager
    return m_expression_manager->evaluate(expression, expression);
}

void ChartsRenderer::plot_chart_expressions(size_t chart_index, const std::set<AnyExpression*>& expressions) {
    // guard against an invalid chart index
    if (chart_index >= m_charts.size())
        return;
    // plot the given expressions on the chart
    m_charts[chart_index]->plot_series(expressions);
    // refresh to show the updated selection
    refresh_charts();
}

size_t ChartsRenderer::position_to_index(float position) const {
    // clamp to valid range and multiply by chart count
    if (m_charts.empty())
        return 0;
    // clamp the position to [0, 1] and compute the corresponding chart index
    const float clamped = std::clamp(position, 0.0f, 1.0f);
    const size_t index = static_cast<size_t>(clamped * static_cast<float>(m_charts.size()));
    return std::min(index, m_charts.size() - 1);
}

void ChartsRenderer::on_idle() {
    // check flag is set
    if (m_render_chart_frames > 0) {
        // decrease counter
        m_render_chart_frames--;
        // render charts panel
        render();
    }
}

void ChartsRenderer::zoom_to_fit(float chart_position) {
    // find the chart index corresponding to the position in the panel
    const size_t chart_index = position_to_index(chart_position);
    // log information
    spdlog::debug("User requested zoom to fit on chart at position {} (index {})", chart_position, chart_index);
    // loop charts
    for (size_t i = 0; i < m_charts.size(); i++) {
        // chart at i
        const auto& chart = m_charts[i];
        // check if this is the chart that triggered the zoom to fit action
        if (i == chart_index) {
            // reset zoom window
            chart->reset_zoom_window(true, true);
            // next
            continue;
        }
        // update horizontal zoom window only, keep vertical zoom as is
        chart->reset_zoom_window(true, false);
    }
    // refresh
    refresh_charts();
}

void ChartsRenderer::autorange(float chart_position) {
    // find the chart index corresponding to the position in the panel
    const size_t chart_index = position_to_index(chart_position);
    // log information
    spdlog::debug("User requested autorange on chart at position {} (index {})", chart_position, chart_index);
    // loop charts
    for (size_t i = 0; i < m_charts.size(); i++) {
        // chart at i
        const auto& chart = m_charts[i];
        // check if this is the chart that triggered the autorange action
        if (i == chart_index) {
            // reset zoom window
            chart->reset_zoom_window(true, true);
            // next
            continue;
        }
        // update horizontal zoom window only, keep vertical zoom as is
        chart->reset_zoom_window(true, false);
    }
    // refresh
    refresh_charts();
}

void ChartsRenderer::zoom_abscissa_extent(float chart_position) {
    // find the chart index corresponding to the position in the panel
    const size_t chart_index = position_to_index(chart_position);
    // log information
    spdlog::debug("User requested zoom abscissa extent on chart at position {} (index {})", chart_position, chart_index);
    // loop charts
    for (const auto& chart : m_charts) {
        // reset zoom window
        chart->reset_zoom_window(true, false);
    }
    // refresh
    refresh_charts();
}

void ChartsRenderer::delete_all_plots(float chart_position) {
    // find the chart index corresponding to the position in the panel
    const size_t chart_index = position_to_index(chart_position);
    // log information
    spdlog::debug("User requested deleting all plots on chart at position {} (index {})", chart_position, chart_index);
    // selected chart
    if (chart_index < m_charts.size()) {
        // clear chart
        m_charts[chart_index]->clear();
        // refresh
        refresh_charts();
    }
}

void ChartsRenderer::delete_chart(float chart_position) {
    // find the chart index corresponding to the position in the panel
    const size_t chart_index = position_to_index(chart_position);
    // log information
    spdlog::debug("User requested deleting chart at position {} (index {})", chart_position, chart_index);
    // delete chart at index
    m_charts.erase(m_charts.begin() + static_cast<int>(chart_index));
    // ensure at least one chart in panel
    if (m_charts.empty())
        add_chart();
    // refresh
    refresh_charts();
}
