#include "im_context.h"

ContextScope::ContextScope(const ChartsPanel& charts_panel) {
    // preserve the active contexts while this panel owns backend calls
    m_imgui_context = ImGui::GetCurrentContext();
    m_implot_context = ImPlot::GetCurrentContext();
    // activate this panel's isolated contexts
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(charts_panel.m_imgui_context));
    ImPlot::SetCurrentContext(static_cast<ImPlotContext*>(charts_panel.m_implot_context));
}

ContextScope::~ContextScope() {
    // restore the contexts active before this scope
    ImPlot::SetCurrentContext(static_cast<ImPlotContext*>(m_implot_context));
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imgui_context));
}
