#include <cmath>
#include <complex>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include <spdlog/spdlog.h>

#include "expression/xyce_evaluator.h"
#include "expression/xyce_parser.h"

namespace
{
    // match the parser node hierarchy without exposing ownership details in each test
    template <typename T>
    const T* as(const ExpressionPtr& expression) {
        return dynamic_cast<const T*>(expression.get());
    }

    // build an identifier node for parser and evaluator tests
    ExpressionPtr make_identifier(const std::string& name) { return std::make_unique<IdentifierNode>(name); }

    // build a number node for parser and evaluator tests
    ExpressionPtr make_number(const std::string& text) { return std::make_unique<NumberNode>(text); }

    // keep scalar values explicit in the assertions
    XyceValue expression_value(double value) { return value; }

    // keep vector values explicit in the assertions
    XyceValue expression_value(std::vector<double>&& values) { return std::make_shared<View<double>>(std::move(values)); }

    // keep complex values explicit in the assertions
    XyceValue expression_value(const std::complex<double>& value) { return value; }

    // keep complex vector values explicit in the assertions
    XyceValue expression_value(std::vector<std::complex<double>>&& values) { return std::make_shared<View<std::complex<double>>>(std::move(values)); }

    // read the first scalar value from a variant for compact checks
    template <typename T>
    T scalar(const XyceValue& value) {
        return scalar_value<T>(value);
    }

    // normalize any value to a real vector for comparisons
    std::vector<double> as_real_vector(const XyceValue& value) {
        auto view = to_real_vector(value);
        return std::vector<double>(view->begin(), view->end());
    }

    // parse and evaluate a single expression in one step
    XyceValue eval(const std::string& text, const std::unordered_map<std::string, XyceValue>& expressions = {}, const std::unordered_map<std::string, FunctionDefinitionNode>& functions = {}, const std::unordered_map<std::string, XyceValue>& constants = {}, const std::vector<std::pair<size_t, size_t>>& step_slices = {}) { return evaluate_expression(*parse_expression(text), expressions, functions, constants, step_slices); }
} // namespace

TEST(XyceEvaluatorChecks, tokenizes_core_symbols) {
    // act
    const auto tokens = tokenize(".func x() {1+2-3*4/5%6^~!&|&&||==!=<=>=@:?,}");
    // assert
    ASSERT_EQ(tokens.front().kind, TokenKind::DIRECTIVE);
    ASSERT_EQ(tokens.back().kind, TokenKind::EOF_TOKEN);
    ASSERT_EQ(tokens[1].kind, TokenKind::IDENTIFIER);
    ASSERT_EQ(tokens[2].kind, TokenKind::LPAREN);
}

