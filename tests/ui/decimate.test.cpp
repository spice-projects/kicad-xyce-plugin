#include <cmath>
#include <numbers>
#include <numeric>
#include <span>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "ui/decimate.h"

// ---------------------------------------------------------------------------
// test helpers
// ---------------------------------------------------------------------------

namespace
{
    // produce n evenly-spaced values in [0, 1]
    std::vector<double> make_linspace(size_t n) {
        std::vector<double> v(n);
        for (size_t i = 0; i < n; ++i)
            v[i] = (n > 1) ? (static_cast<double>(i) / static_cast<double>(n - 1)) : 0.0;
        return v;
    }

    // produce a single cycle of a sine wave with n samples
    std::vector<double> make_sine(size_t n) {
        std::vector<double> v(n);
        for (size_t i = 0; i < n; ++i)
            v[i] = std::sin(2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(n));
        return v;
    }

    // return true when every value in *output* appears in *original*
    bool is_subset_of(const View<double>& output, const std::span<const double>& original) {
        const std::unordered_set<double> original_set(original.begin(), original.end());
        for (size_t i = 0; i < output.size(); ++i)
            if (original_set.find(output[i]) == original_set.end())
                return false;
        return true;
    }

    // return true when every (x, y) pair in the output also appears in the original
    bool xy_pairs_coherent(const View<double>& x_out, const View<double>& y_out, const std::span<const double>& x_orig, const std::span<const double>& y_orig) {
        // build a set of all original (x, y) pairs
        std::unordered_set<size_t> pair_hashes;
        for (size_t i = 0; i < x_orig.size(); ++i) {
            // combine hashes of both coordinates
            size_t hx = std::hash<double>{}(x_orig[i]);
            size_t hy = std::hash<double>{}(y_orig[i]);
            pair_hashes.insert(hx ^ (hy << 32) ^ (hy >> 32));
        }
        for (size_t i = 0; i < x_out.size(); ++i) {
            size_t hx = std::hash<double>{}(x_out[i]);
            size_t hy = std::hash<double>{}(y_out[i]);
            if (pair_hashes.find(hx ^ (hy << 32) ^ (hy >> 32)) == pair_hashes.end())
                return false;
        }
        return true;
    }
} // namespace

// ========================================================================================
// short-circuit: input already within target — both spans returned unchanged
// ========================================================================================

TEST(DecimateXYChecks, nth_point_short_circuit_returns_original_spans) {
    // arrange
    auto y = make_sine(50);
    auto x = make_linspace(50);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 50, DECIMATE_NTH_POINT);
    // assert
    ASSERT_EQ(x_out.data(), x.data());
    ASSERT_EQ(y_out.data(), y.data());
}

TEST(DecimateXYChecks, min_max_short_circuit_returns_original_spans) {
    // arrange
    auto y = make_sine(50);
    auto x = make_linspace(50);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 50, DECIMATE_MIN_MAX);
    // assert
    ASSERT_EQ(x_out.data(), x.data());
    ASSERT_EQ(y_out.data(), y.data());
}

TEST(DecimateXYChecks, m4_short_circuit_returns_original_spans) {
    // arrange
    auto y = make_sine(50);
    auto x = make_linspace(50);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 50, DECIMATE_M4);
    // assert
    ASSERT_EQ(x_out.data(), x.data());
    ASSERT_EQ(y_out.data(), y.data());
}

TEST(DecimateXYChecks, lttb_short_circuit_returns_original_spans) {
    // arrange
    auto y = make_sine(50);
    auto x = make_linspace(50);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 50, DECIMATE_LTTB);
    // assert
    ASSERT_EQ(x_out.data(), x.data());
    ASSERT_EQ(y_out.data(), y.data());
}

TEST(DecimateXYChecks, rdp_short_circuit_returns_original_spans) {
    // arrange
    auto y = make_sine(50);
    auto x = make_linspace(50);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 50, DECIMATE_RDP);
    // assert
    ASSERT_EQ(x_out.data(), x.data());
    ASSERT_EQ(y_out.data(), y.data());
}

TEST(DecimateXYChecks, none_always_returns_original_spans) {
    // arrange
    auto y = make_sine(10);
    auto x = make_linspace(10);
    // act — target is intentionally lower than size; NONE ignores target
    auto [x_out, y_out] = decimate_xy(x, y, 2, DECIMATE_NONE);
    // assert
    ASSERT_EQ(x_out.data(), x.data());
    ASSERT_EQ(y_out.data(), y.data());
}

// ========================================================================================
// output length must not exceed the target
// ========================================================================================

