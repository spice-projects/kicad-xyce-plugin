#pragma once

#include <chrono>
#include <memory>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/panel.h>
#endif

#ifdef __linux__
#include <wx/glcanvas.h>
#endif

#include "../expression/expression_manager.h"
#include "../file/xyce_output_file.h"
#include "../step_information.h"
#include "chart.h"

#ifdef __linux__
using ChartsPanelBase = wxGLCanvas;
#else
using ChartsPanelBase = wxPanel;
#endif

class ChartsPanel : public ChartsPanelBase
{
public:
    using FftResultCallback = std::function<void(std::shared_ptr<XyceOutputFile>)>;

    explicit ChartsPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    ~ChartsPanel() override;

    void update(ExpressionManager& expression_manager, const StepInformation& step_information, const std::string& abscissa_label, AbscissaScale abscissa_scale);

    Chart* add_chart();

    void delete_all_charts();

    void refresh_charts(int frames = 3);

    void display_changed();

    void set_fft_result_callback(FftResultCallback callback) { m_fft_result_callback = std::move(callback); }

private:
    friend class ContextScope;

    WXWidget m_charts_panel = nullptr;
    void* m_imgui_context = nullptr;
    void* m_implot_context = nullptr;
    std::chrono::steady_clock::time_point m_last_frame_time;
    int m_render_chart_frames = 0;

    ExpressionManager* m_expression_manager = nullptr;
    StepInformation const* m_step_information = nullptr;
    std::string m_abscissa_label;
    AbscissaScale m_abscissa_scale = AbscissaScale::LINEAR;
    size_t m_decimate_target = -1;

    std::vector<std::unique_ptr<Chart>> m_charts;
    Chart* m_selected_chart = nullptr;
    size_t m_selected_chart_index = 0;
    std::tuple<float, float, float, float> m_zoom_selection = {-1, -1, -1, -1};
    std::tuple<double, double, double, double> m_zoom_window = {-1, -1, -1, -1};

    ImVec4 m_background_color;

    FftResultCallback m_fft_result_callback;

#ifdef __APPLE__
    void* m_metal_layer = nullptr;
    void* m_command_queue = nullptr;
#endif

#ifdef __linux__
    std::unique_ptr<wxGLContext> m_gl_context;
#endif

    void initialize();

    void terminate();

    void initialize_contexts();

    void terminate_contexts();

    void update_delta_time();

    bool update_bounds();

    [[nodiscard]] size_t compute_decimation_target() const;

    void update_decimation_target();

    void render_frame(const std::function<void()>&);

    void render();

    void process_mouse_event(const wxMouseEvent&);

    void process_mouse_wheel_event(const wxMouseEvent&);

    void process_key_event(const wxKeyEvent&, bool pressed);

    void process_character_event(const wxKeyEvent&);

    void process_focus_event(bool focused);

    void on_mouse_move(wxMouseEvent&);

    void on_mouse_button(wxMouseEvent&);

    void on_mouse_wheel(wxMouseEvent&);

    void on_key_down(wxKeyEvent&);

    void on_key_up(wxKeyEvent&);

    void on_character(wxKeyEvent&);

    void on_set_focus(wxFocusEvent&);

    void on_kill_focus(wxFocusEvent&);

    void on_idle(wxIdleEvent&);

    void on_paint(wxPaintEvent&);

    void on_size(wxSizeEvent&);

    void on_context_menu(const wxContextMenuEvent&);

    void on_menu_zoom_to_fit(wxCommandEvent&);

    void on_menu_autorange(wxCommandEvent&);

    void on_menu_zoom_abscissa_extent(wxCommandEvent&);

    void on_menu_add_remove_plots(wxCommandEvent&);

    void on_menu_delete_all_plots(wxCommandEvent&);

    void on_menu_step_tool(wxCommandEvent&);

    void on_menu_add_chart(wxCommandEvent&);

    void on_menu_delete_chart(wxCommandEvent&);

    void on_menu_new_window(wxCommandEvent&);

    void on_menu_calculate_fft(wxCommandEvent&);
};
