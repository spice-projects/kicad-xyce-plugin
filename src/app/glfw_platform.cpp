#include "glfw_platform.h"

#include "glfw_window_adapter.h"

std::unique_ptr<slint::platform::WindowAdapter> GlFWPlatform::create_window_adapter() {
    // create new instance
    return std::make_unique<GlFWWindowAdapter>();
}
