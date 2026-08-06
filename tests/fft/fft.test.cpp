#include <cmath>
#include <numbers>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "fft/fft.h"

TEST(FftChecks, throws_on_too_few_elements_in_x) {
    // arrange
    const std::vector<double> x = {0.0};
    const std::vector<double> y = {1.0};
    const std::vector<std::span<const double>> y_matrix = {y};
    // act / assert
    EXPECT_THROW(fft::compute_fft_many2(x, y_matrix, 10.0), std::invalid_argument);
}

TEST(FftChecks, throws_on_empty_y_matrix) {
    // arrange
    const std::vector<double> x = {0.0, 1.0, 2.0};
    const std::vector<std::span<const double>> y_matrix;
    // act / assert
    EXPECT_THROW(fft::compute_fft_many2(x, y_matrix, 10.0), std::invalid_argument);
}

TEST(FftChecks, throws_on_mismatched_row_size) {
    // arrange
    const std::vector<double> x = {0.0, 1.0, 2.0};
    const std::vector<double> y = {1.0, 2.0};
    const std::vector<std::span<const double>> y_matrix = {y};
    // act / assert
    EXPECT_THROW(fft::compute_fft_many2(x, y_matrix, 10.0), std::invalid_argument);
}

TEST(FftChecks, throws_on_non_positive_max_frequency) {
    // arrange
    const std::vector<double> x = {0.0, 1.0, 2.0};
    const std::vector<double> y = {1.0, 2.0, 3.0};
    const std::vector<std::span<const double>> y_matrix = {y};
    // act / assert
    EXPECT_THROW(fft::compute_fft_many2(x, y_matrix, -5.0), std::invalid_argument);
}

TEST(FftChecks, throws_on_invalid_bounds) {
    // arrange
    const std::vector<double> x = {0.0, 1.0, 2.0};
    const std::vector<double> y = {1.0, 2.0, 3.0};
    const std::vector<std::span<const double>> y_matrix = {y};
    // act / assert
    EXPECT_THROW(fft::compute_fft_many2(x, y_matrix, 10.0, fft::WindowFunction::RECTANGULAR, false, 2, 1), std::invalid_argument);
    EXPECT_THROW(fft::compute_fft_many2(x, y_matrix, 10.0, fft::WindowFunction::RECTANGULAR, false, 0, 4), std::invalid_argument);
}

TEST(FftChecks, throws_on_too_few_interval_samples) {
    // arrange
    const std::vector<double> x = {0.0, 1.0, 2.0};
    const std::vector<double> y = {1.0, 2.0, 3.0};
    const std::vector<std::span<const double>> y_matrix = {y};
    // act / assert
    EXPECT_THROW(fft::compute_fft_many2(x, y_matrix, 10.0, fft::WindowFunction::RECTANGULAR, false, 0, 1), std::invalid_argument);
}

TEST(FftChecks, throws_on_non_monotonically_increasing_x) {
    // arrange
    const std::vector<double> x = {0.0, 0.5, 0.3, 1.0};
    const std::vector<double> y = {1.0, 2.0, 3.0, 4.0};
    const std::vector<std::span<const double>> y_matrix = {y};
    // act / assert
    EXPECT_THROW(fft::compute_fft_many2(x, y_matrix, 10.0), std::invalid_argument);
}

TEST(FftChecks, computes_fft_for_constant_signal_with_keep_dc) {
    // arrange
    const std::vector<double> x = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    const std::vector<double> y = {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0};
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result = fft::compute_fft_many2(x, y_matrix, 1.0, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, true);
    // assert
    ASSERT_FALSE(result.frequencies.empty());
    ASSERT_EQ(result.values.size(), 1u);
    ASSERT_EQ(result.values[0].size(), result.frequencies.size());
    EXPECT_NEAR(result.frequencies[0], 0.0, 1e-9);
    EXPECT_NEAR(result.values[0][0], 5.0 * (128.0 / 71.0), 1e-4);
}

