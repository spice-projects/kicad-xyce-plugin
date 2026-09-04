#include "glfw_window_adapter.h"

GlFWWindowAdapter::GlFWWindowAdapter() {}

slint::PhysicalSize GlFWWindowAdapter::size() {
    // TODO: Implement this function to return the actual size of the window.
    return slint::PhysicalSize({800, 600});
}

slint::platform::AbstractRenderer& GlFWWindowAdapter::renderer() {
    // TODO: Implement this function to return a reference to the renderer.
    return m_renderer;
}
