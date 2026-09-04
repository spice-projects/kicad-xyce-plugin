#include <windows.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <gpu/ganesh/GrDirectContext.h>
#include <spdlog/spdlog.h>

#include "app.h"

sk_sp<GrDirectContext> platform_create_gr_context(GLFWwindow* window) {
    // get the native Win32 window from GLFW
    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd) {
        // log information
        spdlog::error("Failed to get native Win32 window from GLFW");
        // exit
        return nullptr;
    }
    // noop for now
    return nullptr;
}
