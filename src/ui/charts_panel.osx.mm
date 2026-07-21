#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <AppKit/AppKit.h>

#include <imgui.h>
#include <imgui_impl_osx.h>
#include <imgui_impl_metal.h>
#include <implot.h>
#include <spdlog/spdlog.h>
#include <wx/wx.h>

#include "charts_panel.h"
#include "apple_metal.h"

static constexpr const char* FONT_PATH = KICAD_XYCE_FONTS_DIR "/Inter-Regular.ttf"; 

void ChartsPanel::initialize() {
    // check flag
    if (m_charts_panel)
        return;
    // platform NSView*
    m_charts_panel = GetHandle();
    // get static reference to MetalResourceManager
    auto resource_manager = MetalResourceManager::get_instance();
    // gpu for current view (associated to current display)
    auto gpu = resource_manager->get_gpu(m_charts_panel);
    // create metal layer for panel
    auto metal_layer = [CAMetalLayer layer];
    metal_layer.device = (__bridge id<MTLDevice>)gpu.device;
    metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metal_layer.framebufferOnly = YES;
    // screen
    NSScreen* screen = [m_charts_panel window].screen ?: [NSScreen mainScreen];
    // scale factor for retina displays
    CGFloat scale = [screen backingScaleFactor];
    // set metal layer for panel
    [m_charts_panel setWantsLayer:YES];
    [m_charts_panel setLayer:metal_layer];
    // dimensions
    wxSize sz = GetClientSize();
    metal_layer.bounds = CGRectMake(0, 0, sz.x, sz.y);
    metal_layer.drawableSize = CGSizeMake(sz.x * scale, sz.y * scale);
    metal_layer.contentsScale = scale;
    // create imgui isolated context for this panel
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    // style
    ImGui::StyleColorsClassic();
    // ImGui configuration
    ImGuiIO& io = ImGui::GetIO();
    // font base size
    const float base_size = 12.0f;
    // add font with scaling for retina displays
    io.Fonts->AddFontFromFileTTF(FONT_PATH, base_size * scale);
    io.FontGlobalScale = 1.0f / (float)scale;
    // update style
    ImGuiStyle& style = ImGui::GetStyle();
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;
    // bind platform specific hooks
    ImGui_ImplOSX_Init(m_charts_panel);
    ImGui_ImplMetal_Init((__bridge id<MTLDevice>)gpu.device);
    // update class fields
    m_metal_layer = metal_layer;
    m_command_queue = gpu.command_queue;
    // log information
    spdlog::debug("Display [{}]: initialized Metal layer bounds: {}x{} (scale: {})", [[screen localizedName] UTF8String], sz.x, sz.y, scale);
}

void ChartsPanel::terminate() {
    // check flag
    if (!m_charts_panel)
        return;
    // shudown backend
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplOSX_Shutdown();
    // imgui and implot
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    // update flag
    m_charts_panel = nullptr;
}

void ChartsPanel::render_frame(const std::function<void()>& renderer) {
    // check flag
    if (!m_charts_panel)
        return;
    // use objective-c++ memory management
    @autoreleasepool {
        // cast fields as apple types
        auto metal_layer = (__bridge CAMetalLayer *)m_metal_layer;
        auto command_queue = (__bridge id<MTLCommandQueue>)m_command_queue;
        // drawable
        id<CAMetalDrawable> drawable = [metal_layer nextDrawable];
        if (!drawable)
            return;
        // create render descriptor
        MTLRenderPassDescriptor* render_pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        render_pass_descriptor.colorAttachments[0].texture = drawable.texture;
        render_pass_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        render_pass_descriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.15f, 0.15f, 0.15f, 1.0f);
        render_pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        // create command buffer
        id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
        id<MTLRenderCommandEncoder> renderEncoder = [command_buffer renderCommandEncoderWithDescriptor:render_pass_descriptor];
        // create metal frame
        ImGui_ImplMetal_NewFrame(render_pass_descriptor);
        ImGui_ImplOSX_NewFrame(m_charts_panel);
        // scale used in the metal layer for retina displays
        auto scale = [metal_layer contentsScale];
        // position and size
        wxSize sz = GetClientSize();
        // ImGui io configuration
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)sz.x, (float)sz.y);
        io.DisplayFramebufferScale = ImVec2((float)scale, (float)scale);
        // start frame
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)sz.x, (float)sz.y));
        // render frame content
        renderer();
        // render frame
        ImGui::Render();
        // get rendering commands
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), command_buffer, renderEncoder);
        // process commands
        [renderEncoder endEncoding];
        [command_buffer presentDrawable:drawable];
        [command_buffer commit];
    }
}

void ChartsPanel::update_bounds() {
    // cast fields as apple types
    auto metal_layer = (__bridge CAMetalLayer *)m_metal_layer;
    // current panel size
    wxSize sz = GetClientSize();
    // get current screen
    NSScreen* screen = [m_charts_panel window].screen ?: [NSScreen mainScreen];
    // scale factor for retina displays
    CGFloat scale = [screen backingScaleFactor];
    // update layer
    metal_layer.bounds = CGRectMake(0, 0, sz.x, sz.y);
    metal_layer.drawableSize = CGSizeMake(sz.x * scale, sz.y * scale);
    metal_layer.contentsScale = scale;
}

void ChartsPanel::display_changed() {
    // // check flag
    // if (!m_charts_panel)
    //     return;    
    // // get static reference to MetalResourceManager
    // auto resource_manager = MetalResourceManager::get_instance();
    // // gpu for current view (associated to current display)
    // auto gpu = resource_manager->get_gpu(m_charts_panel);
    // // create metal layer for panel
    // auto metal_layer = [CAMetalLayer layer];
    // metal_layer.device = (__bridge id<MTLDevice>)gpu.device;
    // metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    // metal_layer.framebufferOnly = YES;
    // // screen
    // NSScreen* screen = [m_charts_panel window].screen ?: [NSScreen mainScreen];
    // // scale factor for retina displays
    // CGFloat scale = [screen backingScaleFactor];
    // // set metal layer for panel
    // [m_charts_panel setWantsLayer:YES];
    // [m_charts_panel setLayer:metal_layer];

    // // update class fields
    // m_metal_layer = metal_layer;
    // m_command_queue = gpu.command_queue;
    // // log information
    // spdlog::debug("Display [{}]: initialized Metal layer bounds: {}x{} (scale: {})", [[screen localizedName] UTF8String], sz.x, sz.y, scale);
}
