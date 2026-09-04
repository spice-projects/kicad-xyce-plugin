#pragma once

#include <memory>

#include <slint-platform.h>
#include <slint.h>

class GlFWPlatform : public slint::platform::Platform
{
public:
    GlFWPlatform() {}

    std::unique_ptr<slint::platform::WindowAdapter> create_window_adapter() override;

private:
};