TEST(XyceEvaluatorChecks, tokenizes_numbers_identifiers_and_suffixes) {
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

TEST(XyceEvaluatorChecks, ignores_whitespace_and_tracks_offsets) {
    // arrange
    // act
    const auto tokens = tokenize("  1  +  2  ");
    // assert
    ASSERT_EQ(tokens.size(), 4U);
    ASSERT_EQ(tokens[0].start, 2U);
    ASSERT_EQ(tokens[1].start, 5U);
    ASSERT_EQ(tokens[2].start, 8U);
}

TEST(XyceEvaluatorChecks, rejects_invalid_characters) {
    // arrange
    const auto tokenize_text = [](const std::string& text) { return tokenize(text); };
    // act
    const auto invalid_character = std::string("$");
    // assert
    ASSERT_THROW(tokenize_text(invalid_character), std::invalid_argument);
    ASSERT_THROW(tokenize_text("\u2022foo"), std::invalid_argument);
    ASSERT_THROW(tokenize_text("foo#bar"), std::invalid_argument);
}

TEST(XyceEvaluatorChecks, lexer_instance_matches_free_function) {
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

// ========================================================================================
// parser
// ========================================================================================

TEST(XyceEvaluatorChecks, parses_operators_precedence_and_probes) {
    // arrange
    // act
    const auto tree = parse_expression("a + b * c ? V(out, 0)@1 : id(x)");
    // assert
    ASSERT_NE(as<TernaryOperationNode>(tree), nullptr);
}

TEST(XyceEvaluatorChecks, parses_function_definitions) {
    // arrange
    // act
    const auto tree = parse_function_definition(".func add(a, b) {a + b}");
    // assert
    ASSERT_EQ(tree.name, "add");
    ASSERT_EQ(tree.params.size(), 2U);
    ASSERT_TRUE(dynamic_cast<BinaryOperationNode*>(tree.body.get()) != nullptr);
}

TEST(XyceEvaluatorChecks, parser_nodes_store_expected_values) {
    // arrange
    // act
    NumberNode number("3.14");
    IdentifierNode identifier("vout");
    std::vector<ExpressionPtr> args;
    args.push_back(make_identifier("x"));
    FunctionCallNode call("abs", std::move(args));
    UnaryOperationNode unary(UnaryOperator::NEG, make_identifier("x"));
    BinaryOperationNode binary(make_number("1"), BinaryOperator::ADD, make_number("2"));
    TernaryOperationNode ternary(make_identifier("c"), make_number("1"), make_number("0"));
    StepSelectorNode step(make_identifier("v(out)"), 2);
    // assert
    ASSERT_EQ(number.text, "3.14");
    ASSERT_EQ(identifier.name, "vout");
    ASSERT_EQ(call.name, "abs");
    ASSERT_EQ(unary.operator_value, UnaryOperator::NEG);
    ASSERT_EQ(binary.operator_value, BinaryOperator::ADD);
    ASSERT_TRUE(as<IdentifierNode>(ternary.condition) != nullptr);
    ASSERT_EQ(step.step_index, 2U);
}

TEST(XyceEvaluatorChecks, parse_errors_match_scenarios) {
    // arrange
    const auto parse_text = [](const std::string& text, const bool is_function_definition) {
        if (is_function_definition) {
            (void)parse_function_definition(text);
            return;
        }
        (void)parse_expression(text);
    };
    // act
    const auto invalid_function_definition = std::string(".param x = 1");
    // assert
    ASSERT_THROW(parse_text(invalid_function_definition, true), std::invalid_argument);
    ASSERT_THROW(parse_text("+", false), std::invalid_argument);
    ASSERT_THROW(parse_text("x@0", false), std::invalid_argument);
}

// ========================================================================================
// builtin constants and functions
// ========================================================================================

TEST(XyceEvaluatorChecks, constants_match_expected_values) {
    // arrange
    const auto& constants = BUILTIN_CONSTANTS;
    // act
    const auto pi = scalar<double>(constants.at("pi"));
    // assert
    ASSERT_NEAR(pi, std::acos(-1.0), 1e-12);
    ASSERT_NEAR(scalar<double>(constants.at("e")), std::exp(1.0), 1e-12);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("meg")), 1e6);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("k")), 1e3);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("m")), 1e-3);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("u")), 1e-6);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("n")), 1e-9);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("p")), 1e-12);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("f")), 1e-15);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("g")), 1e9);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("t")), 1e12);
    ASSERT_NEAR(scalar<double>(constants.at("mil")), 25.4e-6, 1e-20);
    ASSERT_EQ(std::get<std::complex<double>>(constants.at("j")), std::complex<double>(0.0, 1.0));
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("mho")), 1.0);
}

