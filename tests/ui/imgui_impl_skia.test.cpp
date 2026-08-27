#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <imgui.h>

#include <core/SkCanvas.h>
#include <core/SkImageInfo.h>
#include <core/SkSurface.h>

#include "ui/font_data.h"
#include "ui/imgui_impl_skia.h"

// golden-pixel checks for the imgui-to-skia replay; every test builds its own
// imgui context, renders one synthetic frame, and asserts exact raster output

TEST(ImGuiSkiaRendererTest, renders_solid_triangle_exactly) {
    // arrange
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(64.0f, 64.0f);
    ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
    const auto info = SkImageInfo::Make(64, 64, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    auto surface = SkSurfaces::Raster(info);
    surface->getCanvas()->clear(SK_ColorWHITE);
    {
        // scoped so the renderer detaches before the context dies
        ImGuiSkiaRenderer renderer;
        // act
        ImGui::NewFrame();
        ImGui::GetForegroundDrawList()->AddTriangleFilled(ImVec2(2, 60), ImVec2(60, 60), ImVec2(31, 4), IM_COL32(255, 0, 0, 255));
        ImGui::Render();
        renderer.render_draw_data(ImGui::GetDrawData(), surface->getCanvas(), 1.0f);
        std::vector<uint8_t> pixels(64 * 64 * 4);
        ASSERT_TRUE(surface->readPixels(info, pixels.data(), 64 * 4, 0, 0));
        // assert
        EXPECT_EQ(pixels[(40 * 64 + 31) * 4 + 0], 255);
        EXPECT_EQ(pixels[(40 * 64 + 31) * 4 + 1], 0);
        EXPECT_EQ(pixels[(40 * 64 + 31) * 4 + 2], 0);
        EXPECT_EQ(pixels[(40 * 64 + 31) * 4 + 3], 255);
        EXPECT_EQ(pixels[(1 * 64 + 1) * 4 + 0], 255);
        EXPECT_EQ(pixels[(1 * 64 + 1) * 4 + 1], 255);
        EXPECT_EQ(pixels[(1 * 64 + 1) * 4 + 2], 255);
        EXPECT_EQ(pixels[(1 * 64 + 1) * 4 + 3], 255);
    }
    ImGui::DestroyContext();
}

TEST(ImGuiSkiaRendererTest, composes_half_alpha_over_transparent_as_premultiplied_src_over) {
    // arrange
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(64.0f, 64.0f);
    ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
    const auto info = SkImageInfo::Make(64, 64, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    auto surface = SkSurfaces::Raster(info);
    {
        ImGuiSkiaRenderer renderer;
        // act
        ImGui::NewFrame();
        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(8, 8), ImVec2(56, 56), IM_COL32(0, 0, 255, 128));
        ImGui::Render();
        renderer.render_draw_data(ImGui::GetDrawData(), surface->getCanvas(), 1.0f);
        std::vector<uint8_t> pixels(64 * 64 * 4);
        ASSERT_TRUE(surface->readPixels(info, pixels.data(), 64 * 4, 0, 0));
        // assert
        EXPECT_EQ(pixels[(32 * 64 + 32) * 4 + 0], 0);
        EXPECT_EQ(pixels[(32 * 64 + 32) * 4 + 1], 0);
        EXPECT_EQ(pixels[(32 * 64 + 32) * 4 + 2], 128);
        EXPECT_EQ(pixels[(32 * 64 + 32) * 4 + 3], 128);
        EXPECT_EQ(pixels[(1 * 64 + 1) * 4 + 0], 0);
        EXPECT_EQ(pixels[(1 * 64 + 1) * 4 + 3], 0);
    }
    ImGui::DestroyContext();
}

TEST(ImGuiSkiaRendererTest, applies_clip_rects_per_command) {
    // arrange
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(64.0f, 64.0f);
    ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
    const auto info = SkImageInfo::Make(64, 64, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    auto surface = SkSurfaces::Raster(info);
    surface->getCanvas()->clear(SK_ColorWHITE);
    {
        ImGuiSkiaRenderer renderer;
        // act
        ImGui::NewFrame();
        auto* draw_list = ImGui::GetForegroundDrawList();
        draw_list->PushClipRect(ImVec2(0, 0), ImVec2(32, 64), true);
        draw_list->AddRectFilled(ImVec2(0, 16), ImVec2(64, 48), IM_COL32(255, 0, 0, 255));
        draw_list->PopClipRect();
        draw_list->PushClipRect(ImVec2(32, 0), ImVec2(64, 64), true);
        draw_list->AddRectFilled(ImVec2(0, 16), ImVec2(64, 48), IM_COL32(0, 128, 0, 255));
        draw_list->PopClipRect();
        ImGui::Render();
        renderer.render_draw_data(ImGui::GetDrawData(), surface->getCanvas(), 1.0f);
        std::vector<uint8_t> pixels(64 * 64 * 4);
        ASSERT_TRUE(surface->readPixels(info, pixels.data(), 64 * 4, 0, 0));
        // assert
        EXPECT_EQ(pixels[(32 * 64 + 16) * 4 + 0], 255);
        EXPECT_EQ(pixels[(32 * 64 + 16) * 4 + 1], 0);
        EXPECT_EQ(pixels[(32 * 64 + 48) * 4 + 0], 0);
        EXPECT_EQ(pixels[(32 * 64 + 48) * 4 + 1], 128);
        EXPECT_EQ(pixels[(8 * 64 + 16) * 4 + 0], 255);
        EXPECT_EQ(pixels[(8 * 64 + 16) * 4 + 1], 255);
        EXPECT_EQ(pixels[(56 * 64 + 48) * 4 + 1], 255);
    }
    ImGui::DestroyContext();
}

TEST(ImGuiSkiaRendererTest, splits_vtx_offset_windows_on_index_overflow) {
    // arrange
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(256.0f, 64.0f);
    ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
    constexpr int width = 256;
    constexpr int height = 64;
    const auto info = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    auto surface = SkSurfaces::Raster(info);
    surface->getCanvas()->clear(SK_ColorWHITE);
    {
        ImGuiSkiaRenderer renderer;
        ImGui::NewFrame();
        auto* draw_list = ImGui::GetForegroundDrawList();
        draw_list->AddTriangleFilled(ImVec2(4, 60), ImVec2(28, 60), ImVec2(16, 36), IM_COL32(255, 0, 0, 255));
        // fill past the 16-bit index space so imgui opens a second vertex window
        int added = 0;
        while (draw_list->CmdBuffer.size() < 2 && added < 100000) {
            const float x = static_cast<float>(64 + (added % 180));
            draw_list->AddTriangleFilled(ImVec2(x, 30), ImVec2(x + 10, 30), ImVec2(x + 5, 20), IM_COL32(0, 0, 255, 255));
            ++added;
        }
        ASSERT_GE(draw_list->CmdBuffer.size(), 2);
        draw_list->AddTriangleFilled(ImVec2(width - 28, 60), ImVec2(width - 4, 60), ImVec2(width - 16, 36), IM_COL32(0, 255, 0, 255));
        // act
        ImGui::Render();
        renderer.render_draw_data(ImGui::GetDrawData(), surface->getCanvas(), 1.0f);
        std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 4));
        ASSERT_TRUE(surface->readPixels(info, pixels.data(), width * 4, 0, 0));
        // assert
        EXPECT_EQ(pixels[(50 * width + 16) * 4 + 0], 255);
        EXPECT_EQ(pixels[(50 * width + 16) * 4 + 1], 0);
        EXPECT_EQ(pixels[(50 * width + width - 16) * 4 + 0], 0);
        EXPECT_EQ(pixels[(50 * width + width - 16) * 4 + 1], 255);
        EXPECT_EQ(pixels[(50 * width + width - 16) * 4 + 2], 0);
    }
    ImGui::DestroyContext();
}

