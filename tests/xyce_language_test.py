from __future__ import annotations

import math

import numpy as np
import pytest

from xyce_language import parse_expression, parse_function_definition, tokenize, TokenKind, XyceEvaluator, XyceLexer
from xyce_language.builtins import BUILTIN_CONSTANTS, BUILTIN_FUNCTIONS
from xyce_language.evaluator import EvaluationContext, _NUMBER_SUFFIXES
from xyce_language.nodes import BinaryOperationNode, BinaryOperator, FunctionCallNode, FunctionDefinitionNode, IdentifierNode, NumberNode, StepSelectorNode, TernaryOperationNode, UnaryOperationNode, UnaryOperator


def _evaluate(text, variables=None, functions=None, constants=None, step_slices=None):
    # parse the expression text and evaluate it with the given context
    tree = parse_expression(text)
    return XyceEvaluator().evaluate(tree, variables, functions, constants, step_slices)


def _wrap(x):
    # wrap a scalar in a single-element list for the builtin calling convention
    return [np.asarray(x)]


def _wrap2(x, y):
    # wrap two scalars in a list for two-argument builtins
    return [np.asarray(x), np.asarray(y)]


def _wrap3(x, y, z):
    # wrap three scalars in a list for three-argument builtins
    return [np.asarray(x), np.asarray(y), np.asarray(z)]


# ---------------------------------------------------------------------------
# TestXyceLexer
# ---------------------------------------------------------------------------


class TestXyceLexer:

    def test_empty_string_produces_eof(self):
        # arrange
        text = ""
        # act
        tokens = tokenize(text)
        # assert
        assert len(tokens) == 1
        assert tokens[0].kind == TokenKind.EOF

    def test_integer_literal(self):
        # arrange
        text = "42"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.NUMBER
        assert tokens[0].text == "42"

    def test_decimal_literal(self):
        # arrange
        text = "3.14"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.NUMBER
        assert tokens[0].text == "3.14"

    def test_scientific_notation(self):
        # arrange
        text = "1e6"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.NUMBER
        assert tokens[0].text == "1e6"

    def test_scientific_notation_negative_exponent(self):
        # arrange
        text = "2.5e-3"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.NUMBER
        assert tokens[0].text == "2.5e-3"

    def test_number_suffix_leaves_identifier_separate(self):
        # arrange — the lexer emits the suffix as a separate IDENTIFIER token
        text = "1MEG"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.NUMBER
        assert tokens[0].text == "1"
        assert tokens[1].kind == TokenKind.IDENTIFIER
        assert tokens[1].text == "MEG"

    def test_plain_identifier(self):
        # arrange
        text = "vout"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.IDENTIFIER
        assert tokens[0].text == "vout"

    def test_identifier_with_underscore(self):
        # arrange
        text = "v_out"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.IDENTIFIER
        assert tokens[0].text == "v_out"

    def test_identifier_starting_with_underscore(self):
        # arrange
        text = "_foo"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.IDENTIFIER
        assert tokens[0].text == "_foo"

    def test_identifier_with_square_brackets(self):
        # arrange — Xyce allows bracket notation in node names
        text = "node[1]"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.IDENTIFIER
        assert tokens[0].text == "node[1]"

    def test_func_directive(self):
        # arrange
        text = ".func"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.DIRECTIVE
        assert tokens[0].text == ".func"

    def test_plus_token(self):
        # arrange
        text = "+"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.PLUS

    def test_minus_token(self):
        # arrange
        text = "-"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.MINUS

    def test_star_token(self):
        # arrange
        text = "*"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.STAR

    def test_slash_token(self):
        # arrange
        text = "/"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.SLASH

    def test_percent_token(self):
        # arrange — % is the modulo operator in Xyce
        text = "%"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.PERCENT

    def test_caret_token(self):
        # arrange — ^ is XOR in Xyce, not power
        text = "^"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.CARET

    def test_tilde_token(self):
        # arrange — ~ is logical NOT in Xyce
        text = "~"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.TILDE

    def test_bang_token(self):
        # arrange — ! is an alternate logical NOT
        text = "!"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.BANG

    def test_ampersand_token(self):
        # arrange — single & is logical AND in Xyce
        text = "&"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.AMPERSAND

    def test_pipe_token(self):
        # arrange — single | is logical OR in Xyce
        text = "|"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.PIPE

    def test_double_ampersand_token(self):
        # arrange — && is a compatibility alias for AND
        text = "&&"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.LOGICAL_AND

    def test_double_pipe_token(self):
        # arrange — || is a compatibility alias for OR
        text = "||"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.LOGICAL_OR

    def test_double_star_token(self):
        # arrange — ** is the power operator in Xyce
        text = "**"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.POWER

    def test_equal_equal_token(self):
        # arrange
        text = "=="
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.EQUAL_EQUAL

    def test_bang_equal_token(self):
        # arrange
        text = "!="
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.BANG_EQUAL

    def test_less_token(self):
        # arrange
        text = "<"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.LESS

    def test_less_equal_token(self):
        # arrange
        text = "<="
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.LESS_EQUAL

    def test_greater_token(self):
        # arrange
        text = ">"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.GREATER

    def test_greater_equal_token(self):
        # arrange
        text = ">="
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.GREATER_EQUAL

    def test_question_token(self):
        # arrange
        text = "?"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.QUESTION

    def test_colon_token(self):
        # arrange
        text = ":"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.COLON

    def test_at_token(self):
        # arrange
        text = "@"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.AT

    def test_lparen_token(self):
        # arrange
        text = "("
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.LPAREN

    def test_rparen_token(self):
        # arrange
        text = ")"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.RPAREN

    def test_lbrace_token(self):
        # arrange
        text = "{"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.LBRACE

    def test_rbrace_token(self):
        # arrange
        text = "}"
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.RBRACE

    def test_comma_token(self):
        # arrange
        text = ","
        # act
        tokens = tokenize(text)
        # assert
        assert tokens[0].kind == TokenKind.COMMA

    def test_whitespace_is_skipped(self):
        # arrange
        text = "  1  +  2  "
        # act
        tokens = tokenize(text)
        # assert — only number, plus, number, eof are emitted
        kinds = [t.kind for t in tokens]
        assert kinds == [TokenKind.NUMBER, TokenKind.PLUS, TokenKind.NUMBER, TokenKind.EOF]

    def test_token_start_offset(self):
        # arrange
        text = "1 + 2"
        # act
        tokens = tokenize(text)
        # assert — first token starts at position 0
        assert tokens[0].start == 0
        assert tokens[0].end == 1

    def test_token_mid_offset(self):
        # arrange
        text = "1 + 2"
        # act
        tokens = tokenize(text)
        # assert — plus token starts at position 2
        assert tokens[1].start == 2

    def test_token_end_offset(self):
        # arrange
        text = "1 + 2"
        # act
        tokens = tokenize(text)
        # assert — last number starts at position 4
        assert tokens[2].start == 4

    def test_unknown_character_raises(self):
        # arrange
        text = "$"
        # act / assert
        with pytest.raises(ValueError):
            tokenize(text)

    def test_lexer_instance_produces_same_result_as_module_function(self):
        # arrange
        text = "1 + x"
        # act
        result_a = XyceLexer().tokenize(text)
        result_b = tokenize(text)
        # assert
        assert result_a == result_b

    def test_bullet_character_is_rejected(self):
        # arrange — QSPICE bullet must not be accepted as an identifier start in Xyce
        text = "\u2022foo"
        # act / assert
        with pytest.raises(ValueError):
            tokenize(text)

    def test_hash_inside_identifier_is_rejected(self):
        # arrange — QSPICE hash must not be accepted inside identifiers in Xyce
        text = "foo#bar"
        # act / assert
        with pytest.raises(ValueError):
            tokenize(text)


