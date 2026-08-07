#pragma once

#include <optional>
#include <span>
#include <vector>

namespace fft
{
    enum class WindowFunction
    {
        RECTANGULAR,
        HAMMING,
        HANNING,
        BLACKMAN
    };

    enum class FftOutput
    {
        MAGNITUDE,
        MAGNITUDE_DB,
        PHASE
    };

    enum class InterpolationAlgorithm
    {
        LINEAR,
        PCHIP,
        CUBIC_SPLINE
    };

    struct FftResult
    {
        std::vector<double> frequencies;
        std::vector<std::vector<double>> values;
    };

    // computes fft for many real-valued signals that share the same time-domain abscissa
    FftResult compute_fft_many(std::span<const double> x, const std::vector<std::span<const double>>& y_matrix, double max_frequency, WindowFunction window = WindowFunction::RECTANGULAR, bool normalize = false, size_t x_left_index = 0, std::optional<size_t> x_right_index = std::nullopt, FftOutput output = FftOutput::MAGNITUDE, bool keep_dc = false, InterpolationAlgorithm algo = InterpolationAlgorithm::LINEAR);
} // namespace fft
