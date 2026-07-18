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

    Chart& add_chart(ExpressionManager& expression_manager, const StepInformation& step_information, Expression<double>& abscissa, const std::string& abscissa_label, AbscissaScale abscissa_scale, size_t decimate_target);

    void delete_all_charts();

    void refresh_charts();

private:
    WXWidget m_charts_panel = nullptr;

    std::vector<Chart> m_charts;

#ifdef __APPLE__
    void* m_metal_layer = nullptr;
    void* m_command_queue = nullptr;
#endif

    void initialize();

    void terminate();

    void on_size(wxSizeEvent&);

    void on_paint(wxPaintEvent&);

    void render_frame(const std::function<void()>&);

    void update_bounds();
};
