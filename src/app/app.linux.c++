#include <spdlog/spdlog.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3native.h>

#include <gpu/ganesh/GrDirectContext.h>
#include <spdlog/spdlog.h>

#include "app.h"

sk_sp<GrDirectContext> platform_create_gr_context(GLFWwindow* /* window */) {
    // noop for now
    return nullptr;
}
