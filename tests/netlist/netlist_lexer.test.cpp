#include <gtest/gtest.h>

#include "netlist/netlist_lexer.h"

TEST(NetlistLexerChecks, tokenizes_empty_string) {
    // arrange / act
    const auto lines = tokenize_netlist("");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_TRUE(lines[0].m_tokens.empty());
}

TEST(NetlistLexerChecks, tokenizes_whitespace_only_line) {
    // arrange / act
    const auto lines = tokenize_netlist("   \t  ");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "   \t  ");
}

TEST(NetlistLexerChecks, tokenizes_full_line_comment_with_asterisk) {
    // arrange / act
    const auto lines = tokenize_netlist("* Simple RLC Circuit Test");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::COMMENT);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "* Simple RLC Circuit Test");
}

TEST(NetlistLexerChecks, tokenizes_full_line_comment_with_semicolon) {
    // arrange / act
    const auto lines = tokenize_netlist("; Semicolon style comment");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::COMMENT);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "; Semicolon style comment");
}

TEST(NetlistLexerChecks, does_not_treat_dollar_sign_as_comment) {
    // arrange / act
    const auto lines = tokenize_netlist("$G_1 1 0 1k");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "$G_1");
}

TEST(NetlistLexerChecks, tokenizes_full_line_comment_with_leading_whitespace) {
    // arrange / act
    const auto lines = tokenize_netlist("  * Indented comment line");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.size(), 2);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "  ");
    ASSERT_EQ(lines[0].m_tokens[1].m_type, NetlistTokenType::COMMENT);
    ASSERT_EQ(lines[0].m_tokens[1].m_text, "* Indented comment line");
}

TEST(NetlistLexerChecks, tokenizes_continuation_line) {
    // arrange / act
    const auto lines = tokenize_netlist("+ 100 200");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.size(), 5);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::CONTINUATION);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "+");
    ASSERT_EQ(lines[0].m_tokens[1].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "100");
    ASSERT_EQ(lines[0].m_tokens[3].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "200");
}

TEST(NetlistLexerChecks, tokenizes_simulation_directives) {
    // arrange / act
    const auto lines = tokenize_netlist(".TRAN 1u 20m 0");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.size(), 7);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DIRECTIVE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, ".TRAN");
    ASSERT_EQ(lines[0].m_tokens[1].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "1u");
    ASSERT_EQ(lines[0].m_tokens[3].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "20m");
    ASSERT_EQ(lines[0].m_tokens[5].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "0");
}

TEST(NetlistLexerChecks, tokenizes_dot_model_directive) {
    // arrange / act
    const auto lines = tokenize_netlist(".MODEL QNPN NPN (BF=100 IS=1e-14)");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DIRECTIVE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, ".MODEL");
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::PLAIN_TEXT);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "QNPN");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::PLAIN_TEXT);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "NPN");
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::OPERATOR);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "(");
}

TEST(NetlistLexerChecks, tokenizes_two_terminal_resistor) {
    // arrange / act
    const auto lines = tokenize_netlist("R1 IN 0 100");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.size(), 7);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "R1");
    ASSERT_EQ(lines[0].m_tokens[1].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "IN");
    ASSERT_EQ(lines[0].m_tokens[3].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "0");
    ASSERT_EQ(lines[0].m_tokens[5].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "100");
}

TEST(NetlistLexerChecks, tokenizes_three_terminal_transistor) {
    // arrange / act
    const auto lines = tokenize_netlist("Q1 C B E NPN_MOD");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.size(), 9);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "Q1");
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "C");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "B");
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "E");
    ASSERT_EQ(lines[0].m_tokens[8].m_type, NetlistTokenType::PLAIN_TEXT);
    ASSERT_EQ(lines[0].m_tokens[8].m_text, "NPN_MOD");
}

TEST(NetlistLexerChecks, tokenizes_four_terminal_mosfet) {
    // arrange / act
    const auto lines = tokenize_netlist("M1 D G S B NMOS_MOD L=1u W=10u");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "M1");
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "D");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "G");
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "S");
    ASSERT_EQ(lines[0].m_tokens[8].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[8].m_text, "B");
}

TEST(NetlistLexerChecks, tokenizes_subcircuit_instance) {
    // arrange / act
    const auto lines = tokenize_netlist("X1 IN OUT VCC GND OPAMP");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "X1");
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "IN");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "OUT");
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "VCC");
    ASSERT_EQ(lines[0].m_tokens[8].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[8].m_text, "GND");
}

