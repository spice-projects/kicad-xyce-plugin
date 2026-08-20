#include <chrono>

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <imgui.h>
#include <imgui_impl_metal.h>
#include <implot.h>
#include <slint.h>
#include <spdlog/spdlog.h>

#include "../apple_metal.h"
#include "../font_data.h"
#include "charts_renderer.h"

namespace
{
    // wx-free ImGui/ImPlot style, using a light palette that matches the slint
    // cupertino widgets instead of wxSystemSettings
    void apply_slint_style() {
        // light theme colors
        const ImVec4 text = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        const ImVec4 muted = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        const ImVec4 background = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
        const ImVec4 panel = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        const ImVec4 border = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        const ImVec4 accent = ImVec4(0.20f, 0.45f, 0.90f, 1.00f);
        // ImGui colors
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = text;
        colors[ImGuiCol_TextDisabled] = muted;
        colors[ImGuiCol_WindowBg] = background;
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = panel;
        colors[ImGuiCol_Border] = border;
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = panel;
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.85f, 0.90f, 0.98f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.75f, 0.82f, 0.95f, 1.00f);
        colors[ImGuiCol_TitleBg] = panel;
        colors[ImGuiCol_TitleBgActive] = panel;
        colors[ImGuiCol_Button] = ImVec4(0.88f, 0.90f, 0.94f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.78f, 0.84f, 0.95f, 1.00f);
        colors[ImGuiCol_ButtonActive] = accent;
        colors[ImGuiCol_Header] = ImVec4(0.85f, 0.90f, 0.98f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.75f, 0.82f, 0.95f, 1.00f);
        colors[ImGuiCol_HeaderActive] = accent;
        colors[ImGuiCol_PlotLines] = accent;
        colors[ImGuiCol_PlotHistogram] = accent;
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.75f, 0.82f, 0.95f, 1.00f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.35f);
        // ImPlot colors
        ImVec4* plot_colors = ImPlot::GetStyle().Colors;
        plot_colors[ImPlotCol_FrameBg] = background;
        plot_colors[ImPlotCol_PlotBg] = panel;
        plot_colors[ImPlotCol_PlotBorder] = border;
        plot_colors[ImPlotCol_LegendBg] = panel;
        plot_colors[ImPlotCol_LegendBorder] = border;
        plot_colors[ImPlotCol_LegendText] = text;
        plot_colors[ImPlotCol_TitleText] = text;
        plot_colors[ImPlotCol_InlayText] = muted;
        plot_colors[ImPlotCol_AxisText] = text;
        plot_colors[ImPlotCol_AxisGrid] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        plot_colors[ImPlotCol_AxisTick] = border;
        plot_colors[ImPlotCol_Crosshairs] = border;
        plot_colors[ImPlotCol_Selection] = ImVec4(0.20f, 0.45f, 0.90f, 0.50f);
    }
} // namespace

// overlay view hosting the metal layer; the window resize event arrives as a
// frame change from appkit (autoresizing), so the renderer raises its own
// redraw by resizing the layer drawable and rendering in setFrameSize:
@interface ChartsOverlayView : NSView {
@private
    CAMetalLayer* _metal_layer;
    ChartsRenderer* _owner;
}

- (void)setup_layer:(CAMetalLayer*)layer owner:(ChartsRenderer*)owner;

@end

@implementation ChartsOverlayView

- (void)setup_layer:(CAMetalLayer*)layer owner:(ChartsRenderer*)owner {
    _metal_layer = layer;
    _owner = owner;
}

- (NSView*)hitTest:(NSPoint)point {
    // the overlay is render-only (mouse events are handled by the slint layer
    // underneath, e.g. the charts context menu); returning nil makes appkit
    // hit-test through this view so pointer events reach the winit view
    return nil;
}

- (void)setFrameSize:(NSSize)new_size {
    // forward to the parent view
    [super setFrameSize:new_size];
    // the window was resized, resize the layer drawable and re-render
    [self resync_drawable];
}

- (void)viewDidChangeBackingProperties {
    // forward to the parent view
    [super viewDidChangeBackingProperties];
    // the backing scale factor changed, resync the drawable and re-render
    [self resync_drawable];
}