TEST(DecimateXYChecks, nth_point_output_length_le_target) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 50, DECIMATE_NTH_POINT);
    // assert
    ASSERT_LE(x_out.size(), 50u);
    ASSERT_LE(y_out.size(), 50u);
}

TEST(DecimateXYChecks, min_max_output_length_le_target) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 50, DECIMATE_MIN_MAX);
    // assert
    ASSERT_LE(x_out.size(), 50u);
    ASSERT_LE(y_out.size(), 50u);
}

TEST(DecimateXYChecks, m4_output_length_le_target) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 50, DECIMATE_M4);
    // assert
    ASSERT_LE(x_out.size(), 50u);
    ASSERT_LE(y_out.size(), 50u);
}

TEST(DecimateXYChecks, rdp_output_length_le_target) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 50, DECIMATE_RDP);
    // assert
    ASSERT_LE(x_out.size(), 50u);
    ASSERT_LE(y_out.size(), 50u);
}

TEST(DecimateXYChecks, lttb_output_length_equals_target) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    size_t target = 150;
    // act
    auto [x_out, y_out] = decimate_xy(x, y, target, DECIMATE_LTTB);
    // assert — LTTB always produces exactly target points
    ASSERT_EQ(x_out.size(), target);
    ASSERT_EQ(y_out.size(), target);
}

// ========================================================================================
// x and y output must have equal length (coherence check)
// ========================================================================================

TEST(DecimateXYChecks, nth_point_output_x_and_y_have_equal_length) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_NTH_POINT);
    // assert
    ASSERT_EQ(x_out.size(), y_out.size());
}

TEST(DecimateXYChecks, min_max_output_x_and_y_have_equal_length) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_MIN_MAX);
    // assert
    ASSERT_EQ(x_out.size(), y_out.size());
}

TEST(DecimateXYChecks, m4_output_x_and_y_have_equal_length) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_M4);
    // assert
    ASSERT_EQ(x_out.size(), y_out.size());
}

TEST(DecimateXYChecks, lttb_output_x_and_y_have_equal_length) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_LTTB);
    // assert
    ASSERT_EQ(x_out.size(), y_out.size());
}

TEST(DecimateXYChecks, rdp_output_x_and_y_have_equal_length) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_RDP);
    // assert
    ASSERT_EQ(x_out.size(), y_out.size());
}

// ========================================================================================
// xy-pair coherence: every output (x[i], y[i]) must be a point from the original data
// ========================================================================================

TEST(DecimateXYChecks, nth_point_xy_pairs_are_coherent) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_NTH_POINT);
    // assert
    ASSERT_TRUE(xy_pairs_coherent(x_out, y_out, x, y));
}

TEST(DecimateXYChecks, min_max_xy_pairs_are_coherent) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_MIN_MAX);
    // assert
    ASSERT_TRUE(xy_pairs_coherent(x_out, y_out, x, y));
}

TEST(DecimateXYChecks, m4_xy_pairs_are_coherent) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_M4);
    // assert
    ASSERT_TRUE(xy_pairs_coherent(x_out, y_out, x, y));
}

TEST(DecimateXYChecks, lttb_xy_pairs_are_coherent) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_LTTB);
    // assert
    ASSERT_TRUE(xy_pairs_coherent(x_out, y_out, x, y));
}

TEST(DecimateXYChecks, rdp_xy_pairs_are_coherent) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_RDP);
    // assert
    ASSERT_TRUE(xy_pairs_coherent(x_out, y_out, x, y));
}

// ========================================================================================
// NTH_POINT
// ========================================================================================

TEST(DecimateXYChecks, nth_point_output_is_subset_of_input) {
    // arrange
    std::vector<double> y(1000);
    std::iota(y.begin(), y.end(), 0.0);
    auto x = make_linspace(1000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 50, DECIMATE_NTH_POINT);
    // assert
    ASSERT_TRUE(is_subset_of(y_out, y));
}

TEST(DecimateXYChecks, nth_point_first_and_last_included) {
    // arrange
    auto y = make_sine(1001);
    auto x = make_linspace(1001);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 100, DECIMATE_NTH_POINT);
    // assert
    ASSERT_DOUBLE_EQ(y_out[0], y[0]);
    ASSERT_DOUBLE_EQ(y_out[y_out.size() - 1], y[y.size() - 1]);
}

// ========================================================================================
// MIN_MAX
// ========================================================================================

