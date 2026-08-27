#include <chrono>
#include <cstdint>
#include <cstdio>

#include <slint.h>
#include <spike.h>

#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkImageInfo.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>
#include <core/SkSurface.h>
#include <core/SkVertices.h>

// phase-0 spike (skia-imgui-implot.md):
// 1. rasters an offscreen kRGBA_8888 surface (red/green halves plus a
//    vertex-colored triangle through the indexed SkVertices::MakeCopy overload)
// 2. reads the pixels straight into a slint::SharedPixelBuffer and shows them
//    as a slint image; linking vcpkg skia beside slint's own vendored skia in
//    this binary is the symbol-collision probe
// 3. logs window().scale_factor() after 2.5s and quits so the spike stays scriptable
int main() {
    // explicit rgba8888 target, never n32 which is bgra on apple platforms
    const auto info = SkImageInfo::Make(256, 256, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    auto surface = SkSurfaces::Raster(info);
    // abort early when the raster backend is unavailable
    if (surface == nullptr) {
        std::printf("spike: failed to create raster surface\n");
        return 1;
    }
    auto* canvas = surface->getCanvas();
    // left half red, right half green: a channel swap is immediately visible
    canvas->clear(SK_ColorRED);
    SkPaint green_paint;
    green_paint.setColor(SK_ColorGREEN);
    canvas->drawRect(SkRect::MakeLTRB(128.0f, 0.0f, 256.0f, 256.0f), green_paint);
    // triangle through the indexed MakeCopy overload the plan relies on
    constexpr SkPoint points[3] = {{32.0f, 224.0f}, {224.0f, 224.0f}, {128.0f, 32.0f}};
    constexpr SkColor colors[3] = {SK_ColorYELLOW, SK_ColorMAGENTA, SK_ColorCYAN};
    constexpr uint16_t indices[3] = {0, 1, 2};
    auto vertices = SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, 3, points, nullptr, colors, 3, indices);
    // report the missing overload instead of failing later in the pipeline
    if (vertices == nullptr) {
        std::printf("spike: indexed SkVertices::MakeCopy overload unavailable\n");
        return 1;
    }
    SkPaint vertex_paint;
    canvas->drawVertices(vertices, SkBlendMode::kSrcOver, vertex_paint);
    // single-copy readback into the shared pixel buffer handed to slint
    slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(256, 256);
    // verify the readback before anything reaches the screen
    if (!surface->readPixels(info, buffer.begin(), 256 * static_cast<int>(sizeof(slint::Rgba8Pixel)), 0, 0)) {
        std::printf("spike: readPixels failed\n");
        return 1;
    }
    // first byte sequence proves the memory order is r,g,b,a
    const auto* bytes = reinterpret_cast<const uint8_t*>(buffer.begin());
    std::printf("spike: first pixel memory order: %u %u %u %u (expect r g b a)\n", static_cast<unsigned>(bytes[0]), static_cast<unsigned>(bytes[1]), static_cast<unsigned>(bytes[2]), static_cast<unsigned>(bytes[3]));
    const slint::Rgba8Pixel& top_left = buffer.begin()[0];
    std::printf("spike: top-left       r=%3u g=%3u b=%3u a=%3u\n", static_cast<unsigned>(top_left.r), static_cast<unsigned>(top_left.g), static_cast<unsigned>(top_left.b), static_cast<unsigned>(top_left.a));
    const slint::Rgba8Pixel& bottom_right = buffer.begin()[256 * 256 - 1];
    std::printf("spike: bottom-right   r=%3u g=%3u b=%3u a=%3u\n", static_cast<unsigned>(bottom_right.r), static_cast<unsigned>(bottom_right.g), static_cast<unsigned>(bottom_right.b), static_cast<unsigned>(bottom_right.a));
    const slint::Rgba8Pixel& center = buffer.begin()[128 * 256 + 128];
    std::printf("spike: triangle-center r=%3u g=%3u b=%3u a=%3u\n", static_cast<unsigned>(center.r), static_cast<unsigned>(center.g), static_cast<unsigned>(center.b), static_cast<unsigned>(center.a));
    // show the frame as a plain slint image inside the layout
    auto ui = spike::SpikeWindow::create();
    ui->set_chart_image(slint::Image(buffer));
    ui->show();
    // quit automatically so the spike stays runnable in scripts
    slint::Timer quit_timer;
    quit_timer.start(slint::TimerMode::SingleShot, std::chrono::milliseconds(2500), [ui]() {
        std::printf("spike: window scale_factor = %.2f\n", ui->window().scale_factor());
        slint::quit_event_loop();
    });
    slint::run_event_loop();
    std::printf("spike: done\n");
    return 0;
}
