#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include <gpu/ganesh/GrDirectContext.h>
#include <gpu/ganesh/mtl/GrMtlBackendContext.h>
#include <gpu/ganesh/mtl/GrMtlDirectContext.h>
#include <spdlog/spdlog.h>

#include "app.h"

sk_sp<GrDirectContext> platform_create_gr_context(GLFWwindow* window) {
    // get the native Cocoa window from GLFW
    NSWindow* ns_window = glfwGetCocoaWindow(window);
    if (!ns_window) {
        // log information
        spdlog::error("Failed to get native NSWindow from GLFW");
        // exit
        return nullptr;
    }
    // get the content view of the NSWindow
    auto content_view = [ns_window contentView];
    // create metal device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        // log information
        spdlog::error("Failed to create Metal device");
        // exit
        return nullptr;
    }
    // create command queue
    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (!queue) {
        // log information
        spdlog::error("Failed to create Metal command queue");
        // release device if queue creation fails
        [device release];
        // exit
        return nullptr;
    }
    // create metal layer
    CAMetalLayer* metal_layer = [CAMetalLayer layer];
    metal_layer.device = device;
    metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metal_layer.drawableSize = CGSizeMake(1280, 720);
    // attach the metal layer to the content view
    [content_view setWantsLayer:YES];
    [content_view setLayer:metal_layer];
    // resize the metal layer to match the content view's bounds
    [content_view setAutoresizesSubviews:YES];
    // create backend context
    GrMtlBackendContext backend;
    // use device and queue to create backend context
    backend.fDevice = sk_cfp<GrMTLHandle>(device);
    backend.fQueue = sk_cfp<GrMTLHandle>(queue);
    // create direct context
    return GrDirectContexts::MakeMetal(backend);
}
