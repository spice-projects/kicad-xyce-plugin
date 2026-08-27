#include <algorithm>
#include <cstdint>
#include <vector>

#include <core/SkImage.h>
#include <core/SkPixmap.h>
#include <core/SkSamplingOptions.h>
#include <core/SkShader.h>
#include <core/SkVertices.h>
#include <imgui.h>

#include "imgui_impl_skia.h"

namespace
{
    // per-texture backend payload whose address doubles as the imtextureid in draw commands
    struct SkiaTexture
    {
        sk_sp<SkImage> image;
        SkPaint paint;
    };
} // namespace

ImGuiSkiaRenderer::ImGuiSkiaRenderer() {
    // capture the context this renderer attaches to
    m_context = ImGui::GetCurrentContext();
    // read io configuration of active context
    ImGuiIO& io = ImGui::GetIO();
    // fail fast when a second renderer attaches to the same context
    IM_ASSERT(io.BackendRendererUserData == nullptr && "skia renderer already attached to this context");
    // advertise multi-texture and vertex-offset support so imgui splits frames accordingly
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset;
    // register this renderer instance on context io
    io.BackendRendererUserData = this;
    // set backend renderer name for diagnostics
    io.BackendRendererName = "imgui_impl_skia";
    // default to rgba32 for atlas and textures
    io.Fonts->TexDesiredFormat = ImTextureFormat_RGBA32;
    // disable antialiasing on fallback paint
    m_fallback_paint.setAntiAlias(false);
    // fallback color is opaque white
    m_fallback_paint.setColor(SK_ColorWHITE);
}

ImGuiSkiaRenderer::~ImGuiSkiaRenderer() {
    // preserve whichever context is active while tearing down
    ImGuiContext* previous = ImGui::GetCurrentContext();
    // reactivate our own context when teardown runs from elsewhere
    if (previous != m_context) {
        // activate our context for cleanup
        ImGui::SetCurrentContext(m_context);
    }
    // release every texture payload still attached to live textures
    for (ImTextureData* texture : ImGui::GetPlatformIO().Textures) {
        // delete allocated backend user data
        delete static_cast<SkiaTexture*>(texture->BackendUserData);
        // clear backend user data pointer
        texture->BackendUserData = nullptr;
        // invalidate texture id
        texture->SetTexID(ImTextureID_Invalid);
    }
    // clear backend renderer name on context io
    ImGui::GetIO().BackendRendererName = nullptr;
    // unregister this renderer from context io
    ImGui::GetIO().BackendRendererUserData = nullptr;
    // restore the previously active context after teardown
    if (previous != m_context) {
        // reactivate previous context
        ImGui::SetCurrentContext(previous);
    }
}

void ImGuiSkiaRenderer::new_frame() {
    // textures pump inside render_draw_data where the pending list is available
}

