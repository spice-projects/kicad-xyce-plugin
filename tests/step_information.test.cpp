#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "step_information.h"

// ========================================================================================
// type traits
// ========================================================================================

static_assert(!std::is_default_constructible_v<StepInformation>);

// ========================================================================================
// constructor / accessors
// ========================================================================================

TEST(StepInformationChecks, constructor_stores_keys_values_and_step_count) {
    // arrange
    const std::vector<std::string> keys = {"R1", "TEMP"};
    const std::vector<std::vector<double>> values = {{1000.0, 27.0}, {2000.0, 85.0}};
    const std::vector<std::pair<double, double>> ranges = {{0.0, 4.0}, {0.0, 4.0}};
    // act
    const StepInformation info(keys, values, ranges);
    // assert
    ASSERT_EQ(info.keys(), keys);
    ASSERT_EQ(info.values(), values);
    ASSERT_EQ(info.length(), 2);
}

TEST(StepInformationChecks, constructor_does_not_modify_lvalue_inputs) {
    // arrange
    std::vector<std::string> keys = {"R1"};
    std::vector<std::vector<double>> values = {{1000.0}};
    std::vector<std::pair<double, double>> ranges = {{0.0, 1.0}};
    const std::vector<std::pair<double, double>> expected_ranges = {{0.0, 1.0}};
    // act
    const StepInformation info(keys, values, ranges);
    // assert
    ASSERT_EQ(keys, std::vector<std::string>({"R1"}));
    ASSERT_EQ(values, std::vector<std::vector<double>>({{1000.0}}));
    ASSERT_EQ(ranges, expected_ranges);
}

// ========================================================================================
// abscissa direction and aggregate bounds
// ========================================================================================

TEST(StepInformationChecks, ascending_ranges_set_ascending_flag_and_global_min_max) {
    // arrange
    const StepInformation info({}, {}, {{2.0, 3.0}, {1.0, 5.0}, {4.0, 4.5}});
    // act/assert
    ASSERT_TRUE(info.is_abscissa_ascending());
    ASSERT_DOUBLE_EQ(info.abscissa_left_value(), 1.0);
    ASSERT_DOUBLE_EQ(info.abscissa_right_value(), 5.0);
}

TEST(StepInformationChecks, descending_ranges_set_descending_flag_and_global_max_min) {
    // arrange
    const StepInformation info({}, {}, {{10.0, 9.0}, {12.0, 8.0}, {11.0, 7.0}});
    // act/assert
    ASSERT_FALSE(info.is_abscissa_ascending());
    ASSERT_DOUBLE_EQ(info.abscissa_left_value(), 12.0);
    ASSERT_DOUBLE_EQ(info.abscissa_right_value(), 7.0);
}

TEST(StepInformationChecks, equal_endpoints_are_treated_as_ascending) {
    // arrange
    const StepInformation info({}, {}, {{3.0, 3.0}, {2.0, 4.0}});
    // act/assert
    ASSERT_TRUE(info.is_abscissa_ascending());
    ASSERT_DOUBLE_EQ(info.abscissa_left_value(), 2.0);
    ASSERT_DOUBLE_EQ(info.abscissa_right_value(), 4.0);
}

TEST(StepInformationChecks, empty_ranges_default_to_zero_bounds_and_ascending) {
    // arrange
    const StepInformation info({}, {}, {});
    // act/assert
    ASSERT_EQ(info.length(), 0);
    ASSERT_TRUE(info.is_abscissa_ascending());
    ASSERT_DOUBLE_EQ(info.abscissa_left_value(), 0.0);
    ASSERT_DOUBLE_EQ(info.abscissa_right_value(), 0.0);
}

// ========================================================================================
// per-step bounds
// ========================================================================================

TEST(StepInformationChecks, step_abscissa_accessors_return_values_for_valid_indices) {
    // arrange
    const StepInformation info({}, {}, {{0.0, 4.0}, {1.0, 5.0}});
    // act/assert
    ASSERT_DOUBLE_EQ(info.step_abscissa_left_value(0), 0.0);
    ASSERT_DOUBLE_EQ(info.step_abscissa_right_value(0), 4.0);
    ASSERT_DOUBLE_EQ(info.step_abscissa_left_value(1), 1.0);
    ASSERT_DOUBLE_EQ(info.step_abscissa_right_value(1), 5.0);
}

TEST(StepInformationChecks, step_abscissa_left_value_throws_for_out_of_range_index) {
    // arrange
    const StepInformation info({}, {}, {{0.0, 4.0}});
    // act/assert
    ASSERT_THROW((void)info.step_abscissa_left_value(1), std::out_of_range);
}

TEST(StepInformationChecks, step_abscissa_right_value_throws_for_out_of_range_index) {
    // arrange
    const StepInformation info({}, {}, {{0.0, 4.0}});
    // act/assert
    ASSERT_THROW((void)info.step_abscissa_right_value(1), std::out_of_range);
}