TEST(NetlistLexerChecks, subcircuit_instance_last_token_is_subcircuit_name) {
    // arrange: Xyce RG 2.3.33 instance form X<name> [node]* <subcircuit name>;
    // the final token on an X line is the subcircuit name, not a node
    const std::string netlist = "XU395 1 2 3 4 5 6 7 LM317";
    // act
    const auto lines = tokenize_netlist(netlist);
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.size(), 17);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "XU395");
    // the seven interface nodes
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "1");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "2");
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "3");
    ASSERT_EQ(lines[0].m_tokens[8].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[8].m_text, "4");
    ASSERT_EQ(lines[0].m_tokens[10].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[10].m_text, "5");
    ASSERT_EQ(lines[0].m_tokens[12].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[12].m_text, "6");
    ASSERT_EQ(lines[0].m_tokens[14].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[14].m_text, "7");
    // the subcircuit name must not be classified as a node; it uses the model
    // type so it shares the colour of numeric value arguments
    ASSERT_EQ(lines[0].m_tokens[16].m_type, NetlistTokenType::MODEL);
    ASSERT_EQ(lines[0].m_tokens[16].m_text, "LM317");
}

TEST(NetlistLexerChecks, subcircuit_instance_with_params_splits_name_from_params) {
    // arrange: Xyce RG 2.3.33 X<name> [node]* <subcircuit name> [PARAMS: ...];
    // the subcircuit name is the token before PARAMS: and the trailing
    // parameter tokens follow their normal classification
    const std::string netlist = "XFELT 1 2 FILTER PARAMS: CENTER=200kHz";
    // act
    const auto lines = tokenize_netlist(netlist);
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "XFELT");
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "1");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "2");
    // the subcircuit name sits before PARAMS: and is not a node
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::MODEL);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "FILTER");
    // PARAMS: keeps its keyword classification
    ASSERT_EQ(lines[0].m_tokens[8].m_type, NetlistTokenType::KEYWORD);
    ASSERT_EQ(lines[0].m_tokens[8].m_text, "PARAMS:");
    // the parameter list after PARAMS: is not node-classified
    ASSERT_EQ(lines[0].m_tokens[10].m_type, NetlistTokenType::PLAIN_TEXT);
    ASSERT_EQ(lines[0].m_tokens[10].m_text, "CENTER");
    ASSERT_EQ(lines[0].m_tokens[11].m_type, NetlistTokenType::OPERATOR);
    ASSERT_EQ(lines[0].m_tokens[11].m_text, "=");
    ASSERT_EQ(lines[0].m_tokens[12].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[12].m_text, "200kHz");
}

TEST(NetlistLexerChecks, tokenizes_xyce_special_y_devices) {
    // arrange / act
    const auto lines = tokenize_netlist("YMEMRISTOR1 N1 N2 MEM_MODEL");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "YMEMRISTOR1");
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "N1");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "N2");
}

TEST(NetlistLexerChecks, tokenizes_inline_comments) {
    // arrange / act
    const auto lines = tokenize_netlist("R1 1 0 1k ; load resistor\nC1 2 0 10u ; filtering capacitor");
    // assert
    ASSERT_EQ(lines.size(), 2);
    ASSERT_EQ(lines[0].m_tokens.back().m_type, NetlistTokenType::COMMENT);
    ASSERT_EQ(lines[0].m_tokens.back().m_text, "; load resistor");
    ASSERT_EQ(lines[1].m_tokens.back().m_type, NetlistTokenType::COMMENT);
    ASSERT_EQ(lines[1].m_tokens.back().m_text, "; filtering capacitor");
}

TEST(NetlistLexerChecks, tokenizes_mathematical_expressions) {
    // arrange / act
    const auto lines = tokenize_netlist(".PARAM R_LOAD = { 10k * (1 + 0.05) }");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.back().m_type, NetlistTokenType::EXPRESSION);
    ASSERT_EQ(lines[0].m_tokens.back().m_text, "{ 10k * (1 + 0.05) }");
}

TEST(NetlistLexerChecks, tokenizes_string_literals) {
    // arrange / act
    const auto lines = tokenize_netlist(".PRINT TRAN FILE=\"output_file.raw\" FORMAT=RAW");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::STRING_LITERAL);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "\"output_file.raw\"");
}

TEST(NetlistLexerChecks, tokenizes_engineering_scale_factors) {
    // arrange / act
    const auto lines = tokenize_netlist("V1 1 0 5.0V\nR1 1 2 10kOhm\nC1 2 0 100nF\nL1 2 3 10mH\nR2 3 0 1Meg");
    // assert
    ASSERT_EQ(lines.size(), 5);
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "5.0V");
    ASSERT_EQ(lines[1].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[1].m_tokens[6].m_text, "10kOhm");
    ASSERT_EQ(lines[2].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[2].m_tokens[6].m_text, "100nF");
    ASSERT_EQ(lines[3].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[3].m_tokens[6].m_text, "10mH");
    ASSERT_EQ(lines[4].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[4].m_tokens[6].m_text, "1Meg");
}