void ImGuiSkiaRenderer::upload_texture(ImTextureData& texture) {
    // linear filtering for texture sampling
    const SkSamplingOptions sampling(SkFilterMode::kLinear);
    // target skia image to populate
    sk_sp<SkImage> image;
    // describe the pixel block handed over by imgui
    if (texture.Format == ImTextureFormat_Alpha8) {
        // pixel count of alpha8 buffer
        const int count = texture.Width * texture.Height;
        // cast source pointer to byte array
        const auto* src = static_cast<const uint8_t*>(texture.GetPixels());
        // allocate rgba32 destination buffer
        std::vector<uint32_t> rgba(static_cast<size_t>(count));
        // expand alpha8 to rgba32 white with alpha so shader modulation preserves vertex rgb
        for (int i = 0; i < count; ++i) {
            // read source alpha byte
            const uint32_t a = src[i];
            // pack opaque white with source alpha
            rgba[static_cast<size_t>(i)] = (a << 24) | 0x00FFFFFFu;
        }
        // image info for expanded rgba32 surface
        const auto info = SkImageInfo::Make(texture.Width, texture.Height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        // copy expanded rgba32 pixels into owned raster image
        image = SkImages::RasterFromPixmapCopy(SkPixmap(info, rgba.data(), static_cast<size_t>(texture.Width * 4)));
    }
    else {
        // describe rgba32 pixel buffer from imgui
        const auto info = SkImageInfo::Make(texture.Width, texture.Height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        // copy into an owned image so the atlas buffer can grow independently
        image = SkImages::RasterFromPixmapCopy(SkPixmap(info, texture.GetPixels(), static_cast<size_t>(texture.GetPitch())));
    }
    // reuse the payload slot when the texture was uploaded before
    auto* holder = static_cast<SkiaTexture*>(texture.BackendUserData);
    // allocate new texture holder if not present
    if (holder == nullptr) {
        // create new texture container
        holder = new SkiaTexture();
        // store container in backend user data
        texture.BackendUserData = holder;
        // cast holder pointer to imgui texture identifier
        texture.SetTexID(reinterpret_cast<ImTextureID>(holder));
    }
    // swap in the refreshed image
    holder->image = std::move(image);
    // map normalized [0, 1] uv coordinates to pixel coordinates in the image shader
    const SkMatrix local_matrix = SkMatrix::Scale(1.0f / static_cast<float>(texture.Width), 1.0f / static_cast<float>(texture.Height));
    // reset paint to clean state
    holder->paint.reset();
    // disable antialiasing on shader paint
    holder->paint.setAntiAlias(false);
    // base paint color is white
    holder->paint.setColor(SK_ColorWHITE);
    // bind clamped image shader with inverse scale matrix
    holder->paint.setShader(holder->image->makeShader(SkTileMode::kClamp, SkTileMode::kClamp, sampling, local_matrix));
    // report completion back to imgui
    texture.SetStatus(ImTextureStatus_OK);
}

void ImGuiSkiaRenderer::pump_textures(ImDrawData& draw_data) {
    // nothing pending when the frame carries no texture list
    if (draw_data.Textures == nullptr) {
        return;
    }
    // honor every pending request; incremental updates re-upload fully for now
    for (ImTextureData* texture : *draw_data.Textures) {
        // skip empty slots defensively
        if (texture == nullptr) {
            continue;
        }
        // handle creation or update request
        if (texture->Status == ImTextureStatus_WantCreate || texture->Status == ImTextureStatus_WantUpdates) {
            // upload updated texture pixels
            upload_texture(*texture);
        }
        // handle destruction request
        else if (texture->Status == ImTextureStatus_WantDestroy) {
            // free texture holder memory
            delete static_cast<SkiaTexture*>(texture->BackendUserData);
            // clear backend pointer
            texture->BackendUserData = nullptr;
            // invalidate texture id
            texture->SetTexID(ImTextureID_Invalid);
            // mark texture as destroyed
            texture->SetStatus(ImTextureStatus_Destroyed);
        }
    }
}

const SkPaint& ImGuiSkiaRenderer::paint_for_command(const ImDrawCmd& cmd) const {
    // resolve the payload pointer carried as texture id
    const ImTextureID id = cmd.GetTexID();
    // unknown ids fall back to untextured drawing instead of crashing
    if (id == ImTextureID_Invalid) {
        return m_fallback_paint;
    }
    // bake-free lookup: the paint lives on the payload uploaded during pumping
    const auto* holder = reinterpret_cast<const SkiaTexture*>(id);
    // guard against ids whose payload never materialized
    if (holder == nullptr || holder->image == nullptr) {
        return m_fallback_paint;
    }
    // return referenced texture paint
    return holder->paint;
}

void ImGuiSkiaRenderer::render_draw_data(ImDrawData* draw_data, SkCanvas* canvas, float canvas_scale) {
    // nothing to draw without valid draw data or a target canvas
    if (draw_data == nullptr || !draw_data->Valid || canvas == nullptr) {
        return;
    }
    // bring every pending texture create/update/destroy up to date first
    pump_textures(*draw_data);
    // scale logical draw coordinates onto the physical target before anything else
    canvas->save();
    // apply canvas scale factor if not 1.0
    if (canvas_scale != 1.0f) {
        canvas->scale(canvas_scale, canvas_scale);
    }
    // draw data coordinates are relative to DisplayPos
    canvas->translate(-draw_data->DisplayPos.x, -draw_data->DisplayPos.y);
    // replay every draw list of the frame
    for (int list_index = 0; list_index < draw_data->CmdListsCount; ++list_index) {
        // current draw list
        const ImDrawList* cmd_list = draw_data->CmdLists[list_index];
        // vertex buffer pointer
        const ImDrawVert* imgui_vertices = cmd_list->VtxBuffer.Data;
        // index buffer pointer
        const ImDrawIdx* indices_base = cmd_list->IdxBuffer.Data;
        // size the staging arrays once per list; capacity persists across frames
        const int vertex_count = cmd_list->VtxBuffer.Size;
        // resize staging positions buffer
        m_staging_positions.resize(static_cast<size_t>(vertex_count));
        // resize staging texture coordinates buffer
        m_staging_tex_coords.resize(static_cast<size_t>(vertex_count));
        // resize staging colors buffer
        m_staging_colors.resize(static_cast<size_t>(vertex_count));
        // de-interleave pos/uv/col once per list; skia needs tightly packed arrays
        for (int i = 0; i < vertex_count; ++i) {
            // source vertex reference
            const ImDrawVert& vertex = imgui_vertices[i];
            // copy position
            m_staging_positions[static_cast<size_t>(i)] = SkPoint::Make(vertex.pos.x, vertex.pos.y);
            // copy texture coordinates
            m_staging_tex_coords[static_cast<size_t>(i)] = SkPoint::Make(vertex.uv.x, vertex.uv.y);
            // raw packed vertex color
            const uint32_t col = vertex.col;
            // rebuild through the channel-shift macros; this build packs colors as bgra while SkColor is argb
            m_staging_colors[static_cast<size_t>(i)] = SkColorSetARGB(static_cast<uint8_t>((col >> IM_COL32_A_SHIFT) & 0xFFu), static_cast<uint8_t>((col >> IM_COL32_R_SHIFT) & 0xFFu), static_cast<uint8_t>((col >> IM_COL32_G_SHIFT) & 0xFFu), static_cast<uint8_t>((col >> IM_COL32_B_SHIFT) & 0xFFu));
        }
        // replay every command of the list
        for (int cmd_index = 0; cmd_index < cmd_list->CmdBuffer.Size; ++cmd_index) {
            // current draw command
            const ImDrawCmd& cmd = cmd_list->CmdBuffer[cmd_index];
            // pass unknown callbacks through; this backend keeps no resettable state
            if (cmd.UserCallback != nullptr) {
                // invoke custom callback
                cmd.UserCallback(cmd_list, &cmd);
                // proceed to next command
                continue;
            }
            // skip empty commands
            if (cmd.ElemCount == 0) {
                continue;
            }
            // find the highest index referenced by this command slice
            unsigned int max_referenced_index = 0;
            // inspect index range for command
            for (unsigned int i = 0; i < cmd.ElemCount; ++i) {
                // update maximum referenced vertex index
                max_referenced_index = (std::max)(max_referenced_index, static_cast<unsigned int>(indices_base[cmd.IdxOffset + i]));
            }
            // tight range [VtxOffset, max+1]: satisfies shared pools and the skvertices 65535 cap
            const unsigned int drawable_vertices = max_referenced_index + 1;
            // pick the baked paint of the referenced texture
            const SkPaint& paint = paint_for_command(cmd);
            // copy the referenced window into an indexed skvertices blob
            const auto vertices = SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, static_cast<int>(drawable_vertices), m_staging_positions.data() + cmd.VtxOffset, m_staging_tex_coords.data() + cmd.VtxOffset, m_staging_colors.data() + cmd.VtxOffset, static_cast<int>(cmd.ElemCount), indices_base + cmd.IdxOffset);
            // scope the clip rect of this command
            const SkAutoCanvasRestore restore(canvas, true);
            // apply command scissor rectangle in canvas space
            canvas->clipRect(SkRect::MakeLTRB(cmd.ClipRect.x, cmd.ClipRect.y, cmd.ClipRect.z, cmd.ClipRect.w));
            // modulate combines the texture shader color with the vertex colors
            canvas->drawVertices(vertices, SkBlendMode::kModulate, paint);
        }
    }
    // drop the scale and translation applied above
    canvas->restore();
}
