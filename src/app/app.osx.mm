#import <AppKit/AppKit.h>

#include <spdlog/spdlog.h>

#include "app.h"

void platform_initialize() {
    // log information
    spdlog::info("Starting KiCad Xyce Plugin on macOS");
}
