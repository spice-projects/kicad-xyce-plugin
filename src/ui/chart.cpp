#include <wx/window.h>
#include <imgui_internal.h>

#include "chart.h"

Chart::Chart(wxWindow* parent, const wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE) {
    // bind event handlers
    Bind(wxEVT_PAINT, &Chart::on_paint, this);
    Bind(wxEVT_SIZE, &Chart::on_size, this);
    Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) { return; });
}

#ifndef __APPLE__
void Chart::initialize() {
    // check flag
    if (m_initialized)
        return;
    // update flag
    m_initialized = true;
}
#endif

void Chart::on_size(wxSizeEvent& event) {
    // check we have initialized
    if (m_initialized && m_metal_layer)
        update_bounds();
    // skip even
    event.Skip();
}

void Chart::on_paint(wxPaintEvent&) {
    // required call
    wxPaintDC dc(this);
    // initialize widget (only the first time)
    initialize();
    // set panel context
    ImGui::SetCurrentContext(m_imgui_ctx);
    // ImPlot::SetCurrentContext(m_implot_ctx);

    // render
    render_frame([]()-> void {
        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                     ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoCollapse |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                                     ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("chart", nullptr, flags);

        // ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red Text");
        // ImGui::Text("Hello World");

        ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(0, 0, 0, 255)); // Normal background
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(20, 20, 20, 255)); // Slightly lighter on hover

        ImGui::Selectable("Clickable Text with Black Background", true);

        ImGui::PopStyleColor(2);

        ImGui::End();
    });
}