TEST(NetlistLexerChecks, reconstruction_invariant_preserves_exact_source_text) {
    // arrange
    const std::string source = "* Simple RLC Series Circuit - Transient Analysis Test\n"
                               "V1 IN 0 PULSE(0 5 0 1n 1n 10m 20m)\n"
                               "R1 IN N1 100 ; current limiting resistor\n"
                               "L1 N1 N2 10mH\n"
                               "C1 N2 0 1uF\n"
                               "\n"
                               ".PREPROCESS REPLACEGROUND TRUE\n"
                               ".TRAN 1u 20m 0\n"
                               ".PRINT TRAN FORMAT=RAW FILE=tran-simple-01.raw V(*) I(*)\n"
                               "+ NP=1024 WINDOW=HANN\n"
                               ".END\n";
    // act
    const auto lines = tokenize_netlist(source);
    std::string reconstructed;
    for (size_t i = 0; i < lines.size(); ++i) {
        for (const auto& token : lines[i].m_tokens) {
            reconstructed += token.m_text;
        }
        if (i + 1 < lines.size()) {
            reconstructed += "\n";
        }
    }
    // assert
    ASSERT_EQ(reconstructed, source);
}

TEST(NetlistLexerChecks, handles_crlf_and_lf_newlines) {
    // arrange
    const std::string crlf_text = "R1 1 0 100\r\nR2 2 0 200\r\n";
    const std::string lf_text = "R1 1 0 100\nR2 2 0 200\n";
    // act
    const auto crlf_lines = tokenize_netlist(crlf_text);
    const auto lf_lines = tokenize_netlist(lf_text);
    // assert
    ASSERT_EQ(crlf_lines.size(), 3);
    ASSERT_EQ(lf_lines.size(), 3);
    ASSERT_EQ(crlf_lines[0].m_tokens.size(), lf_lines[0].m_tokens.size());
    ASSERT_EQ(crlf_lines[1].m_tokens.size(), lf_lines[1].m_tokens.size());
    ASSERT_TRUE(crlf_lines[2].m_tokens.empty());
    ASSERT_TRUE(lf_lines[2].m_tokens.empty());
}

TEST(NetlistLexerChecks, tokenizes_negative_and_exponent_numbers) {
    // arrange / act
    const auto lines = tokenize_netlist("V1 1 0 -5.0V\nI1 2 0 1e-6\nC1 3 0 2.5e-12F");
    // assert
    ASSERT_EQ(lines.size(), 3);
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "-5.0V");
    ASSERT_EQ(lines[1].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[1].m_tokens[6].m_text, "1e-6");
    ASSERT_EQ(lines[2].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[2].m_tokens[6].m_text, "2.5e-12F");
}

TEST(NetlistLexerChecks, tokenizes_lowercase_device_names) {
    // arrange / act
    const auto lines = tokenize_netlist("r_load in out 50\nc_filt out 0 10u");
    // assert
    ASSERT_EQ(lines.size(), 2);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "r_load");
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "in");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "out");
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "50");
    ASSERT_EQ(lines[1].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[1].m_tokens[0].m_text, "c_filt");
}

TEST(NetlistLexerChecks, tokenizes_nested_dc_sweeps) {
    // arrange / act
    const auto lines = tokenize_netlist(".DC V1 0 5 0.5 R1:R 1k 10k 1k TEMP 25 125 25");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DIRECTIVE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, ".DC");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "0");
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "5");
    ASSERT_EQ(lines[0].m_tokens[8].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[8].m_text, "0.5");
}

TEST(NetlistLexerChecks, handles_unclosed_string_and_expression_gracefully) {
    // arrange / act
    const auto lines = tokenize_netlist(".PRINT FILE=\"unclosed_string\nR1 1 0 {1k + 2k");
    // assert
    ASSERT_EQ(lines.size(), 2);
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::STRING_LITERAL);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "\"unclosed_string");
    ASSERT_EQ(lines[1].m_tokens[6].m_type, NetlistTokenType::EXPRESSION);
    ASSERT_EQ(lines[1].m_tokens[6].m_text, "{1k + 2k");
}

TEST(NetlistLexerChecks, tokenizes_pulse_source_parameters) {
    // arrange / act
    const auto lines = tokenize_netlist("V1 IN 0 PULSE(0 5 0 1n 1n 10m 20m)");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "V1");
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "IN");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "0");
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::KEYWORD);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "PULSE");
    ASSERT_EQ(lines[0].m_tokens[7].m_type, NetlistTokenType::OPERATOR);
    ASSERT_EQ(lines[0].m_tokens[7].m_text, "(");
    ASSERT_EQ(lines[0].m_tokens[8].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[8].m_text, "0");
    ASSERT_EQ(lines[0].m_tokens[20].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[20].m_text, "20m");
    ASSERT_EQ(lines[0].m_tokens[21].m_type, NetlistTokenType::OPERATOR);
    ASSERT_EQ(lines[0].m_tokens[21].m_text, ")");
}

