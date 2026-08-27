#include <gtest/gtest.h>

#include <imgui.h>
#include <implot.h>

#include <core/SkSurface.h>

#include "ui/charts_renderer.h"

TEST(ChartsRendererTest, constructs_and_renders_demo_frame) {
    // arrange: create a dummy publish function that captures the emitted image
    slint::Image captured_image;
    bool image_published = false;

    ChartsRenderer renderer([&captured_image, &image_published](slint::Image image) {
        captured_image = std::move(image);
        image_published = true;
    });

    // act: set a viewport and trigger a render
    renderer.set_viewport(400.0f, 300.0f, 2.0);
    renderer.render();

    // assert: a frame was published with correct physical dimensions
    ASSERT_TRUE(image_published);
    const auto size = captured_image.size();
    EXPECT_EQ(size.width, 800);
    EXPECT_EQ(size.height, 600);

    // assert: the image buffer is not empty (contains rendered pixels)
    const auto rgba = captured_image.to_rgba8();
    ASSERT_TRUE(rgba);
    const auto* data = rgba->begin();
    bool has_non_zero = false;
    for (int y = 0; y < static_cast<int>(size.height) && !has_non_zero; ++y) {
        for (int x = 0; x < static_cast<int>(size.width) && !has_non_zero; ++x) {
            const auto pixel = data[y * size.width + x];
            if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0 || pixel.a != 0) {
                has_non_zero = true;
            }
        }
    }
    EXPECT_TRUE(has_non_zero);
}

TEST(ChartsRendererTest, viewport_change_triggers_surface_reallocation) {
    slint::Image captured_image;
    bool image_published = false;

    ChartsRenderer renderer([&captured_image, &image_published](slint::Image image) {
        captured_image = std::move(image);
        image_published = true;
    });

    // initial viewport
    renderer.set_viewport(200.0f, 150.0f, 1.0);
    renderer.render();
    ASSERT_TRUE(image_published);
    EXPECT_EQ(captured_image.size().width, 200);
    EXPECT_EQ(captured_image.size().height, 150);

    // change viewport - should reallocate surface
    image_published = false;
    renderer.set_viewport(400.0f, 300.0f, 2.0);
    renderer.render();
    ASSERT_TRUE(image_published);
    EXPECT_EQ(captured_image.size().width, 800);
    EXPECT_EQ(captured_image.size().height, 600);
}

TEST(ChartsRendererTest, chart_count_starts_at_zero) {
    ChartsRenderer renderer([](slint::Image) {});
    EXPECT_EQ(renderer.chart_count(), 0);
}

TEST(ChartsRendererTest, hidden_panel_does_not_publish) {
    // arrange: capture any published images
    slint::Image captured_image;
    bool image_published = false;

    ChartsRenderer renderer([&captured_image, &image_published](slint::Image image) {
        captured_image = std::move(image);
        image_published = true;
    });

    // act: set a visible viewport, then render
    renderer.set_viewport(200.0f, 150.0f, 1.0);
    renderer.render();
    ASSERT_TRUE(image_published);

    // act: reset the viewport and render again
    image_published = false;
    renderer.reset_viewport();
    renderer.render();

    // assert: no image published with a cleared viewport
    EXPECT_FALSE(image_published);
}

TEST(ChartsRendererTest, renders_after_viewport_set_then_frames_scheduled) {
    // arrange: capture published images, simulating the show_charts_view flow
    // where ensure_charts_renderer runs before set_charts_visible(true)
    slint::Image captured_image;
    bool image_published = false;

    ChartsRenderer renderer([&captured_image, &image_published](slint::Image image) {
        captured_image = std::move(image);
        image_published = true;
    });

    // act: set viewport first (as the slint init handler would fire it),
    // then render
    renderer.set_viewport(400.0f, 300.0f, 1.0);
    renderer.render();

    // assert: a frame was published with correct dimensions
    ASSERT_TRUE(image_published);
    EXPECT_EQ(captured_image.size().width, 400);
    EXPECT_EQ(captured_image.size().height, 300);
}