TEST(XyceEvaluatorChecks, core_builtin_functions_work) {
    // arrange
    const auto& functions = BUILTIN_FUNCTIONS;
    // act
    const auto log_value = scalar<double>(functions.at("log")({expression_value(100.0)}));
    // assert
    ASSERT_DOUBLE_EQ(log_value, 2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("log10")({expression_value(1000.0)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("ln")({expression_value(std::exp(1.0))})), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("abs")({expression_value(-3.0)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("sqrt")({expression_value(9.0)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("db")({expression_value(10.0)})), 20.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("real")({expression_value(std::complex<double>(3.0, 4.0))})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("imag")({expression_value(std::complex<double>(3.0, 4.0))})), 4.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("phase")({expression_value(std::complex<double>(0.0, 1.0))})), 90.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("sin")({expression_value(0.0)})), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("cos")({expression_value(0.0)})), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("tan")({expression_value(0.0)})), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("atan2")({expression_value(1.0), expression_value(1.0)})), std::atan2(1.0, 1.0));
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("sgn")({expression_value(-3.0)})), -1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("sign")({expression_value(-3.0), expression_value(2.0)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("uramp")({expression_value(-5.0)})), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("stp")({expression_value(1.0)})), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("round")({expression_value(2.7)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("nint")({expression_value(1.5)})), 2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("floor")({expression_value(2.9)})), 2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("ceil")({expression_value(2.1)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("int")({expression_value(-2.9)})), -2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("pow")({expression_value(2.0), expression_value(10.0)})), 1024.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("pwr")({expression_value(-2.0), expression_value(3.0)})), 8.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("pwrs")({expression_value(-2.0), expression_value(3.0)})), -8.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("fmod")({expression_value(10.0), expression_value(3.0)})), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("min")({expression_value(3.0), expression_value(1.0), expression_value(2.0)})), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("max")({expression_value(3.0), expression_value(1.0), expression_value(2.0)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("limit")({expression_value(5.0), expression_value(1.0), expression_value(10.0)})), 5.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("if")({expression_value(1.0), expression_value(10.0), expression_value(20.0)})), 10.0);
}

TEST(XyceEvaluatorChecks, builtin_function_errors_match_scenarios) {
    // arrange
    const auto& functions = BUILTIN_FUNCTIONS;
    // act
    const auto invalid_abs_args = std::vector<XyceValue>{expression_value(1.0), expression_value(2.0)};
    // assert
    ASSERT_THROW(functions.at("abs")(invalid_abs_args), std::invalid_argument);
    ASSERT_THROW(functions.at("sign")({expression_value(1.0)}), std::invalid_argument);
    ASSERT_THROW(functions.at("fmod")({expression_value(1.0)}), std::invalid_argument);
    ASSERT_THROW(functions.at("ddt")({expression_value(1.0)}), std::logic_error);
    ASSERT_THROW(functions.at("sdt")({expression_value(1.0)}), std::logic_error);
}

// ========================================================================================
// evaluator
// ========================================================================================

TEST(XyceLanguageEvaluator, arithmetic_logical_relational_and_ternary_work) {
    // arrange
    const auto evaluate = [](const std::string& expression) { return eval(expression); };
    // act
    const auto addition = scalar<double>(evaluate("1 + 2"));
    // assert
    ASSERT_DOUBLE_EQ(addition, 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("5 - 3")), 2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("4 * 3")), 12.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("10 / 4")), 2.5);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("10 % 3")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("2 ** 10")), 1024.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("-5")), -5.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("+7")), 7.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("1 | 0")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("1 || 0")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("0 | 0")), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("1 & 1")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("1 && 0")), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("1 ^ 0")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("1 ^ 1")), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("~0")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("!1")), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("3 == 3")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("3 != 4")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("2 < 5")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("3 <= 3")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("5 > 2")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("3 >= 3")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("1 ? 10 : 20")), 10.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("0 ? 10 : 20")), 20.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("(1 + 2) * 4")), 12.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("1 + 2 * 3")), 7.0);
    ASSERT_DOUBLE_EQ(scalar<double>(evaluate("1 + 2 + 3 + 4")), 10.0);
}

TEST(XyceLanguageEvaluator, variables_constants_suffixes_and_builtin_calls_work) {
    // arrange
    // act
    const std::unordered_map<std::string, XyceValue> expressions{
        {"x", expression_value(7.0)},
        {"v(out)", expression_value(3.3)},
        {"v(a)", expression_value(5.0)},
        {"v(b)", expression_value(2.0)},
    };
    const std::unordered_map<std::string, FunctionDefinitionNode> functions{
        {"sq", parse_function_definition(".func sq(x) {x * x}")},
        {"add", parse_function_definition(".func add(a, b) {a + b}")},
    };
    // assert
    ASSERT_DOUBLE_EQ(scalar<double>(eval("x", expressions)), 7.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("X", expressions)), 7.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("pi")), std::acos(-1.0));
    ASSERT_DOUBLE_EQ(scalar<double>(eval("MEG")), 1e6);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("1T")), 1e12);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("1MIL")), 25.4e-6);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("abs(-3)")), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("sqrt(9)")), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("log(100)")), 2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("ln(1)")), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("sin(0)")), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("cos(0)")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("exp(1)")), std::exp(1.0));
    ASSERT_DOUBLE_EQ(scalar<double>(eval("min(3, 1, 2)")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("max(3, 1, 2)")), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("sgn(5)")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("sign(3, -1)")), -3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("stp(1)")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("nint(2.7)")), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("fmod(10, 3)")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("if(1, 10, 20)")), 10.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("sq(3)", {}, functions)), 9.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("add(2, 5)", {}, functions)), 7.0);
}

