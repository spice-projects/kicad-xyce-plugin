#include <gtest/gtest.h>

#include "expression/probe_names.h"

TEST(ProbeNamesChecks, s_family_valid) {
    // arrange / act / assert
    ASSERT_TRUE(is_network_parameter_probe_name("s11"));
    ASSERT_TRUE(is_network_parameter_probe_name("s00"));
    ASSERT_TRUE(is_network_parameter_probe_name("s99"));
    ASSERT_TRUE(is_network_parameter_probe_name("s12"));
}

TEST(ProbeNamesChecks, z_family_valid) {
    // arrange / act / assert
    ASSERT_TRUE(is_network_parameter_probe_name("z11"));
    ASSERT_TRUE(is_network_parameter_probe_name("z00"));
    ASSERT_TRUE(is_network_parameter_probe_name("z99"));
    ASSERT_TRUE(is_network_parameter_probe_name("z21"));
}

TEST(ProbeNamesChecks, y_family_valid) {
    // arrange / act / assert
    ASSERT_TRUE(is_network_parameter_probe_name("y11"));
    ASSERT_TRUE(is_network_parameter_probe_name("y00"));
    ASSERT_TRUE(is_network_parameter_probe_name("y99"));
    ASSERT_TRUE(is_network_parameter_probe_name("y22"));
}

TEST(ProbeNamesChecks, h_family_valid) {
    // arrange / act / assert
    ASSERT_TRUE(is_network_parameter_probe_name("h11"));
    ASSERT_TRUE(is_network_parameter_probe_name("h00"));
    ASSERT_TRUE(is_network_parameter_probe_name("h99"));
    ASSERT_TRUE(is_network_parameter_probe_name("h12"));
}

TEST(ProbeNamesChecks, all_digit_combinations_valid) {
    // arrange / act
    bool all_valid = true;
    for (char d1 = '0'; d1 <= '9'; ++d1) {
        for (char d2 = '0'; d2 <= '9'; ++d2) {
            const char name[4] = {'s', d1, d2, '\0'};
            if (!is_network_parameter_probe_name(name)) {
                all_valid = false;
            }
        }
    }
    // assert
    ASSERT_TRUE(all_valid);
}

TEST(ProbeNamesChecks, empty_name_is_invalid) {
    // arrange / act
    const auto result = is_network_parameter_probe_name("");
    // assert
    ASSERT_FALSE(result);
}

TEST(ProbeNamesChecks, name_too_short_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("s"));
    ASSERT_FALSE(is_network_parameter_probe_name("s1"));
    ASSERT_FALSE(is_network_parameter_probe_name("z"));
    ASSERT_FALSE(is_network_parameter_probe_name("y2"));
    ASSERT_FALSE(is_network_parameter_probe_name("h"));
}

TEST(ProbeNamesChecks, name_too_long_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("s111"));
    ASSERT_FALSE(is_network_parameter_probe_name("z123"));
    ASSERT_FALSE(is_network_parameter_probe_name("y1234"));
    ASSERT_FALSE(is_network_parameter_probe_name("h2111"));
    ASSERT_FALSE(is_network_parameter_probe_name("s1111"));
}

TEST(ProbeNamesChecks, uppercase_family_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("S11"));
    ASSERT_FALSE(is_network_parameter_probe_name("Z11"));
    ASSERT_FALSE(is_network_parameter_probe_name("Y11"));
    ASSERT_FALSE(is_network_parameter_probe_name("H11"));
}

TEST(ProbeNamesChecks, mixed_case_family_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("S11"));
    ASSERT_FALSE(is_network_parameter_probe_name("zA1"));
    ASSERT_FALSE(is_network_parameter_probe_name("y1A"));
}

TEST(ProbeNamesChecks, non_network_family_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("a11"));
    ASSERT_FALSE(is_network_parameter_probe_name("x11"));
    ASSERT_FALSE(is_network_parameter_probe_name("t11"));
    ASSERT_FALSE(is_network_parameter_probe_name("g11"));
    ASSERT_FALSE(is_network_parameter_probe_name("r11"));
    ASSERT_FALSE(is_network_parameter_probe_name("v11"));
    ASSERT_FALSE(is_network_parameter_probe_name("i11"));
}

TEST(ProbeNamesChecks, digits_as_family_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("011"));
    ASSERT_FALSE(is_network_parameter_probe_name("511"));
    ASSERT_FALSE(is_network_parameter_probe_name("911"));
}

