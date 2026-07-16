#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <AppKit/AppKit.h>

#include <imgui.h>
#include <imgui_impl_osx.h>
#include <imgui_impl_metal.h>
#include <wx/wx.h>

#include "chart.h"
#include "apple_metal.h"

void Chart::initialize() {
    // check flag
    if (m_initialized)
        return;
    // platform NSView*
    m_widget = GetHandle();
    // get static reference to MetalResourceManager
    auto resource_manager = MetalResourceManager::get_instance();
    // gpu for current view (associated to current display)
    MetalGPUHandle gpu = resource_manager->get_gpu(m_widget);
    // create metal layer for panel
    CAMetalLayer* metal_layer = [CAMetalLayer layer];
    metal_layer.device = (__bridge id<MTLDevice>)gpu.device;
    metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metal_layer.framebufferOnly = YES;
    // scale
    CGFloat scale = [[NSScreen mainScreen] backingScaleFactor];
    metal_layer.contentsScale = scale;
    // set metal layer for panel
    [m_widget setWantsLayer:YES];
    [m_widget setLayer:metal_layer];
    // dimensions
    wxSize sz = GetClientSize();
    metal_layer.bounds = CGRectMake(0, 0, sz.x, sz.y);
    metal_layer.drawableSize = CGSizeMake(sz.x * scale, sz.y * scale);
    // create imgui isolated context for this panel
    m_imgui_ctx = ImGui::CreateContext();
    // m_implot_ctx = ImPlot::CreateImPlotContext();
    // style
    ImGui::StyleColorsDark();
    // bind platform specific hooks
    ImGui_ImplOSX_Init(m_widget);
    ImGui_ImplMetal_Init((__bridge id<MTLDevice>)gpu.device);
    // update class fields
    m_metal_layer = metal_layer;
    m_command_queue = gpu.command_queue;
    m_initialized = true;
}

void Chart::render_frame(const std::function<void()>& renderer) {
    // use objective-c++ memory management
    @autoreleasepool {
        // cast fields as apple types
        CAMetalLayer* metal_layer = (__bridge CAMetalLayer *)m_metal_layer;
        id<MTLCommandQueue> command_queue = (__bridge id<MTLCommandQueue>)m_command_queue;
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
        ImGui_ImplOSX_NewFrame(m_widget);
        // start frame
        ImGui::NewFrame();
        // position and size
        wxSize sz = GetClientSize();
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

void Chart::update_bounds() {
    // cast fields as apple types
    CAMetalLayer* metal_layer = (__bridge CAMetalLayer *)m_metal_layer;
    // current panel size
    wxSize sz = GetClientSize();
    CGFloat scale = [[NSScreen mainScreen] backingScaleFactor];
    // update layer
    metal_layer.bounds = CGRectMake(0, 0, sz.x, sz.y);
    metal_layer.drawableSize = CGSizeMake(sz.x * scale, sz.y * scale);
}
