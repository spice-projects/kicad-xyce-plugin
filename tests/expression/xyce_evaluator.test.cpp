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

// ========================================================================================
// evaluation context
// ========================================================================================

TEST(XyceEvaluatorChecks, context_default_constructs_empty) {
    // arrange / act
    EvaluationContext context{};
    // assert
    ASSERT_TRUE(context.expressions.empty());
    ASSERT_TRUE(context.variables.empty());
    ASSERT_TRUE(context.functions.empty());
    ASSERT_TRUE(context.constants.empty());
    ASSERT_TRUE(context.step_slices.empty());
}

TEST(XyceEvaluatorChecks, context_stores_expressions_by_pointer) {
    // arrange
    const XyceValue value = expression_value(1.0);
    // act
    const EvaluationContext context{{{"x", &value}}, {}, {}, {}, {}};
    // assert
    ASSERT_EQ(context.expressions.size(), 1U);
    ASSERT_EQ(context.expressions.at("x"), &value);
}

TEST(XyceEvaluatorChecks, context_stores_variables_by_value) {
    // arrange / act
    const EvaluationContext context{{}, {{"x", expression_value(1.0)}}, {}, {}, {}};
    // assert
    ASSERT_EQ(context.variables.size(), 1U);
    ASSERT_DOUBLE_EQ(scalar<double>(context.variables.at("x")), 1.0);
}

TEST(XyceEvaluatorChecks, context_stores_functions_by_pointer) {
    // arrange
    const auto square = parse_function_definition(".func sq(x) {x * x}");
    // act
    const EvaluationContext context{{}, {}, {{"sq", &square}}, {}, {}};
    // assert
    ASSERT_EQ(context.functions.size(), 1U);
    ASSERT_EQ(context.functions.at("sq"), &square);
    ASSERT_EQ(context.functions.at("sq")->params.size(), 1U);
}

TEST(XyceEvaluatorChecks, context_stores_constants_by_pointer) {
    // arrange
    const XyceValue value = expression_value(42.0);
    // act
    const EvaluationContext context{{}, {}, {}, {{"answer", &value}}, {}};
    // assert
    ASSERT_EQ(context.constants.size(), 1U);
    ASSERT_EQ(context.constants.at("answer"), &value);
}