- (void)resync_drawable {
    // the layer may not be attached yet
    if (!_metal_layer) {
        return;
    }
    // recompute the drawable size for the current backing scale factor
    const CGFloat scale = self.window ? self.window.backingScaleFactor : 1.0;
    const CGSize bounds = self.bounds.size;
    _metal_layer.drawableSize = CGSizeMake(bounds.width * scale, bounds.height * scale);
    _metal_layer.contentsScale = scale;
    // schedule a redraw on the owning renderer; the redraw loop is driven by
    // the render timer, so rendering must never happen from an appkit layout
    // callback (it would re-enter ImGui mid-frame), only the counter is bumped
    if (_owner && bounds.width > 0.0 && bounds.height > 0.0) {
        _owner->refresh_charts(1);
    }
}

@end

void ChartsRenderer::attach(slint::Window& window) {
    // check flag
    if (m_view)
        return;
    // the native content view may not exist yet on the very first show
    auto super_view = window.appkit_view();
    if (!super_view)
        return;
    // get static reference to MetalResourceManager
    auto resource_manager = MetalResourceManager::get_instance();
    // gpu for current view (associated to current display)
    auto gpu = resource_manager->get_gpu((__bridge void*)super_view);
    // create the overlay view with the metal layer
    auto overlay_view = [[ChartsOverlayView alloc] initWithFrame:super_view.bounds];
    auto metal_layer = [CAMetalLayer layer];
    metal_layer.device = (__bridge id<MTLDevice>)gpu.device;
    metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metal_layer.framebufferOnly = YES;
    metal_layer.opaque = YES;
    // bind the layer and resources to the overlay view
    [overlay_view setup_layer:metal_layer owner:this];
    // make the overlay view layer-backed
    overlay_view.wantsLayer = YES;
    overlay_view.layer = metal_layer;
    // track the content view size automatically; the window resize then raises
    // setFrameSize: on the overlay instead of requiring timer polling
    overlay_view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    // put the overlay above the slint content in z-order
    [super_view addSubview:overlay_view];
    // update class fields
    m_view = (__bridge void*)overlay_view;
    m_layer = (__bridge void*)metal_layer;
    m_command_queue = gpu.command_queue;
    // start the render timer
    m_render_timer.start(slint::TimerMode::Repeated, std::chrono::milliseconds(16), [this]() { on_idle(); });
    // log information
    spdlog::debug("ChartsRenderer: attached overlay view");
}

void ChartsRenderer::detach() {
    // check flag
    if (!m_view)
        return;
    // cast fields as apple types
    auto overlay_view = (__bridge NSView*)m_view;
    // remove the overlay view from its superview
    [overlay_view removeFromSuperview];
    // release the overlay view
    [overlay_view release];
    // update flag
    m_view = nullptr;
    m_layer = nullptr;
    m_command_queue = nullptr;
    // log information
    spdlog::debug("ChartsRenderer: detached overlay view");
}

void ChartsRenderer::set_frame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, double scale) {
    // check flag
    if (!m_view)
        return;
    // cast fields as apple types
    auto overlay_view = (__bridge NSView*)m_view;
    auto metal_layer = (__bridge CAMetalLayer*)m_layer;
    // position the overlay within the content view (physical to points)
    overlay_view.frame = CGRectMake(x / scale, y / scale, width / scale, height / scale);
    // update the layer drawable size for the backing scale factor
    metal_layer.drawableSize = CGSizeMake(width, height);
    metal_layer.contentsScale = scale;
    // store the geometry for the decimation target
    m_backing_scale = scale;
    m_logical_width = width;
    // log information
    spdlog::debug("ChartsRenderer: frame {}x{} at ({},{}) scale {}", width, height, x, y, scale);
}