TEST(XyceLanguageEvaluator, probes_and_steps_work) {
    // arrange
    // act
    const std::unordered_map<std::string, XyceValue> expressions{
        {"v(out)", expression_value(3.3)},
        {"v(a)", expression_value(5.0)},
        {"v(b)", expression_value(2.0)},
        {"i(r1)", expression_value(0.01)},
    };
    const std::vector<std::pair<size_t, size_t>> steps{{0, 3}, {3, 6}};
    const std::unordered_map<std::string, XyceValue> arrays{
        {"v(out)", expression_value(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0})},
    };
    // assert
    ASSERT_DOUBLE_EQ(scalar<double>(eval("V(out)", expressions)), 3.3);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("V(a, b)", expressions)), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("V(out, 0)", expressions)), 3.3);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("I(r1)", expressions)), 0.01);
    ASSERT_NO_THROW(eval("V(out)@1", arrays, {}, {}, steps));
    ASSERT_EQ(as_real_vector(eval("V(out)@1", arrays, {}, {}, steps)), std::vector<double>({1.0, 2.0, 3.0}));
    ASSERT_EQ(as_real_vector(eval("V(out)@2", arrays, {}, {}, steps)), std::vector<double>({4.0, 5.0, 6.0}));
    ASSERT_THROW(eval("V(out)@5", arrays, {}, {}, steps), std::invalid_argument);
    ASSERT_THROW(eval("V(out)@1", arrays), std::invalid_argument);
}

TEST(XyceLanguageEvaluator, arrays_and_edge_cases_work) {
    // arrange
    // act
    const std::unordered_map<std::string, XyceValue> expressions{
        {"x", expression_value(std::vector<double>{1.0, 2.0, 3.0})},
        {"a", expression_value(std::vector<double>{0.0, 0.0, 1.0, 1.0})},
        {"b", expression_value(std::vector<double>{0.0, 1.0, 0.0, 1.0})},
    };
    // assert
    ASSERT_EQ(as_real_vector(eval("x + 1", expressions)), std::vector<double>({2.0, 3.0, 4.0}));
    ASSERT_EQ(as_real_vector(eval("x * 2", expressions)), std::vector<double>({2.0, 4.0, 6.0}));
    ASSERT_EQ(as_real_vector(eval("x % 3", expressions)), std::vector<double>({1.0, 2.0, 0.0}));
    ASSERT_EQ(as_real_vector(eval("x > 0 ? 1 : 0", expressions)), std::vector<double>({1.0, 1.0, 1.0}));
    ASSERT_EQ(as_real_vector(eval("a ^ b", expressions)), std::vector<double>({0.0, 1.0, 1.0, 0.0}));
    ASSERT_DOUBLE_EQ(scalar<double>(eval("1 / 0")), std::numeric_limits<double>::infinity());
    ASSERT_DOUBLE_EQ(scalar<double>(eval("2 ** -1")), 0.5);
    ASSERT_DOUBLE_EQ(std::abs(scalar<double>(eval("-7 % 3"))), 2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("(x > 0) & (x < 10)", {{"x", expression_value(5.0)}})), 1.0);
    ASSERT_EQ(as_real_vector(eval("if(x, x, 0)", {{"x", expression_value(std::vector<double>{0.0, 1.0, 2.0})}})), std::vector<double>({0.0, 1.0, 2.0}));
}

