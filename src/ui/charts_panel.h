#pragma once

#include <wx/panel.h>
#include <wx/wx.h>

#include "chart.h"

#include "../step_information.h"
#include "../expression/expression.h"
#include "../expression/expression_manager.h"

class ChartsPanel : public wxPanel
{
public:
    explicit ChartsPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    ~ChartsPanel() override;

    void update_charts(ExpressionManager& expression_manager, const StepInformation& step_information, const std::string& abscissa_label, AbscissaScale abscissa_scale, size_t decimate_target);

    Chart* add_chart();

    void delete_all_charts();

    void refresh_charts();

private:
    WXWidget m_charts_panel = nullptr;
    bool m_render_charts = false;

    ExpressionManager* m_expression_manager = nullptr;
    StepInformation const* m_step_information = nullptr;
    std::string m_abscissa_label;
    AbscissaScale m_abscissa_scale = AbscissaScale::LINEAR;
    size_t m_decimate_target = -1;

    std::vector<std::unique_ptr<Chart>> m_charts;
    Chart *m_selected_chart = nullptr;
    size_t m_selected_chart_index = 0;
    std::tuple<float, float, float, float> m_zoom_selection = {-1, -1, -1, -1};
    std::tuple<double, double, double, double> m_zoom_window = {-1, -1, -1, -1};

#ifdef __APPLE__
    void* m_metal_layer = nullptr;
    void* m_command_queue = nullptr;
#endif

    void initialize();

    void terminate();

    void on_mouse_move(wxMouseEvent&);

    void on_mouse_button(wxMouseEvent&);

    void on_mouse_wheel(wxMouseEvent&);

    void on_idle(wxIdleEvent&);

    void on_paint(wxPaintEvent&);

    void on_size(wxSizeEvent&);

    void on_context_menu(const wxContextMenuEvent&);

    void on_menu_zoom_to_fit(wxCommandEvent&);

    void on_menu_autorange(wxCommandEvent&);

    void on_menu_zoom_abscissa_extent(wxCommandEvent&);

    void on_menu_delete_all_plots(wxCommandEvent&);

    void on_menu_add_chart(wxCommandEvent&);

    void on_menu_delete_chart(wxCommandEvent&);

    void render_frame(const std::function<void()>&);

    void update_bounds();

    void render();
};