TEST(DecimateXYChecks, min_max_output_contains_bucket_extremes) {
    // arrange
    std::vector<double> y(1000, 0.0);
    y[250] = 99.0;
    y[750] = -99.0;
    auto x = make_linspace(1000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 100, DECIMATE_MIN_MAX);
    // assert — both spike values must survive decimation
    bool found_pos = false;
    bool found_neg = false;
    for (double v : y_out) {
        if (v == 99.0)
            found_pos = true;
        if (v == -99.0)
            found_neg = true;
    }
    ASSERT_TRUE(found_pos);
    ASSERT_TRUE(found_neg);
}

TEST(DecimateXYChecks, min_max_output_is_subset_of_input) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_MIN_MAX);
    // assert
    ASSERT_TRUE(is_subset_of(y_out, y));
}

TEST(DecimateXYChecks, min_max_flat_signal_deduplication) {
    // arrange
    std::vector<double> y(10000, 1.0);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_MIN_MAX);
    // assert — deduplication must respect the target budget
    ASSERT_LE(y_out.size(), 200u);
    for (double v : y_out)
        ASSERT_DOUBLE_EQ(v, 1.0);
}

TEST(DecimateXYChecks, min_max_output_contains_no_nan) {
    // arrange
    auto y = make_sine(5000);
    auto x = make_linspace(5000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 100, DECIMATE_MIN_MAX);
    // assert
    for (double v : y_out)
        ASSERT_FALSE(std::isnan(v));
}

// ========================================================================================
// M4
// ========================================================================================

TEST(DecimateXYChecks, m4_output_contains_bucket_extremes) {
    // arrange
    std::vector<double> y(1000, 0.0);
    y[99] = 50.0;
    y[900] = -50.0;
    auto x = make_linspace(1000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 100, DECIMATE_M4);
    // assert
    bool found_pos = false;
    bool found_neg = false;
    for (double v : y_out) {
        if (v == 50.0)
            found_pos = true;
        if (v == -50.0)
            found_neg = true;
    }
    ASSERT_TRUE(found_pos);
    ASSERT_TRUE(found_neg);
}

TEST(DecimateXYChecks, m4_first_and_last_of_input_preserved) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_M4);
    // assert
    ASSERT_DOUBLE_EQ(y_out[0], y[0]);
    ASSERT_DOUBLE_EQ(y_out[y_out.size() - 1], y[y.size() - 1]);
}

TEST(DecimateXYChecks, m4_output_is_subset_of_input) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_M4);
    // assert
    ASSERT_TRUE(is_subset_of(y_out, y));
}

TEST(DecimateXYChecks, m4_flat_signal_deduplication) {
    // arrange
    std::vector<double> y(10000, 3.14);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 200, DECIMATE_M4);
    // assert
    ASSERT_LE(y_out.size(), 200u);
    for (double v : y_out)
        ASSERT_DOUBLE_EQ(v, 3.14);
}

TEST(DecimateXYChecks, m4_small_target_one_returns_one_point) {
    // arrange
    auto y = make_sine(500);
    auto x = make_linspace(500);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 1, DECIMATE_M4);
    // assert
    ASSERT_EQ(y_out.size(), 1u);
}

TEST(DecimateXYChecks, m4_small_target_preserves_endpoints_for_target_ge_2) {
    // arrange
    auto y = make_sine(500);
    auto x = make_linspace(500);
    for (size_t target = 2; target <= 4; ++target) {
        // act
        auto [x_out, y_out] = decimate_xy(x, y, target, DECIMATE_M4);
        // assert — m4 guarantees first and last samples are kept
        ASSERT_LE(y_out.size(), target);
        ASSERT_DOUBLE_EQ(y_out[0], y[0]);
        ASSERT_DOUBLE_EQ(y_out[y_out.size() - 1], y[y.size() - 1]);
    }
}

// ========================================================================================
// LTTB
// ========================================================================================

TEST(DecimateXYChecks, lttb_first_and_last_preserved) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 100, DECIMATE_LTTB);
    // assert
    ASSERT_DOUBLE_EQ(y_out[0], y[0]);
    ASSERT_DOUBLE_EQ(y_out[y_out.size() - 1], y[y.size() - 1]);
}

TEST(DecimateXYChecks, lttb_output_is_subset_of_input) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 100, DECIMATE_LTTB);
    // assert
    ASSERT_TRUE(is_subset_of(y_out, y));
}

TEST(DecimateXYChecks, lttb_target_one_returns_first_point) {
    // arrange
    auto y = make_sine(500);
    auto x = make_linspace(500);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 1, DECIMATE_LTTB);
    // assert — must return exactly the first sample
    ASSERT_EQ(y_out.size(), 1u);
    ASSERT_DOUBLE_EQ(y_out[0], y[0]);
}

