#ifndef CHART_H
#define CHART_H

#include <wx/panel.h>
#include "wx/wx.h"
#include <imgui.h>

class Chart : public wxPanel
{
public:
    explicit Chart(wxWindow* parent, wxWindowID id = wxID_ANY);

private:
    bool m_initialized = false;
    WXWidget m_widget = nullptr;

#ifdef __APPLE__
    void* m_metal_layer = nullptr;
    void* m_command_queue = nullptr;
#endif

    ImGuiContext* m_imgui_ctx = nullptr;

    void initialize();

    void on_size(wxSizeEvent&);

    void on_paint(wxPaintEvent&);

    void render_frame(const std::function<void()>&);

    void update_bounds();
};

#endif