# ---------------------------------------------------------------------------
# TestXyceParser
# ---------------------------------------------------------------------------


class TestXyceParser:

    def test_number_literal_produces_number_node(self):
        # arrange
        text = "42"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, NumberNode)
        assert tree.text == "42"

    def test_identifier_produces_identifier_node(self):
        # arrange
        text = "vout"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, IdentifierNode)
        assert tree.name == "vout"

    def test_addition(self):
        # arrange
        text = "a + b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.ADD

    def test_subtraction(self):
        # arrange
        text = "a - b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.SUB

    def test_multiplication(self):
        # arrange
        text = "a * b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.MUL

    def test_division(self):
        # arrange
        text = "a / b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.DIV

    def test_modulo(self):
        # arrange
        text = "a % b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.MOD

    def test_power(self):
        # arrange — ** is the power operator in Xyce
        text = "a ** b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.POW

    def test_caret_is_xor_not_power(self):
        # arrange — ^ is XOR in Xyce, not power
        text = "a ^ b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.LOGICAL_XOR

    def test_pipe_is_logical_or(self):
        # arrange — single | is logical OR
        text = "a | b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.LOGICAL_OR

    def test_double_pipe_is_logical_or(self):
        # arrange — || is also logical OR (compatibility form)
        text = "a || b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.LOGICAL_OR

    def test_ampersand_is_logical_and(self):
        # arrange — single & is logical AND
        text = "a & b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.LOGICAL_AND

    def test_double_ampersand_is_logical_and(self):
        # arrange — && is also logical AND (compatibility form)
        text = "a && b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.LOGICAL_AND

    def test_equal_equal(self):
        # arrange
        text = "a == b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.EQUAL

    def test_bang_equal(self):
        # arrange
        text = "a != b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.NOT_EQUAL

    def test_less(self):
        # arrange
        text = "a < b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.LESS

    def test_less_equal(self):
        # arrange
        text = "a <= b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.LESS_EQUAL

    def test_greater(self):
        # arrange
        text = "a > b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.GREATER

    def test_greater_equal(self):
        # arrange
        text = "a >= b"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.GREATER_EQUAL

    def test_unary_minus(self):
        # arrange
        text = "-x"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, UnaryOperationNode)
        assert tree.operator == UnaryOperator.NEG

    def test_unary_plus(self):
        # arrange
        text = "+x"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, UnaryOperationNode)
        assert tree.operator == UnaryOperator.POS

    def test_tilde_is_logical_not(self):
        # arrange — ~ is the logical NOT operator in Xyce
        text = "~x"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, UnaryOperationNode)
        assert tree.operator == UnaryOperator.NOT

    def test_bang_is_logical_not(self):
        # arrange — ! is also logical NOT (compatibility form)
        text = "!x"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, UnaryOperationNode)
        assert tree.operator == UnaryOperator.NOT

    def test_ternary(self):
        # arrange
        text = "a ? b : c"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, TernaryOperationNode)

    def test_ternary_is_right_associative(self):
        # arrange — nested ternary must parse right-associatively
        text = "a ? b ? c : d : e"
        # act
        tree = parse_expression(text)
        # assert — outer condition is 'a'; the true-branch is itself a ternary
        assert isinstance(tree, TernaryOperationNode)
        assert isinstance(tree.if_true, TernaryOperationNode)

    def test_parentheses_override_precedence(self):
        # arrange
        text = "(a + b) * c"
        # act
        tree = parse_expression(text)
        # assert — top-level is MUL; left child is ADD
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.MUL
        assert isinstance(tree.left, BinaryOperationNode)
        assert tree.left.operator == BinaryOperator.ADD

    def test_function_call_no_args(self):
        # arrange
        text = "f()"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, FunctionCallNode)
        assert tree.name == "f"
        assert len(tree.args) == 0

    def test_function_call_args_is_list(self):
        # arrange
        text = "abs(x)"
        # act
        tree = parse_expression(text)
        # assert — args is always a list, not a tuple
        assert isinstance(tree.args, list)

    def test_function_call_one_arg(self):
        # arrange
        text = "abs(x)"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, FunctionCallNode)
        assert len(tree.args) == 1

    def test_function_call_two_args(self):
        # arrange
        text = "atan2(y, x)"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, FunctionCallNode)
        assert len(tree.args) == 2

    def test_probe_v_single_arg(self):
        # arrange
        text = "V(out)"
        # act
        tree = parse_expression(text)
        # assert — probe argument is an IdentifierNode with the node name
        assert isinstance(tree, FunctionCallNode)
        assert tree.name == "V"
        assert isinstance(tree.args[0], IdentifierNode)
        assert tree.args[0].name == "out"

    def test_probe_v_differential(self):
        # arrange
        text = "V(vp, vn)"
        # act
        tree = parse_expression(text)
        # assert — differential probe has two node-name arguments
        assert isinstance(tree, FunctionCallNode)
        assert len(tree.args) == 2
        assert tree.args[0].name == "vp"
        assert tree.args[1].name == "vn"

    def test_probe_v_numeric_ground_node(self):
        # arrange — V(out, 0) where 0 is the numeric ground node
        text = "V(out, 0)"
        # act
        tree = parse_expression(text)
        # assert — ground is represented as IdentifierNode("0")
        assert isinstance(tree, FunctionCallNode)
        assert isinstance(tree.args[1], IdentifierNode)
        assert tree.args[1].name == "0"

    def test_probe_i(self):
        # arrange
        text = "I(r1)"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, FunctionCallNode)
        assert tree.name == "I"

    def test_probe_v_hyphenated_node_name(self):
        # arrange — KiCad auto-generated net names contain hyphens
        text = "V(net-_u304a-g2_)"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, FunctionCallNode)
        assert tree.args[0].name == "net-_u304a-g2_"

    def test_step_selector_on_probe(self):
        # arrange
        text = "V(out)@1"
        # act
        tree = parse_expression(text)
        # assert — StepSelectorNode wraps the probe with step index 1
        assert isinstance(tree, StepSelectorNode)
        assert tree.step_index == 1
        assert isinstance(tree.base, FunctionCallNode)

    def test_step_selector_on_identifier(self):
        # arrange
        text = "x@2"
        # act
        tree = parse_expression(text)
        # assert
        assert isinstance(tree, StepSelectorNode)
        assert tree.step_index == 2

    def test_step_selector_zero_raises(self):
        # arrange — step indices are 1-based; 0 is invalid
        text = "x@0"
        # act / assert
        with pytest.raises(ValueError, match="invalid"):
            parse_expression(text)

    def test_func_definition_no_params(self):
        # arrange
        text = ".func f() {42}"
        # act
        tree = parse_function_definition(text)
        # assert
        assert isinstance(tree, FunctionDefinitionNode)
        assert tree.name == "f"
        assert len(tree.params) == 0

    def test_func_definition_one_param(self):
        # arrange
        text = ".func sq(x) {x * x}"
        # act
        tree = parse_function_definition(text)
        # assert
        assert isinstance(tree, FunctionDefinitionNode)
        assert len(tree.params) == 1
        assert tree.params[0] == "x"

    def test_func_definition_two_params(self):
        # arrange
        text = ".func add(a, b) {a + b}"
        # act
        tree = parse_function_definition(text)
        # assert
        assert tree.params[0] == "a"
        assert tree.params[1] == "b"

    def test_wrong_directive_raises(self):
        # arrange — only .func is a valid function definition directive
        text = ".param x = 1"
        # act / assert
        with pytest.raises(ValueError):
            parse_function_definition(text)

    def test_dangling_unary_raises(self):
        # arrange — a bare unary operator with no operand is invalid
        text = "+"
        # act / assert
        with pytest.raises(ValueError):
            parse_expression(text)

    def test_precedence_mul_over_add(self):
        # arrange
        text = "a + b * c"
        # act
        tree = parse_expression(text)
        # assert — top is ADD; right child is MUL
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.ADD
        assert isinstance(tree.right, BinaryOperationNode)
        assert tree.right.operator == BinaryOperator.MUL

    def test_precedence_or_less_than_xor_less_than_and(self):
        # arrange — standard C bit-logic precedence: | < ^ < &
        text = "a | b ^ c & d"
        # act
        tree = parse_expression(text)
        # assert — top is OR; right of OR is XOR; right of XOR is AND
        assert tree.operator == BinaryOperator.LOGICAL_OR
        assert tree.right.operator == BinaryOperator.LOGICAL_XOR
        assert tree.right.right.operator == BinaryOperator.LOGICAL_AND

    def test_precedence_relational_over_logical(self):
        # arrange
        text = "a < b & c > d"
        # act
        tree = parse_expression(text)
        # assert — AND is the top-level; both children are comparisons
        assert tree.operator == BinaryOperator.LOGICAL_AND
        assert tree.left.operator == BinaryOperator.LESS
        assert tree.right.operator == BinaryOperator.GREATER

    def test_power_is_right_associative(self):
        # arrange
        text = "a ** b ** c"
        # act
        tree = parse_expression(text)
        # assert — a ** (b ** c), so right child is also POW
        assert isinstance(tree, BinaryOperationNode)
        assert tree.operator == BinaryOperator.POW
        assert isinstance(tree.right, BinaryOperationNode)
        assert tree.right.operator == BinaryOperator.POW

    def test_id_is_not_a_probe_family(self):
        # arrange — 'id' was a QSPICE probe family; it is not in Xyce
        text = "id(x)"
        # act
        tree = parse_expression(text)
        # assert — id(x) is treated as a regular function call, not a probe
        assert isinstance(tree, FunctionCallNode)
        assert isinstance(tree.args[0], IdentifierNode)


