#pragma once

#include <memory>

#include <slint-platform.h>
#include <slint.h>

#include "glfw_window_adapter.h"

class GlFWPlatform : public slint::platform::Platform
{
public:
    GlFWPlatform(std::unique_ptr<GlFWWindowAdapter> adapter) :
        m_adapter(std::move(adapter)) {}

    std::unique_ptr<slint::platform::WindowAdapter> create_window_adapter() override { return m_adapter; }

private:
    std::unique_ptr<GlFWWindowAdapter> m_adapter;
};