TEST(XyceLanguageEvaluator, user_functions_and_errors_work) {
    // arrange
    // act
    const auto square = parse_function_definition(".func sq(x) {x * x}");
    const auto add = parse_function_definition(".func add(a, b) {a + b}");
    const auto mysqrt = parse_function_definition(".func mysqrt(x) {sqrt(x)}");
    const auto recursive = parse_function_definition(".func f(x) {f(x - 1)}");
    const std::unordered_map<std::string, FunctionDefinitionNode> functions{
        {"sq", square},
        {"add", add},
        {"mysqrt", mysqrt},
        {"f", recursive},
    };
    // assert
    ASSERT_DOUBLE_EQ(scalar<double>(eval("sq(3)", {}, functions)), 9.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("add(2, 5)", {}, functions)), 7.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("mysqrt(16)", {}, functions)), 4.0);
    ASSERT_THROW(eval("f(5)", {}, functions), std::invalid_argument);
    ASSERT_THROW(eval("sq(1, 2)", {}, functions), std::invalid_argument);
    ASSERT_THROW(eval("no_such_fn(1)"), std::invalid_argument);
}

TEST(XyceLanguageEvaluator, node_and_context_types_match) {
    // arrange
    // act
    EvaluationContext context{{}, {{"x", expression_value(1.0)}}, {}, {}, {}};
    NumberNode number("3.14");
    IdentifierNode identifier("vout");
    std::vector<ExpressionPtr> args;
    args.push_back(make_identifier("x"));
    FunctionCallNode call("abs", std::move(args));
    BinaryOperationNode binary(make_number("1"), BinaryOperator::ADD, make_number("2"));
    UnaryOperationNode unary(UnaryOperator::NEG, make_identifier("x"));
    TernaryOperationNode ternary(make_identifier("c"), make_number("1"), make_number("0"));
    StepSelectorNode step(make_identifier("v(out)"), 1);
    // assert
    ASSERT_EQ(number.text, "3.14");
    ASSERT_EQ(identifier.name, "vout");
    ASSERT_EQ(call.args.size(), 1U);
    ASSERT_EQ(binary.operator_value, BinaryOperator::ADD);
    ASSERT_EQ(unary.operator_value, UnaryOperator::NEG);
    ASSERT_TRUE(as<IdentifierNode>(ternary.condition) != nullptr);
    ASSERT_EQ(step.step_index, 1U);
}

// ========================================================================================
// extended evaluator coverage
// ========================================================================================

TEST(XyceLanguageEvaluator, logical_short_circuit_and_vector_paths) {
    // assert
    ASSERT_DOUBLE_EQ(scalar<double>(eval("0 && 1")), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("0 || 1")), 1.0);
    const std::unordered_map<std::string, XyceValue> expressions{
        {"a", expression_value(std::vector<double>{0.0, 1.0})},
        {"b", expression_value(std::vector<double>{1.0, 1.0})},
    };
    ASSERT_EQ(as_real_vector(eval("a && b", expressions)), std::vector<double>({0.0, 1.0}));
    ASSERT_EQ(as_real_vector(eval("a || b", expressions)), std::vector<double>({1.0, 1.0}));
}

TEST(XyceLanguageEvaluator, scalar_left_broadcast_real) {
    // arrange
    const std::unordered_map<std::string, XyceValue> expressions{
        {"x", expression_value(std::vector<double>{1.0, 2.0})},
    };
    // assert
    ASSERT_EQ(as_real_vector(eval("1 + x", expressions)), std::vector<double>({2.0, 3.0}));
}

TEST(XyceLanguageEvaluator, vector_size_mismatch_throws) {
    // arrange
    const std::unordered_map<std::string, XyceValue> expressions{
        {"x", expression_value(std::vector<double>{1.0, 2.0, 3.0})},
        {"y", expression_value(std::vector<double>{1.0, 2.0})},
    };
    // assert
    ASSERT_THROW(eval("x + y", expressions), std::invalid_argument);
}

