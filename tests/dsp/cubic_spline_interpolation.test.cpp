#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "dsp/cubic_spline_interpolation.h"

TEST(CubicSplineInterpolationChecks, two_points_produces_linear_interpolation) {
    // arrange: with only 2 points, natural cubic spline reduces to linear (M_0 = M_1 = 0)
    const std::vector<double> x_old = {0.0, 2.0};
    const std::vector<double> y_old = {0.0, 10.0};
    const std::vector<double> x_new = {0.5, 1.0, 1.5};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 2.5);
    EXPECT_DOUBLE_EQ(result[1], 5.0);
    EXPECT_DOUBLE_EQ(result[2], 7.5);
}

TEST(CubicSplineInterpolationChecks, linear_data_produces_linear_interpolation) {
    // arrange: perfectly linear data yields zero second derivatives everywhere
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0};
    const std::vector<double> y_old = {0.0, 10.0, 20.0, 30.0};
    const std::vector<double> x_new = {0.5, 1.5, 2.5};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 5.0);
    EXPECT_DOUBLE_EQ(result[1], 15.0);
    EXPECT_DOUBLE_EQ(result[2], 25.0);
}

TEST(CubicSplineInterpolationChecks, three_point_symmetric_bell_shape) {
    // arrange: (0,0), (1,1), (2,0) produces a symmetric bell
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y_old = {0.0, 1.0, 0.0};
    const std::vector<double> x_new = {0.5, 1.5};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert: symmetric around x=1, both evaluate to 0.6875
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], 0.6875);
    EXPECT_DOUBLE_EQ(result[1], 0.6875);
}

TEST(CubicSplineInterpolationChecks, exact_grid_point_returns_exact_value) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0};
    const std::vector<double> y_old = {0.0, 10.0, 20.0, 30.0};
    const std::vector<double> x_new = {0.0, 1.0, 2.0, 3.0};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 4u);
    EXPECT_DOUBLE_EQ(result[0], 0.0);
    EXPECT_DOUBLE_EQ(result[1], 10.0);
    EXPECT_DOUBLE_EQ(result[2], 20.0);
    EXPECT_DOUBLE_EQ(result[3], 30.0);
}

TEST(CubicSplineInterpolationChecks, clamps_below_domain) {
    // arrange
    const std::vector<double> x_old = {1.0, 2.0, 3.0};
    const std::vector<double> y_old = {10.0, 20.0, 30.0};
    const std::vector<double> x_new = {-5.0, 0.0, 0.999};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 10.0);
    EXPECT_DOUBLE_EQ(result[1], 10.0);
    EXPECT_DOUBLE_EQ(result[2], 10.0);
}

TEST(CubicSplineInterpolationChecks, clamps_above_domain) {
    // arrange
    const std::vector<double> x_old = {1.0, 2.0, 3.0};
    const std::vector<double> y_old = {10.0, 20.0, 30.0};
    const std::vector<double> x_new = {3.001, 5.0, 100.0};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 30.0);
    EXPECT_DOUBLE_EQ(result[1], 30.0);
    EXPECT_DOUBLE_EQ(result[2], 30.0);
}

TEST(CubicSplineInterpolationChecks, clamps_mixed_out_of_bounds) {
    // arrange
    const std::vector<double> x_old = {2.0, 4.0, 6.0};
    const std::vector<double> y_old = {1.0, 5.0, 9.0};
    const std::vector<double> x_new = {-1.0, 3.0, 8.0};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 1.0);
    EXPECT_DOUBLE_EQ(result[1], 3.0);
    EXPECT_DOUBLE_EQ(result[2], 9.0);
}

TEST(CubicSplineInterpolationChecks, batch_multiple_signals) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0};
    const std::vector<double> y0 = {0.0, 10.0, 20.0, 30.0};
    const std::vector<double> y1 = {100.0, 110.0, 120.0, 130.0};
    const std::vector<double> y2 = {-10.0, 0.0, 10.0, 20.0};
    const std::span<const double> y_old_spans[] = {y0, y1, y2};
    const std::vector<double> x_new = {0.5, 1.5, 2.5};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, std::span<const std::span<const double>>(y_old_spans), x_new);
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