# ---------------------------------------------------------------------------
# TestXyceBuiltins
# ---------------------------------------------------------------------------


class TestXyceBuiltins:

    # --- log is log10 in Xyce (critical semantic difference from QSPICE) ---

    def test_log_is_log10(self):
        # arrange
        args = _wrap(100.0)
        # act
        result = BUILTIN_FUNCTIONS["log"](args)
        # assert
        assert abs(float(result) - 2.0) < 1e-12

    def test_log10_is_log10(self):
        # arrange
        args = _wrap(1000.0)
        # act
        result = BUILTIN_FUNCTIONS["log10"](args)
        # assert
        assert abs(float(result) - 3.0) < 1e-12

    def test_ln_is_natural_log(self):
        # arrange — LN(x) must return the natural logarithm, not log10
        args = _wrap(math.e)
        # act
        result = BUILTIN_FUNCTIONS["ln"](args)
        # assert
        assert abs(float(result) - 1.0) < 1e-12

    # --- abs ---

    def test_abs_positive(self):
        # arrange
        args = _wrap(3.0)
        # act
        result = BUILTIN_FUNCTIONS["abs"](args)
        # assert
        assert float(result) == 3.0

    def test_abs_negative(self):
        # arrange
        args = _wrap(-3.0)
        # act
        result = BUILTIN_FUNCTIONS["abs"](args)
        # assert
        assert float(result) == 3.0

    def test_abs_wrong_arity_raises(self):
        # arrange
        args = _wrap2(1.0, 2.0)
        # act / assert
        with pytest.raises(ValueError):
            BUILTIN_FUNCTIONS["abs"](args)

    # --- sqrt ---

    def test_sqrt(self):
        # arrange
        args = _wrap(9.0)
        # act
        result = BUILTIN_FUNCTIONS["sqrt"](args)
        # assert
        assert abs(float(result) - 3.0) < 1e-12

    # --- db ---

    def test_db(self):
        # arrange — db(x) = 20 * log10(|x|)
        args = _wrap(10.0)
        # act
        result = BUILTIN_FUNCTIONS["db"](args)
        # assert
        assert abs(float(result) - 20.0) < 1e-10

    # --- real / re / r ---

    def test_real(self):
        # arrange
        args = _wrap(3.0 + 4.0j)
        # act
        result = BUILTIN_FUNCTIONS["real"](args)
        # assert
        assert float(result) == 3.0

    def test_re_alias(self):
        # arrange
        args = _wrap(2.0 + 5.0j)
        # act
        result = BUILTIN_FUNCTIONS["re"](args)
        # assert
        assert float(result) == 2.0

    def test_r_alias(self):
        # arrange
        args = _wrap(1.0 + 2.0j)
        # act
        result = BUILTIN_FUNCTIONS["r"](args)
        # assert
        assert float(result) == 1.0

    # --- imag / img ---

    def test_imag(self):
        # arrange
        args = _wrap(3.0 + 4.0j)
        # act
        result = BUILTIN_FUNCTIONS["imag"](args)
        # assert
        assert float(result) == 4.0

    def test_img_alias(self):
        # arrange
        args = _wrap(3.0 + 7.0j)
        # act
        result = BUILTIN_FUNCTIONS["img"](args)
        # assert
        assert float(result) == 7.0

    # --- angle / ph / phase ---

    def test_ph_zero_for_real_positive(self):
        # arrange
        args = _wrap(1.0 + 0.0j)
        # act
        result = BUILTIN_FUNCTIONS["ph"](args)
        # assert
        assert abs(float(result)) < 1e-12

    def test_phase_ninety_for_pure_imaginary(self):
        # arrange
        args = _wrap(0.0 + 1.0j)
        # act
        result = BUILTIN_FUNCTIONS["phase"](args)
        # assert
        assert abs(float(result) - 90.0) < 1e-10

    # --- mag / m ---

    def test_mag(self):
        # arrange — mag(3+4j) = 5
        args = _wrap(3.0 + 4.0j)
        # act
        result = BUILTIN_FUNCTIONS["mag"](args)
        # assert
        assert abs(float(result) - 5.0) < 1e-12

    def test_m_alias(self):
        # arrange
        args = _wrap(0.0 + 2.0j)
        # act
        result = BUILTIN_FUNCTIONS["m"](args)
        # assert
        assert abs(float(result) - 2.0) < 1e-12

    # --- sin / cos / tan ---

    def test_sin_zero(self):
        # arrange
        args = _wrap(0.0)
        # act
        result = BUILTIN_FUNCTIONS["sin"](args)
        # assert
        assert abs(float(result)) < 1e-12

    def test_cos_zero(self):
        # arrange
        args = _wrap(0.0)
        # act
        result = BUILTIN_FUNCTIONS["cos"](args)
        # assert
        assert abs(float(result) - 1.0) < 1e-12

    def test_tan_zero(self):
        # arrange
        args = _wrap(0.0)
        # act
        result = BUILTIN_FUNCTIONS["tan"](args)
        # assert
        assert abs(float(result)) < 1e-12

    # --- asin / acos / atan / arctan / atan2 ---

    def test_asin(self):
        # arrange
        args = _wrap(1.0)
        # act
        result = BUILTIN_FUNCTIONS["asin"](args)
        # assert
        assert abs(float(result) - math.pi / 2) < 1e-12

    def test_acos(self):
        # arrange
        args = _wrap(1.0)
        # act
        result = BUILTIN_FUNCTIONS["acos"](args)
        # assert
        assert abs(float(result)) < 1e-12

    def test_atan(self):
        # arrange
        args = _wrap(1.0)
        # act
        result = BUILTIN_FUNCTIONS["atan"](args)
        # assert
        assert abs(float(result) - math.pi / 4) < 1e-12

    def test_arctan_alias(self):
        # arrange
        args = _wrap(1.0)
        # act
        result = BUILTIN_FUNCTIONS["arctan"](args)
        # assert
        assert abs(float(result) - math.pi / 4) < 1e-12

    def test_atan2(self):
        # arrange
        args = _wrap2(1.0, 1.0)
        # act
        result = BUILTIN_FUNCTIONS["atan2"](args)
        # assert
        assert abs(float(result) - math.pi / 4) < 1e-12

    # --- sinh / cosh / tanh ---

    def test_sinh_zero(self):
        # arrange
        args = _wrap(0.0)
        # act
        result = BUILTIN_FUNCTIONS["sinh"](args)
        # assert
        assert abs(float(result)) < 1e-12

    def test_cosh_zero(self):
        # arrange
        args = _wrap(0.0)
        # act
        result = BUILTIN_FUNCTIONS["cosh"](args)
        # assert
        assert abs(float(result) - 1.0) < 1e-12

    def test_tanh_zero(self):
        # arrange
        args = _wrap(0.0)
        # act
        result = BUILTIN_FUNCTIONS["tanh"](args)
        # assert
        assert abs(float(result)) < 1e-12

    # --- asinh / acosh / atanh ---

    def test_asinh_zero(self):
        # arrange
        args = _wrap(0.0)
        # act
        result = BUILTIN_FUNCTIONS["asinh"](args)
        # assert
        assert abs(float(result)) < 1e-12

    def test_acosh_one(self):
        # arrange — acosh(1) = 0
        args = _wrap(1.0)
        # act
        result = BUILTIN_FUNCTIONS["acosh"](args)
        # assert
        assert abs(float(result)) < 1e-12

    def test_atanh_zero(self):
        # arrange
        args = _wrap(0.0)
        # act
        result = BUILTIN_FUNCTIONS["atanh"](args)
        # assert
        assert abs(float(result)) < 1e-12

    # --- exp ---

    def test_exp_one(self):
        # arrange
        args = _wrap(1.0)
        # act
        result = BUILTIN_FUNCTIONS["exp"](args)
        # assert
        assert abs(float(result) - math.e) < 1e-12

    # --- conj ---

    def test_conj(self):
        # arrange
        args = _wrap(2.0 + 3.0j)
        # act
        result = BUILTIN_FUNCTIONS["conj"](args)
        # assert
        assert abs(complex(result) - (2.0 - 3.0j)) < 1e-12

    # --- sqr ---

    def test_sqr(self):
        # arrange
        args = _wrap(4.0)
        # act
        result = BUILTIN_FUNCTIONS["sqr"](args)
        # assert
        assert abs(float(result) - 16.0) < 1e-12

    # --- sgn (one-argument sign function) ---

    def test_sgn_positive(self):
        # arrange
        args = _wrap(5.0)
        # act
        result = BUILTIN_FUNCTIONS["sgn"](args)
        # assert
        assert float(result) == 1.0

    def test_sgn_negative(self):
        # arrange
        args = _wrap(-3.0)
        # act
        result = BUILTIN_FUNCTIONS["sgn"](args)
        # assert
        assert float(result) == -1.0

    def test_sgn_zero(self):
        # arrange
        args = _wrap(0.0)
        # act
        result = BUILTIN_FUNCTIONS["sgn"](args)
        # assert
        assert float(result) == 0.0

    # --- sign(x, y) — two-argument Xyce SIGN function ---

    def test_sign_positive_y(self):
        # arrange — SIGN(x, y) = |x| * sgn(y); positive y yields |x|
        args = _wrap2(3.0, 2.0)
        # act
        result = BUILTIN_FUNCTIONS["sign"](args)
        # assert
        assert abs(float(result) - 3.0) < 1e-12

    def test_sign_negative_y(self):
        # arrange — negative y flips the result to -|x|
        args = _wrap2(3.0, -1.0)
        # act
        result = BUILTIN_FUNCTIONS["sign"](args)
        # assert
        assert abs(float(result) - (-3.0)) < 1e-12

    def test_sign_negative_x_positive_y(self):
        # arrange — |x| is always positive regardless of x's own sign
        args = _wrap2(-4.0, 1.0)
        # act
        result = BUILTIN_FUNCTIONS["sign"](args)
        # assert
        assert abs(float(result) - 4.0) < 1e-12

    def test_sign_wrong_arity_raises(self):
        # arrange — SIGN requires exactly two arguments
        args = _wrap(1.0)
        # act / assert
        with pytest.raises(ValueError):
            BUILTIN_FUNCTIONS["sign"](args)

    # --- uramp ---

    def test_uramp_positive(self):
        # arrange
        args = _wrap(3.0)
        # act
        result = BUILTIN_FUNCTIONS["uramp"](args)
        # assert
        assert float(result) == 3.0

    def test_uramp_negative(self):
        # arrange — uramp clamps negative values to zero
        args = _wrap(-5.0)
        # act
        result = BUILTIN_FUNCTIONS["uramp"](args)
        # assert
        assert float(result) == 0.0

    # --- stp (Heaviside step function) ---

    def test_stp_positive(self):
        # arrange
        args = _wrap(1.0)
        # act
        result = BUILTIN_FUNCTIONS["stp"](args)
        # assert
        assert float(result) == 1.0

    def test_stp_zero(self):
        # arrange — stp(0) = 0 (not active at exactly zero)
        args = _wrap(0.0)
        # act
        result = BUILTIN_FUNCTIONS["stp"](args)
        # assert
        assert float(result) == 0.0

    def test_stp_negative(self):
        # arrange
        args = _wrap(-1.0)
        # act
        result = BUILTIN_FUNCTIONS["stp"](args)
        # assert
        assert float(result) == 0.0

    # --- round / nint ---

    def test_round(self):
        # arrange
        args = _wrap(2.7)
        # act
        result = BUILTIN_FUNCTIONS["round"](args)
        # assert
        assert float(result) == 3.0

    def test_nint(self):
        # arrange — nint is nearest-integer, equivalent to round
        args = _wrap(1.5)
        # act
        result = BUILTIN_FUNCTIONS["nint"](args)
        # assert
        assert float(result) == 2.0

    # --- floor / ceil ---

    def test_floor(self):
        # arrange
        args = _wrap(2.9)
        # act
        result = BUILTIN_FUNCTIONS["floor"](args)
        # assert
        assert float(result) == 2.0

    def test_ceil(self):
        # arrange
        args = _wrap(2.1)
        # act
        result = BUILTIN_FUNCTIONS["ceil"](args)
        # assert
        assert float(result) == 3.0

    # --- int ---

    def test_int_truncates_positive(self):
        # arrange
        args = _wrap(2.9)
        # act
        result = BUILTIN_FUNCTIONS["int"](args)
        # assert
        assert float(result) == 2.0

    def test_int_truncates_negative_toward_zero(self):
        # arrange
        args = _wrap(-2.9)
        # act
        result = BUILTIN_FUNCTIONS["int"](args)
        # assert
        assert float(result) == -2.0

    # --- pow / pwr / pwrs ---

    def test_pow(self):
        # arrange
        args = _wrap2(2.0, 10.0)
        # act
        result = BUILTIN_FUNCTIONS["pow"](args)
        # assert
        assert abs(float(result) - 1024.0) < 1e-9

    def test_pwr(self):
        # arrange — pwr(x, y) = |x|^y
        args = _wrap2(-2.0, 3.0)
        # act
        result = BUILTIN_FUNCTIONS["pwr"](args)
        # assert
        assert abs(float(result) - 8.0) < 1e-9

    def test_pwrs(self):
        # arrange — pwrs(x, y) = sgn(x) * |x|^y
        args = _wrap2(-2.0, 3.0)
        # act
        result = BUILTIN_FUNCTIONS["pwrs"](args)
        # assert
        assert abs(float(result) - (-8.0)) < 1e-9

    # --- fmod ---

    def test_fmod(self):
        # arrange
        args = _wrap2(10.0, 3.0)
        # act
        result = BUILTIN_FUNCTIONS["fmod"](args)
        # assert
        assert abs(float(result) - 1.0) < 1e-12

    def test_fmod_wrong_arity_raises(self):
        # arrange — fmod requires exactly two arguments
        args = _wrap(1.0)
        # act / assert
        with pytest.raises(ValueError):
            BUILTIN_FUNCTIONS["fmod"](args)

    # --- min / max ---

    def test_min_two_args(self):
        # arrange
        args = _wrap2(3.0, 1.0)
        # act
        result = BUILTIN_FUNCTIONS["min"](args)
        # assert
        assert float(result) == 1.0

    def test_max_two_args(self):
        # arrange
        args = _wrap2(3.0, 7.0)
        # act
        result = BUILTIN_FUNCTIONS["max"](args)
        # assert
        assert float(result) == 7.0

    def test_min_single_arg(self):
        # arrange
        args = _wrap(5.0)
        # act
        result = BUILTIN_FUNCTIONS["min"](args)
        # assert
        assert float(result) == 5.0

    def test_min_no_args_raises(self):
        # arrange
        args = []
        # act / assert
        with pytest.raises(ValueError):
            BUILTIN_FUNCTIONS["min"](args)

    # --- limit ---

    def test_limit_within_range(self):
        # arrange
        args = _wrap3(5.0, 1.0, 10.0)
        # act
        result = BUILTIN_FUNCTIONS["limit"](args)
        # assert
        assert float(result) == 5.0

    def test_limit_below_min(self):
        # arrange
        args = _wrap3(-5.0, 1.0, 10.0)
        # act
        result = BUILTIN_FUNCTIONS["limit"](args)
        # assert
        assert float(result) == 1.0

    def test_limit_above_max(self):
        # arrange
        args = _wrap3(20.0, 1.0, 10.0)
        # act
        result = BUILTIN_FUNCTIONS["limit"](args)
        # assert
        assert float(result) == 10.0

    # --- if ---

    def test_if_true_branch(self):
        # arrange
        args = _wrap3(1.0, 10.0, 20.0)
        # act
        result = BUILTIN_FUNCTIONS["if"](args)
        # assert
        assert float(result) == 10.0

    def test_if_false_branch(self):
        # arrange
        args = _wrap3(0.0, 10.0, 20.0)
        # act
        result = BUILTIN_FUNCTIONS["if"](args)
        # assert
        assert float(result) == 20.0

    def test_if_wrong_arity_raises(self):
        # arrange — if requires exactly three arguments
        args = _wrap2(1.0, 2.0)
        # act / assert
        with pytest.raises(ValueError):
            BUILTIN_FUNCTIONS["if"](args)

    # --- ddt / sdt raise NotImplementedError ---

    def test_ddt_raises_not_implemented(self):
        # arrange — ddt is a time-domain operator unsupported in post-processing
        args = _wrap(1.0)
        # act / assert
        with pytest.raises(NotImplementedError):
            BUILTIN_FUNCTIONS["ddt"](args)

    def test_sdt_raises_not_implemented(self):
        # arrange — sdt is a time-domain integral unsupported in post-processing
        args = _wrap(1.0)
        # act / assert
        with pytest.raises(NotImplementedError):
            BUILTIN_FUNCTIONS["sdt"](args)