TEST(XyceLanguageEvaluator, complex_binary_operations_broadcast) {
    // assert
    const auto j_plus_1 = scalar<std::complex<double>>(eval("j + 1"));
    ASSERT_DOUBLE_EQ(j_plus_1.real(), 1.0);
    ASSERT_DOUBLE_EQ(j_plus_1.imag(), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j - j")), 0.0);
    ASSERT_DOUBLE_EQ(scalar<std::complex<double>>(eval("j * j")).real(), -1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j / j")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<std::complex<double>>(eval("j ** 2")).real(), -1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j == j")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j != 1")), 1.0);
    const std::unordered_map<std::string, XyceValue> expressions{
        {"cv", expression_value(std::vector<std::complex<double>>{{1.0, 1.0}, {2.0, 2.0}})},
        {"cv2", expression_value(std::vector<std::complex<double>>{{1.0, 0.0}, {0.0, 1.0}})},
        {"small", expression_value(std::vector<std::complex<double>>{{1.0, 0.0}})},
    };
    ASSERT_EQ(as_real_vector(eval("cv + cv2", expressions)), std::vector<double>({2.0, 2.0}));
    ASSERT_EQ(as_real_vector(eval("1 + cv", expressions)), std::vector<double>({2.0, 3.0}));
    ASSERT_EQ(as_real_vector(eval("cv + 1", expressions)), std::vector<double>({2.0, 3.0}));
    ASSERT_THROW(eval("cv + small", expressions), std::invalid_argument);
}

TEST(XyceLanguageEvaluator, complex_relational_operators_use_real_part) {
    // arrange
    spdlog::set_level(spdlog::level::warn);
    // assert
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j < 1")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j <= 1")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j > 1")), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j >= 1")), 0.0);
}

TEST(XyceLanguageEvaluator, unary_negate_and_not_cover_all_value_types) {
    // assert
    const auto neg_j = scalar<std::complex<double>>(eval("-j"));
    ASSERT_DOUBLE_EQ(neg_j.real(), 0.0);
    ASSERT_DOUBLE_EQ(neg_j.imag(), -1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("!j")), 1.0);
    ASSERT_EQ(as_real_vector(eval("-x", {{"x", expression_value(std::vector<double>{1.0, -2.0})}})), std::vector<double>({-1.0, 2.0}));
    ASSERT_EQ(as_real_vector(eval("!x", {{"x", expression_value(std::vector<double>{0.0, 2.0})}})), std::vector<double>({1.0, 0.0}));
    ASSERT_EQ(as_real_vector(eval("-cv", {{"cv", expression_value(std::vector<std::complex<double>>{{1.0, 2.0}, {3.0, -1.0}})}})), std::vector<double>({-1.0, -3.0}));
    ASSERT_EQ(as_real_vector(eval("!cv", {{"cv", expression_value(std::vector<std::complex<double>>{{0.0, 1.0}, {1.0, 0.0}})}})), std::vector<double>({1.0, 0.0}));
}

struct UnknownExpressionNode final : ExpressionNode {};

TEST(XyceLanguageEvaluator, unsupported_constructs_throw) {
    // assert
    UnaryOperationNode unary(static_cast<UnaryOperator>(99), make_number("1"));
    ASSERT_THROW(evaluate_expression(unary), std::invalid_argument);
    BinaryOperationNode binary(make_number("1"), static_cast<BinaryOperator>(99), make_number("2"));
    ASSERT_THROW(evaluate_expression(binary), std::invalid_argument);
    UnknownExpressionNode unknown;
    ASSERT_THROW(evaluate_expression(unknown), std::invalid_argument);
    // complex probe argument cannot be flattened into a probe name
    std::vector<ExpressionPtr> complex_args;
    complex_args.push_back(std::make_unique<BinaryOperationNode>(make_number("1"), BinaryOperator::ADD, make_number("2")));
    FunctionCallNode bad_probe("v", std::move(complex_args));
    ASSERT_THROW(evaluate_expression(bad_probe), std::invalid_argument);
    // numeric probe argument is accepted but the probe itself is unknown
    std::vector<ExpressionPtr> number_args;
    number_args.push_back(make_number("0"));
    FunctionCallNode number_probe("v", std::move(number_args));
    ASSERT_THROW(evaluate_expression(number_probe), std::invalid_argument);
}

TEST(XyceLanguageEvaluator, probe_resolves_from_function_local_variable) {
    // arrange
    const std::vector<std::string> params{"v(out)"};
    std::vector<ExpressionPtr> args;
    args.push_back(make_identifier("out"));
    const FunctionDefinitionNode probe_fn("probe_fn", params, std::make_unique<FunctionCallNode>("V", std::move(args)));
    const std::unordered_map<std::string, FunctionDefinitionNode> functions{{"probe_fn", probe_fn}};
    // assert
    ASSERT_DOUBLE_EQ(scalar<double>(eval("probe_fn(9)", {}, functions)), 9.0);
}

