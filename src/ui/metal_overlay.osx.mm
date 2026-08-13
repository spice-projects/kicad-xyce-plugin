#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <spdlog/spdlog.h>

#include "apple_metal.h"
#include "metal_overlay.h"

// overlay view hosting the metal layer; the window resize event arrives as a
// frame change from appkit (autoresizing), so the metal surface raises its own
// redraw by resizing the layer drawable and rendering in setFrameSize:
@interface MetalOverlayView : NSView {
@private
    CAMetalLayer* _metal_layer;
    MetalOverlay* _owner;
}

- (void)setup_layer:(CAMetalLayer*)layer owner:(MetalOverlay*)owner;

@end

@implementation MetalOverlayView

- (void)setup_layer:(CAMetalLayer*)layer owner:(MetalOverlay*)owner {
    _metal_layer = layer;
    _owner = owner;
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
    // raise the redraw on the owning overlay
    if (_owner) {
        _owner->render();
    }
}

@end

MetalOverlay::~MetalOverlay() {
    // detach the overlay view from its content view
    detach();
}

void MetalOverlay::attach(void* content_view) {
    // check flag
    if (m_view)
        return;
    // platform NSView*
    auto super_view = (__bridge NSView*)content_view;
    // get static reference to MetalResourceManager
    auto resource_manager = MetalResourceManager::get_instance();
    // gpu for current view (associated to current display)
    auto gpu = resource_manager->get_gpu(content_view);
    // create the overlay view with the metal layer
    auto overlay_view = [[MetalOverlayView alloc] initWithFrame:super_view.bounds];
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
    // log information
    spdlog::debug("MetalOverlay: attached overlay view");
}

void MetalOverlay::detach() {
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
    spdlog::debug("MetalOverlay: detached overlay view");
}

void MetalOverlay::set_frame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, double scale) {
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
    // log information
    spdlog::debug("MetalOverlay: frame {}x{} at ({},{}) scale {}", width, height, x, y, scale);
}

void MetalOverlay::render() {
    // check flag
    if (!m_view)
        return;
    // use objective-c++ memory management
    @autoreleasepool {
        // cast fields as apple types
        auto metal_layer = (__bridge CAMetalLayer*)m_layer;
        auto command_queue = (__bridge id<MTLCommandQueue>)m_command_queue;
        // drawable
        id<CAMetalDrawable> drawable = [metal_layer nextDrawable];
        if (!drawable)
            return;
        // create render descriptor
        MTLRenderPassDescriptor* render_pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        render_pass_descriptor.colorAttachments[0].texture = drawable.texture;
        render_pass_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        render_pass_descriptor.colorAttachments[0].clearColor = MTLClearColorMake(1.0, 0.0, 0.0, 1.0);
        render_pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        // create command buffer
        id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
        id<MTLRenderCommandEncoder> render_encoder = [command_buffer renderCommandEncoderWithDescriptor:render_pass_descriptor];
        // process commands
        [render_encoder endEncoding];
        [command_buffer presentDrawable:drawable];
        [command_buffer commit];
    }
}