# ---------------------------------------------------------------------------
# TestXyceBuiltinConstants
# ---------------------------------------------------------------------------


class TestXyceBuiltinConstants:

    def test_pi(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["pi"]
        # assert
        assert abs(float(value) - math.pi) < 1e-12

    def test_e(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["e"]
        # assert
        assert abs(float(value) - math.e) < 1e-12

    def test_meg(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["meg"]
        # assert
        assert float(value) == 1e6

    def test_k(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["k"]
        # assert
        assert float(value) == 1e3

    def test_m(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["m"]
        # assert
        assert float(value) == 1e-3

    def test_u(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["u"]
        # assert
        assert float(value) == 1e-6

    def test_n(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["n"]
        # assert
        assert float(value) == 1e-9

    def test_p(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["p"]
        # assert
        assert float(value) == 1e-12

    def test_f(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["f"]
        # assert
        assert float(value) == 1e-15

    def test_g(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["g"]
        # assert
        assert float(value) == 1e9

    def test_t(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["t"]
        # assert
        assert float(value) == 1e12

    def test_mil(self):
        # arrange / act — 1 mil = 25.4 micrometres
        value = BUILTIN_CONSTANTS["mil"]
        # assert
        assert abs(float(value) - 25.4e-6) < 1e-20

    def test_j(self):
        # arrange / act — j is the imaginary unit
        value = BUILTIN_CONSTANTS["j"]
        # assert
        assert complex(value) == 1j

    def test_mho(self):
        # arrange / act
        value = BUILTIN_CONSTANTS["mho"]
        # assert
        assert float(value) == 1.0


# ---------------------------------------------------------------------------
# TestXyceEvaluatorArithmetic
# ---------------------------------------------------------------------------


class TestXyceEvaluatorArithmetic:

    def test_add_scalars(self):
        # arrange
        text = "1 + 2"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(3.0)

    def test_subtract_scalars(self):
        # arrange
        text = "5 - 3"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(2.0)

    def test_multiply_scalars(self):
        # arrange
        text = "4 * 3"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(12.0)

    def test_divide_scalars(self):
        # arrange
        text = "10 / 4"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(2.5)

    def test_modulo_scalars(self):
        # arrange
        text = "10 % 3"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_power_scalars(self):
        # arrange
        text = "2 ** 10"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1024.0)

    def test_unary_minus(self):
        # arrange
        text = "-5"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(-5.0)

    def test_unary_plus(self):
        # arrange
        text = "+7"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(7.0)

    def test_parentheses_override_precedence(self):
        # arrange
        text = "(1 + 2) * 4"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(12.0)

    def test_mul_before_add(self):
        # arrange
        text = "1 + 2 * 3"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(7.0)

    def test_chained_addition(self):
        # arrange
        text = "1 + 2 + 3 + 4"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(10.0)


# ---------------------------------------------------------------------------
# TestXyceEvaluatorLogical
# ---------------------------------------------------------------------------


class TestXyceEvaluatorLogical:

    def test_or_single_pipe_true(self):
        # arrange
        text = "1 | 0"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_or_double_pipe_true(self):
        # arrange
        text = "1 || 0"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_or_both_false(self):
        # arrange
        text = "0 | 0"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)

    def test_and_single_ampersand_true(self):
        # arrange
        text = "1 & 1"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_and_double_ampersand_false(self):
        # arrange
        text = "1 && 0"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)

    def test_xor_different_operands(self):
        # arrange — 1 XOR 0 = true
        text = "1 ^ 0"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_xor_same_true_operands(self):
        # arrange — 1 XOR 1 = false
        text = "1 ^ 1"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)

    def test_xor_same_false_operands(self):
        # arrange — 0 XOR 0 = false
        text = "0 ^ 0"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)

    def test_not_tilde_of_false(self):
        # arrange — ~0 = NOT false = 1
        text = "~0"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_not_tilde_of_true(self):
        # arrange — ~1 = NOT true = 0
        text = "~1"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)

    def test_not_bang_of_false(self):
        # arrange — !0 = NOT false = 1
        text = "!0"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_not_bang_of_true(self):
        # arrange — !1 = NOT true = 0
        text = "!1"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)


# ---------------------------------------------------------------------------
# TestXyceEvaluatorRelational
# ---------------------------------------------------------------------------


class TestXyceEvaluatorRelational:

    def test_equal_true(self):
        # arrange
        text = "3 == 3"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_equal_false(self):
        # arrange
        text = "3 == 4"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)

    def test_not_equal_true(self):
        # arrange
        text = "3 != 4"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_less_true(self):
        # arrange
        text = "2 < 5"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_less_false(self):
        # arrange
        text = "5 < 2"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)

    def test_less_equal_when_equal(self):
        # arrange
        text = "3 <= 3"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_greater_true(self):
        # arrange
        text = "5 > 2"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_greater_equal_when_equal(self):
        # arrange
        text = "3 >= 3"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)


# ---------------------------------------------------------------------------
# TestXyceEvaluatorTernary
# ---------------------------------------------------------------------------


class TestXyceEvaluatorTernary:

    def test_true_condition_selects_first_branch(self):
        # arrange
        text = "1 ? 10 : 20"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(10.0)

    def test_false_condition_selects_second_branch(self):
        # arrange
        text = "0 ? 10 : 20"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(20.0)

    def test_ternary_with_variable_condition(self):
        # arrange — acts as abs(x) for negative x
        text = "x > 0 ? x : -x"
        variables = {"x": -5.0}
        # act
        result = _evaluate(text, variables=variables)
        # assert
        assert result == pytest.approx(5.0)

    def test_ternary_with_array_condition(self):
        # arrange
        text = "x > 0 ? 1 : -1"
        variables = {"x": np.array([-1.0, 0.5, 2.0])}
        # act
        result = _evaluate(text, variables=variables)
        # assert — element-wise selection via np.where
        np.testing.assert_allclose(result, [-1.0, 1.0, 1.0])


# ---------------------------------------------------------------------------
# TestXyceEvaluatorVariables
# ---------------------------------------------------------------------------


class TestXyceEvaluatorVariables:

    def test_variable_lookup(self):
        # arrange
        text = "x"
        variables = {"x": 7.0}
        # act
        result = _evaluate(text, variables=variables)
        # assert
        assert result == pytest.approx(7.0)

    def test_variable_lookup_is_case_insensitive(self):
        # arrange
        text = "X"
        variables = {"x": 5.0}
        # act
        result = _evaluate(text, variables=variables)
        # assert
        assert result == pytest.approx(5.0)

    def test_constant_pi(self):
        # arrange
        text = "pi"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(math.pi)

    def test_constant_e(self):
        # arrange
        text = "e"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(math.e)

    def test_constant_meg(self):
        # arrange
        text = "MEG"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1e6)

    def test_constant_mil(self):
        # arrange
        text = "mil"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(25.4e-6)

    def test_constant_j(self):
        # arrange
        text = "j"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1j)

    def test_unknown_identifier_raises(self):
        # arrange
        text = "no_such_var"
        # act / assert
        with pytest.raises(ValueError, match="Unknown identifier"):
            _evaluate(text)

    def test_variable_overrides_builtin_constant(self):
        # arrange — user-supplied variables take precedence over builtin constants
        text = "pi"
        variables = {"pi": 3.0}
        # act
        result = _evaluate(text, variables=variables)
        # assert
        assert result == pytest.approx(3.0)

    def test_custom_constant(self):
        # arrange
        text = "c"
        constants = {"c": 3e8}
        # act
        result = _evaluate(text, constants=constants)
        # assert
        assert result == pytest.approx(3e8)


