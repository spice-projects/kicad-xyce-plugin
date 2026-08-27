#include <algorithm>
#include <cstdint>
#include <vector>

#include <core/SkImage.h>
#include <core/SkPixmap.h>
#include <core/SkSamplingOptions.h>
#include <core/SkShader.h>
#include <core/SkVertices.h>

#include "imgui_impl_skia.h"

namespace
{
    // per-texture backend payload; its address doubles as the imtextureid stored in draw commands
    struct SkiaTexture
    {
        sk_sp<SkImage> image;
        SkPaint paint;
    };
} // namespace

ImGuiSkiaRenderer::ImGuiSkiaRenderer() {
    // capture the context this renderer attaches to
    m_context = ImGui::GetCurrentContext();
    // the io of that context carries the backend capabilities
    ImGuiIO& io = ImGui::GetIO();
    // fail fast when a second renderer attaches to the same context
    IM_ASSERT(io.BackendRendererUserData == nullptr && "skia renderer already attached to this context");
    // advertise multi-texture and vertex-offset support so imgui splits frames accordingly
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset;
    // register this renderer so a second attachment to the same context fails fast
    io.BackendRendererUserData = this;
    io.Fonts->TexDesiredFormat = ImTextureFormat_Alpha8;
    m_fallback_paint.setAntiAlias(false);
    m_fallback_paint.setColor(SK_ColorWHITE);
}

ImGuiSkiaRenderer::~ImGuiSkiaRenderer() {
    // preserve whichever context is active while tearing down
    ImGuiContext* previous = ImGui::GetCurrentContext();
    // reactivate our own context when teardown runs from elsewhere
    if (previous != m_context)
        ImGui::SetCurrentContext(m_context);
    // release every texture payload still attached to live textures
    for (ImTextureData* texture : ImGui::GetPlatformIO().Textures) {
        delete static_cast<SkiaTexture*>(texture->BackendUserData);
        texture->BackendUserData = nullptr;
        texture->SetTexID(ImTextureID_Invalid);
    }
    // unregister this renderer from the context io
    ImGui::GetIO().BackendRendererUserData = nullptr;
    // restore the previously active context after our teardown
    if (previous != m_context)
        ImGui::SetCurrentContext(previous);
}

void ImGuiSkiaRenderer::new_frame() {
    // textures pump inside render_draw_data where the pending list is available
}

void ImGuiSkiaRenderer::upload_texture(ImTextureData& texture) {
    const SkSamplingOptions sampling(SkFilterMode::kLinear);
    const auto color_type = texture.Format == ImTextureFormat_Alpha8 ? kAlpha_8_SkColorType : kRGBA_8888_SkColorType;
    const auto alpha_type = texture.Format == ImTextureFormat_Alpha8 ? kPremul_SkAlphaType : kUnpremul_SkAlphaType;
    // describe the pixel block handed over by imgui
    const auto info = SkImageInfo::Make(texture.Width, texture.Height, color_type, alpha_type);
    // copy into an owned image so the atlas buffer can grow independently
    auto image = SkImages::RasterFromPixmapCopy(SkPixmap(info, texture.GetPixels(), static_cast<size_t>(texture.GetPitch())));
    // reuse the payload slot when the texture was uploaded before
    auto* holder = static_cast<SkiaTexture*>(texture.BackendUserData);
    if (holder == nullptr) {
        holder = new SkiaTexture();
        texture.BackendUserData = holder;
        texture.SetTexID(reinterpret_cast<ImTextureID>(holder));
    }
    // swap in the refreshed image
    holder->image = std::move(image);
    // rebuild the baked paint around the new image shader
    holder->paint.reset();
    holder->paint.setAntiAlias(false);
    holder->paint.setColor(SK_ColorWHITE);
    holder->paint.setShader(holder->image->makeShader(sampling));
    // report completion back to imgui
    texture.SetStatus(ImTextureStatus_OK);
}

void ImGuiSkiaRenderer::pump_textures(ImDrawData& draw_data) {
    // nothing pending when the frame carries no texture list
    if (draw_data.Textures == nullptr)
        return;
    // honor every pending request; incremental updates re-upload fully for now
    for (ImTextureData* texture : *draw_data.Textures) {
        // skip empty slots defensively
        if (texture == nullptr)
            continue;
        if (texture->Status == ImTextureStatus_WantCreate || texture->Status == ImTextureStatus_WantUpdates)
            upload_texture(*texture);
        else if (texture->Status == ImTextureStatus_WantDestroy) {
            delete static_cast<SkiaTexture*>(texture->BackendUserData);
            texture->BackendUserData = nullptr;
            texture->SetTexID(ImTextureID_Invalid);
            texture->SetStatus(ImTextureStatus_Destroyed);
        }
    }
}