TEST(ImGuiSkiaRendererTest, renders_alpha8_glyph_mask_as_white) {
    // arrange
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(64.0f, 64.0f);
    ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
    ImFontConfig font_config{};
    font_config.FontDataOwnedByAtlas = false;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(const_cast<void*>(static_cast<const void*>(Inter_Regular_ttf)), static_cast<int>(Inter_Regular_ttf_len), 24.0f, &font_config);
    const auto info = SkImageInfo::Make(64, 64, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    auto surface = SkSurfaces::Raster(info);
    {
        ImGuiSkiaRenderer renderer;
        // act
        ImGui::NewFrame();
        ImGui::GetForegroundDrawList()->AddText(ImVec2(4, 20), IM_COL32(255, 255, 255, 255), "Ag");
        ImGui::Render();
        renderer.render_draw_data(ImGui::GetDrawData(), surface->getCanvas(), 1.0f);
        std::vector<uint8_t> pixels(64 * 64 * 4);
        ASSERT_TRUE(surface->readPixels(info, pixels.data(), 64 * 4, 0, 0));
        int best_index = -1;
        uint8_t best_alpha = 0;
        for (int y = 16; y < 52; ++y) {
            for (int x = 2; x < 44; ++x) {
                const uint8_t alpha = pixels[(y * 64 + x) * 4 + 3];
                if (alpha > best_alpha) {
                    best_alpha = alpha;
                    best_index = (y * 64 + x) * 4;
                }
            }
        }
        // assert
        ASSERT_GT(best_alpha, 200);
        EXPECT_EQ(pixels[best_index + 0], 255);
        EXPECT_EQ(pixels[best_index + 1], 255);
        EXPECT_EQ(pixels[best_index + 2], 255);
        EXPECT_EQ(pixels[(60 * 64 + 60) * 4 + 3], 0);
    }
    ImGui::DestroyContext();
}
