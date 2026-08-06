#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/gdicmn.h>
#endif

#include <GL/gl.h>

#include "charts_panel.h"
#include "im_context.h"
#include "wxwidgets_imgui.h"

static constexpr const char* FONT_PATH = KICAD_XYCE_FONTS_DIR "/Inter-Regular.ttf";

void ChartsPanel::initialize() {
    // check whether this panel already has a rendering context
    if (m_charts_panel)
        return;
    // retain the native handle for the panel lifetime
    m_charts_panel = GetHandle();
    // create the OpenGL context associated with this canvas
    m_gl_context = std::make_unique<wxGLContext>(this);
    // activate the OpenGL context before initializing the renderer
    SetCurrent(*m_gl_context);
    // create isolated contexts for this panel
    initialize_contexts();
    // activate this panel's isolated contexts for renderer initialization
    ContextScope context_scope(*this);
    // configure the shared application appearance
    PlatformStyle();
    // configure DPI-aware fonts
    auto& io = ImGui::GetIO();
    const float scale = static_cast<float>(GetDPIScaleFactor());
    io.Fonts->AddFontFromFileTTF(FONT_PATH, 14.0f * scale);
    io.FontGlobalScale = 1.0f / scale;
    // initialize the OpenGL renderer backend
    ImGui_ImplOpenGL3_Init("#version 130");
}

void ChartsPanel::terminate() {
    // check whether this panel was initialized
    if (!m_charts_panel)
        return;
    // activate the OpenGL context before releasing renderer resources
    SetCurrent(*m_gl_context);
    // preserve the active contexts while releasing this panel's renderer
    auto* previous_imgui_context = ImGui::GetCurrentContext();
    auto* previous_implot_context = ImPlot::GetCurrentContext();
    // retain this panel's context addresses before releasing them
    auto* imgui_context = static_cast<ImGuiContext*>(m_imgui_context);
    auto* implot_context = static_cast<ImPlotContext*>(m_implot_context);
    // activate this panel's isolated contexts for renderer teardown
    ImGui::SetCurrentContext(imgui_context);
    ImPlot::SetCurrentContext(implot_context);
    // release OpenGL resources while the graphics context remains current
    ImGui_ImplOpenGL3_Shutdown();
    // release the isolated contexts after the renderer backend
    terminate_contexts();
    // restore the contexts that were active before teardown
    ImPlot::SetCurrentContext(previous_implot_context == implot_context ? nullptr : previous_implot_context);
    ImGui::SetCurrentContext(previous_imgui_context == imgui_context ? nullptr : previous_imgui_context);
    // release the OpenGL context after all OpenGL resources
    m_gl_context.reset();
    // clear the native panel handle
    m_charts_panel = nullptr;
}

bool ChartsPanel::update_bounds() {
    // the OpenGL viewport is updated during each frame
    return m_charts_panel != nullptr;
}

void ChartsPanel::render_frame(const std::function<void()>& renderer) {
    // check whether the renderer was initialized
    if (!m_charts_panel || !m_gl_context)
        return;
    // activate this canvas' OpenGL context
    SetCurrent(*m_gl_context);
    // activate this panel's isolated contexts for the complete frame
    ContextScope context_scope(*this);
    // determine the current canvas size
    const wxSize size = GetClientSize();
    if (size.x <= 0 || size.y <= 0)
        return;
    // configure the OpenGL framebuffer
    glViewport(0, 0, size.x, size.y);
    glClearColor(m_background_color.x, m_background_color.y, m_background_color.z, m_background_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    // start the renderer and ImGui frames
    ImGui_ImplOpenGL3_NewFrame();
    // update the per-panel frame duration
    update_delta_time();
    ImGui::NewFrame();
    // configure the panel viewport
    auto& io = ImGui::GetIO();
    const float scale = static_cast<float>(GetDPIScaleFactor());
    io.DisplaySize = ImVec2(static_cast<float>(size.x), static_cast<float>(size.y));
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(size.x), static_cast<float>(size.y)));
    // render the chart contents
    renderer();
    // render ImGui draw data and present the canvas
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SwapBuffers();
}

void ChartsPanel::display_changed() {
    // recompute the decimation target for the new display scale
    update_decimation_target();
    // request a frame using the new DPI scale
    refresh_charts();
}