TEST(FftChecks, computes_fft_for_constant_signal_subtracts_dc) {
    // arrange
    const std::vector<double> x = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    const std::vector<double> y = {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0};
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result = fft::compute_fft_many2(x, y_matrix, 1.0, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
    // assert
    ASSERT_FALSE(result.frequencies.empty());
    ASSERT_EQ(result.values.size(), 1u);
    EXPECT_NEAR(result.values[0][0], 0.0, 1e-4);
}

TEST(FftChecks, computes_fft_for_sine_wave_correct_frequency_and_amplitude) {
    // arrange: 1 Hz sine wave with amplitude 2.0 on a 10-second grid
    std::vector<double> x;
    std::vector<double> y;
    for (int i = 0; i < 200; ++i) {
        double t = i * 0.05;
        x.push_back(t);
        y.push_back(2.0 * std::sin(2.0 * std::numbers::pi * 1.0 * t));
    }
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result = fft::compute_fft_many2(x, y_matrix, 4.0, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
    // assert
    ASSERT_FALSE(result.frequencies.empty());
    size_t peak_bin = 0;
    double max_mag = 0.0;
    for (size_t i = 0; i < result.values[0].size(); ++i) {
        if (result.values[0][i] > max_mag) {
            max_mag = result.values[0][i];
            peak_bin = i;
        }
    }
    EXPECT_NEAR(result.frequencies[peak_bin], 1.0, 0.1);
    EXPECT_NEAR(max_mag, 2.0 * (512.0 / 399.0), 0.15);
}

TEST(FftChecks, computes_fft_normalization_peaks_at_one) {
    // arrange: 1 Hz sine wave with amplitude 2.0
    std::vector<double> x;
    std::vector<double> y;
    for (int i = 0; i < 200; ++i) {
        double t = i * 0.05;
        x.push_back(t);
        y.push_back(2.0 * std::sin(2.0 * std::numbers::pi * 1.0 * t));
    }
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result = fft::compute_fft_many2(x, y_matrix, 4.0, fft::WindowFunction::RECTANGULAR, true, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
    // assert
    ASSERT_FALSE(result.frequencies.empty());
    double max_mag = 0.0;
    for (double val : result.values[0]) {
        max_mag = (std::max)(max_mag, val);
    }
    EXPECT_NEAR(max_mag, 1.0, 1e-9);
}

TEST(FftChecks, computes_fft_with_magnitude_db_output) {
    // arrange
    const std::vector<double> x = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    const std::vector<double> y = {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0};
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result = fft::compute_fft_many2(x, y_matrix, 1.0, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE_DB, true);
    // assert
    ASSERT_FALSE(result.frequencies.empty());
    EXPECT_NEAR(result.values[0][0], 20.0 * std::log10(5.0 * (128.0 / 71.0)), 1e-4);
}

TEST(FftChecks, computes_fft_with_phase_output) {
    // arrange: cosine wave (phase should be close to 0 degrees at peak)
    std::vector<double> x;
    std::vector<double> y;
    for (int i = 0; i < 200; ++i) {
        double t = i * 0.05;
        x.push_back(t);
        y.push_back(3.0 * std::cos(2.0 * std::numbers::pi * 1.0 * t));
    }
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result_mag = fft::compute_fft_many2(x, y_matrix, 4.0, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
    const auto result_phase = fft::compute_fft_many2(x, y_matrix, 4.0, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::PHASE, false);
    // assert
    ASSERT_EQ(result_mag.values[0].size(), result_phase.values[0].size());
    size_t peak_bin = 0;
    double max_mag = 0.0;
    for (size_t i = 0; i < result_mag.values[0].size(); ++i) {
        if (result_mag.values[0][i] > max_mag) {
            max_mag = result_mag.values[0][i];
            peak_bin = i;
        }
    }
    EXPECT_NEAR(result_phase.values[0][peak_bin], -27.888, 0.5);
}

TEST(FftChecks, integer_cycle_sine_has_single_bin_peak) {
    // arrange
    const double fs = 2048.0;
    const double f_tone = 123.0;
    const size_t n = 2048;
    std::vector<double> x;
    std::vector<double> y;
    x.reserve(n);
    y.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) / fs;
        x.push_back(t);
        y.push_back(std::sin(2.0 * std::numbers::pi * f_tone * t));
    }
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result = fft::compute_fft_many2(x, y_matrix, 204.8, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
    // assert
    ASSERT_EQ(result.frequencies.size(), 1025u);
    size_t peak_bin = 0;
    double max_mag = 0.0;
    for (size_t i = 0; i < result.values[0].size(); ++i) {
        if (result.values[0][i] > max_mag) {
            max_mag = result.values[0][i];
            peak_bin = i;
        }
    }
    EXPECT_NEAR(result.frequencies[peak_bin], f_tone, fs / n);
    EXPECT_NEAR(max_mag, 1.0, 0.01);
    for (size_t i = 0; i < result.values[0].size(); ++i) {
        if (i != peak_bin) {
            EXPECT_LT(result.values[0][i], 1e-3);
        }
    }
}

TEST(FftChecks, two_tone_amplitudes_preserved) {
    // arrange
    const double fs = 8192.0;
    const double f1 = 300.0;
    const double f2 = 1200.0;
    const double a1 = 1.0;
    const double a2 = 0.5;
    const size_t n = 4096;
    std::vector<double> x;
    std::vector<double> y;
    x.reserve(n);
    y.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) / fs;
        x.push_back(t);
        y.push_back(a1 * std::sin(2.0 * std::numbers::pi * f1 * t) + a2 * std::sin(2.0 * std::numbers::pi * f2 * t));
    }
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result = fft::compute_fft_many2(x, y_matrix, 819.2, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
    // assert
    size_t idx1 = 0;
    size_t idx2 = 0;
    double min_diff1 = 1e9;
    double min_diff2 = 1e9;
    for (size_t i = 0; i < result.frequencies.size(); ++i) {
        double diff1 = std::abs(result.frequencies[i] - f1);
        if (diff1 < min_diff1) {
            min_diff1 = diff1;
            idx1 = i;
        }
        double diff2 = std::abs(result.frequencies[i] - f2);
        if (diff2 < min_diff2) {
            min_diff2 = diff2;
            idx2 = i;
        }
    }
    EXPECT_NEAR(result.values[0][idx1], a1, 0.02);
    EXPECT_NEAR(result.values[0][idx2], a2, 0.02);
}

TEST(FftChecks, parseval_energy_conservation) {
    // arrange
    const double fs = 1000.0;
    const size_t n = 2048;
    std::vector<double> x;
    std::vector<double> y;
    x.reserve(n);
    y.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) / fs;
        x.push_back(t);
        // deterministically structured signal
        y.push_back(std::sin(2.0 * std::numbers::pi * 50.0 * t) + 0.5 * std::cos(2.0 * std::numbers::pi * 150.0 * t) + 0.2 * std::sin(2.0 * std::numbers::pi * 300.0 * t));
    }
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result = fft::compute_fft_many2(x, y_matrix, 100.0, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, true);
    // assert
    double time_energy = 0.0;
    for (double val : y) {
        time_energy += val * val;
    }
    double freq_energy = 0.0;
    freq_energy += result.values[0][0] * result.values[0][0];
    const size_t nyq = 1024;
    freq_energy += result.values[0][nyq] * result.values[0][nyq];
    double sum_inter = 0.0;
    for (size_t i = 1; i < nyq; ++i) {
        sum_inter += result.values[0][i] * result.values[0][i];
    }
    freq_energy += 0.5 * sum_inter;
    freq_energy *= static_cast<double>(n);
    EXPECT_NEAR(time_energy, freq_energy, 1e-4);
}

