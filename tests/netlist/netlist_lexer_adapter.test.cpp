#include <gtest/gtest.h>

#include "netlist/netlist_lexer.h"
#include "netlist/netlist_lexer_adapter.h"

// token_type_to_color: comment tokens are grey in dark mode
TEST(NetlistLexerAdapterChecks, comment_token_is_grey_dark_mode) {
    // arrange / act
    const auto color = token_type_to_color(NetlistTokenType::COMMENT, true);
    // assert
    ASSERT_EQ(color, slint::Color::from_rgb_uint8(0x6A, 0x6A, 0x6A));
}

// token_type_to_color: comment tokens are grey in light mode
TEST(NetlistLexerAdapterChecks, comment_token_is_grey_light_mode) {
    // arrange / act
    const auto color = token_type_to_color(NetlistTokenType::COMMENT, false);
    // assert
    ASSERT_EQ(color, slint::Color::from_rgb_uint8(0x6A, 0x6A, 0x6A));
}

// token_type_to_color: directive colour is distinct between themes
TEST(NetlistLexerAdapterChecks, directive_colour_differs_between_themes) {
    // arrange / act / assert
    ASSERT_NE(token_type_to_color(NetlistTokenType::DIRECTIVE, true), token_type_to_color(NetlistTokenType::DIRECTIVE, false));
}

// token_type_to_color: plain text colour differs from the directive colour in light mode
TEST(NetlistLexerAdapterChecks, directive_colour_differs_from_plain_text) {
    // arrange / act / assert
    ASSERT_NE(token_type_to_color(NetlistTokenType::DIRECTIVE, false), token_type_to_color(NetlistTokenType::PLAIN_TEXT, false));
}

// token_type_to_color: model names share the numeric colour so trailing value
// and model arguments (e.g. 20k on a resistor line, LM317 on a subcircuit line)
// render with the same colour
TEST(NetlistLexerAdapterChecks, model_colour_matches_number_colour_dark_mode) {
    // arrange / act / assert
    ASSERT_EQ(token_type_to_color(NetlistTokenType::MODEL, true), token_type_to_color(NetlistTokenType::NUMBER, true));
}

TEST(NetlistLexerAdapterChecks, model_colour_matches_number_colour_light_mode) {
    // arrange / act / assert
    ASSERT_EQ(token_type_to_color(NetlistTokenType::MODEL, false), token_type_to_color(NetlistTokenType::NUMBER, false));
}

// build_netlist_highlight_model: model row count equals tokenized line count
TEST(NetlistLexerAdapterChecks, model_row_count_matches_token_lines) {
    // arrange
    const std::string netlist = "* comment\nR1 1 0 1k";
    // act
    const auto token_lines = tokenize_netlist(netlist);
    const auto model = build_netlist_highlight_model(token_lines, false);
    // assert
    ASSERT_EQ(model->row_count(), token_lines.size());
}

// build_netlist_highlight_model: line numbers are 1-based and sequential
TEST(NetlistLexerAdapterChecks, line_numbers_are_sequential) {
    // arrange
    const std::string netlist = "* a\n* b\n* c";
    // act
    const auto token_lines = tokenize_netlist(netlist);
    const auto model = build_netlist_highlight_model(token_lines, false);
    // assert
    for (std::size_t i = 0; i < model->row_count(); ++i)
        ASSERT_EQ(model->row_data(i).value().line_number, static_cast<int>(i + 1));
}

// build_netlist_highlight_model: no token text is a bare newline
TEST(NetlistLexerAdapterChecks, bare_newline_tokens_are_dropped) {
    // arrange
    const std::string netlist = "* comment";
    // act
    const auto model = build_netlist_highlight_model(tokenize_netlist(netlist), false);
    // assert
    ASSERT_GE(model->row_count(), 1u);
    const auto line = model->row_data(0).value();
    for (std::size_t i = 0; i < line.tokens->row_count(); ++i)
        ASSERT_NE(std::string(line.tokens->row_data(i).value().text), "\n");
}

// build_netlist_highlight_model: empty string produces exactly one line
TEST(NetlistLexerAdapterChecks, empty_netlist_produces_one_line) {
    // arrange
    const std::string netlist;
    // act
    const auto token_lines = tokenize_netlist(netlist);
    const auto model = build_netlist_highlight_model(token_lines, false);
    // assert
    ASSERT_EQ(model->row_count(), 1u);
}

// build_netlist_highlight_model: the leading token of a directive line carries
// the directive colour and its text
TEST(NetlistLexerAdapterChecks, directive_line_tokens_carry_directive_colour) {
    // arrange
    const std::string netlist = ".tran 1n 10u";
    // act
    const auto model = build_netlist_highlight_model(tokenize_netlist(netlist), false);
    // assert
    ASSERT_GE(model->row_count(), 1u);
    const auto line = model->row_data(0).value();
    ASSERT_GE(line.tokens->row_count(), 1u);
    const auto first = line.tokens->row_data(0).value();
    ASSERT_EQ(std::string(first.text), ".tran");
    ASSERT_EQ(first.color, token_type_to_color(NetlistTokenType::DIRECTIVE, false));
}