TEST(DecimateXYChecks, lttb_target_two_returns_first_and_last) {
    // arrange
    auto y = make_sine(500);
    auto x = make_linspace(500);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 2, DECIMATE_LTTB);
    // assert
    ASSERT_EQ(y_out.size(), 2u);
    ASSERT_DOUBLE_EQ(y_out[0], y[0]);
    ASSERT_DOUBLE_EQ(y_out[y_out.size() - 1], y[y.size() - 1]);
}

TEST(DecimateXYChecks, lttb_uses_real_x_axis_from_decimate_xy) {
    // arrange — use a logarithmic x axis to verify the real x is forwarded
    size_t n = 10000;
    size_t target = 100;
    std::vector<double> x(n);
    for (size_t i = 0; i < n; ++i)
        x[i] = std::pow(10.0, 1.0 + 5.0 * static_cast<double>(i) / static_cast<double>(n - 1));
    auto y = make_sine(n);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, target, DECIMATE_LTTB);
    // assert — length must match target and pairs must be coherent
    ASSERT_EQ(x_out.size(), target);
    ASSERT_EQ(y_out.size(), target);
    ASSERT_TRUE(xy_pairs_coherent(x_out, y_out, x, y));
}

// ========================================================================================
// RDP
// ========================================================================================

TEST(DecimateXYChecks, rdp_output_is_subset_of_input) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 100, DECIMATE_RDP);
    // assert
    ASSERT_TRUE(is_subset_of(y_out, y));
}

TEST(DecimateXYChecks, rdp_first_and_last_preserved) {
    // arrange
    auto y = make_sine(10000);
    auto x = make_linspace(10000);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 100, DECIMATE_RDP);
    // assert
    ASSERT_DOUBLE_EQ(y_out[0], y[0]);
    ASSERT_DOUBLE_EQ(y_out[y_out.size() - 1], y[y.size() - 1]);
}

// ========================================================================================
// invalid parameter tests
// ========================================================================================

TEST(DecimateXYChecks, length_mismatch_raises_invalid_argument) {
    // arrange
    auto x = make_linspace(10);
    auto y = make_sine(9);
    // act/assert — all algorithms must validate input lengths
    ASSERT_THROW(decimate_xy(x, y, 5, DECIMATE_NTH_POINT), std::invalid_argument);
    ASSERT_THROW(decimate_xy(x, y, 5, DECIMATE_MIN_MAX), std::invalid_argument);
    ASSERT_THROW(decimate_xy(x, y, 5, DECIMATE_M4), std::invalid_argument);
    ASSERT_THROW(decimate_xy(x, y, 5, DECIMATE_LTTB), std::invalid_argument);
    ASSERT_THROW(decimate_xy(x, y, 5, DECIMATE_RDP), std::invalid_argument);
    ASSERT_THROW(decimate_xy(x, y, 5, DECIMATE_NONE), std::invalid_argument);
}

TEST(DecimateXYChecks, target_zero_raises_invalid_argument) {
    // arrange
    auto x = make_linspace(10);
    auto y = make_sine(10);
    // act/assert
    ASSERT_THROW(decimate_xy(x, y, 0, DECIMATE_NTH_POINT), std::invalid_argument);
}

TEST(DecimateXYChecks, unknown_algorithm_raises_invalid_argument) {
    // arrange
    auto x = make_linspace(1000);
    auto y = make_sine(1000);
    // act/assert — a value outside the known constants must be rejected
    ASSERT_THROW(decimate_xy(x, y, 100, 999), std::invalid_argument);
}

TEST(DecimateXYChecks, none_with_target_zero_is_accepted) {
    // arrange — NONE bypasses decimation so a zero target is allowed
    auto x = make_linspace(10);
    auto y = make_sine(10);
    // act/assert — must not throw
    ASSERT_NO_THROW(decimate_xy(x, y, 0, DECIMATE_NONE));
}

// ========================================================================================
// min-max and m4 small-target edge cases
// ========================================================================================

TEST(DecimateXYChecks, min_max_small_target_one_returns_one_point) {
    // arrange
    auto y = make_sine(500);
    auto x = make_linspace(500);
    // act
    auto [x_out, y_out] = decimate_xy(x, y, 1, DECIMATE_MIN_MAX);
    // assert
    ASSERT_EQ(y_out.size(), 1u);
}

TEST(DecimateXYChecks, min_max_small_target_respects_budget) {
    // arrange
    auto y = make_sine(500);
    auto x = make_linspace(500);
    for (size_t target = 1; target <= 4; ++target) {
        // act
        auto [x_out, y_out] = decimate_xy(x, y, target, DECIMATE_MIN_MAX);
        // assert
        ASSERT_LE(y_out.size(), target);
    }
}