TEST(FftChecks, dc_amplitude_is_one_for_unit_dc_signal) {
    // arrange
    const size_t n = 512;
    const double fs = 1000.0;
    std::vector<double> x;
    std::vector<double> y;
    x.reserve(n);
    y.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        x.push_back(static_cast<double>(i) / fs);
        y.push_back(1.0);
    }
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result = fft::compute_fft_many2(x, y_matrix, 100.0, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, true);
    // assert
    EXPECT_NEAR(result.values[0][0], 1.0, 0.001);
}

TEST(FftChecks, nyquist_bin_not_doubled_for_even_n) {
    // arrange
    const size_t n = 512;
    const double fs = 1000.0;
    const double f_nyq = fs / 2.0;
    std::vector<double> x;
    std::vector<double> y;
    x.reserve(n);
    y.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) / fs;
        x.push_back(t);
        y.push_back(std::cos(2.0 * std::numbers::pi * f_nyq * t));
    }
    const std::vector<std::span<const double>> y_matrix = {y};
    // act
    const auto result = fft::compute_fft_many2(x, y_matrix, 100.0, fft::WindowFunction::RECTANGULAR, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
    // assert
    const size_t nyq_bin = 256;
    EXPECT_NEAR(result.values[0][nyq_bin], 1.0, 0.01);
}

