// #include <string>
// #include <type_traits>

// #include <gtest/gtest.h>

// #include "../../src/expression/xyce_parser.h"

// namespace
// {
//     template <typename T>
//     const T* as(const ExpressionPtr& expression)
//     {
//         return dynamic_cast<const T*>(expression.get());
//     }
// } // namespace

// // ========================================================================================
// // type traits
// // ========================================================================================

// static_assert(!std::is_copy_constructible_v<XyceParser>);
// static_assert(!std::is_copy_assignable_v<XyceParser>);

// // ========================================================================================
// // parse_expression
// // ========================================================================================

// TEST(XyceParserChecks, number_literal_produces_number_node)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("42");
//     // assert
//     const auto* node = as<NumberNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->text, "42");
// }

// TEST(XyceParserChecks, identifier_produces_identifier_node)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("vout");
//     // assert
//     const auto* node = as<IdentifierNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->name, "vout");
// }

// TEST(XyceParserChecks, addition)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a + b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::ADD);
// }

// TEST(XyceParserChecks, subtraction)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a - b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::SUB);
// }

// TEST(XyceParserChecks, multiplication)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a * b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::MUL);
// }

// TEST(XyceParserChecks, division)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a / b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::DIV);
// }

// TEST(XyceParserChecks, modulo)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a % b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::MOD);
// }

// TEST(XyceParserChecks, power)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a ** b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::POW);
// }

// TEST(XyceParserChecks, caret_is_xor_not_power)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a ^ b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_XOR);
// }

// TEST(XyceParserChecks, pipe_is_logical_or)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a | b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_OR);
// }

// TEST(XyceParserChecks, double_pipe_is_logical_or)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a || b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_OR);
// }

// TEST(XyceParserChecks, ampersand_is_logical_and)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a & b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_AND);
// }

// TEST(XyceParserChecks, double_ampersand_is_logical_and)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a && b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_AND);
// }

// TEST(XyceParserChecks, equal_equal)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a == b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::EQUAL);
// }

// TEST(XyceParserChecks, bang_equal)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a != b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::NOT_EQUAL);
// }

// TEST(XyceParserChecks, less)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a < b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::LESS);
// }

// TEST(XyceParserChecks, less_equal)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a <= b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::LESS_EQUAL);
// }

// TEST(XyceParserChecks, greater)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a > b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::GREATER);
// }

// TEST(XyceParserChecks, greater_equal)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a >= b");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::GREATER_EQUAL);
// }

// TEST(XyceParserChecks, unary_minus)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("-x");
//     // assert
//     const auto* node = as<UnaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, UnaryOperator::NEG);
// }

// TEST(XyceParserChecks, unary_plus)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("+x");
//     // assert
//     const auto* node = as<UnaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, UnaryOperator::POS);
// }

// TEST(XyceParserChecks, tilde_is_logical_not)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("~x");
//     // assert
//     const auto* node = as<UnaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, UnaryOperator::NOT);
// }

// TEST(XyceParserChecks, bang_is_logical_not)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("!x");
//     // assert
//     const auto* node = as<UnaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, UnaryOperator::NOT);
// }

// TEST(XyceParserChecks, ternary)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a ? b : c");
//     // assert
//     ASSERT_NE(as<TernaryOperationNode>(tree), nullptr);
// }

// TEST(XyceParserChecks, ternary_is_right_associative)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a ? b ? c : d : e");
//     // assert
//     const auto* node = as<TernaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_NE(as<TernaryOperationNode>(node->if_true), nullptr);
// }

// TEST(XyceParserChecks, parentheses_override_precedence)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("(a + b) * c");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::MUL);
//     const auto* left = as<BinaryOperationNode>(node->left);
//     ASSERT_NE(left, nullptr);
//     ASSERT_EQ(left->operator_value, BinaryOperator::ADD);
// }

// TEST(XyceParserChecks, function_call_no_args)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("f()");
//     // assert
//     const auto* node = as<FunctionCallNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->name, "f");
//     ASSERT_TRUE(node->args.empty());
// }

// TEST(XyceParserChecks, function_call_one_arg)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("abs(x)");
//     // assert
//     const auto* node = as<FunctionCallNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->args.size(), 1U);
// }

// TEST(XyceParserChecks, function_call_two_args)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("atan2(y, x)");
//     // assert
//     const auto* node = as<FunctionCallNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->args.size(), 2U);
// }

// TEST(XyceParserChecks, probe_v_single_arg)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("V(out)");
//     // assert
//     const auto* node = as<FunctionCallNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->name, "V");
//     const auto* arg = as<IdentifierNode>(node->args[0]);
//     ASSERT_NE(arg, nullptr);
//     ASSERT_EQ(arg->name, "out");
// }

// TEST(XyceParserChecks, probe_v_differential)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("V(vp, vn)");
//     // assert
//     const auto* node = as<FunctionCallNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->args.size(), 2U);
//     ASSERT_EQ(as<IdentifierNode>(node->args[0])->name, "vp");
//     ASSERT_EQ(as<IdentifierNode>(node->args[1])->name, "vn");
// }