TEST(XyceEvaluatorChecks, context_stores_step_slices) {
    // arrange
    const std::vector<std::pair<size_t, size_t>> slices{{0, 3}, {3, 6}};
    // act
    const EvaluationContext context{{}, {}, {}, {}, slices};
    // assert
    ASSERT_EQ(context.step_slices, slices);
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

struct UnknownExpressionNode final : ExpressionNode
{
};

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

TEST(XyceLanguageEvaluator, loader_resolves_then_caches) {
    // arrange
    std::unordered_map<std::string, XyceValue> expressions;
    int load_calls = 0;
    auto loader = [&load_calls](const std::string& key) -> std::optional<XyceValue> {
        ++load_calls;
        if (key == "x")
            return expression_value(7.0);
        return std::nullopt;
    };
    // act: first evaluation loads and caches the identifier
    const auto first = scalar<double>(evaluate_expression(*parse_expression("x"), expressions, loader, {}, {}, {}));
    // act: second evaluation must reuse the cache
    const auto second = scalar<double>(evaluate_expression(*parse_expression("x"), expressions, loader, {}, {}, {}));
    // assert
    ASSERT_DOUBLE_EQ(first, 7.0);
    ASSERT_DOUBLE_EQ(second, 7.0);
    ASSERT_EQ(load_calls, 1);
    ASSERT_EQ(expressions.size(), 1U);
}

TEST(XyceLanguageEvaluator, loader_returns_nullopt_for_unknown_identifier) {
    // arrange
    std::unordered_map<std::string, XyceValue> expressions;
    auto loader = [](const std::string&) -> std::optional<XyceValue> { return std::nullopt; };
    // assert
    ASSERT_THROW(evaluate_expression(*parse_expression("missing"), expressions, loader, {}, {}, {}), std::invalid_argument);
}

TEST(XyceLanguageEvaluator, loader_nullopt_unknown_probe) {
    // arrange
    std::unordered_map<std::string, XyceValue> expressions;
    auto loader = [](const std::string&) -> std::optional<XyceValue> { return std::nullopt; };
    // assert
    ASSERT_THROW(evaluate_expression(*parse_expression("V(out)"), expressions, loader, {}, {}, {}), std::invalid_argument);
}

TEST(XyceLanguageEvaluator, loader_lazy_loads_probe_values) {
    // arrange
    std::unordered_map<std::string, XyceValue> expressions;
    auto loader = [](const std::string& key) -> std::optional<XyceValue> {
        if (key == "v(out)")
            return expression_value(3.3);
        return std::nullopt;
    };
    // act
    const auto result = scalar<double>(evaluate_expression(*parse_expression("V(out)"), expressions, loader, {}, {}, {}));
    // assert
    ASSERT_DOUBLE_EQ(result, 3.3);
    ASSERT_EQ(expressions.size(), 1U);
    ASSERT_TRUE(expressions.contains("v(out)"));
}

TEST(XyceLanguageEvaluator, find_variable_recurses_into_parent_scope) {
    // arrange
    const auto g = parse_function_definition(".func g(y) {x}");
    const auto f = parse_function_definition(".func f(x) {g(x)}");
    const std::unordered_map<std::string, FunctionDefinitionNode> functions{{"f", f}, {"g", g}};
    // act / assert: g's body reads x from the caller f's local scope
    ASSERT_DOUBLE_EQ(scalar<double>(eval("f(5)", {}, functions)), 5.0);
}

TEST(XyceLanguageEvaluator, mutual_recursion_throws_via_ancestor_stack) {
    // arrange
    const auto g = parse_function_definition(".func g(x) {f(x)}");
    const auto f = parse_function_definition(".func f(x) {g(x)}");
    const std::unordered_map<std::string, FunctionDefinitionNode> functions{{"f", f}, {"g", g}};
    // assert
    ASSERT_THROW(eval("g(1)", {}, functions), std::invalid_argument);
}

TEST(XyceLanguageEvaluator, mixed_scalar_vector_logical_and_or) {
    // arrange
    const std::unordered_map<std::string, XyceValue> expressions{
        {"x", expression_value(std::vector<double>{0.0, 1.0})},
    };
    // assert
    ASSERT_EQ(as_real_vector(eval("1 && x", expressions)), std::vector<double>({0.0, 1.0}));
    ASSERT_EQ(as_real_vector(eval("x && 1", expressions)), std::vector<double>({0.0, 1.0}));
    ASSERT_EQ(as_real_vector(eval("0 || x", expressions)), std::vector<double>({0.0, 1.0}));
    ASSERT_EQ(as_real_vector(eval("x || 0", expressions)), std::vector<double>({0.0, 1.0}));
    // scalar-true right operand short-circuits OR to scalar 1.0
    ASSERT_DOUBLE_EQ(scalar<double>(eval("x || 1", expressions)), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("0 && x", expressions)), 0.0);
}

TEST(XyceLanguageEvaluator, differential_probe_only_second_node_throws) {
    // arrange
    const std::unordered_map<std::string, XyceValue> expressions{
        {"v(b)", expression_value(5.0)},
    };
    // assert: first node missing means the differential cannot resolve
    ASSERT_THROW(eval("V(a,b)", expressions), std::invalid_argument);
}

TEST(XyceLanguageEvaluator, multi_arg_probe_key_direct_hit) {
    // arrange
    const std::unordered_map<std::string, XyceValue> expressions{
        {"v(a, b)", expression_value(9.0)},
    };
    // assert
    ASSERT_DOUBLE_EQ(scalar<double>(eval("V(a,b)", expressions)), 9.0);
}

