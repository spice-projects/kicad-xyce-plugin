#include <cmath>
#include <complex>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "xyce_evaluator.h"
#include "xyce_parser.h"

namespace
{
    // match the parser node hierarchy without exposing ownership details in each test
    template <typename T>
    const T* as(const ExpressionPtr& expression)
    {
        return dynamic_cast<const T*>(expression.get());
    }

    // build an identifier node for parser and evaluator tests
    ExpressionPtr make_identifier(const std::string& name)
    {
        return std::make_unique<IdentifierNode>(name);
    }

    // build a number node for parser and evaluator tests
    ExpressionPtr make_number(const std::string& text)
    {
        return std::make_unique<NumberNode>(text);
    }

    // keep scalar values explicit in the assertions
    XyceValue expression_value(double value)
    {
        return value;
    }

    // keep vector values explicit in the assertions
    XyceValue expression_value(const std::vector<double>& values)
    {
        return values;
    }

    // keep complex values explicit in the assertions
    XyceValue expression_value(const std::complex<double>& value)
    {
        return value;
    }

    // read the first scalar value from a variant for compact checks
    template <typename T>
    T scalar(const XyceValue& value)
    {
        return std::visit([](const auto& inner) -> T {
            using Inner = std::decay_t<decltype(inner)>;
            if constexpr (std::is_same_v<Inner, double>) {
                return static_cast<T>(inner);
            }
            else if constexpr (std::is_same_v<Inner, std::complex<double>>) {
                return static_cast<T>(inner.real());
            }
            else {
                if constexpr (std::is_same_v<typename Inner::value_type, double>) {
                    return static_cast<T>(inner.front());
                }
                else {
                    return static_cast<T>(inner.front().real());
                }
            }
        }, value);
    }

    // normalize any value to a real vector for comparisons
    std::vector<double> as_real_vector(const XyceValue& value)
    {
        return std::visit([](const auto& inner) -> std::vector<double> {
            using Inner = std::decay_t<decltype(inner)>;
            if constexpr (std::is_same_v<Inner, double>) {
                return {inner};
            }
            else if constexpr (std::is_same_v<Inner, std::complex<double>>) {
                return {inner.real()};
            }
            else if constexpr (std::is_same_v<typename Inner::value_type, double>) {
                return std::vector<double>(inner.begin(), inner.end());
            }
            else {
                std::vector<double> out;
                out.reserve(inner.size());
                for (const auto& element : inner) {
                    out.push_back(element.real());
                }
                return out;
            }
        }, value);
    }

    // parse and evaluate a single expression in one step
    XyceValue eval(const std::string& text, const std::unordered_map<std::string, XyceValue>& expressions = {}, const std::unordered_map<std::string, FunctionDefinitionNode>& functions = {}, const std::unordered_map<std::string, XyceValue>& constants = {}, const std::optional<std::vector<std::pair<size_t, size_t>>>& step_slices = std::nullopt)
    {
        return evaluate_expression(*parse_expression(text), expressions, functions, constants, step_slices);
    }
} // namespace

// ========================================================================================
// lexer
// ========================================================================================

TEST(XyceLanguageLexer, tokenizes_core_symbols)
{
    // arrange
    // act
    const auto tokens = tokenize(".func x() {1+2-3*4/5%6^~!&|&&||==!=<=>=@:?,}");
    // assert
    ASSERT_EQ(tokens.front().kind, TokenKind::DIRECTIVE);
    ASSERT_EQ(tokens.back().kind, TokenKind::EOF_TOKEN);
    ASSERT_EQ(tokens[1].kind, TokenKind::IDENTIFIER);
    ASSERT_EQ(tokens[2].kind, TokenKind::LPAREN);
}

TEST(XyceLanguageLexer, tokenizes_numbers_identifiers_and_suffixes)
{
    // arrange
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

TEST(XyceLanguageLexer, ignores_whitespace_and_tracks_offsets)
{
    // arrange
    // act
    const auto tokens = tokenize("  1  +  2  ");
    // assert
    ASSERT_EQ(tokens.size(), 4U);
    ASSERT_EQ(tokens[0].start, 2U);
    ASSERT_EQ(tokens[1].start, 5U);
    ASSERT_EQ(tokens[2].start, 8U);
}

TEST(XyceLanguageLexer, rejects_invalid_characters)
{
    // arrange
    const auto tokenize_text = [](const std::string& text) { return tokenize(text); };
    // act
    const auto invalid_character = std::string("$");
    // assert
    ASSERT_THROW(tokenize_text(invalid_character), std::invalid_argument);
    ASSERT_THROW(tokenize_text("\u2022foo"), std::invalid_argument);
    ASSERT_THROW(tokenize_text("foo#bar"), std::invalid_argument);
}

TEST(XyceLanguageLexer, lexer_instance_matches_free_function)
{
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

TEST(XyceLanguageParser, parses_operators_precedence_and_probes)
{
    // arrange
    // act
    const auto tree = parse_expression("a + b * c ? V(out, 0)@1 : id(x)");
    // assert
    ASSERT_NE(as<TernaryOperationNode>(tree), nullptr);
}

TEST(XyceLanguageParser, parses_function_definitions)
{
    // arrange
    // act
    const auto tree = parse_function_definition(".func add(a, b) {a + b}");
    // assert
    ASSERT_EQ(tree.name, "add");
    ASSERT_EQ(tree.params.size(), 2U);
    ASSERT_TRUE(dynamic_cast<BinaryOperationNode*>(tree.body.get()) != nullptr);
}

TEST(XyceLanguageParser, parser_nodes_store_expected_values)
{
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

TEST(XyceLanguageParser, parse_errors_match_scenarios)
{
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

TEST(XyceLanguageBuiltins, constants_match_expected_values)
{
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

TEST(XyceLanguageBuiltins, core_builtin_functions_work)
{
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

TEST(XyceLanguageBuiltins, builtin_function_errors_match_scenarios)
{
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

TEST(XyceLanguageEvaluator, arithmetic_logical_relational_and_ternary_work)
{
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

TEST(XyceLanguageEvaluator, variables_constants_suffixes_and_builtin_calls_work)
{
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

TEST(XyceLanguageEvaluator, probes_and_steps_work)
{
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

TEST(XyceLanguageEvaluator, arrays_and_edge_cases_work)
{
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

TEST(XyceLanguageEvaluator, user_functions_and_errors_work)
{
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

TEST(XyceLanguageEvaluator, node_and_context_types_match)
{
    // arrange
    // act
    EvaluationContext context{{}, {{"x", expression_value(1.0)}}, {}, {}, std::nullopt};
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
    ASSERT_TRUE(context.step_slices == std::nullopt);
    ASSERT_EQ(number.text, "3.14");
    ASSERT_EQ(identifier.name, "vout");
    ASSERT_EQ(call.args.size(), 1U);
    ASSERT_EQ(binary.operator_value, BinaryOperator::ADD);
    ASSERT_EQ(unary.operator_value, UnaryOperator::NEG);
    ASSERT_TRUE(as<IdentifierNode>(ternary.condition) != nullptr);
    ASSERT_EQ(step.step_index, 1U);
}