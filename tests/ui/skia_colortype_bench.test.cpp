#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

#include <slint.h>

#include <core/SkCanvas.h>
#include <core/SkImageInfo.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>
#include <core/SkSurface.h>

namespace
{
    constexpr int kSurfaceWidth = 1800;
    constexpr int kSurfaceHeight = 558;
    constexpr int kIterations = 20;

    struct BenchResult
    {
        std::vector<long long> render_us;
        std::vector<long long> readback_us;
        std::vector<long long> total_us;
    };

    void draw_representative_content(SkCanvas* canvas, int w, int h) {
        SkPaint bg;
        bg.setColor(SkColorSetARGB(255, 237, 237, 237));
        canvas->drawRect(SkRect::MakeWH(static_cast<float>(w), static_cast<float>(h)), bg);

        SkPaint grid;
        grid.setColor(SkColorSetARGB(255, 217, 217, 217));
        grid.setStrokeWidth(1.0f);
        for (int y = 50; y < h; y += 50)
            canvas->drawLine(0.0f, static_cast<float>(y), static_cast<float>(w), static_cast<float>(y), grid);

        for (int x = 100; x < w; x += 100)
            canvas->drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x), static_cast<float>(h), grid);

        SkPaint trace1;
        trace1.setColor(SkColorSetARGB(255, 51, 115, 230));
        trace1.setStrokeWidth(2.0f);
        trace1.setAntiAlias(true);
        SkPaint trace2;
        trace2.setColor(SkColorSetARGB(255, 230, 115, 51));
        trace2.setStrokeWidth(2.0f);
        trace2.setAntiAlias(true);
        for (int x = 0; x < w - 50; x += 5) {
            float x0 = static_cast<float>(x);
            float x1 = static_cast<float>(x + 5);
            float y0 = static_cast<float>(h) * 0.2f + static_cast<float>(h) * 0.6f * (0.5f + 0.5f * sinf(static_cast<float>(x) * 0.01f));
            float y1 = static_cast<float>(h) * 0.2f + static_cast<float>(h) * 0.6f * (0.5f + 0.5f * sinf(static_cast<float>(x + 5) * 0.01f));
            float y2 = static_cast<float>(h) * 0.5f + static_cast<float>(h) * 0.3f * (0.5f + 0.5f * cosf(static_cast<float>(x) * 0.03f));
            float y3 = static_cast<float>(h) * 0.5f + static_cast<float>(h) * 0.3f * (0.5f + 0.5f * cosf(static_cast<float>(x + 5) * 0.03f));
            canvas->drawLine(x0, y0, x1, y1, trace1);
            canvas->drawLine(x0, y2, x1, y3, trace2);
        }

        SkPaint label_area;
        label_area.setColor(SkColorSetARGB(255, 250, 250, 250));
        canvas->drawRect(SkRect::MakeLTRB(0.0f, static_cast<float>(h) - 30.0f, static_cast<float>(w), static_cast<float>(h)), label_area);
    }

    BenchResult run_benchmark(SkColorType color_type) {
        BenchResult result;
        result.render_us.reserve(kIterations);
        result.readback_us.reserve(kIterations);
        result.total_us.reserve(kIterations);

        for (int i = 0; i < kIterations; ++i) {
            const auto info = SkImageInfo::Make(kSurfaceWidth, kSurfaceHeight, color_type, kPremul_SkAlphaType);
            auto surface = SkSurfaces::Raster(info);
            auto* canvas = surface->getCanvas();

            const auto t0 = std::chrono::steady_clock::now();
            draw_representative_content(canvas, kSurfaceWidth, kSurfaceHeight);
            const auto t1 = std::chrono::steady_clock::now();

            slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(kSurfaceWidth, kSurfaceHeight);
            const auto readback_info = SkImageInfo::Make(kSurfaceWidth, kSurfaceHeight, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
            if (!surface->readPixels(readback_info, buffer.begin(), kSurfaceWidth * static_cast<int>(sizeof(slint::Rgba8Pixel)), 0, 0)) {
                ADD_FAILURE() << "readPixels failed at iteration " << i;
                continue;
            }
            const slint::Image image(buffer);
            (void)image;
            const auto t2 = std::chrono::steady_clock::now();

            result.render_us.push_back(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
            result.readback_us.push_back(std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count());
            result.total_us.push_back(std::chrono::duration_cast<std::chrono::microseconds>(t2 - t0).count());
        }

        return result;
    }

    void print_stats(const char* label, const std::vector<long long>& values) {
        auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
        long long sum = std::accumulate(values.begin(), values.end(), 0LL);
        long long avg = sum / static_cast<long long>(values.size());
        std::printf("  %-25s  min=%5lldus  avg=%5lldus  max=%5lldus\n", label, *min_it, avg, *max_it);
    }

    void print_report(const char* ct_label, const BenchResult& result) {
        std::printf("\n=== %s (%dx%d, %d iterations) ===\n", ct_label, kSurfaceWidth, kSurfaceHeight, kIterations);
        print_stats("render (clear+draw)", result.render_us);
        print_stats("readback (pixels+slint)", result.readback_us);
        print_stats("total", result.total_us);
        std::fflush(stdout);
    }

    void verify_pixel_contract(SkColorType surface_ct) {
        const auto info = SkImageInfo::Make(2, 1, surface_ct, kPremul_SkAlphaType);
        auto surface = SkSurfaces::Raster(info);
        auto* canvas = surface->getCanvas();
        SkPaint red;
        red.setColor(SK_ColorRED);
        canvas->drawRect(SkRect::MakeLTRB(0.0f, 0.0f, 1.0f, 1.0f), red);
        SkPaint green;
        green.setColor(SK_ColorGREEN);
        canvas->drawRect(SkRect::MakeLTRB(1.0f, 0.0f, 2.0f, 1.0f), green);

        slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(2, 1);
        const auto readback_info = SkImageInfo::Make(2, 1, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        ASSERT_TRUE(surface->readPixels(readback_info, buffer.begin(), 2 * static_cast<int>(sizeof(slint::Rgba8Pixel)), 0, 0));

        EXPECT_EQ(buffer.begin()[0].r, 255);
        EXPECT_EQ(buffer.begin()[0].g, 0);
        EXPECT_EQ(buffer.begin()[0].b, 0);
        EXPECT_EQ(buffer.begin()[0].a, 255);
        EXPECT_EQ(buffer.begin()[1].r, 0);
        EXPECT_EQ(buffer.begin()[1].g, 255);
        EXPECT_EQ(buffer.begin()[1].b, 0);
        EXPECT_EQ(buffer.begin()[1].a, 255);
    }
} // namespace

TEST(SkiaColorTypeBench, DISABLED_kRGBA_8888) {
    // arrange / act
    verify_pixel_contract(kRGBA_8888_SkColorType);
    auto result = run_benchmark(kRGBA_8888_SkColorType);
    print_report("kRGBA_8888 CPU", result);
}

TEST(SkiaColorTypeBench, DISABLED_kBGRA_8888) {
    // arrange / act
    verify_pixel_contract(kBGRA_8888_SkColorType);
    auto result = run_benchmark(kBGRA_8888_SkColorType);
    print_report("kBGRA_8888 CPU", result);
}
