#import <AppKit/AppKit.h>
#import <Metal/Metal.h>

#import "apple_metal.h"

MetalResourceManager* MetalResourceManager::get_instance() {
    // create instance
    static MetalResourceManager* instance = new MetalResourceManager();
    // exit
    return instance;
}

MetalGPUHandle MetalResourceManager::get_gpu(void* handle) {
    // control access to operation
    std::lock_guard<std::mutex> lock(m_mutex);
    // NSView pointer
    NSView* view = (__bridge NSView*)handle;
    // target device
    id<MTLDevice> target_device = nil;
    // screen for device
    NSScreen* screen = [view window].screen;
    if (screen != nil) {
        // find target device for screen
        NSDictionary* description = [screen deviceDescription];
        NSNumber* display_id_number = [description objectForKey:@"NSScreenNumber"];
        CGDirectDisplayID display_id = [display_id_number unsignedIntValue];
        target_device = CGDirectDisplayCopyCurrentMetalDevice(display_id);
    }
    // check we found device
    if (target_device == nil) {
        // check we have found the default device before
        if (m_default_device == nil) {
            // default device & command queue
            auto default_device = MTLCreateSystemDefaultDevice();
            m_default_queue = [default_device newCommandQueue];
            // update field
            m_default_device = default_device;
        }
        // exit
        return {m_default_device, m_default_queue};
    }
    // device key for dict lookup
    void* device_key = (__bridge void*)target_device;
    // find key in cache
    if (auto it = m_cache.find(device_key); it != m_cache.end()) {
        // return cache item
        return it->second;
    }
    // new GPU
    MetalGPUHandle new_handle;
    // device & command queue
    new_handle.device = target_device;
    new_handle.command_queue = [target_device newCommandQueue];
    // store it in cache
    m_cache[device_key] = new_handle;
    // use it
    return new_handle;
}