TEST(XyceLanguageEvaluator, probe_grounded_first_argument_negates) {
    // assert
    ASSERT_DOUBLE_EQ(scalar<double>(eval("V(0, b)", {{"v(b)", expression_value(5.0)}})), -5.0);
    const auto negated = scalar<std::complex<double>>(eval("V(0, b)", {{"v(b)", expression_value(std::complex<double>(0.0, 2.0))}}));
    ASSERT_DOUBLE_EQ(negated.real(), 0.0);
    ASSERT_DOUBLE_EQ(negated.imag(), -2.0);
    ASSERT_EQ(as_real_vector(eval("V(0, b)", {{"v(b)", expression_value(std::vector<double>{1.0, 2.0, 3.0})}})), std::vector<double>({-1.0, -2.0, -3.0}));
    ASSERT_EQ(as_real_vector(eval("V(0, b)", {{"v(b)", expression_value(std::vector<std::complex<double>>{{1.0, 2.0}, {3.0, 4.0}})}})), std::vector<double>({-1.0, -3.0}));
}

TEST(XyceLanguageEvaluator, differential_probe_complex_and_single_reference) {
    // arrange
    const std::unordered_map<std::string, XyceValue> complex_probes{
        {"v(a)", expression_value(std::complex<double>(3.0, 1.0))},
        {"v(b)", expression_value(std::complex<double>(1.0, 0.0))},
    };
    // assert
    ASSERT_EQ(as_real_vector(eval("V(a, b)", complex_probes)), std::vector<double>({2.0}));
    // single reference value is returned when the second node is missing
    ASSERT_DOUBLE_EQ(scalar<double>(eval("V(a, b)", {{"v(a)", expression_value(5.0)}})), 5.0);
}

TEST(XyceLanguageEvaluator, network_parameter_probes_resolve_when_present) {
    // arrange
    const std::unordered_map<std::string, XyceValue> probes{
        {"s11(1, 2)", expression_value(3.0)},
    };
    // assert
    ASSERT_DOUBLE_EQ(scalar<double>(eval("S11(1, 2)", probes)), 3.0);
    ASSERT_THROW(eval("S11(1, 2)"), std::invalid_argument);
    // complex arguments disable the probe interpretation
    std::vector<ExpressionPtr> args;
    args.push_back(std::make_unique<BinaryOperationNode>(make_number("1"), BinaryOperator::ADD, make_number("2")));
    FunctionCallNode s11("s11", std::move(args));
    ASSERT_THROW(evaluate_expression(s11), std::invalid_argument);
}

TEST(XyceLanguageEvaluator, step_selector_slices_scalar_and_complex) {
    // arrange
    const std::vector<std::pair<size_t, size_t>> steps{{0, 2}, {2, 4}};
    // assert
    ASSERT_DOUBLE_EQ(scalar<double>(eval("x@1", {{"x", expression_value(7.0)}}, {}, {}, steps)), 7.0);
    const auto complex_scalar = scalar<std::complex<double>>(eval("j@1", {}, {}, {}, steps));
    ASSERT_DOUBLE_EQ(complex_scalar.real(), 0.0);
    ASSERT_DOUBLE_EQ(complex_scalar.imag(), 1.0);
    const std::unordered_map<std::string, XyceValue> cv{
        {"cv", expression_value(std::vector<std::complex<double>>{{1.0, 1.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0}})},
    };
    ASSERT_EQ(as_real_vector(eval("cv@1", cv, {}, {}, steps)), std::vector<double>({1.0, 2.0}));
}

TEST(XyceLanguageEvaluator, unknown_identifier_and_user_constants) {
    // assert
    ASSERT_THROW(eval("no_such_identifier"), std::invalid_argument);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("myval", {}, {}, {{"myval", expression_value(42.0)}})), 42.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("MYVAL", {}, {}, {{"myval", expression_value(42.0)}})), 42.0);
}
