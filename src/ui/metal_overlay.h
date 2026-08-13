#pragma once

#include <cstdint>

// proof-of-concept metal overlay rendered on top of the slint content view;
// a solid-color clear keeps the renderer minimal until chart rendering lands

class MetalOverlay
{
public:
    MetalOverlay() = default;

    ~MetalOverlay();

    MetalOverlay(const MetalOverlay&) = delete;

    MetalOverlay& operator=(const MetalOverlay&) = delete;

    // attach the overlay view to the given content view (must be a NSView*)
    void attach(void* content_view);

    // detach the overlay view from its content view
    void detach();

    // position the overlay view within the content view, in physical pixels;
    // the scale factor accounts for retina backing
    void set_frame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, double scale);

    // render a solid-color frame; red makes the overlay region obvious
    void render();

private:
    void* m_view = nullptr;

    void* m_layer = nullptr;

    void* m_command_queue = nullptr;
};
