#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "expression/xyce_parser.h"

namespace
{
    template <typename T>
    const T* as(const ExpressionPtr& expression) {
        return dynamic_cast<const T*>(expression.get());
    }
} // namespace

static_assert(!std::is_copy_constructible_v<XyceParser>);
static_assert(!std::is_copy_assignable_v<XyceParser>);

// ========================================================================================
// lexer
// ========================================================================================

TEST(XyceParserChecks, tokenizes_core_symbols) {
    // act
    const auto tokens = tokenize(".func x() {1+2-3*4/5%6^~!&|&&||==!=<=>=@:?,}");
    // assert
    ASSERT_EQ(tokens.front().kind, TokenKind::DIRECTIVE);
    ASSERT_EQ(tokens.back().kind, TokenKind::EOF_TOKEN);
    ASSERT_EQ(tokens[1].kind, TokenKind::IDENTIFIER);
    ASSERT_EQ(tokens[2].kind, TokenKind::LPAREN);
}

TEST(XyceParserChecks, tokenizes_numbers_identifiers_and_suffixes) {
    // act
    const auto tokens = tokenize("42 3.14 1e6 2.5e-3 1MEG v_out node[1]");
    // assert
    ASSERT_EQ(tokens[0].kind, TokenKind::NUMBER);
    ASSERT_EQ(tokens[1].kind, TokenKind::NUMBER);
    ASSERT_EQ(tokens[2].kind, TokenKind::NUMBER);
    ASSERT_EQ(tokens[3].kind, TokenKind::NUMBER);
    ASSERT_EQ(tokens[4].kind, TokenKind::NUMBER);
    ASSERT_EQ(tokens[5].kind, TokenKind::IDENTIFIER);
    ASSERT_EQ(tokens[6].kind, TokenKind::IDENTIFIER);
}

TEST(XyceParserChecks, ignores_whitespace_and_tracks_offsets) {
    // arrange
    // act
    const auto tokens = tokenize("  1  +  2  ");
    // assert
    ASSERT_EQ(tokens.size(), 4U);
    ASSERT_EQ(tokens[0].start, 2U);
    ASSERT_EQ(tokens[1].start, 5U);
    ASSERT_EQ(tokens[2].start, 8U);
}

TEST(XyceParserChecks, rejects_invalid_characters) {
    // arrange
    const auto tokenize_text = [](const std::string& text) { return tokenize(text); };
    // act
    const auto invalid_character = std::string("$");
    // assert
    ASSERT_THROW(tokenize_text(invalid_character), std::invalid_argument);
    ASSERT_THROW(tokenize_text("\u2022foo"), std::invalid_argument);
    ASSERT_THROW(tokenize_text("foo#bar"), std::invalid_argument);
}

TEST(XyceParserChecks, lexer_instance_matches_free_function) {
    // arrange
    // act
    const auto a = XyceLexer{}.tokenize("1 + x");
    const auto b = tokenize("1 + x");
    // assert
    ASSERT_EQ(a.size(), b.size());
    ASSERT_EQ(a[0].kind, b[0].kind);
    ASSERT_EQ(a[1].kind, b[1].kind);
    ASSERT_EQ(a[2].kind, b[2].kind);
}

TEST(XyceParserChecks, number_literal_produces_number_node) {
    // arrange
    // act
    const auto tree = parse_expression("42");
    // assert
    const auto* node = as<NumberNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->text, "42");
}

TEST(XyceParserChecks, identifier_produces_identifier_node) {
    // arrange
    // act
    const auto tree = parse_expression("vout");
    // assert
    const auto* node = as<IdentifierNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->name, "vout");
}