void ChartsRenderer::initialize() {
    // check flag
    if (m_view == nullptr || m_imgui_context != nullptr)
        return;
    // create isolated contexts for this renderer
    initialize_contexts();
    // activate this renderer's isolated contexts for renderer initialization
    ChartsContextScope context_scope(*this);
    // style
    apply_slint_style();
    // ImGui configuration
    ImGuiIO& io = ImGui::GetIO();
    // font base size
    const float base_size = 14.0f;
    // keep the embedded font data owned by the generated array
    ImFontConfig font_cfg{};
    font_cfg.FontDataOwnedByAtlas = false;
    // screen scale factor for the current display
    const double scale = m_backing_scale > 0.0 ? m_backing_scale : 1.0;
    // add the embedded font with scaling for retina displays
    io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(static_cast<const void*>(Inter_Regular_ttf)), static_cast<int>(Inter_Regular_ttf_len), base_size * static_cast<float>(scale), &font_cfg);
    io.FontGlobalScale = 1.0f / static_cast<float>(scale);
    // update style
    ImGuiStyle& style = ImGui::GetStyle();
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;
    // bind the renderer backend
    auto resource_manager = MetalResourceManager::get_instance();
    auto gpu = resource_manager->get_gpu(m_view);
    ImGui_ImplMetal_Init((__bridge id<MTLDevice>)gpu.device);
    // background color for the panel clear, matches the slint alternate
    // background
    m_background_color = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
    // log information
    spdlog::debug("ChartsRenderer: initialized");
}

void ChartsRenderer::terminate() {
    // check flag
    if (!m_imgui_context)
        return;
    // preserve the active contexts while releasing this renderer's backend
    auto* previous_imgui_context = ImGui::GetCurrentContext();
    auto* previous_implot_context = ImPlot::GetCurrentContext();
    // retain this renderer's context addresses before releasing them
    auto* imgui_context = static_cast<ImGuiContext*>(m_imgui_context);
    auto* implot_context = static_cast<ImPlotContext*>(m_implot_context);
    // activate this renderer's isolated contexts for renderer teardown
    ImGui::SetCurrentContext(imgui_context);
    ImPlot::SetCurrentContext(implot_context);
    // shutdown renderer backend
    ImGui_ImplMetal_Shutdown();
    // release isolated contexts after the renderer backend
    terminate_contexts();
    // restore the contexts that were active before teardown
    ImPlot::SetCurrentContext(previous_implot_context == implot_context ? nullptr : previous_implot_context);
    ImGui::SetCurrentContext(previous_imgui_context == imgui_context ? nullptr : previous_imgui_context);
}

void ChartsRenderer::render_frame(const std::function<void()>& renderer) {
    // check flag
    if (!m_view || !m_imgui_context)
        return;
    // use objective-c++ memory management
    @autoreleasepool {
        // activate this renderer's isolated contexts for the complete frame
        ChartsContextScope context_scope(*this);
        // cast fields as apple types
        auto metal_layer = (__bridge CAMetalLayer*)m_layer;
        auto command_queue = (__bridge id<MTLCommandQueue>)m_command_queue;
        // degenerate frames (the overlay is sized to zero when the charts panel is hidden) must not enter ImGui; plotting into a zero-size viewport
        // leaves windows in an unbalanced state and breaks later frames
        const CGSize bounds = [metal_layer bounds].size;
        if (bounds.width <= 0.0 || bounds.height <= 0.0)
            return;
        // drawable
        id<CAMetalDrawable> drawable = [metal_layer nextDrawable];
        if (!drawable)
            return;
        // create render descriptor
        MTLRenderPassDescriptor* render_pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        render_pass_descriptor.colorAttachments[0].texture = drawable.texture;
        render_pass_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        render_pass_descriptor.colorAttachments[0].clearColor = MTLClearColorMake(m_background_color.x, m_background_color.y, m_background_color.z, m_background_color.w);
        render_pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        // create command buffer
        id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
        id<MTLRenderCommandEncoder> render_encoder = [command_buffer renderCommandEncoderWithDescriptor:render_pass_descriptor];
        // create metal frame
        ImGui_ImplMetal_NewFrame(render_pass_descriptor);
        // update the per-renderer frame duration
        update_delta_time();
        // scale used in the metal layer for retina displays
        const CGFloat scale = [metal_layer contentsScale];
        // ImGui io configuration
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(bounds.width), static_cast<float>(bounds.height));
        io.DisplayFramebufferScale = ImVec2(static_cast<float>(scale), static_cast<float>(scale));
        // start frame
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        // render frame content
        renderer();
        // render frame
        ImGui::Render();
        // get rendering commands
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), command_buffer, render_encoder);
        // process commands
        [render_encoder endEncoding];
        [command_buffer presentDrawable:drawable];
        [command_buffer commit];
    }
}