TEST(NetlistLexerChecks, tokenizes_global_nodes_starting_with_dollar_g) {
    // arrange / act
    const auto lines = tokenize_netlist("Vpin1 $G_GlobalNode1 0 1\nVpin2 $GVDD 0 5");
    // assert
    ASSERT_EQ(lines.size(), 2);
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "$G_GlobalNode1");
    ASSERT_EQ(lines[1].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[1].m_tokens[2].m_text, "$GVDD");
}

TEST(NetlistLexerChecks, tokenizes_differential_pins_with_plus_and_minus) {
    // arrange / act
    const auto lines = tokenize_netlist("R1 IN+ IN- 100\nV1 1+ 1- 5.0");
    // assert
    ASSERT_EQ(lines.size(), 2);
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "IN+");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "IN-");
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "100");
    ASSERT_EQ(lines[1].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[1].m_tokens[2].m_text, "1+");
    ASSERT_EQ(lines[1].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[1].m_tokens[4].m_text, "1-");
}

TEST(NetlistLexerChecks, tokenizes_devices_with_hyphens) {
    // arrange / act
    const auto lines = tokenize_netlist("R-1 1 0 1k");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "R-1");
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "1");
}

TEST(NetlistLexerChecks, tokenizes_kicad_net_name_with_hyphens_and_underscores) {
    // arrange: KiCad-style net names contain embedded hyphens followed by
    // underscores (e.g. Net-_U303A-G2_); the name must stay a single node token
    const std::string netlist = "R318 OT Net-_U303A-G2_ 100";
    // act
    const auto lines = tokenize_netlist(netlist);
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.size(), 7);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "R318");
    ASSERT_EQ(lines[0].m_tokens[1].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "OT");
    ASSERT_EQ(lines[0].m_tokens[3].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "Net-_U303A-G2_");
    ASSERT_EQ(lines[0].m_tokens[5].m_type, NetlistTokenType::WHITESPACE);
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "100");
}

TEST(NetlistLexerChecks, tokenizes_exponentiation_and_relational_operators) {
    // arrange / act
    const auto lines = tokenize_netlist(".PARAM P = { A ** 2 + (B == C) + (D != E) + (F <= G) }");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens.back().m_type, NetlistTokenType::EXPRESSION);
}

TEST(NetlistLexerChecks, tokenizes_complex_numbers_with_j_suffix) {
    // arrange / act
    const auto lines = tokenize_netlist(".PARAM a0 = 2.0J\n.PARAM b0 = 1.5e-3j");
    // assert
    ASSERT_EQ(lines.size(), 2);
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "2.0J");
    ASSERT_EQ(lines[1].m_tokens[6].m_type, NetlistTokenType::NUMBER);
    ASSERT_EQ(lines[1].m_tokens[6].m_text, "1.5e-3j");
}

TEST(NetlistLexerChecks, tokenizes_xyce_y_device_separate_name_syntax) {
    // arrange / act
    const auto lines = tokenize_netlist("YMEMRISTOR M1 1 2 MEM_MODEL");
    // assert
    ASSERT_EQ(lines.size(), 1);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, "YMEMRISTOR");
    ASSERT_EQ(lines[0].m_tokens[2].m_type, NetlistTokenType::DEVICE);
    ASSERT_EQ(lines[0].m_tokens[2].m_text, "M1");
    ASSERT_EQ(lines[0].m_tokens[4].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[4].m_text, "1");
    ASSERT_EQ(lines[0].m_tokens[6].m_type, NetlistTokenType::NODE);
    ASSERT_EQ(lines[0].m_tokens[6].m_text, "2");
}

TEST(NetlistLexerChecks, tokenizes_extended_xyce_directives) {
    // arrange / act
    const auto lines = tokenize_netlist(".GLOBAL_PARAM TEMP=27\n.FUNC MY_FN(X) {X*2}\n.DCVOLT 1 0.5");
    // assert
    ASSERT_EQ(lines.size(), 3);
    ASSERT_EQ(lines[0].m_tokens[0].m_type, NetlistTokenType::DIRECTIVE);
    ASSERT_EQ(lines[0].m_tokens[0].m_text, ".GLOBAL_PARAM");
    ASSERT_EQ(lines[1].m_tokens[0].m_type, NetlistTokenType::DIRECTIVE);
    ASSERT_EQ(lines[1].m_tokens[0].m_text, ".FUNC");
    ASSERT_EQ(lines[2].m_tokens[0].m_type, NetlistTokenType::DIRECTIVE);
    ASSERT_EQ(lines[2].m_tokens[0].m_text, ".DCVOLT");
}