TEST(FftChecks, all_window_functions_recover_unit_amplitude) {
    // arrange
    const size_t n = 1024;
    const double fs = 1024.0;
    const double f_tone = 100.0;
    std::vector<double> x;
    std::vector<double> y;
    x.reserve(n);
    y.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) / fs;
        x.push_back(t);
        y.push_back(std::sin(2.0 * std::numbers::pi * f_tone * t));
    }
    const std::vector<std::span<const double>> y_matrix = {y};
    const fft::WindowFunction windows[] = {fft::WindowFunction::RECTANGULAR, fft::WindowFunction::HAMMING, fft::WindowFunction::HANNING, fft::WindowFunction::BLACKMAN};
    const double tolerances[] = {0.01, 0.05, 0.05, 0.05};
    for (size_t w = 0; w < 4; ++w) {
        // act
        const auto result = fft::compute_fft_many2(x, y_matrix, 102.4, windows[w], false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
        // assert
        double max_mag = 0.0;
        for (size_t i = 0; i < result.values[0].size(); ++i) {
            if (result.values[0][i] > max_mag) {
                max_mag = result.values[0][i];
            }
        }
        EXPECT_NEAR(max_mag, 1.0, tolerances[w]);
    }
}

TEST(FftChecks, many2_matches_individual_runs) {
    // arrange
    const size_t n = 1024;
    const double fs = 8192.0;
    std::vector<double> x;
    std::vector<double> y1;
    std::vector<double> y2;
    x.reserve(n);
    y1.reserve(n);
    y2.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) / fs;
        x.push_back(t);
        y1.push_back(std::sin(2.0 * std::numbers::pi * 500.0 * t));
        y2.push_back(0.5 * std::sin(2.0 * std::numbers::pi * 1200.0 * t));
    }
    const std::vector<std::span<const double>> y_matrix = {y1, y2};
    const std::vector<std::span<const double>> y_matrix1 = {y1};
    const std::vector<std::span<const double>> y_matrix2 = {y2};
    // act
    const auto result_many = fft::compute_fft_many2(x, y_matrix, 409.6, fft::WindowFunction::HANNING, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
    const auto result1 = fft::compute_fft_many2(x, y_matrix1, 409.6, fft::WindowFunction::HANNING, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
    const auto result2 = fft::compute_fft_many2(x, y_matrix2, 409.6, fft::WindowFunction::HANNING, false, 0, std::nullopt, fft::FftOutput::MAGNITUDE, false);
    // assert
    ASSERT_EQ(result_many.frequencies.size(), result1.frequencies.size());
    ASSERT_EQ(result_many.frequencies.size(), result2.frequencies.size());
    ASSERT_EQ(result_many.values.size(), 2u);
    for (size_t i = 0; i < result_many.frequencies.size(); ++i) {
        EXPECT_DOUBLE_EQ(result_many.frequencies[i], result1.frequencies[i]);
        EXPECT_DOUBLE_EQ(result_many.values[0][i], result1.values[0][i]);
        EXPECT_DOUBLE_EQ(result_many.values[1][i], result2.values[0][i]);
    }
}