# ---------------------------------------------------------------------------
# TestXyceEvaluatorNumberSuffixes
# ---------------------------------------------------------------------------


class TestXyceEvaluatorNumberSuffixes:

    def test_suffix_t(self):
        # arrange
        text = "1T"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1e12)

    def test_suffix_g(self):
        # arrange
        text = "1G"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1e9)

    def test_suffix_meg(self):
        # arrange
        text = "1MEG"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1e6)

    def test_suffix_k(self):
        # arrange
        text = "1K"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1e3)

    def test_suffix_m(self):
        # arrange
        text = "1M"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1e-3)

    def test_suffix_u(self):
        # arrange
        text = "1U"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1e-6)

    def test_suffix_n(self):
        # arrange
        text = "1N"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1e-9)

    def test_suffix_p(self):
        # arrange
        text = "1P"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1e-12)

    def test_suffix_f(self):
        # arrange
        text = "1F"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1e-15)

    def test_suffix_mil(self):
        # arrange
        text = "1MIL"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(25.4e-6)

    def test_mil_in_suffix_table(self):
        # arrange / act
        value = _NUMBER_SUFFIXES.get("MIL")
        # assert — MIL must be present with the correct scale factor
        assert value is not None
        assert abs(value - 25.4e-6) < 1e-20