TEST(ProbeNamesChecks, punctuation_as_family_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("-11"));
    ASSERT_FALSE(is_network_parameter_probe_name(".11"));
    ASSERT_FALSE(is_network_parameter_probe_name("_11"));
    ASSERT_FALSE(is_network_parameter_probe_name(" 11"));
}

TEST(ProbeNamesChecks, second_character_non_digit_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("sA1"));
    ASSERT_FALSE(is_network_parameter_probe_name("s_1"));
    ASSERT_FALSE(is_network_parameter_probe_name("s.1"));
    ASSERT_FALSE(is_network_parameter_probe_name("s-1"));
    ASSERT_FALSE(is_network_parameter_probe_name("s 1"));
    ASSERT_FALSE(is_network_parameter_probe_name("ss1"));
}

TEST(ProbeNamesChecks, third_character_non_digit_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("s1A"));
    ASSERT_FALSE(is_network_parameter_probe_name("s1_"));
    ASSERT_FALSE(is_network_parameter_probe_name("s1."));
    ASSERT_FALSE(is_network_parameter_probe_name("s1-"));
    ASSERT_FALSE(is_network_parameter_probe_name("s1 "));
    ASSERT_FALSE(is_network_parameter_probe_name("s1s"));
}

TEST(ProbeNamesChecks, both_trailing_characters_non_digit_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("sAA"));
    ASSERT_FALSE(is_network_parameter_probe_name("s__"));
    ASSERT_FALSE(is_network_parameter_probe_name("s.."));
}

TEST(ProbeNamesChecks, digit_boundaries) {
    // arrange / act / assert
    ASSERT_TRUE(is_network_parameter_probe_name("s00"));
    ASSERT_TRUE(is_network_parameter_probe_name("s09"));
    ASSERT_TRUE(is_network_parameter_probe_name("s10"));
    ASSERT_TRUE(is_network_parameter_probe_name("s90"));
    ASSERT_TRUE(is_network_parameter_probe_name("s99"));
}

TEST(ProbeNamesChecks, embedded_probe_in_longer_name_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("s11s"));
    ASSERT_FALSE(is_network_parameter_probe_name("xs11"));
}

TEST(ProbeNamesChecks, high_bit_first_character_is_invalid) {
    // arrange / act / assert
    ASSERT_FALSE(is_network_parameter_probe_name("s\xFF\xFF"));
    ASSERT_FALSE(is_network_parameter_probe_name("\xE9\x31\x31"));
    ASSERT_FALSE(is_network_parameter_probe_name("s\x80"));
}

TEST(ProbeNamesChecks, high_bit_byte_sweep_all_positions) {
    // arrange
    bool all_invalid = true;
    // act: sweep 0x80..0xFF in each of the three positions
    for (int hi = 0x80; hi <= 0xFF; ++hi) {
        const std::string in_family{static_cast<char>(hi), '1', '1'};
        const std::string in_digit1{'s', static_cast<char>(hi), '1'};
        const std::string in_digit2{'s', '1', static_cast<char>(hi)};
        // assert
        if (is_network_parameter_probe_name(in_family) || is_network_parameter_probe_name(in_digit1) || is_network_parameter_probe_name(in_digit2)) {
            all_invalid = false;
        }
    }
    ASSERT_TRUE(all_invalid);
}

TEST(ProbeNamesChecks, non_null_terminated_view_bounds) {
    // arrange: longer buffer, explicit length 3 without a NUL terminator
    const std::string buffer = "s11ZZZZ";
    // act
    const auto result = is_network_parameter_probe_name(std::string_view(buffer.data(), 3));
    // assert
    ASSERT_TRUE(result);
}

TEST(ProbeNamesChecks, embedded_nul_view_is_invalid) {
    // arrange: view containing an embedded NUL within a longer buffer
    const std::string buffer = "s1\0ABCDEF";
    // act
    const auto result = is_network_parameter_probe_name(std::string_view(buffer.data(), 3));
    // assert
    ASSERT_FALSE(result);
}

TEST(ProbeNamesChecks, non_null_terminated_too_short_view_is_invalid) {
    // arrange: explicit length 2 from a longer buffer
    const std::string buffer = "s11ZZZZ";
    // act
    const auto result = is_network_parameter_probe_name(std::string_view(buffer.data(), 2));
    // assert
    ASSERT_FALSE(result);
}
