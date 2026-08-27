#pragma once

#include <vector>

#include <imgui.h>

#include <core/SkCanvas.h>
#include <core/SkPaint.h>

// replays dear imgui draw data into a skia canvas on behalf of one imgui context;
// blending contract: straight-alpha colors with explicit src-over, never kModulate
class ImGuiSkiaRenderer
{
public:
    // attach to the active imgui context and request an alpha8 font atlas
    ImGuiSkiaRenderer();

    // the renderer owns per-texture payloads and never shares them
    ImGuiSkiaRenderer(const ImGuiSkiaRenderer&) = delete;

    // the renderer owns per-texture payloads and never shares them
    ImGuiSkiaRenderer& operator=(const ImGuiSkiaRenderer&) = delete;

    // detach from the context and release every texture payload still attached
    ~ImGuiSkiaRenderer();

    // per-frame hook kept for parity with official backends; textures pump at render time
    void new_frame();

    // replay one frame; canvas_scale maps logical draw coordinates onto physical canvas pixels
    void render_draw_data(ImDrawData* draw_data, SkCanvas* canvas, float canvas_scale);

private:
    // build or refresh the skia image backing one imgui texture
    void upload_texture(ImTextureData& texture);

    // honor create/update/destroy requests for every texture pending on the draw data
    void pump_textures(ImDrawData& draw_data);

    // resolve the paint of one command; falls back to untextured white for unknown ids
    [[nodiscard]] const SkPaint& paint_for_command(const ImDrawCmd& cmd) const;

    // context this renderer attached to, reactivated for a safe teardown
    ImGuiContext* m_context = nullptr;

    // de-interleaved positions of the draw list being replayed, indexed by vertex position
    std::vector<SkPoint> m_staging_positions;

    // de-interleaved texture coordinates of the draw list being replayed
    std::vector<SkPoint> m_staging_tex_coords;

    // channel-reordered colors of the draw list being replayed
    std::vector<SkColor> m_staging_colors;

    // paint for commands referencing an unknown texture id
    SkPaint m_fallback_paint;
};
