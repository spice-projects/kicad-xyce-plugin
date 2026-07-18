#pragma once

#include <span>
#include <tuple>

std::tuple<std::span<const double>, std::span<const double>> decimate_xy(const std::span<const double>& abscissa_values, const std::span<const double>& ordinate_values, size_t decimate_target, int algorithm) {
    return {abscissa_values, ordinate_values};
};