TEST(CubicSplineInterpolationChecks, non_uniform_grid) {
    // arrange: f(x) = x^3 at points 0, 2, 3
    const std::vector<double> x_old = {0.0, 2.0, 3.0};
    const std::vector<double> y_old = {0.0, 8.0, 27.0};
    const std::vector<double> x_new = {1.0, 2.5};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert: verified by manual computation
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], 0.25);
    EXPECT_DOUBLE_EQ(result[1], 16.5625);
}

TEST(CubicSplineInterpolationChecks, negative_values) {
    // arrange
    const std::vector<double> x_old = {-10.0, -5.0, 0.0};
    const std::vector<double> y_old = {-1.0, -0.5, 0.0};
    const std::vector<double> x_new = {-7.5, -2.5};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], -0.75);
    EXPECT_DOUBLE_EQ(result[1], -0.25);
}

TEST(CubicSplineInterpolationChecks, decreasing_y_values) {
    // arrange: linear decreasing data produces linear interpolation (M_i = 0 everywhere)
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y_old = {10.0, 5.0, 0.0};
    const std::vector<double> x_new = {0.5, 1.5};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert: linear interpolation since data is perfectly linear
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], 7.5);
    EXPECT_DOUBLE_EQ(result[1], 2.5);
}

TEST(CubicSplineInterpolationChecks, single_signal_wraps_to_batch) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y_old = {0.0, 10.0, 20.0};
    const std::vector<double> x_new = {0.25, 0.75, 1.5};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    const std::span<const double> y_spans[] = {y_old};
    const auto batch_result = fft::interpolate_cubic_spline(x_old, std::span<const std::span<const double>>(y_spans), x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    ASSERT_EQ(batch_result.size(), 1u);
    ASSERT_EQ(batch_result[0].size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], batch_result[0][0]);
    EXPECT_DOUBLE_EQ(result[1], batch_result[0][1]);
    EXPECT_DOUBLE_EQ(result[2], batch_result[0][2]);
}

TEST(CubicSplineInterpolationChecks, four_point_cubic_spline_continuity) {
    // arrange: non-linear data requiring proper tridiagonal solve
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0};
    const std::vector<double> y_old = {0.0, 0.0, 1.0, 0.0};
    const std::vector<double> x_new = {0.5, 1.0, 1.5, 2.0, 2.5};
    // act
    const auto result = fft::interpolate_cubic_spline(x_old, y_old, x_new);
    // assert: exact grid points
    ASSERT_EQ(result.size(), 5u);
    EXPECT_DOUBLE_EQ(result[1], 0.0);
    EXPECT_DOUBLE_EQ(result[3], 1.0);
}

TEST(CubicSplineInterpolationChecks, throws_on_size_mismatch) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y_old = {0.0, 1.0};
    const std::vector<double> x_new = {0.5};
    // act / assert
    EXPECT_THROW(fft::interpolate_cubic_spline(x_old, y_old, x_new), std::invalid_argument);
}

TEST(CubicSplineInterpolationChecks, batch_throws_on_size_mismatch) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y0 = {0.0, 10.0, 20.0};
    const std::vector<double> y1 = {100.0, 110.0};
    const std::span<const double> y_old_spans[] = {y0, y1};
    const std::vector<double> x_new = {0.5};
    // act / assert
    EXPECT_THROW(fft::interpolate_cubic_spline(x_old, std::span<const std::span<const double>>(y_old_spans), x_new), std::invalid_argument);
}

TEST(CubicSplineInterpolationChecks, throws_on_single_old_point) {
    // arrange
    const std::vector<double> x_old = {0.0};
    const std::vector<double> y_old = {5.0};
    const std::vector<double> x_new = {0.5};
    // act / assert
    EXPECT_THROW(fft::interpolate_cubic_spline(x_old, y_old, x_new), std::invalid_argument);
}

TEST(CubicSplineInterpolationChecks, throws_on_empty_new_grid) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y_old = {0.0, 10.0, 20.0};
    const std::vector<double> x_new;
    // act / assert
    EXPECT_THROW(fft::interpolate_cubic_spline(x_old, y_old, x_new), std::invalid_argument);
}
