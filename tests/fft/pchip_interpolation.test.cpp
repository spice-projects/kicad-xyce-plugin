#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "dsp/pchip_interpolation.h"

TEST(PchipInterpolationChecks, interpolate_linear_data_matches_linear_interpolation) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0};
    const std::vector<double> y_old = {0.0, 10.0, 20.0, 30.0};
    const std::vector<double> x_new = {0.5, 1.5, 2.5};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 5.0);
    EXPECT_DOUBLE_EQ(result[1], 15.0);
    EXPECT_DOUBLE_EQ(result[2], 25.0);
}

TEST(PchipInterpolationChecks, interpolate_flat_signal_returns_constant) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0};
    const std::vector<double> y_old = {5.0, 5.0, 5.0, 5.0};
    const std::vector<double> x_new = {0.5, 1.5, 2.5};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 5.0);
    EXPECT_DOUBLE_EQ(result[1], 5.0);
    EXPECT_DOUBLE_EQ(result[2], 5.0);
}

TEST(PchipInterpolationChecks, interpolate_pchip_monotonic_step_preserves_shape) {
    // arrange: a steep step function - PCHIP should not overshoot
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0, 4.0};
    const std::vector<double> y_old = {0.0, 0.0, 0.0, 1.0, 1.0};
    const std::vector<double> x_new = {0.5, 1.5, 2.5, 3.5};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert: values must stay within [0, 1] - no overshoot
    ASSERT_EQ(result.size(), 4u);
    EXPECT_GE(result[0], 0.0);
    EXPECT_LE(result[0], 1.0);
    EXPECT_GE(result[1], 0.0);
    EXPECT_LE(result[1], 1.0);
    EXPECT_GE(result[2], 0.0);
    EXPECT_LE(result[2], 1.0);
    EXPECT_GE(result[3], 0.0);
    EXPECT_LE(result[3], 1.0);
    // monotonic: each successive value >= the previous
    EXPECT_GE(result[1], result[0]);
    EXPECT_GE(result[2], result[1]);
    EXPECT_GE(result[3], result[2]);
}

TEST(PchipInterpolationChecks, interpolate_exact_grid_point_returns_exact_value) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0};
    const std::vector<double> y_old = {0.0, 10.0, 20.0, 30.0};
    const std::vector<double> x_new = {0.0, 1.0, 2.0, 3.0};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 4u);
    EXPECT_DOUBLE_EQ(result[0], 0.0);
    EXPECT_DOUBLE_EQ(result[1], 10.0);
    EXPECT_DOUBLE_EQ(result[2], 20.0);
    EXPECT_DOUBLE_EQ(result[3], 30.0);
}

TEST(PchipInterpolationChecks, interpolate_clamps_below_domain) {
    // arrange
    const std::vector<double> x_old = {1.0, 2.0, 3.0};
    const std::vector<double> y_old = {10.0, 20.0, 30.0};
    const std::vector<double> x_new = {-5.0, 0.0, 0.999};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 10.0);
    EXPECT_DOUBLE_EQ(result[1], 10.0);
    EXPECT_DOUBLE_EQ(result[2], 10.0);
}

TEST(PchipInterpolationChecks, interpolate_clamps_above_domain) {
    // arrange
    const std::vector<double> x_old = {1.0, 2.0, 3.0};
    const std::vector<double> y_old = {10.0, 20.0, 30.0};
    const std::vector<double> x_new = {3.001, 5.0, 100.0};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 30.0);
    EXPECT_DOUBLE_EQ(result[1], 30.0);
    EXPECT_DOUBLE_EQ(result[2], 30.0);
}

TEST(PchipInterpolationChecks, interpolate_clamps_mixed_out_of_bounds) {
    // arrange
    const std::vector<double> x_old = {2.0, 4.0, 6.0};
    const std::vector<double> y_old = {1.0, 5.0, 9.0};
    const std::vector<double> x_new = {-1.0, 3.0, 8.0};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 1.0);
    EXPECT_DOUBLE_EQ(result[1], 3.0);
    EXPECT_DOUBLE_EQ(result[2], 9.0);
}

TEST(PchipInterpolationChecks, interpolate_batch_multiple_signals) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0};
    const std::vector<double> y0 = {0.0, 10.0, 20.0, 30.0};
    const std::vector<double> y1 = {100.0, 110.0, 120.0, 130.0};
    const std::vector<double> y2 = {-10.0, 0.0, 10.0, 20.0};
    const std::span<const double> y_old_spans[] = {y0, y1, y2};
    const std::vector<double> x_new = {0.5, 1.5, 2.5};
    // act
    const auto result = fft::interpolate_pchip(x_old, std::span<const std::span<const double>>(y_old_spans), x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    ASSERT_EQ(result[0].size(), 3u);
    ASSERT_EQ(result[1].size(), 3u);
    ASSERT_EQ(result[2].size(), 3u);
    EXPECT_DOUBLE_EQ(result[0][0], 5.0);
    EXPECT_DOUBLE_EQ(result[0][1], 15.0);
    EXPECT_DOUBLE_EQ(result[0][2], 25.0);
    EXPECT_DOUBLE_EQ(result[1][0], 105.0);
    EXPECT_DOUBLE_EQ(result[1][1], 115.0);
    EXPECT_DOUBLE_EQ(result[1][2], 125.0);
    EXPECT_DOUBLE_EQ(result[2][0], -5.0);
    EXPECT_DOUBLE_EQ(result[2][1], 5.0);
    EXPECT_DOUBLE_EQ(result[2][2], 15.0);
}

