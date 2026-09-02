#include <cstdint>

#include <gtest/gtest.h>

#include <slint.h>

#include <core/SkCanvas.h>
#include <core/SkImageInfo.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>
#include <core/SkSurface.h>

// pins the skia-to-slint handoff contract used by the chart pipeline: pixels
// must reach a slint::SharedPixelBuffer<slint::Rgba8Pixel> byte-exact in
// r,g,b,a memory order, never n32/bgra swapped
//
// the pipeline creates a kN32 surface (bgra on apple) and reads back as
// rgba8888 so skia does the swizzle; these tests verify that swizzle

TEST(SkiaRasterRoundtripTest, pixel_values_survive_readback_exactly) {
    // arrange
    constexpr int width = 4;
    constexpr int height = 1;
    const auto surface_info = SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType);
    auto surface = SkSurfaces::Raster(surface_info);
    auto* canvas = surface->getCanvas();
    canvas->clear(SK_ColorBLACK);
    SkPaint red_paint;
    red_paint.setColor(SK_ColorRED);
    canvas->drawRect(SkRect::MakeLTRB(0.0f, 0.0f, 1.0f, 1.0f), red_paint);
    SkPaint green_paint;
    green_paint.setColor(SK_ColorGREEN);
    canvas->drawRect(SkRect::MakeLTRB(1.0f, 0.0f, 2.0f, 1.0f), green_paint);
    SkPaint blue_paint;
    blue_paint.setColor(SK_ColorBLUE);
    canvas->drawRect(SkRect::MakeLTRB(2.0f, 0.0f, 3.0f, 1.0f), blue_paint);
    SkPaint veil_paint;
    veil_paint.setColor(SkColorSetARGB(128, 255, 255, 255));
    canvas->drawRect(SkRect::MakeLTRB(3.0f, 0.0f, 4.0f, 1.0f), veil_paint);
    // act
    slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(width, height);
    const auto readback_info = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    ASSERT_TRUE(surface->readPixels(readback_info, buffer.begin(), width * static_cast<int>(sizeof(slint::Rgba8Pixel)), 0, 0));
    // assert
    EXPECT_EQ(buffer.begin()[0], (slint::Rgba8Pixel{255, 0, 0, 255}));
    EXPECT_EQ(buffer.begin()[1], (slint::Rgba8Pixel{0, 255, 0, 255}));
    EXPECT_EQ(buffer.begin()[2], (slint::Rgba8Pixel{0, 0, 255, 255}));
    EXPECT_EQ(buffer.begin()[3], (slint::Rgba8Pixel{128, 128, 128, 255}));
}

TEST(SkiaRasterRoundtripTest, memory_layout_is_rgb_byte_order) {
    // arrange
    constexpr int width = 4;
    constexpr int height = 1;
    const auto surface_info = SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType);
    auto surface = SkSurfaces::Raster(surface_info);
    auto* canvas = surface->getCanvas();
    canvas->clear(SK_ColorBLACK);
    SkPaint red_paint;
    red_paint.setColor(SK_ColorRED);
    canvas->drawRect(SkRect::MakeLTRB(0.0f, 0.0f, 1.0f, 1.0f), red_paint);
    SkPaint green_paint;
    green_paint.setColor(SK_ColorGREEN);
    canvas->drawRect(SkRect::MakeLTRB(1.0f, 0.0f, 2.0f, 1.0f), green_paint);
    // act
    slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(width, height);
    const auto readback_info = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    ASSERT_TRUE(surface->readPixels(readback_info, buffer.begin(), width * static_cast<int>(sizeof(slint::Rgba8Pixel)), 0, 0));
    const auto* bytes = reinterpret_cast<const uint8_t*>(buffer.begin());
    // assert
    EXPECT_EQ(bytes[0], 255);
    EXPECT_EQ(bytes[1], 0);
    EXPECT_EQ(bytes[2], 0);
    EXPECT_EQ(bytes[3], 255);
    EXPECT_EQ(bytes[4], 0);
    EXPECT_EQ(bytes[5], 255);
    EXPECT_EQ(bytes[6], 0);
    EXPECT_EQ(bytes[7], 255);
}

TEST(SkiaRasterRoundtripTest, slint_image_roundtrip_preserves_pixel_values) {
    // arrange
    constexpr int width = 4;
    constexpr int height = 1;
    const auto surface_info = SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType);
    auto surface = SkSurfaces::Raster(surface_info);
    surface->getCanvas()->clear(SK_ColorRED);
    // act
    slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(width, height);
    const auto readback_info = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    ASSERT_TRUE(surface->readPixels(readback_info, buffer.begin(), width * static_cast<int>(sizeof(slint::Rgba8Pixel)), 0, 0));
    const slint::Image image(buffer);
    const auto round_tripped = image.to_rgba8();
    // assert
    ASSERT_TRUE(round_tripped.has_value());
    ASSERT_EQ(round_tripped->width(), buffer.width());
    ASSERT_EQ(round_tripped->height(), buffer.height());
    for (int i = 0; i < width * height; ++i)
        EXPECT_EQ(round_tripped->begin()[i], buffer.begin()[i]);
}