# ---------------------------------------------------------------------------
# TestXyceEvaluatorBuiltinCalls
# ---------------------------------------------------------------------------


class TestXyceEvaluatorBuiltinCalls:

    def test_abs_call(self):
        # arrange
        text = "abs(-3)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(3.0)

    def test_sqrt_call(self):
        # arrange
        text = "sqrt(9)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(3.0)

    def test_log_is_log10_via_expression(self):
        # arrange — evaluating LOG via expression must invoke log base 10
        text = "log(100)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(2.0)

    def test_ln_is_natural_log_via_expression(self):
        # arrange
        text = "ln(1)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)

    def test_sin_call(self):
        # arrange
        text = "sin(0)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)

    def test_cos_call(self):
        # arrange
        text = "cos(0)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_exp_call(self):
        # arrange
        text = "exp(1)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(math.e)

    def test_min_call_multiple_args(self):
        # arrange
        text = "min(3, 1, 2)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_max_call_multiple_args(self):
        # arrange
        text = "max(3, 1, 2)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(3.0)

    def test_sgn_positive_via_expression(self):
        # arrange
        text = "sgn(5)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_sign_two_arg_via_expression(self):
        # arrange — SIGN(x, y) = sgn(y) * |x|
        text = "sign(3, -1)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(-3.0)

    def test_stp_call(self):
        # arrange
        text = "stp(1)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_nint_call(self):
        # arrange
        text = "nint(2.7)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(3.0)

    def test_fmod_call(self):
        # arrange
        text = "fmod(10, 3)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(1.0)

    def test_if_call_true_branch(self):
        # arrange
        text = "if(1, 10, 20)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(10.0)

    def test_if_call_false_branch(self):
        # arrange
        text = "if(0, 10, 20)"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(20.0)

    def test_unknown_function_raises(self):
        # arrange
        text = "no_such_fn(1)"
        # act / assert
        with pytest.raises(ValueError, match="Unknown function"):
            _evaluate(text)

    def test_wrong_arity_raises(self):
        # arrange — abs takes exactly one argument
        text = "abs(1, 2)"
        # act / assert
        with pytest.raises(ValueError):
            _evaluate(text)


