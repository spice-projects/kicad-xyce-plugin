#pragma once

#include <vector>

#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <imgui.h>

// replays dear imgui draw data onto a skia canvas for an attached imgui context
class ImGuiSkiaRenderer
{
public:
    // attach to the active imgui context and configure multi-texture backend
    ImGuiSkiaRenderer();

    // the renderer owns per-texture payloads and never shares them
    ImGuiSkiaRenderer(const ImGuiSkiaRenderer&) = delete;

    // the renderer owns per-texture payloads and never shares them
    ImGuiSkiaRenderer& operator=(const ImGuiSkiaRenderer&) = delete;

    // detach from the context and release every texture payload still attached
    ~ImGuiSkiaRenderer();

    // per-frame hook kept for parity with official backends
    void new_frame();

    // replay one frame onto the physical canvas with the given device scale
    void render_draw_data(ImDrawData* draw_data, SkCanvas* canvas, float canvas_scale);

private:
    // upload or refresh the skia image and baked shader for one texture
    void upload_texture(ImTextureData& texture);

    // honor create update destroy requests for textures attached to the draw data
    void pump_textures(ImDrawData& draw_data);

    // resolve the baked paint for a command falling back to untextured white
    [[nodiscard]] const SkPaint& paint_for_command(const ImDrawCmd& cmd) const;

    // context this renderer attached to during initialization
    ImGuiContext* m_context = nullptr;

    // de-interleaved vertex positions of the active draw list
    std::vector<SkPoint> m_staging_positions;

    // de-interleaved texture coordinates of the active draw list
    std::vector<SkPoint> m_staging_tex_coords;

    // channel-reordered vertex colors of the active draw list
    std::vector<SkColor> m_staging_colors;

    // fallback paint used for untextured geometry or invalid texture ids
    SkPaint m_fallback_paint;
};