TEST(XyceLanguageEvaluator, mixed_complex_scalar_real_vector_broadcast) {
    // arrange
    const std::unordered_map<std::string, XyceValue> expressions{
        {"x", expression_value(std::vector<double>{1.0, 2.0})},
    };
    // act
    const auto sum = to_complex_vector(eval("j + x", expressions));
    // assert
    ASSERT_NE(sum, nullptr);
    ASSERT_EQ(sum->size(), 2U);
    EXPECT_DOUBLE_EQ((*sum)[0].real(), 1.0);
    EXPECT_DOUBLE_EQ((*sum)[0].imag(), 1.0);
    EXPECT_DOUBLE_EQ((*sum)[1].real(), 2.0);
    EXPECT_DOUBLE_EQ((*sum)[1].imag(), 1.0);
    // real vector + complex vector ADD
    const std::unordered_map<std::string, XyceValue> expressions2{
        {"x", expression_value(std::vector<double>{1.0, 2.0})},
        {"cv", expression_value(std::vector<std::complex<double>>{{1.0, 1.0}, {0.0, 1.0}})},
    };
    const auto sum2 = to_complex_vector(eval("x + cv", expressions2));
    ASSERT_EQ(sum2->size(), 2U);
    EXPECT_DOUBLE_EQ((*sum2)[0].real(), 2.0);
    EXPECT_DOUBLE_EQ((*sum2)[0].imag(), 1.0);
    EXPECT_DOUBLE_EQ((*sum2)[1].real(), 2.0);
    EXPECT_DOUBLE_EQ((*sum2)[1].imag(), 1.0);
}

TEST(XyceLanguageEvaluator, vector_relational_and_equality_elementwise) {
    // arrange
    const std::unordered_map<std::string, XyceValue> expressions{
        {"x", expression_value(std::vector<double>{1.0, 2.0, 5.0})},
    };
    // assert
    ASSERT_EQ(as_real_vector(eval("x < 3", expressions)), std::vector<double>({1.0, 1.0, 0.0}));
    ASSERT_EQ(as_real_vector(eval("x == 2", expressions)), std::vector<double>({0.0, 1.0, 0.0}));
    ASSERT_EQ(as_real_vector(eval("x != 2", expressions)), std::vector<double>({1.0, 0.0, 1.0}));
}

TEST(XyceLanguageEvaluator, relational_complex_warn_disabled_at_err_level) {
    // arrange
    const auto previous_level = spdlog::get_level();
    spdlog::set_level(spdlog::level::err);
    // act / assert: same real-part result as at warn level, no warning emitted
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j < 1")), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j > 1")), 0.0);
    // restore
    spdlog::set_level(previous_level);
}

TEST(XyceLanguageEvaluator, minor_operator_and_selector_edges) {
    // arrange
    const std::unordered_map<std::string, XyceValue> expressions{
        {"x", expression_value(std::vector<double>{1.0, 2.0})},
        {"a", expression_value(std::vector<double>{10.0, 20.0})},
        {"b", expression_value(std::vector<double>{30.0, 40.0})},
        {"cond", expression_value(std::vector<double>{1.0, 0.0})},
    };
    const std::vector<std::pair<size_t, size_t>> steps{{0, 1}, {1, 2}};
    // assert: unary POS on a vector
    ASSERT_EQ(as_real_vector(eval("+x", expressions)), std::vector<double>({1.0, 2.0}));
    // assert: complex MOD uses the real part
    ASSERT_DOUBLE_EQ(scalar<double>(eval("j % 3")), 0.0);
    // assert: ternary with vector condition and vector branches
    ASSERT_EQ(as_real_vector(eval("cond ? a : b", expressions)), std::vector<double>({10.0, 40.0}));
    // assert: empty probe args are rejected as an unknown function
    ASSERT_THROW(eval("V()"), std::invalid_argument);
    // assert: out-of-range step selector
    ASSERT_THROW(eval("x@0", expressions, {}, {}, steps), std::invalid_argument);
    ASSERT_THROW(eval("x@5", expressions, {}, {}, steps), std::invalid_argument);
}