# ---------------------------------------------------------------------------
# TestXyceEvaluatorUserFunctions
# ---------------------------------------------------------------------------


class TestXyceEvaluatorUserFunctions:

    def test_simple_user_function(self):
        # arrange
        definition = parse_function_definition(".func sq(x) {x * x}")
        functions = {"sq": definition}
        # act
        result = _evaluate("sq(3)", functions=functions)
        # assert
        assert result == pytest.approx(9.0)

    def test_two_param_user_function(self):
        # arrange
        definition = parse_function_definition(".func add(a, b) {a + b}")
        functions = {"add": definition}
        # act
        result = _evaluate("add(2, 5)", functions=functions)
        # assert
        assert result == pytest.approx(7.0)

    def test_user_function_calling_builtin(self):
        # arrange — user function body can reference a builtin function
        definition = parse_function_definition(".func mysqrt(x) {sqrt(x)}")
        functions = {"mysqrt": definition}
        # act
        result = _evaluate("mysqrt(16)", functions=functions)
        # assert
        assert result == pytest.approx(4.0)

    def test_recursive_function_raises(self):
        # arrange
        definition = parse_function_definition(".func f(x) {f(x - 1)}")
        functions = {"f": definition}
        # act / assert
        with pytest.raises(ValueError, match="Recursive"):
            _evaluate("f(5)", functions=functions)

    def test_wrong_function_arity_raises(self):
        # arrange — sq takes one argument; calling with two must fail
        definition = parse_function_definition(".func sq(x) {x * x}")
        functions = {"sq": definition}
        # act / assert
        with pytest.raises(ValueError):
            _evaluate("sq(1, 2)", functions=functions)

    def test_function_lookup_is_case_insensitive(self):
        # arrange
        definition = parse_function_definition(".func sq(x) {x * x}")
        functions = {"sq": definition}
        # act — call using uppercase name
        result = _evaluate("SQ(4)", functions=functions)
        # assert
        assert result == pytest.approx(16.0)


# ---------------------------------------------------------------------------
# TestXyceEvaluatorProbes
# ---------------------------------------------------------------------------


class TestXyceEvaluatorProbes:

    def test_v_probe_direct(self):
        # arrange
        variables = {"v(out)": np.asarray(3.3)}
        # act
        result = _evaluate("V(out)", variables=variables)
        # assert
        assert result == pytest.approx(3.3)

    def test_v_probe_differential(self):
        # arrange — V(a, b) = V(a) - V(b) from individual node variables
        variables = {"v(a)": np.asarray(5.0), "v(b)": np.asarray(2.0)}
        # act
        result = _evaluate("V(a, b)", variables=variables)
        # assert
        assert result == pytest.approx(3.0)

    def test_v_probe_differential_with_ground(self):
        # arrange — V(out, 0) collapses to V(out) when ground is zero
        variables = {"v(out)": np.asarray(1.8)}
        # act
        result = _evaluate("V(out, 0)", variables=variables)
        # assert
        assert result == pytest.approx(1.8)

    def test_i_probe(self):
        # arrange
        variables = {"i(r1)": np.asarray(0.01)}
        # act
        result = _evaluate("I(r1)", variables=variables)
        # assert
        assert result == pytest.approx(0.01)

    def test_probe_not_found_raises(self):
        # arrange — no variable for the requested node
        variables = {}
        # act / assert
        with pytest.raises(ValueError, match="Unknown probe"):
            _evaluate("V(missing_node)", variables=variables)

    def test_id_is_not_a_probe_family(self):
        # arrange — 'id' was removed from Xyce probe families
        variables = {}
        # act / assert
        with pytest.raises((ValueError, TypeError)):
            _evaluate("id(x)", variables=variables)


# ---------------------------------------------------------------------------
# TestXyceEvaluatorStepSelectors
# ---------------------------------------------------------------------------