const SkPaint& ImGuiSkiaRenderer::paint_for_command(const ImDrawCmd& cmd) const {
    // resolve the payload pointer carried as texture id
    const ImTextureID id = cmd.GetTexID();
    // unknown ids fall back to untextured drawing instead of crashing
    if (id == ImTextureID_Invalid)
        return m_fallback_paint;
    // bake-free lookup: the paint lives on the payload uploaded during pumping
    const auto* holder = reinterpret_cast<const SkiaTexture*>(id);
    // guard against ids whose payload never materialized
    if (holder->image == nullptr)
        return m_fallback_paint;
    return holder->paint;
}

void ImGuiSkiaRenderer::render_draw_data(ImDrawData* draw_data, SkCanvas* canvas, float canvas_scale) {
    // nothing to draw without valid draw data or a target canvas
    if (draw_data == nullptr || !draw_data->Valid || canvas == nullptr)
        return;
    // bring every pending texture create/update/destroy up to date first
    pump_textures(*draw_data);
    // scale logical draw coordinates onto the physical target before anything else
    canvas->save();
    if (canvas_scale != 1.0f)
        canvas->scale(canvas_scale, canvas_scale);
    // draw data coordinates are relative to DisplayPos
    canvas->translate(-draw_data->DisplayPos.x, -draw_data->DisplayPos.y);
    // replay every draw list of the frame
    for (int list_index = 0; list_index < draw_data->CmdListsCount; ++list_index) {
        const ImDrawList* cmd_list = draw_data->CmdLists[list_index];
        const ImDrawVert* imgui_vertices = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* indices_base = cmd_list->IdxBuffer.Data;
        // size the staging arrays once per list; capacity persists across frames
        const int vertex_count = cmd_list->VtxBuffer.Size;
        m_staging_positions.resize(static_cast<size_t>(vertex_count));
        m_staging_tex_coords.resize(static_cast<size_t>(vertex_count));
        m_staging_colors.resize(static_cast<size_t>(vertex_count));
        // de-interleave pos/uv/col once per list; skia needs tightly packed arrays
        for (int i = 0; i < vertex_count; ++i) {
            const ImDrawVert& vertex = imgui_vertices[i];
            m_staging_positions[static_cast<size_t>(i)] = SkPoint::Make(vertex.pos.x, vertex.pos.y);
            m_staging_tex_coords[static_cast<size_t>(i)] = SkPoint::Make(vertex.uv.x, vertex.uv.y);
            const uint32_t col = vertex.col;
            // rebuild through the channel-shift macros; this build packs colors as bgra while SkColor is argb
            m_staging_colors[static_cast<size_t>(i)] = SkColorSetARGB(static_cast<uint8_t>((col >> IM_COL32_A_SHIFT) & 0xFFu), static_cast<uint8_t>((col >> IM_COL32_R_SHIFT) & 0xFFu), static_cast<uint8_t>((col >> IM_COL32_G_SHIFT) & 0xFFu), static_cast<uint8_t>((col >> IM_COL32_B_SHIFT) & 0xFFu));
        }
        // replay every command of the list
        for (int cmd_index = 0; cmd_index < cmd_list->CmdBuffer.Size; ++cmd_index) {
            const ImDrawCmd& cmd = cmd_list->CmdBuffer[cmd_index];
            // pass unknown callbacks through; this backend keeps no resettable state
            if (cmd.UserCallback != nullptr) {
                cmd.UserCallback(cmd_list, &cmd);
                continue;
            }
            // find the highest index referenced by this command slice
            unsigned int max_referenced_index = 0;
            for (unsigned int i = 0; i < cmd.ElemCount; ++i)
                max_referenced_index = (std::max)(max_referenced_index, static_cast<unsigned int>(indices_base[cmd.IdxOffset + i]));
            // tight range [VtxOffset, max+1]: satisfies shared pools and the skvertices 65535 cap
            const unsigned int drawable_vertices = max_referenced_index + 1;
            // pick the baked paint of the referenced texture
            const SkPaint& paint = paint_for_command(cmd);
            // copy the referenced window into an indexed skvertices blob
            const auto vertices = SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, static_cast<int>(drawable_vertices), m_staging_positions.data() + cmd.VtxOffset, m_staging_tex_coords.data() + cmd.VtxOffset, m_staging_colors.data() + cmd.VtxOffset, static_cast<int>(cmd.ElemCount), indices_base + cmd.IdxOffset);
            // scope the clip rect of this command
            const SkAutoCanvasRestore restore(canvas, true);
            canvas->clipRect(SkRect::MakeLTRB(cmd.ClipRect.x, cmd.ClipRect.y, cmd.ClipRect.z, cmd.ClipRect.w));
            // explicit src-over composes the straight-alpha colors correctly
            canvas->drawVertices(vertices, SkBlendMode::kSrcOver, paint);
        }
    }
    // drop the scale and translation applied above
    canvas->restore();
}