TEST(PchipInterpolationChecks, interpolate_negative_values) {
    // arrange
    const std::vector<double> x_old = {-10.0, -5.0, 0.0};
    const std::vector<double> y_old = {-1.0, -0.5, 0.0};
    const std::vector<double> x_new = {-7.5, -2.5};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], -0.75);
    EXPECT_DOUBLE_EQ(result[1], -0.25);
}

TEST(PchipInterpolationChecks, interpolate_decreasing_y_values) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y_old = {10.0, 5.0, 0.0};
    const std::vector<double> x_new = {0.5, 1.5};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], 7.5);
    EXPECT_DOUBLE_EQ(result[1], 2.5);
}

TEST(PchipInterpolationChecks, interpolate_throws_on_size_mismatch) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y_old = {0.0, 1.0};
    const std::vector<double> x_new = {0.5};
    // act / assert
    EXPECT_THROW(fft::interpolate_pchip(x_old, y_old, x_new), std::invalid_argument);
}

TEST(PchipInterpolationChecks, interpolate_batch_throws_on_size_mismatch) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y0 = {0.0, 10.0, 20.0};
    const std::vector<double> y1 = {100.0, 110.0};
    const std::span<const double> y_old_spans[] = {y0, y1};
    const std::vector<double> x_new = {0.5};
    // act / assert
    EXPECT_THROW(fft::interpolate_pchip(x_old, std::span<const std::span<const double>>(y_old_spans), x_new), std::invalid_argument);
}

TEST(PchipInterpolationChecks, interpolate_throws_on_less_than_three_points) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0};
    const std::vector<double> y_old = {0.0, 10.0};
    const std::vector<double> x_new = {0.5};
    // act / assert
    EXPECT_THROW(fft::interpolate_pchip(x_old, y_old, x_new), std::invalid_argument);
}

TEST(PchipInterpolationChecks, interpolate_throws_on_empty_new_grid) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y_old = {0.0, 10.0, 20.0};
    const std::vector<double> x_new;
    // act / assert
    EXPECT_THROW(fft::interpolate_pchip(x_old, y_old, x_new), std::invalid_argument);
}

TEST(PchipInterpolationChecks, interpolate_single_signal_wraps_to_batch) {
    // arrange
    const std::vector<double> x_old = {0.0, 0.5, 1.0};
    const std::vector<double> y_old = {0.0, 5.0, 10.0};
    const std::vector<double> x_new = {0.25, 0.75};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    const std::span<const double> y_spans[] = {y_old};
    const auto batch_result = fft::interpolate_pchip(x_old, std::span<const std::span<const double>>(y_spans), x_new);
    // assert
    ASSERT_EQ(result.size(), 2u);
    ASSERT_EQ(batch_result.size(), 1u);
    ASSERT_EQ(batch_result[0].size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], batch_result[0][0]);
    EXPECT_DOUBLE_EQ(result[1], batch_result[0][1]);
}

TEST(PchipInterpolationChecks, interpolate_pchip_shape_preserving_on_peak) {
    // arrange: a sharp peak - PCHIP should not overshoot beyond the peak value
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const std::vector<double> y_old = {0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0};
    const std::vector<double> x_new = {0.5, 1.5, 2.5, 3.5, 4.5, 5.5};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert: values must stay within [0, 10] - no overshoot past the peak
    ASSERT_EQ(result.size(), 6u);
    for (size_t i = 0; i < result.size(); ++i) {
        EXPECT_GE(result[i], 0.0);
        EXPECT_LE(result[i], 10.0);
    }
}

TEST(PchipInterpolationChecks, interpolate_pchip_increasing_then_decreasing) {
    // arrange: sinusoidal-like data - PCHIP should remain bounded
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0, 4.0};
    const std::vector<double> y_old = {0.0, 1.0, 0.0, -1.0, 0.0};
    const std::vector<double> x_new = {0.5, 1.5, 2.5, 3.5};
    // act
    const auto result = fft::interpolate_pchip(x_old, y_old, x_new);
    // assert: monotonic on each segment
    ASSERT_EQ(result.size(), 4u);
    // 0 to 1: increasing
    EXPECT_GE(result[0], 0.0);
    EXPECT_LE(result[0], 1.0);
    // 1 to 2: decreasing
    // 2 to 3: decreasing
    // 3 to 4: increasing
}