class TestXyceEvaluatorStepSelectors:

    def test_step_selector_extracts_first_slice(self):
        # arrange
        data = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0])
        slices = (slice(0, 3), slice(3, 6))
        variables = {"v(out)": data}
        # act
        result = _evaluate("V(out)@1", variables=variables, step_slices=slices)
        # assert
        np.testing.assert_allclose(result, [1.0, 2.0, 3.0])

    def test_step_selector_extracts_second_slice(self):
        # arrange
        data = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0])
        slices = (slice(0, 3), slice(3, 6))
        variables = {"v(out)": data}
        # act
        result = _evaluate("V(out)@2", variables=variables, step_slices=slices)
        # assert
        np.testing.assert_allclose(result, [4.0, 5.0, 6.0])

    def test_step_selector_out_of_range_raises(self):
        # arrange — only one step available; @5 is out of range
        data = np.array([1.0, 2.0])
        slices = (slice(0, 2),)
        variables = {"v(out)": data}
        # act / assert
        with pytest.raises(ValueError, match="out of range"):
            _evaluate("V(out)@5", variables=variables, step_slices=slices)

    def test_step_selector_without_slices_raises(self):
        # arrange — step metadata is required for the @n syntax
        variables = {"v(out)": np.array([1.0, 2.0])}
        # act / assert
        with pytest.raises(ValueError, match="metadata"):
            _evaluate("V(out)@1", variables=variables)


# ---------------------------------------------------------------------------
# TestXyceEvaluatorArrays
# ---------------------------------------------------------------------------


class TestXyceEvaluatorArrays:

    def test_array_addition(self):
        # arrange
        variables = {"x": np.array([1.0, 2.0, 3.0])}
        # act
        result = _evaluate("x + 1", variables=variables)
        # assert
        np.testing.assert_allclose(result, [2.0, 3.0, 4.0])

    def test_array_multiplication(self):
        # arrange
        variables = {"x": np.array([1.0, 2.0, 3.0])}
        # act
        result = _evaluate("x * 2", variables=variables)
        # assert
        np.testing.assert_allclose(result, [2.0, 4.0, 6.0])

    def test_array_modulo(self):
        # arrange
        variables = {"x": np.array([7.0, 8.0, 9.0])}
        # act
        result = _evaluate("x % 3", variables=variables)
        # assert
        np.testing.assert_allclose(result, [1.0, 2.0, 0.0])

    def test_array_ternary(self):
        # arrange
        variables = {"x": np.array([-1.0, 0.0, 1.0])}
        # act
        result = _evaluate("x > 0 ? 1 : 0", variables=variables)
        # assert
        np.testing.assert_allclose(result, [0.0, 0.0, 1.0])

    def test_array_logical_xor(self):
        # arrange
        variables = {"a": np.array([0.0, 0.0, 1.0, 1.0]), "b": np.array([0.0, 1.0, 0.0, 1.0])}
        # act
        result = _evaluate("a ^ b", variables=variables)
        # assert
        np.testing.assert_allclose(result, [0.0, 1.0, 1.0, 0.0])


# ---------------------------------------------------------------------------
# TestXyceNodes
# ---------------------------------------------------------------------------


class TestXyceNodes:

    def test_logical_xor_operator_value(self):
        # arrange / act
        value = BinaryOperator.LOGICAL_XOR.value
        # assert — Xyce uses ^ for XOR
        assert value == "^"

    def test_mod_operator_value(self):
        # arrange / act
        value = BinaryOperator.MOD.value
        # assert — Xyce uses % for modulo
        assert value == "%"

    def test_number_node_stores_text(self):
        # arrange / act
        node = NumberNode("3.14")
        # assert
        assert node.text == "3.14"

    def test_identifier_node_stores_name(self):
        # arrange / act
        node = IdentifierNode("vout")
        # assert
        assert node.name == "vout"

    def test_function_call_node(self):
        # arrange / act
        node = FunctionCallNode("abs", [IdentifierNode("x")])
        # assert
        assert node.name == "abs"
        assert len(node.args) == 1

    def test_binary_operation_node(self):
        # arrange / act
        node = BinaryOperationNode(NumberNode("1"), BinaryOperator.ADD, NumberNode("2"))
        # assert
        assert node.operator == BinaryOperator.ADD

    def test_unary_operation_node(self):
        # arrange / act
        node = UnaryOperationNode(UnaryOperator.NEG, IdentifierNode("x"))
        # assert
        assert node.operator == UnaryOperator.NEG

    def test_ternary_operation_node_stores_condition(self):
        # arrange / act
        node = TernaryOperationNode(IdentifierNode("c"), NumberNode("1"), NumberNode("0"))
        # assert
        assert isinstance(node.condition, IdentifierNode)

    def test_step_selector_node_stores_index(self):
        # arrange
        base = FunctionCallNode("v", [IdentifierNode("out")])
        # act
        node = StepSelectorNode(base, 1)
        # assert
        assert node.step_index == 1

    def test_function_definition_node(self):
        # arrange
        body = BinaryOperationNode(IdentifierNode("x"), BinaryOperator.MUL, IdentifierNode("x"))
        # act
        node = FunctionDefinitionNode("sq", ["x"], body)
        # assert
        assert node.name == "sq"
        assert node.params[0] == "x"


# ---------------------------------------------------------------------------
# TestXyceEvaluationContext
# ---------------------------------------------------------------------------


class TestXyceEvaluationContext:

    def test_context_stores_variables(self):
        # arrange / act
        ctx = EvaluationContext({"x": np.asarray(1.0)}, {}, {})
        # assert
        assert "x" in ctx.variables

    def test_context_stores_step_slices(self):
        # arrange
        slices = (slice(0, 3),)
        # act
        ctx = EvaluationContext({}, {}, {}, step_slices=slices)
        # assert
        assert ctx.step_slices == slices

    def test_context_step_slices_none_by_default(self):
        # arrange / act
        ctx = EvaluationContext({}, {}, {})
        # assert
        assert ctx.step_slices is None


# ---------------------------------------------------------------------------
# TestXyceEvaluatorEdgeCases
# ---------------------------------------------------------------------------


class TestXyceEvaluatorEdgeCases:

    def test_zero_division_produces_inf(self):
        # arrange
        text = "1 / 0"
        # act
        result = _evaluate(text)
        # assert — numpy yields inf rather than raising on float division by zero
        assert math.isinf(result)

    def test_nested_function_calls(self):
        # arrange
        text = "abs(sin(0))"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.0)

    def test_complex_expression_with_variable(self):
        # arrange
        text = "2 * sqrt(x) + 1"
        variables = {"x": 4.0}
        # act
        result = _evaluate(text, variables=variables)
        # assert
        assert result == pytest.approx(5.0)

    def test_negative_power(self):
        # arrange
        text = "2 ** -1"
        # act
        result = _evaluate(text)
        # assert
        assert result == pytest.approx(0.5)

    def test_modulo_negative_numerator(self):
        # arrange
        text = "-7 % 3"
        # act
        result = _evaluate(text)
        # assert — numpy mod: |-7 % 3| = 2
        assert abs(result) == pytest.approx(2.0)

    def test_chained_comparisons_with_and(self):
        # arrange — (x > 0) & (x < 10) as a boolean expression
        text = "(x > 0) & (x < 10)"
        variables = {"x": 5.0}
        # act
        result = _evaluate(text, variables=variables)
        # assert
        assert result == pytest.approx(1.0)

    def test_if_with_array_arguments(self):
        # arrange
        text = "if(x, x, 0)"
        variables = {"x": np.array([0.0, 1.0, 2.0])}
        # act
        result = _evaluate(text, variables=variables)
        # assert
        np.testing.assert_allclose(result, [0.0, 1.0, 2.0])