TEST(XyceParserChecks, addition) {
    // arrange
    // act
    const auto tree = parse_expression("a + b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::ADD);
}

TEST(XyceParserChecks, subtraction) {
    // arrange
    // act
    const auto tree = parse_expression("a - b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::SUB);
}

TEST(XyceParserChecks, multiplication) {
    // arrange
    // act
    const auto tree = parse_expression("a * b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::MUL);
}

TEST(XyceParserChecks, division) {
    // arrange
    // act
    const auto tree = parse_expression("a / b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::DIV);
}

TEST(XyceParserChecks, modulo) {
    // arrange
    // act
    const auto tree = parse_expression("a % b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::MOD);
}

TEST(XyceParserChecks, power) {
    // arrange
    // act
    const auto tree = parse_expression("a ** b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::POW);
}

TEST(XyceParserChecks, caret_is_xor_not_power) {
    // arrange
    // act
    const auto tree = parse_expression("a ^ b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_XOR);
}

TEST(XyceParserChecks, pipe_is_logical_or) {
    // arrange
    // act
    const auto tree = parse_expression("a | b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_OR);
}

TEST(XyceParserChecks, double_pipe_is_logical_or) {
    // arrange
    // act
    const auto tree = parse_expression("a || b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_OR);
}

TEST(XyceParserChecks, ampersand_is_logical_and) {
    // arrange
    // act
    const auto tree = parse_expression("a & b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_AND);
}

TEST(XyceParserChecks, double_ampersand_is_logical_and) {
    // arrange
    // act
    const auto tree = parse_expression("a && b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_AND);
}

TEST(XyceParserChecks, equal_equal) {
    // arrange
    // act
    const auto tree = parse_expression("a == b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::EQUAL);
}

TEST(XyceParserChecks, bang_equal) {
    // arrange
    // act
    const auto tree = parse_expression("a != b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::NOT_EQUAL);
}

TEST(XyceParserChecks, less) {
    // arrange
    // act
    const auto tree = parse_expression("a < b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::LESS);
}

TEST(XyceParserChecks, less_equal) {
    // arrange
    // act
    const auto tree = parse_expression("a <= b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::LESS_EQUAL);
}

TEST(XyceParserChecks, greater) {
    // arrange
    // act
    const auto tree = parse_expression("a > b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::GREATER);
}

TEST(XyceParserChecks, greater_equal) {
    // arrange
    // act
    const auto tree = parse_expression("a >= b");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::GREATER_EQUAL);
}

TEST(XyceParserChecks, unary_minus) {
    // arrange
    // act
    const auto tree = parse_expression("-x");
    // assert
    const auto* node = as<UnaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, UnaryOperator::NEG);
}

TEST(XyceParserChecks, unary_plus) {
    // arrange
    // act
    const auto tree = parse_expression("+x");
    // assert
    const auto* node = as<UnaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, UnaryOperator::POS);
}

TEST(XyceParserChecks, tilde_is_logical_not) {
    // arrange
    // act
    const auto tree = parse_expression("~x");
    // assert
    const auto* node = as<UnaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, UnaryOperator::NOT);
}

TEST(XyceParserChecks, bang_is_logical_not) {
    // arrange
    // act
    const auto tree = parse_expression("!x");
    // assert
    const auto* node = as<UnaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, UnaryOperator::NOT);
}

TEST(XyceParserChecks, ternary) {
    // arrange
    // act
    const auto tree = parse_expression("a ? b : c");
    // assert
    ASSERT_NE(as<TernaryOperationNode>(tree), nullptr);
}

TEST(XyceParserChecks, ternary_is_right_associative) {
    // arrange
    // act
    const auto tree = parse_expression("a ? b ? c : d : e");
    // assert
    const auto* node = as<TernaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_NE(as<TernaryOperationNode>(node->if_true), nullptr);
}

TEST(XyceParserChecks, parses_operators_precedence_and_probes) {
    // arrange
    // act
    const auto tree = parse_expression("a + b * c ? V(out, 0)@1 : id(x)");
    // assert
    ASSERT_NE(as<TernaryOperationNode>(tree), nullptr);
}

TEST(XyceParserChecks, parentheses_override_precedence) {
    // arrange
    // act
    const auto tree = parse_expression("(a + b) * c");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::MUL);
    const auto* left = as<BinaryOperationNode>(node->left);
    ASSERT_NE(left, nullptr);
    ASSERT_EQ(left->operator_value, BinaryOperator::ADD);
}

TEST(XyceParserChecks, function_call_no_args) {
    // arrange
    // act
    const auto tree = parse_expression("f()");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->name, "f");
    ASSERT_TRUE(node->args.empty());
}

TEST(XyceParserChecks, function_call_one_arg) {
    // arrange
    // act
    const auto tree = parse_expression("abs(x)");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->args.size(), 1U);
}

TEST(XyceParserChecks, function_call_two_args) {
    // arrange
    // act
    const auto tree = parse_expression("atan2(y, x)");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->args.size(), 2U);
}

TEST(XyceParserChecks, probe_v_single_arg) {
    // arrange
    // act
    const auto tree = parse_expression("V(out)");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->name, "V");
    const auto* arg = as<IdentifierNode>(node->args[0]);
    ASSERT_NE(arg, nullptr);
    ASSERT_EQ(arg->name, "out");
}

TEST(XyceParserChecks, probe_v_differential) {
    // arrange
    // act
    const auto tree = parse_expression("V(vp, vn)");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->args.size(), 2U);
    ASSERT_EQ(as<IdentifierNode>(node->args[0])->name, "vp");
    ASSERT_EQ(as<IdentifierNode>(node->args[1])->name, "vn");
}

TEST(XyceParserChecks, probe_v_numeric_ground_node) {
    // arrange
    // act
    const auto tree = parse_expression("V(out, 0)");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(as<IdentifierNode>(node->args[1])->name, "0");
}

TEST(XyceParserChecks, probe_i) {
    // arrange
    // act
    const auto tree = parse_expression("I(r1)");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->name, "I");
}

TEST(XyceParserChecks, probe_v_hyphenated_node_name) {
    // arrange
    // act
    const auto tree = parse_expression("V(net-_u304a-g2_)");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(as<IdentifierNode>(node->args[0])->name, "net-_u304a-g2_");
}

TEST(XyceParserChecks, step_selector_on_probe) {
    // arrange
    // act
    const auto tree = parse_expression("V(out)@1");
    // assert
    const auto* node = as<StepSelectorNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->step_index, 1U);
    ASSERT_NE(as<FunctionCallNode>(node->base), nullptr);
}

TEST(XyceParserChecks, step_selector_on_identifier) {
    // arrange
    // act
    const auto tree = parse_expression("x@2");
    // assert
    const auto* node = as<StepSelectorNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->step_index, 2U);
}

TEST(XyceParserChecks, step_selector_zero_raises) {
    // arrange
    const auto text = "x@0";
    // act
    const auto parse = [&text]() { (void)parse_expression(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, func_definition_no_params) {
    // arrange
    // act
    const auto tree = parse_function_definition(".func f() {42}");
    // assert
    ASSERT_EQ(tree.name, "f");
    ASSERT_TRUE(tree.params.empty());
}

TEST(XyceParserChecks, func_definition_one_param) {
    // arrange
    // act
    const auto tree = parse_function_definition(".func sq(x) {x * x}");
    // assert
    ASSERT_EQ(tree.name, "sq");
    ASSERT_EQ(tree.params.size(), 1U);
    ASSERT_EQ(tree.params[0], "x");
}

TEST(XyceParserChecks, func_definition_two_params) {
    // arrange
    // act
    const auto tree = parse_function_definition(".func add(a, b) {a + b}");
    // assert
    ASSERT_EQ(tree.name, "add");
    ASSERT_EQ(tree.params.size(), 2U);
    ASSERT_EQ(tree.params[0], "a");
    ASSERT_EQ(tree.params[1], "b");
    ASSERT_TRUE(dynamic_cast<BinaryOperationNode*>(tree.body.get()) != nullptr);
}

TEST(XyceParserChecks, wrong_directive_raises) {
    // arrange
    const auto text = ".param x = 1";
    // act
    const auto parse = [&text]() { (void)parse_function_definition(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, dangling_unary_raises) {
    // arrange
    const auto text = "+";
    // act
    const auto parse = [&text]() { (void)parse_expression(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, precedence_mul_over_add) {
    // arrange
    // act
    const auto tree = parse_expression("a + b * c");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::ADD);
    ASSERT_EQ(as<BinaryOperationNode>(node->right)->operator_value, BinaryOperator::MUL);
}

TEST(XyceParserChecks, precedence_or_less_than_xor_less_than_and) {
    // arrange
    // act
    const auto tree = parse_expression("a | b ^ c & d");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_OR);
    const auto* right = as<BinaryOperationNode>(node->right);
    ASSERT_NE(right, nullptr);
    ASSERT_EQ(right->operator_value, BinaryOperator::LOGICAL_XOR);
    const auto* nested = as<BinaryOperationNode>(right->right);
    ASSERT_NE(nested, nullptr);
    ASSERT_EQ(nested->operator_value, BinaryOperator::LOGICAL_AND);
}

TEST(XyceParserChecks, precedence_relational_over_logical) {
    // arrange
    // act
    const auto tree = parse_expression("a < b & c > d");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_AND);
    ASSERT_EQ(as<BinaryOperationNode>(node->left)->operator_value, BinaryOperator::LESS);
    ASSERT_EQ(as<BinaryOperationNode>(node->right)->operator_value, BinaryOperator::GREATER);
}

TEST(XyceParserChecks, power_is_right_associative) {
    // arrange
    // act
    const auto tree = parse_expression("a ** b ** c");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::POW);
    ASSERT_EQ(as<BinaryOperationNode>(node->right)->operator_value, BinaryOperator::POW);
}

TEST(XyceParserChecks, id_is_not_a_probe_family) {
    // arrange
    // act
    const auto tree = parse_expression("id(x)");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_NE(as<IdentifierNode>(node->args[0]), nullptr);
}

TEST(XyceParserChecks, nodes_store_expected_values) {
    // arrange
    // act
    NumberNode number("3.14");
    IdentifierNode identifier("vout");
    std::vector<ExpressionPtr> args;
    args.push_back(std::make_unique<IdentifierNode>("x"));
    FunctionCallNode call("abs", std::move(args));
    UnaryOperationNode unary(UnaryOperator::NEG, std::make_unique<IdentifierNode>("x"));
    BinaryOperationNode binary(std::make_unique<NumberNode>("1"), BinaryOperator::ADD, std::make_unique<NumberNode>("2"));
    TernaryOperationNode ternary(std::make_unique<IdentifierNode>("c"), std::make_unique<NumberNode>("1"), std::make_unique<NumberNode>("0"));
    StepSelectorNode step(std::make_unique<IdentifierNode>("v(out)"), 2);
    // assert
    ASSERT_EQ(number.text, "3.14");
    ASSERT_EQ(identifier.name, "vout");
    ASSERT_EQ(call.name, "abs");
    ASSERT_EQ(unary.operator_value, UnaryOperator::NEG);
    ASSERT_EQ(binary.operator_value, BinaryOperator::ADD);
    ASSERT_TRUE(as<IdentifierNode>(ternary.condition) != nullptr);
    ASSERT_EQ(step.step_index, 2U);
}

TEST(XyceParserChecks, implicit_suffix_multiplication_number) {
    // arrange
    // act
    const auto tree = parse_expression("1K");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::MUL);
    ASSERT_EQ(as<NumberNode>(node->left)->text, "1");
    ASSERT_EQ(as<IdentifierNode>(node->right)->name, "K");
}

TEST(XyceParserChecks, implicit_suffix_multiplication_meg) {
    // arrange
    // act
    const auto tree = parse_expression("10MEG");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::MUL);
    ASSERT_EQ(as<NumberNode>(node->left)->text, "10");
    ASSERT_EQ(as<IdentifierNode>(node->right)->name, "MEG");
}

TEST(XyceParserChecks, implicit_suffix_multiplication_parenthized_expression) {
    // arrange
    // act
    const auto tree = parse_expression("(a+2)K");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::MUL);
    const auto* right = as<IdentifierNode>(node->right);
    ASSERT_NE(right, nullptr);
    ASSERT_EQ(right->name, "K");
    const auto* left = as<BinaryOperationNode>(node->left);
    ASSERT_NE(left, nullptr);
    ASSERT_EQ(left->operator_value, BinaryOperator::ADD);
}

TEST(XyceParserChecks, implicit_suffix_multiplication_after_function_call_throws) {
    // arrange
    const auto text = "f()K";
    // act
    const auto parse = [&text]() { (void)parse_expression(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, consume_mismatch_adjacent_numbers_throws) {
    // arrange
    const auto text = "1 2";
    // act
    const auto parse = [&text]() { (void)parse_expression(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, consume_mismatch_ternary_missing_colon_throws) {
    // arrange
    const auto text = "a ? b";
    // act
    const auto parse = [&text]() { (void)parse_expression(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, consume_mismatch_func_missing_identifier_throws) {
    // arrange
    const auto text = ".func f( {1}";
    // act
    const auto parse = [&text]() { (void)parse_function_definition(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, consume_mismatch_func_missing_rbrace_throws) {
    // arrange
    const auto text = ".func f() {1";
    // act
    const auto parse = [&text]() { (void)parse_function_definition(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, step_selector_non_numeric_index_throws) {
    // arrange
    const auto text = "x@y";
    // act
    const auto parse = [&text]() { (void)parse_expression(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, step_selector_fractional_index_throws) {
    // arrange
    const auto text = "x@1.5";
    // act
    const auto parse = [&text]() { (void)parse_expression(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, step_selector_exponent_index_throws) {
    // arrange
    const auto text = "x@1e2";
    // act
    const auto parse = [&text]() { (void)parse_expression(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, step_selector_overflow_index_throws) {
    // arrange
    const auto text = "x@999999999999999999999";
    // act
    const auto parse = [&text]() { (void)parse_expression(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, lexer_directive_errors) {
    // arrange
    const auto tokenize_text = [](const std::string& text) { (void)tokenize(text); };
    // assert
    ASSERT_THROW(tokenize_text("."), std::invalid_argument);
    ASSERT_THROW(tokenize_text(".5"), std::invalid_argument);
    ASSERT_THROW(tokenize_text("1.."), std::invalid_argument);
    ASSERT_THROW(tokenize_text(".."), std::invalid_argument);
    // a directive with a valid identifier head is accepted
    EXPECT_NO_THROW(tokenize_text(".x"));
}

TEST(XyceParserChecks, lexer_exponent_errors) {
    // arrange
    const auto tokenize_text = [](const std::string& text) { (void)tokenize(text); };
    // assert
    ASSERT_THROW(tokenize_text("1e"), std::invalid_argument);
    ASSERT_THROW(tokenize_text("1e+"), std::invalid_argument);
    ASSERT_THROW(tokenize_text("1e-"), std::invalid_argument);
}

TEST(XyceParserChecks, ternary_nested_false_branch) {
    // arrange
    // act
    const auto tree = parse_expression("a ? b : c ? d : e");
    // assert
    const auto* node = as<TernaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    const auto* false_branch = as<TernaryOperationNode>(node->if_false);
    ASSERT_NE(false_branch, nullptr);
}

TEST(XyceParserChecks, func_definition_uppercase_directive) {
    // arrange
    // act
    const auto tree = parse_function_definition(".FUNC f() {1}");
    // assert
    ASSERT_EQ(tree.name, "f");
    ASSERT_TRUE(tree.params.empty());
}

TEST(XyceParserChecks, func_definition_non_identifier_param_throws) {
    // arrange
    const auto text = ".func f(1) {x}";
    // act
    const auto parse = [&text]() { (void)parse_function_definition(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, func_definition_empty_body_throws) {
    // arrange
    const auto text = ".func f() {}";
    // act
    const auto parse = [&text]() { (void)parse_function_definition(text); };
    // assert
    ASSERT_THROW(parse(), std::invalid_argument);
}

TEST(XyceParserChecks, lowercase_probe_family_parses_as_function_call) {
    // arrange
    // act
    const auto tree = parse_expression("v(out)");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->name, "v");
}

TEST(XyceParserChecks, parse_probe_node_name_fragments) {
    // arrange
    // act
    const auto tree_plus = parse_expression("V(net+5)");
    // assert
    const auto* node_plus = as<FunctionCallNode>(tree_plus);
    ASSERT_NE(node_plus, nullptr);
    ASSERT_EQ(as<IdentifierNode>(node_plus->args[0])->name, "net+5");
    const auto tree_slash = parse_expression("V(a/b)");
    const auto* node_slash = as<FunctionCallNode>(tree_slash);
    ASSERT_NE(node_slash, nullptr);
    ASSERT_EQ(as<IdentifierNode>(node_slash->args[0])->name, "a/b");
    const auto tree_colon = parse_expression("V(net:3)");
    const auto* node_colon = as<FunctionCallNode>(tree_colon);
    ASSERT_NE(node_colon, nullptr);
    ASSERT_EQ(as<IdentifierNode>(node_colon->args[0])->name, "net:3");
}

TEST(XyceParserChecks, exponent_variants) {
    // arrange
    // act
    const auto tree_upper = parse_expression("1E3");
    ASSERT_EQ(as<NumberNode>(tree_upper)->text, "1E3");
    const auto tree_plus = parse_expression("1e+3");
    ASSERT_EQ(as<NumberNode>(tree_plus)->text, "1e+3");
    const auto tree_trailing = parse_expression("3.");
    ASSERT_EQ(as<NumberNode>(tree_trailing)->text, "3.");
}

TEST(XyceParserChecks, chained_operators) {
    // arrange
    // act
    const auto tree_and = parse_expression("a && b && c");
    const auto* and_node = as<BinaryOperationNode>(tree_and);
    ASSERT_NE(and_node, nullptr);
    ASSERT_EQ(and_node->operator_value, BinaryOperator::LOGICAL_AND);
    // left-associative: ((a && b) && c)
    ASSERT_NE(as<BinaryOperationNode>(and_node->left), nullptr);
    const auto tree_add = parse_expression("a + b + c + d");
    const auto* add_node = as<BinaryOperationNode>(tree_add);
    ASSERT_NE(add_node, nullptr);
    ASSERT_EQ(add_node->operator_value, BinaryOperator::ADD);
    const auto tree_mul = parse_expression("a * b * c");
    const auto* mul_node = as<BinaryOperationNode>(tree_mul);
    ASSERT_NE(mul_node, nullptr);
    ASSERT_EQ(mul_node->operator_value, BinaryOperator::MUL);
}

TEST(XyceParserChecks, power_unary_precedence) {
    // arrange
    // act
    const auto tree = parse_expression("2 ** -3");
    // assert
    const auto* node = as<BinaryOperationNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->operator_value, BinaryOperator::POW);
    ASSERT_NE(as<UnaryOperationNode>(node->right), nullptr);
}

TEST(XyceParserChecks, empty_and_garbage_input_throws) {
    // arrange
    const auto parse = [](const std::string& text) { (void)parse_expression(text); };
    // assert
    ASSERT_THROW(parse(""), std::invalid_argument);
    ASSERT_THROW(parse(" "), std::invalid_argument);
    ASSERT_THROW(parse(")"), std::invalid_argument);
}

TEST(XyceParserChecks, probes_with_three_args_and_nested) {
    // arrange
    // act
    const auto tree_three = parse_expression("V(a, b, c)");
    const auto* node_three = as<FunctionCallNode>(tree_three);
    ASSERT_NE(node_three, nullptr);
    ASSERT_EQ(node_three->args.size(), 3U);
    const auto tree_ternary_arg = parse_expression("f(a ? b : c)");
    const auto* node_ternary_arg = as<FunctionCallNode>(tree_ternary_arg);
    ASSERT_NE(node_ternary_arg, nullptr);
    ASSERT_NE(as<TernaryOperationNode>(node_ternary_arg->args[0]), nullptr);
}

TEST(XyceParserChecks, nested_probe_inside_non_probe_call) {
    // arrange
    // act
    const auto tree = parse_expression("g(V(a), 0)");
    // assert
    const auto* node = as<FunctionCallNode>(tree);
    ASSERT_NE(node, nullptr);
    ASSERT_NE(as<FunctionCallNode>(node->args[0]), nullptr);
}

TEST(XyceParserChecks, parser_instance_reuse) {
    // arrange
    XyceParser parser;
    // act
    const auto first = parser.parse_expression("a + b");
    const auto second = parser.parse_expression("c * d");
    // assert
    ASSERT_NE(as<BinaryOperationNode>(first), nullptr);
    const auto* second_node = as<BinaryOperationNode>(second);
    ASSERT_NE(second_node, nullptr);
    ASSERT_EQ(second_node->operator_value, BinaryOperator::MUL);
}
