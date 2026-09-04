#pragma once

#include <slint-platform.h>
#include <slint.h>

class GlFWWindowAdapter : public slint::platform::WindowAdapter
{
public:
    GlFWWindowAdapter();

    slint::PhysicalSize size() override;

    slint::platform::AbstractRenderer& renderer() override;

private:
    slint::platform::SoftwareRenderer m_renderer{slint::platform::SoftwareRenderer::RepaintBufferType::NewBuffer};
};