// TEST(XyceParserChecks, probe_v_numeric_ground_node)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("V(out, 0)");
//     // assert
//     const auto* node = as<FunctionCallNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(as<IdentifierNode>(node->args[1])->name, "0");
// }

// TEST(XyceParserChecks, probe_i)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("I(r1)");
//     // assert
//     const auto* node = as<FunctionCallNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->name, "I");
// }

// TEST(XyceParserChecks, probe_v_hyphenated_node_name)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("V(net-_u304a-g2_)");
//     // assert
//     const auto* node = as<FunctionCallNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(as<IdentifierNode>(node->args[0])->name, "net-_u304a-g2_");
// }

// TEST(XyceParserChecks, step_selector_on_probe)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("V(out)@1");
//     // assert
//     const auto* node = as<StepSelectorNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->step_index, 1U);
//     ASSERT_NE(as<FunctionCallNode>(node->base), nullptr);
// }

// TEST(XyceParserChecks, step_selector_on_identifier)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("x@2");
//     // assert
//     const auto* node = as<StepSelectorNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->step_index, 2U);
// }

// TEST(XyceParserChecks, step_selector_zero_raises)
// {
//     // arrange
//     const auto text = "x@0";
//     // act
//     const auto parse = [&text]() { (void)parse_expression(text); };
//     // assert
//     ASSERT_THROW(parse(), std::invalid_argument);
// }

// TEST(XyceParserChecks, func_definition_no_params)
// {
//     // arrange
//     // act
//     const auto tree = parse_function_definition(".func f() {42}");
//     // assert
//     ASSERT_EQ(tree.name, "f");
//     ASSERT_TRUE(tree.params.empty());
// }

// TEST(XyceParserChecks, func_definition_one_param)
// {
//     // arrange
//     // act
//     const auto tree = parse_function_definition(".func sq(x) {x * x}");
//     // assert
//     ASSERT_EQ(tree.name, "sq");
//     ASSERT_EQ(tree.params.size(), 1U);
//     ASSERT_EQ(tree.params[0], "x");
// }

// TEST(XyceParserChecks, func_definition_two_params)
// {
//     // arrange
//     // act
//     const auto tree = parse_function_definition(".func add(a, b) {a + b}");
//     // assert
//     ASSERT_EQ(tree.params[0], "a");
//     ASSERT_EQ(tree.params[1], "b");
// }

// TEST(XyceParserChecks, wrong_directive_raises)
// {
//     // arrange
//     const auto text = ".param x = 1";
//     // act
//     const auto parse = [&text]() { (void)parse_function_definition(text); };
//     // assert
//     ASSERT_THROW(parse(), std::invalid_argument);
// }

// TEST(XyceParserChecks, dangling_unary_raises)
// {
//     // arrange
//     const auto text = "+";
//     // act
//     const auto parse = [&text]() { (void)parse_expression(text); };
//     // assert
//     ASSERT_THROW(parse(), std::invalid_argument);
// }

// TEST(XyceParserChecks, precedence_mul_over_add)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a + b * c");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::ADD);
//     ASSERT_EQ(as<BinaryOperationNode>(node->right)->operator_value, BinaryOperator::MUL);
// }

// TEST(XyceParserChecks, precedence_or_less_than_xor_less_than_and)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a | b ^ c & d");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_OR);
//     const auto* right = as<BinaryOperationNode>(node->right);
//     ASSERT_NE(right, nullptr);
//     ASSERT_EQ(right->operator_value, BinaryOperator::LOGICAL_XOR);
//     const auto* nested = as<BinaryOperationNode>(right->right);
//     ASSERT_NE(nested, nullptr);
//     ASSERT_EQ(nested->operator_value, BinaryOperator::LOGICAL_AND);
// }

// TEST(XyceParserChecks, precedence_relational_over_logical)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a < b & c > d");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::LOGICAL_AND);
//     ASSERT_EQ(as<BinaryOperationNode>(node->left)->operator_value, BinaryOperator::LESS);
//     ASSERT_EQ(as<BinaryOperationNode>(node->right)->operator_value, BinaryOperator::GREATER);
// }

// TEST(XyceParserChecks, power_is_right_associative)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("a ** b ** c");
//     // assert
//     const auto* node = as<BinaryOperationNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_EQ(node->operator_value, BinaryOperator::POW);
//     ASSERT_EQ(as<BinaryOperationNode>(node->right)->operator_value, BinaryOperator::POW);
// }

// TEST(XyceParserChecks, id_is_not_a_probe_family)
// {
//     // arrange
//     // act
//     const auto tree = parse_expression("id(x)");
//     // assert
//     const auto* node = as<FunctionCallNode>(tree);
//     ASSERT_NE(node, nullptr);
//     ASSERT_NE(as<IdentifierNode>(node->args[0]), nullptr);
// }