#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "dsp/linear_interpolation.h"

TEST(LinearInterpolationChecks, interpolate_midpoint_between_two_points) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0};
    const std::vector<double> y_old = {0.0, 10.0};
    const std::vector<double> x_new = {0.5};
    // act
    const auto result = fft::interpolate_linear(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result[0], 5.0);
}

TEST(LinearInterpolationChecks, interpolate_batch_multiple_signals) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y0 = {0.0, 10.0, 20.0};
    const std::vector<double> y1 = {100.0, 110.0, 120.0};
    const std::vector<double> y2 = {-10.0, 0.0, 10.0};
    const std::span<const double> y_old_spans[] = {y0, y1, y2};
    const std::vector<double> x_new = {0.5, 1.5};
    // act
    const auto result = fft::interpolate_linear(x_old, std::span<const std::span<const double>>(y_old_spans), x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    ASSERT_EQ(result[0].size(), 2u);
    ASSERT_EQ(result[1].size(), 2u);
    ASSERT_EQ(result[2].size(), 2u);
    EXPECT_DOUBLE_EQ(result[0][0], 5.0);
    EXPECT_DOUBLE_EQ(result[0][1], 15.0);
    EXPECT_DOUBLE_EQ(result[1][0], 105.0);
    EXPECT_DOUBLE_EQ(result[1][1], 115.0);
    EXPECT_DOUBLE_EQ(result[2][0], -5.0);
    EXPECT_DOUBLE_EQ(result[2][1], 5.0);
}

TEST(LinearInterpolationChecks, interpolate_exact_grid_point_returns_exact_value) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0, 3.0};
    const std::vector<double> y_old = {0.0, 10.0, 20.0, 30.0};
    const std::vector<double> x_new = {0.0, 1.0, 2.0, 3.0};
    // act
    const auto result = fft::interpolate_linear(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 4u);
    EXPECT_DOUBLE_EQ(result[0], 0.0);
    EXPECT_DOUBLE_EQ(result[1], 10.0);
    EXPECT_DOUBLE_EQ(result[2], 20.0);
    EXPECT_DOUBLE_EQ(result[3], 30.0);
}

TEST(LinearInterpolationChecks, interpolate_clamps_below_domain) {
    // arrange
    const std::vector<double> x_old = {1.0, 2.0, 3.0};
    const std::vector<double> y_old = {10.0, 20.0, 30.0};
    const std::vector<double> x_new = {-5.0, 0.0, 0.999};
    // act
    const auto result = fft::interpolate_linear(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 10.0);
    EXPECT_DOUBLE_EQ(result[1], 10.0);
    EXPECT_DOUBLE_EQ(result[2], 10.0);
}

TEST(LinearInterpolationChecks, interpolate_clamps_above_domain) {
    // arrange
    const std::vector<double> x_old = {1.0, 2.0, 3.0};
    const std::vector<double> y_old = {10.0, 20.0, 30.0};
    const std::vector<double> x_new = {3.001, 5.0, 100.0};
    // act
    const auto result = fft::interpolate_linear(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 30.0);
    EXPECT_DOUBLE_EQ(result[1], 30.0);
    EXPECT_DOUBLE_EQ(result[2], 30.0);
}

TEST(LinearInterpolationChecks, interpolate_clamps_mixed_out_of_bounds) {
    // arrange
    const std::vector<double> x_old = {2.0, 4.0, 6.0};
    const std::vector<double> y_old = {1.0, 5.0, 9.0};
    const std::vector<double> x_new = {-1.0, 3.0, 8.0};
    // act
    const auto result = fft::interpolate_linear(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 1.0);
    EXPECT_DOUBLE_EQ(result[1], 3.0);
    EXPECT_DOUBLE_EQ(result[2], 9.0);
}

TEST(LinearInterpolationChecks, interpolate_non_uniform_grid) {
    // arrange
    const std::vector<double> x_old = {0.0, 0.1, 0.5, 2.0};
    const std::vector<double> y_old = {0.0, 1.0, 5.0, 20.0};
    const std::vector<double> x_new = {0.3, 1.0, 1.5};
    // act
    const auto result = fft::interpolate_linear(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 3.0);
    EXPECT_DOUBLE_EQ(result[1], 10.0);
    EXPECT_DOUBLE_EQ(result[2], 15.0);
}

TEST(LinearInterpolationChecks, interpolate_negative_values) {
    // arrange
    const std::vector<double> x_old = {-10.0, -5.0, 0.0};
    const std::vector<double> y_old = {-1.0, -0.5, 0.0};
    const std::vector<double> x_new = {-7.5, -2.5};
    // act
    const auto result = fft::interpolate_linear(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], -0.75);
    EXPECT_DOUBLE_EQ(result[1], -0.25);
}

TEST(LinearInterpolationChecks, interpolate_decreasing_y_values) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y_old = {10.0, 5.0, 0.0};
    const std::vector<double> x_new = {0.5, 1.5};
    // act
    const auto result = fft::interpolate_linear(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], 7.5);
    EXPECT_DOUBLE_EQ(result[1], 2.5);
}

TEST(LinearInterpolationChecks, interpolate_large_new_grid) {
    // arrange
    const std::vector<double> x_old = {0.0, 10.0};
    const std::vector<double> y_old = {0.0, 100.0};
    std::vector<double> x_new;
    for (int i = 0; i <= 1000; ++i) {
        x_new.push_back(i * 0.01);
    }
    // act
    const auto result = fft::interpolate_linear(x_old, y_old, x_new);
    // assert
    ASSERT_EQ(result.size(), 1001u);
    EXPECT_DOUBLE_EQ(result[0], 0.0);
    EXPECT_DOUBLE_EQ(result[500], 50.0);
    EXPECT_DOUBLE_EQ(result[1000], 100.0);
}

TEST(LinearInterpolationChecks, interpolate_throws_on_size_mismatch) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y_old = {0.0, 1.0};
    const std::vector<double> x_new = {0.5};
    // act / assert
    EXPECT_THROW(fft::interpolate_linear(x_old, y_old, x_new), std::invalid_argument);
}

TEST(LinearInterpolationChecks, interpolate_batch_throws_on_size_mismatch) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0, 2.0};
    const std::vector<double> y0 = {0.0, 10.0, 20.0};
    const std::vector<double> y1 = {100.0, 110.0}; // mismatched size
    const std::span<const double> y_old_spans[] = {y0, y1};
    const std::vector<double> x_new = {0.5};
    // act / assert
    EXPECT_THROW(fft::interpolate_linear(x_old, std::span<const std::span<const double>>(y_old_spans), x_new), std::invalid_argument);
}

TEST(LinearInterpolationChecks, interpolate_throws_on_single_old_point) {
    // arrange
    const std::vector<double> x_old = {0.0};
    const std::vector<double> y_old = {5.0};
    const std::vector<double> x_new = {0.5};
    // act / assert
    EXPECT_THROW(fft::interpolate_linear(x_old, y_old, x_new), std::invalid_argument);
}

TEST(LinearInterpolationChecks, interpolate_throws_on_empty_new_grid) {
    // arrange
    const std::vector<double> x_old = {0.0, 1.0};
    const std::vector<double> y_old = {0.0, 10.0};
    const std::vector<double> x_new;
    // act / assert
    EXPECT_THROW(fft::interpolate_linear(x_old, y_old, x_new), std::invalid_argument);
}
