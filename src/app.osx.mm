#import <AppKit/AppKit.h>

#include <spdlog/spdlog.h>

#include "app.h"
#include "ui/icon_data.h"

void platform_initialize() {
    // log information
    spdlog::info("Starting KiCad Xyce Plugin on macOS");
    // set application dock icon from embedded png bytes
    NSData* png_data = [NSData dataWithBytes:window_icon_512x512_png length:window_icon_512x512_png_len];
    // create nsimage from png data
    NSImage* image = [[NSImage alloc] initWithData:png_data];
    // check if image was loaded
    if (image) {
        // set the application icon in the dock
        [NSApp setApplicationIconImage:image];
    }
}
