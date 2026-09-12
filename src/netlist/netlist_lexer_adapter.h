#pragma once

#include <memory>
#include <vector>

#include <slint.h>

#include <main_window.h>

#include "netlist_lexer.h"

// map a token type to a colour for the given theme
inline slint::Color token_type_to_color(NetlistTokenType type, bool dark_mode)
{
    if (dark_mode) {
        switch (type) {
            case NetlistTokenType::COMMENT:       return slint::Color::from_rgb_uint8(0x6A, 0x6A, 0x6A);
            case NetlistTokenType::DIRECTIVE:     return slint::Color::from_rgb_uint8(0xFF, 0x7B, 0xFF);
            case NetlistTokenType::DEVICE:        return slint::Color::from_rgb_uint8(0x56, 0xB6, 0xFF);
            case NetlistTokenType::NODE:          return slint::Color::from_rgb_uint8(0x4E, 0xC9, 0x94);
            case NetlistTokenType::NUMBER:        return slint::Color::from_rgb_uint8(0xFF, 0xC0, 0x66);
            case NetlistTokenType::EXPRESSION:    return slint::Color::from_rgb_uint8(0xFF, 0xFF, 0x66);
            case NetlistTokenType::STRING_LITERAL:return slint::Color::from_rgb_uint8(0xCE, 0x91, 0x78);
            case NetlistTokenType::CONTINUATION:  return slint::Color::from_rgb_uint8(0xA0, 0xA0, 0xA0);
            case NetlistTokenType::KEYWORD:       return slint::Color::from_rgb_uint8(0xFF, 0x88, 0x88);
            case NetlistTokenType::MODEL:         return slint::Color::from_rgb_uint8(0xFF, 0xC0, 0x66);
            case NetlistTokenType::OPERATOR:      return slint::Color::from_rgb_uint8(0xE0, 0xE0, 0xE0);
            case NetlistTokenType::WHITESPACE:    return slint::Color::from_rgb_uint8(0xCC, 0xCC, 0xCC);
            default:                              return slint::Color::from_rgb_uint8(0xCC, 0xCC, 0xCC);
        }
    }
    // light theme
    switch (type) {
        case NetlistTokenType::COMMENT:       return slint::Color::from_rgb_uint8(0x6A, 0x6A, 0x6A);
        case NetlistTokenType::DIRECTIVE:     return slint::Color::from_rgb_uint8(0xA0, 0x20, 0xA0);
        case NetlistTokenType::DEVICE:        return slint::Color::from_rgb_uint8(0x00, 0x55, 0xCC);
        case NetlistTokenType::NODE:          return slint::Color::from_rgb_uint8(0x00, 0x88, 0x55);
        case NetlistTokenType::NUMBER:        return slint::Color::from_rgb_uint8(0xCC, 0x66, 0x00);
        case NetlistTokenType::EXPRESSION:    return slint::Color::from_rgb_uint8(0x99, 0x77, 0x00);
        case NetlistTokenType::STRING_LITERAL:return slint::Color::from_rgb_uint8(0xA3, 0x46, 0x00);
        case NetlistTokenType::CONTINUATION:  return slint::Color::from_rgb_uint8(0x55, 0x55, 0x55);
        case NetlistTokenType::KEYWORD:       return slint::Color::from_rgb_uint8(0xCC, 0x22, 0x22);
        case NetlistTokenType::MODEL:         return slint::Color::from_rgb_uint8(0xCC, 0x66, 0x00);
        case NetlistTokenType::OPERATOR:      return slint::Color::from_rgb_uint8(0x22, 0x22, 0x22);
        case NetlistTokenType::WHITESPACE:    return slint::Color::from_rgb_uint8(0x00, 0x00, 0x00);
        default:                              return slint::Color::from_rgb_uint8(0x00, 0x00, 0x00);
    }
}

// convert tokenised lines produced by tokenize_netlist() into the Slint model
// consumed by NetlistEditor.  called on every netlist load, user edit, and
// theme change; each line carries a 1-based number and its own token model.
[[nodiscard]] inline std::shared_ptr<slint::VectorModel<main_window::HighlightedLine>> build_netlist_highlight_model(const std::vector<NetlistTokenLine>& lines, bool dark_mode)
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
            out_token.color = token_type_to_color(token.m_type, dark_mode);
            tokens->push_back(out_token);
        }
        main_window::HighlightedLine out_line;
        out_line.line_number = line_number++;
        out_line.tokens = tokens;
        model->push_back(out_line);
    }
    return model;
}
