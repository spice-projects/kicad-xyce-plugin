#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <slint.h>

#include "../charts/chart.h"

// slint window abstraction; the platform backend extracts the native content
// view (NSView on macOS, HWND on Windows, native surface on Linux)
namespace slint
{
    class Window;
}

// wx-free scope guard that activates a ChartsRenderer's isolated ImGui/ImPlot
// contexts for the duration of a scope, replacing the wx-coupled im_context.h
class ChartsRenderer;

class ChartsContextScope
{
public:
    explicit ChartsContextScope(const ChartsRenderer& charts_renderer);

    ~ChartsContextScope();

    ChartsContextScope(const ChartsContextScope&) = delete;

    ChartsContextScope& operator=(const ChartsContextScope&) = delete;

private:
    void* m_imgui_context = nullptr;
    void* m_implot_context = nullptr;
};

// platform-neutral charts renderer hosting an isolated Dear ImGui/ImPlot stack
// on top of the slint content view; the platform backend (per-platform .mm/.c++)
// owns the native view, layer, font and the render_frame command encoding
class ChartsRenderer
{
public:
    ChartsRenderer() = default;

    ~ChartsRenderer();

    ChartsRenderer(const ChartsRenderer&) = delete;

    ChartsRenderer& operator=(const ChartsRenderer&) = delete;

    // attach the renderer view to the slint window; the platform backend
    // extracts the native content view (NSView on macOS)
    void attach(slint::Window& window);

    // whether the renderer is attached to the native content view; false while
    // the native view does not exist yet (e.g. right after the window is shown)
    [[nodiscard]] bool attached() const { return m_view != nullptr; }

    // detach the renderer view from its content view
    void detach();

    // position the renderer within the content view, in physical pixels; the
    // scale factor accounts for retina backing
    void set_frame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, double scale);

    // initialize the native layer, isolated contexts, font and ImGui backend
    void initialize();

    // teardown the ImGui backend and release the isolated contexts
    void terminate();

    // render one frame: activate contexts, begin an ImGui frame, invoke the
    // renderer callback, and encode the draw data to the platform backend
    void render_frame(const std::function<void()>& renderer);

    // render the charts panel content
    void render();

    // number of charts currently managed by this renderer
    [[nodiscard]] size_t chart_count() const { return m_charts.size(); }

    // data path, wired in stage 2
    void update(ExpressionManager& expression_manager, const StepInformation& step_information, AbscissaScale abscissa_scale, const std::vector<std::vector<std::string>>& suggested_plots);

    Chart* add_chart();

    void delete_all_charts();

    void refresh_charts(int frames = 3);

    // translate a relative Y position [0..1] to a chart index, clamped to valid range
    [[nodiscard]] size_t position_to_index(float position) const;

    // all expressions known to the loaded file, from the expression manager
    [[nodiscard]] std::vector<AnyExpression*> all_expressions() const;

    // current abscissa value range [min, max] from the loaded step information
    [[nodiscard]] std::pair<double, double> abscissa_range() const;

    // expressions currently plotted on the chart at the given index
    [[nodiscard]] std::vector<AnyExpression*> chart_selected_expressions(size_t chart_index) const;

    // steps currently selected on the chart at the given index
    [[nodiscard]] std::set<size_t> chart_selected_steps(size_t chart_index) const;

    // apply the given step selection to the chart at the given index and refresh
    void set_chart_selected_steps(size_t chart_index, const std::set<size_t>& steps);

    // step information of the loaded file, or nullptr when not loaded
    [[nodiscard]] const StepInformation* step_information() const;

    // evaluate a custom expression against the expression manager, or nullptr when invalid
    AnyExpression* evaluate_expression(const std::string& expression);

    // plot the given expressions on the chart at the given index and refresh
    void plot_chart_expressions(size_t chart_index, const std::set<AnyExpression*>& expressions);

    // position is a float [0..1] relative Y from the context menu
    void zoom_to_fit(float chart_position);

    void autorange(float chart_position);

    void zoom_abscissa_extent(float chart_position);

    void delete_all_plots(float chart_position);

    void delete_chart(float chart_position);

    // interactive drag-zoom lifecycle
    void zoom_drag_started(float x, float y);

    void zoom_drag_moved(float x, float y);

    void zoom_drag_ended();

    void zoom_drag_canceled();

    // hover readout callback and interaction
    using HoverCallback = std::function<void(const std::string&)>;

    void set_hover_callback(HoverCallback callback) { m_hover_callback = std::move(callback); }

    void hover_moved(float x, float y);

    void hover_ended();

private:
    friend class ChartsContextScope;

    void initialize_contexts();

    void terminate_contexts();

    void update_delta_time();

    [[nodiscard]] size_t compute_decimation_target() const;

    void update_decimation_target();

    void on_idle();

    void publish_hover();

    void* m_view = nullptr;

    void* m_imgui_context = nullptr;

    void* m_implot_context = nullptr;

    std::chrono::steady_clock::time_point m_last_frame_time;

    slint::Timer m_render_timer;
    int m_render_chart_frames = 0;

    // data path
    ExpressionManager* m_expression_manager = nullptr;
    StepInformation const* m_step_information = nullptr;
    AbscissaScale m_abscissa_scale = AbscissaScale::LINEAR;
    size_t m_decimate_target = -1;
    double m_backing_scale = 1.0;
    uint32_t m_logical_width = 0;

    std::vector<std::unique_ptr<Chart>> m_charts;
    size_t m_selected_chart_index = 0;
    std::tuple<float, float, float, float> m_zoom_selection = {-1, -1, -1, -1};

    // hover readout state
    slint::Timer m_hover_timer;
    double m_hover_abscissa_value = 0.0;
    size_t m_hover_chart_index = 0;
    bool m_hover_in_plot = false;
    std::string m_last_hover_text;
    HoverCallback m_hover_callback;

    ImVec4 m_background_color;

#ifdef __APPLE__
    void* m_layer = nullptr;
    void* m_command_queue = nullptr;
#endif
};
