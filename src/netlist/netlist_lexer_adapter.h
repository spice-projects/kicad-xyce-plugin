#pragma once

#include <memory>
#include <vector>

#include <slint.h>

#include <main_window.h>

#include "netlist_lexer.h"

// map a token type to a colour for the given theme; foreground carries the
// theme's default text colour for tokens that follow the editor text (nodes,
// plain text).  the palette matches the colour table from issue #210.
inline slint::Color token_type_to_color(NetlistTokenType type, bool dark_mode, slint::Color foreground)
{
    if (dark_mode) {
        switch (type) {
            case NetlistTokenType::COMMENT:       return slint::Color::from_rgb_uint8(0x8b, 0x94, 0x9e);
            case NetlistTokenType::DIRECTIVE:     return slint::Color::from_rgb_uint8(0x79, 0xc0, 0xff);
            case NetlistTokenType::DEVICE:        return slint::Color::from_rgb_uint8(0xff, 0xa6, 0x57);
            case NetlistTokenType::NODE:          return foreground;
            case NetlistTokenType::NUMBER:        return slint::Color::from_rgb_uint8(0xa5, 0xd6, 0xff);
            case NetlistTokenType::EXPRESSION:    return slint::Color::from_rgb_uint8(0xff, 0x7b, 0x72);
            case NetlistTokenType::STRING_LITERAL:return slint::Color::from_rgb_uint8(0x7e, 0xe7, 0x87);
            case NetlistTokenType::CONTINUATION:  return slint::Color::from_rgb_uint8(0xd2, 0xa8, 0xff);
            case NetlistTokenType::KEYWORD:       return slint::Color::from_rgb_uint8(0xff, 0x88, 0x88);
            case NetlistTokenType::MODEL:         return slint::Color::from_rgb_uint8(0xa5, 0xd6, 0xff);
            case NetlistTokenType::OPERATOR:      return slint::Color::from_rgb_uint8(0xE0, 0xE0, 0xE0);
            case NetlistTokenType::WHITESPACE:    return foreground;
            default:                              return foreground;
        }
    }
    // light theme
    switch (type) {
        case NetlistTokenType::COMMENT:       return slint::Color::from_rgb_uint8(0x6a, 0x73, 0x7d);
        case NetlistTokenType::DIRECTIVE:     return slint::Color::from_rgb_uint8(0x05, 0x50, 0xae);
        case NetlistTokenType::DEVICE:        return slint::Color::from_rgb_uint8(0x95, 0x38, 0x00);
        case NetlistTokenType::NODE:          return foreground;
        case NetlistTokenType::NUMBER:        return slint::Color::from_rgb_uint8(0x0a, 0x30, 0x69);
        case NetlistTokenType::EXPRESSION:    return slint::Color::from_rgb_uint8(0xcf, 0x22, 0x2e);
        case NetlistTokenType::STRING_LITERAL:return slint::Color::from_rgb_uint8(0x11, 0x63, 0x29);
        case NetlistTokenType::CONTINUATION:  return slint::Color::from_rgb_uint8(0x82, 0x50, 0xdf);
        case NetlistTokenType::KEYWORD:       return slint::Color::from_rgb_uint8(0xCC, 0x22, 0x22);
        case NetlistTokenType::MODEL:         return slint::Color::from_rgb_uint8(0x0a, 0x30, 0x69);
        case NetlistTokenType::OPERATOR:      return slint::Color::from_rgb_uint8(0x22, 0x22, 0x22);
        case NetlistTokenType::WHITESPACE:    return foreground;
        default:                              return foreground;
    }
}

// convert tokenised lines produced by tokenize_netlist() into the Slint model
// consumed by NetlistEditor.  called on every netlist load, user edit, and
// theme change; each line carries a 1-based number and its own token model.
[[nodiscard]] inline std::shared_ptr<slint::VectorModel<main_window::HighlightedLine>> build_netlist_highlight_model(const std::vector<NetlistTokenLine>& lines, bool dark_mode, slint::Color foreground)
{
    // one HighlightedLine per tokenised line
    auto model = std::make_shared<slint::VectorModel<main_window::HighlightedLine>>();
    // 1-based line number for the gutter
    int line_number = 1;
    for (const auto& line : lines) {
        // per-line token model consumed by the row repeater
        auto tokens = std::make_shared<slint::VectorModel<main_window::HighlightedToken>>();
        for (const auto& token : line.m_tokens) {
            main_window::HighlightedToken out_token;
            out_token.text = slint::SharedString(token.m_text);
            out_token.color = token_type_to_color(token.m_type, dark_mode, foreground);
            tokens->push_back(out_token);
        }
        main_window::HighlightedLine out_line;
        out_line.line_number = line_number++;
        out_line.tokens = tokens;
        model->push_back(out_line);
    }
    return model;
}
