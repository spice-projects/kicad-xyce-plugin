#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "util.h"

TEST(UtilChecks, get_environment_variable_returns_value_when_set) {
    // arrange
    const char* name = "KICAD_XYCE_TEST_ENV_VAR";
    const char* value = "test-value";
#ifdef _WIN32
    ASSERT_EQ(_putenv_s(name, value), 0);
#else
    ASSERT_EQ(setenv(name, value, 1), 0);
#endif
    // act
    const auto result = get_environment_variable(name);
    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, value);
#ifdef _WIN32
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

TEST(UtilChecks, get_environment_variable_returns_nullopt_when_unset) {
    // arrange
    const char* name = "KICAD_XYCE_TEST_ENV_VAR_MISSING";
#ifdef _WIN32
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
    // act
    const auto result = get_environment_variable(name);
    // assert
    EXPECT_FALSE(result.has_value());
}

TEST(UtilChecks, to_upper_converts_lowercase_and_mixed_case) {
    // arrange / act
    const auto result = to_upper("Mixed Case 123");
    // assert
    EXPECT_EQ(result, "MIXED CASE 123");
}

TEST(UtilChecks, to_upper_handles_empty_string) {
    // arrange / act
    const auto result = to_upper("");
    // assert
    EXPECT_TRUE(result.empty());
}

TEST(UtilChecks, to_lower_converts_uppercase_and_mixed_case) {
    // arrange / act
    const auto result = to_lower("MiXeD Case 123");
    // assert
    EXPECT_EQ(result, "mixed case 123");
}

TEST(UtilChecks, to_lower_handles_empty_string) {
    // arrange / act
    const auto result = to_lower("");
    // assert
    EXPECT_TRUE(result.empty());
}

TEST(UtilChecks, tokenize_splits_on_whitespace) {
    // arrange / act
    const auto tokens = tokenize("  R1  1 0 100k \n\t");
    // assert
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0], "R1");
    EXPECT_EQ(tokens[1], "1");
    EXPECT_EQ(tokens[2], "0");
    EXPECT_EQ(tokens[3], "100k");
}

TEST(UtilChecks, tokenize_returns_single_token_for_no_whitespace) {
    // arrange / act
    const auto tokens = tokenize("V(1)");
    // assert
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0], "V(1)");
}

TEST(UtilChecks, tokenize_returns_empty_for_blank_input) {
    // arrange / act
    const auto tokens = tokenize("   \n\t  ");
    // assert
    EXPECT_TRUE(tokens.empty());
}

TEST(UtilChecks, tokenize_returns_empty_for_empty_input) {
    // arrange / act
    const auto tokens = tokenize("");
    // assert
    EXPECT_TRUE(tokens.empty());
}

TEST(UtilChecks, split_by_splits_on_delimiter_and_skips_repeats) {
    // arrange / act
    const auto parts = split_by("a,,b,c", ',');
    // assert
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(UtilChecks, split_by_handles_leading_and_trailing_delimiters) {
    // arrange / act
    const auto parts = split_by(",a,", ',');
    // assert
    ASSERT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "a");
}

TEST(UtilChecks, split_by_returns_single_token_without_delimiter) {
    // arrange / act
    const auto parts = split_by("abc", ',');
    // assert
    ASSERT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "abc");
}

TEST(UtilChecks, split_by_returns_empty_for_empty_input) {
    // arrange / act
    const auto parts = split_by("", ',');
    // assert
    EXPECT_TRUE(parts.empty());
}

TEST(UtilChecks, split_by_splits_on_space_delimiter) {
    // arrange / act
    const auto parts = split_by("a b c", ' ');
    // assert
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(UtilChecks, strip_chars_removes_all_occurrences) {
    // arrange / act
    const auto result = strip_chars("banana", "na");
    // assert
    EXPECT_EQ(result, "b");
}

TEST(UtilChecks, strip_chars_keeps_string_when_no_chars_to_strip) {
    // arrange / act
    const auto result = strip_chars("hello", "");
    // assert
    EXPECT_EQ(result, "hello");
}

TEST(UtilChecks, strip_chars_keeps_unlisted_characters) {
    // arrange / act
    const auto result = strip_chars("a.b-c_d", ".-");
    // assert
    EXPECT_EQ(result, "abc_d");
}

TEST(UtilChecks, strip_chars_handles_empty_input) {
    // arrange / act
    const auto result = strip_chars("", "abc");
    // assert
    EXPECT_TRUE(result.empty());
}
