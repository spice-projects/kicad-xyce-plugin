from __future__ import annotations

import logging

import numpy as np

from kicad_xyce_plugin.expression import Expression, ExpressionManager
from kicad_xyce_plugin.expression.nodes import BinaryOperationNode, BinaryOperator, FunctionCallNode, IdentifierNode, NumberNode, StepSelectorNode, TernaryOperationNode, UnaryOperationNode, UnaryOperator


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _expr(name, data, unit=""):
    return Expression(name, np.array(data, dtype=np.float64), unit)


def _make_manager(expressions=None, step_slices=None):
    return ExpressionManager(expressions or [], step_slices=step_slices)


# ---------------------------------------------------------------------------
# TestExpressionManagerInit
# ---------------------------------------------------------------------------


class TestExpressionManagerInit:

    def test_empty_expressions_produces_empty_context(self):
        # arrange / act
        manager = _make_manager()
        # assert
        assert manager.expressions == []

    def test_expressions_stored_with_lowercase_keys(self):
        # arrange
        expr = _expr("V(R1)", [1.0, 2.0], "V")
        # act
        manager = _make_manager([expr])
        # assert — lookup is case-insensitive
        assert manager.evaluate("V(R1)") is not None

    def test_step_slices_property_stores_provided_slices(self):
        # arrange
        slices = (slice(0, 5), slice(5, 10))
        # act
        manager = _make_manager(step_slices=slices)
        # assert
        assert manager.step_slices == slices

    def test_step_slices_property_returns_none_when_not_given(self):
        # arrange / act
        manager = _make_manager()
        # assert
        assert manager.step_slices is None

    def test_multiple_expressions_all_stored(self):
        # arrange
        e1 = _expr("V(A)", [1.0], "V")
        e2 = _expr("I(R1)", [0.5], "A")
        # act
        manager = _make_manager([e1, e2])
        # assert
        assert len(manager.expressions) == 2


# ---------------------------------------------------------------------------
# TestExpressionManagerEvaluate
# ---------------------------------------------------------------------------


class TestExpressionManagerEvaluate:

    def test_evaluate_existing_expression_returns_cached_object(self):
        # arrange
        expr = _expr("v(a)", [1.0], "V")
        manager = _make_manager([expr])
        # act
        result = manager.evaluate("v(a)")
        # assert — the exact same object that was stored
        assert result is expr

    def test_evaluate_case_insensitive_lookup(self):
        # arrange
        expr = _expr("V(A)", [1.0], "V")
        manager = _make_manager([expr])
        # act
        result = manager.evaluate("V(A)")
        # assert
        assert result is not None

    def test_evaluate_node_name_with_plus_sign(self):
        # arrange
        v1 = _expr("V(/POWER_SUPPLY/HB+)", [1.0, 2.0, 3.0], "V")
        v2 = _expr("V(HGND)", [0.1, 0.2, 0.3], "V")
        manager = _make_manager([v1, v2])
        # act
        result = manager.evaluate("V(/POWER_SUPPLY/HB+)-V(HGND)")
        # assert
        assert result is not None
        np.testing.assert_allclose(result.data, [0.9, 1.8, 2.7])
        assert result.unit == "V"

    def test_evaluate_node_name_with_colon(self):
        # arrange
        i1 = _expr("I(XL201:L1)", [0.5, 0.6, 0.7], "A")
        i2 = _expr("I(C207)", [0.1, 0.2, 0.3], "A")
        manager = _make_manager([i1, i2])
        # act
        result = manager.evaluate("I(XL201:L1)-I(C207)")
        # assert
        assert result is not None
        np.testing.assert_allclose(result.data, [0.4, 0.4, 0.4])
        assert result.unit == "A"

    def test_evaluate_arithmetic_expression_computes_data(self):
        # arrange
        v = _expr("v(a)", [2.0, 4.0], "V")
        manager = _make_manager([v])
        # act
        result = manager.evaluate("v(a)*2")
        # assert
        assert result is not None
        np.testing.assert_allclose(result.data, [4.0, 8.0])

    def test_evaluate_uses_provided_name(self):
        # arrange
        v = _expr("v(a)", [1.0], "V")
        manager = _make_manager([v])
        # act
        result = manager.evaluate("v(a)", name="my_result")
        # assert
        assert result is not None
        assert result.name == "my_result"

    def test_evaluate_without_name_uses_formatted_expression(self):
        # arrange
        v = _expr("v(a)", [2.0], "V")
        i = _expr("i(r1)", [0.5], "A")
        manager = _make_manager([v, i])
        # act
        result = manager.evaluate("v(a)*i(r1)")
        # assert — formatted name wraps the multiplication in parens
        assert result is not None
        assert result.name == "(v(a)*i(r1))"

    def test_evaluate_infers_unit_volts_times_amps(self):
        # arrange
        v = _expr("v(a)", [2.0], "V")
        i = _expr("i(r1)", [0.5], "A")
        manager = _make_manager([v, i])
        # act
        result = manager.evaluate("v(a)*i(r1)")
        # assert
        assert result is not None
        assert result.unit == "W"

    def test_evaluate_invalid_expression_returns_none_and_logs_warning(self, caplog):
        # arrange
        manager = _make_manager()
        # act
        with caplog.at_level(logging.WARNING):
            result = manager.evaluate("1 +")
        # assert
        assert result is None
        assert any("Failed to evaluate expression" in r.message for r in caplog.records)

    def test_evaluate_caches_result_for_subsequent_calls(self):
        # arrange
        v = _expr("v(a)", [1.0], "V")
        manager = _make_manager([v])
        # act
        first = manager.evaluate("v(a)*2")
        second = manager.evaluate("v(a)*2")
        # assert — same object returned on repeat call
        assert first is second

    def test_evaluate_named_result_cached_by_name(self):
        # arrange
        v = _expr("v(a)", [1.0], "V")
        manager = _make_manager([v])
        # act
        first = manager.evaluate("v(a)", name="my_signal")
        second = manager.evaluate("v(a)", name="my_signal")
        # assert
        assert first is second

    def test_evaluate_source_is_expression_manager(self):
        # arrange
        v = _expr("v(a)", [1.0], "V")
        manager = _make_manager([v])
        # act
        result = manager.evaluate("v(a)*2")
        # assert
        assert result is not None
        assert result.source == "expression manager"

    def test_evaluate_numeric_literal_is_dimensionless(self):
        # arrange
        manager = _make_manager()
        # act
        result = manager.evaluate("3.14")
        # assert
        assert result is not None
        assert result.unit == ""
        np.testing.assert_allclose(result.data, [3.14])


# ---------------------------------------------------------------------------
# TestRematerialize
# ---------------------------------------------------------------------------


class TestRematerialize:

    def test_returns_data_unchanged_when_no_step_slices(self):
        # arrange
        manager = _make_manager()
        data = np.array([1.0, 2.0, 3.0])
        ast = NumberNode("1.0")
        # act
        result = manager._rematerialize(data, ast)
        # assert
        assert result is data

    def test_returns_data_unchanged_when_scalar(self):
        # arrange
        manager = _make_manager(step_slices=(slice(0, 3), slice(3, 6)))
        data = np.array(5.0)
        ast = NumberNode("5.0")
        # act
        result = manager._rematerialize(data, ast)
        # assert
        assert result is data

    def test_returns_data_unchanged_when_length_matches_total_points(self):
        # arrange
        manager = _make_manager(step_slices=(slice(0, 3), slice(3, 6)))
        data = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0])  # len == total_points == 6
        ast = StepSelectorNode(IdentifierNode("x"), 1)
        # act
        result = manager._rematerialize(data, ast)
        # assert
        assert result is data

    def test_returns_data_unchanged_when_no_step_selector_in_ast(self):
        # arrange
        manager = _make_manager(step_slices=(slice(0, 3), slice(3, 6)))
        data = np.array([1.0, 2.0, 3.0])  # len == step_length but no step selector
        ast = IdentifierNode("x")
        # act
        result = manager._rematerialize(data, ast)
        # assert
        assert result is data

    def test_returns_data_unchanged_when_length_does_not_match_step_length(self):
        # arrange
        manager = _make_manager(step_slices=(slice(0, 3), slice(3, 6)))
        data = np.array([1.0, 2.0])  # len = 2 != step_length = 3
        ast = StepSelectorNode(IdentifierNode("x"), 1)
        # act
        result = manager._rematerialize(data, ast)
        # assert
        assert result is data

    def test_tiles_data_across_steps_when_all_conditions_met(self):
        # arrange
        manager = _make_manager(step_slices=(slice(0, 3), slice(3, 6)))
        data = np.array([1.0, 2.0, 3.0])  # len == step_length == 3
        ast = StepSelectorNode(IdentifierNode("x"), 1)
        # act
        result = manager._rematerialize(data, ast)
        # assert — data tiled across 2 steps
        np.testing.assert_array_equal(result, [1.0, 2.0, 3.0, 1.0, 2.0, 3.0])

    def test_tiles_across_three_steps(self):
        # arrange
        manager = _make_manager(step_slices=(slice(0, 2), slice(2, 4), slice(4, 6)))
        data = np.array([10.0, 20.0])
        ast = StepSelectorNode(IdentifierNode("v"), 2)
        # act
        result = manager._rematerialize(data, ast)
        # assert
        np.testing.assert_array_equal(result, [10.0, 20.0, 10.0, 20.0, 10.0, 20.0])


# ---------------------------------------------------------------------------
# TestHasStepSelector
# ---------------------------------------------------------------------------


class TestHasStepSelector:

    def test_number_node_returns_false(self):
        # arrange
        node = NumberNode("1.0")
        # act / assert
        assert not ExpressionManager._has_step_selector(node)

    def test_identifier_node_returns_false(self):
        # arrange
        node = IdentifierNode("x")
        # act / assert
        assert not ExpressionManager._has_step_selector(node)

    def test_step_selector_node_returns_true(self):
        # arrange
        node = StepSelectorNode(IdentifierNode("x"), 1)
        # act / assert
        assert ExpressionManager._has_step_selector(node)

    def test_unary_node_with_step_selector_operand_returns_true(self):
        # arrange
        node = UnaryOperationNode(UnaryOperator.NEG, StepSelectorNode(IdentifierNode("x"), 1))
        # act / assert
        assert ExpressionManager._has_step_selector(node)

    def test_unary_node_without_step_selector_returns_false(self):
        # arrange
        node = UnaryOperationNode(UnaryOperator.NEG, IdentifierNode("x"))
        # act / assert
        assert not ExpressionManager._has_step_selector(node)

    def test_binary_node_with_step_selector_on_left_returns_true(self):
        # arrange
        node = BinaryOperationNode(StepSelectorNode(IdentifierNode("x"), 1), BinaryOperator.ADD, NumberNode("1"))
        # act / assert
        assert ExpressionManager._has_step_selector(node)

    def test_binary_node_with_step_selector_on_right_returns_true(self):
        # arrange
        node = BinaryOperationNode(NumberNode("1"), BinaryOperator.ADD, StepSelectorNode(IdentifierNode("x"), 1))
        # act / assert
        assert ExpressionManager._has_step_selector(node)

    def test_binary_node_without_step_selector_returns_false(self):
        # arrange
        node = BinaryOperationNode(IdentifierNode("a"), BinaryOperator.MUL, IdentifierNode("b"))
        # act / assert
        assert not ExpressionManager._has_step_selector(node)

    def test_ternary_node_with_step_selector_in_condition_returns_true(self):
        # arrange
        selector = StepSelectorNode(IdentifierNode("c"), 1)
        node = TernaryOperationNode(selector, NumberNode("1"), NumberNode("0"))
        # act / assert
        assert ExpressionManager._has_step_selector(node)

    def test_ternary_node_with_step_selector_in_if_true_returns_true(self):
        # arrange
        selector = StepSelectorNode(IdentifierNode("x"), 1)
        node = TernaryOperationNode(NumberNode("1"), selector, NumberNode("0"))
        # act / assert
        assert ExpressionManager._has_step_selector(node)

    def test_ternary_node_with_step_selector_in_if_false_returns_true(self):
        # arrange
        selector = StepSelectorNode(IdentifierNode("x"), 1)
        node = TernaryOperationNode(NumberNode("1"), NumberNode("0"), selector)
        # act / assert
        assert ExpressionManager._has_step_selector(node)

    def test_ternary_node_without_step_selector_returns_false(self):
        # arrange
        node = TernaryOperationNode(IdentifierNode("c"), IdentifierNode("a"), IdentifierNode("b"))
        # act / assert
        assert not ExpressionManager._has_step_selector(node)

    def test_function_call_with_step_selector_arg_returns_true(self):
        # arrange
        node = FunctionCallNode("abs", [StepSelectorNode(IdentifierNode("x"), 1)])
        # act / assert
        assert ExpressionManager._has_step_selector(node)

    def test_function_call_without_step_selector_arg_returns_false(self):
        # arrange
        node = FunctionCallNode("abs", [IdentifierNode("x")])
        # act / assert
        assert not ExpressionManager._has_step_selector(node)

    def test_function_call_unknown_function_without_step_selector_in_args_returns_false(self):
        # arrange
        node = FunctionCallNode("unknown", [IdentifierNode("a")])
        # act / assert
        assert not ExpressionManager._has_step_selector(node)


# ---------------------------------------------------------------------------
# TestInferUnit
# ---------------------------------------------------------------------------


class TestInferUnit:

    def _manager(self):
        return _make_manager()

    def test_number_node_is_dimensionless(self):
        # arrange
        manager = self._manager()
        node = NumberNode("3.14")
        # act
        unit = manager._infer_unit(node, {})
        # assert
        assert unit == ""

    def test_identifier_node_resolved_from_context(self):
        # arrange
        manager = self._manager()
        node = IdentifierNode("v_a")
        # act
        unit = manager._infer_unit(node, {"v_a": "V"})
        # assert
        assert unit == "V"

    def test_identifier_node_built_in_constant_mho(self):
        # arrange
        manager = self._manager()
        node = IdentifierNode("mho")
        # act
        unit = manager._infer_unit(node, {})
        # assert
        assert unit == "S"

    def test_identifier_node_built_in_constant_s(self):
        # arrange
        manager = self._manager()
        node = IdentifierNode("s")
        # act
        unit = manager._infer_unit(node, {})
        # assert
        assert unit == "s"

    def test_identifier_node_unknown_is_dimensionless(self):
        # arrange
        manager = self._manager()
        node = IdentifierNode("xyz")
        # act
        unit = manager._infer_unit(node, {})
        # assert
        assert unit == ""

    def test_unary_node_preserves_operand_unit(self):
        # arrange
        manager = self._manager()
        node = UnaryOperationNode(UnaryOperator.NEG, IdentifierNode("v"))
        # act
        unit = manager._infer_unit(node, {"v": "V"})
        # assert
        assert unit == "V"

    def test_binary_node_delegates_to_propagate_binary_unit(self):
        # arrange
        manager = self._manager()
        node = BinaryOperationNode(IdentifierNode("v"), BinaryOperator.DIV, IdentifierNode("i"))
        # act
        unit = manager._infer_unit(node, {"v": "V", "i": "A"})
        # assert — V/A = Ω
        assert unit == "Ω"

    def test_function_call_v_returns_volts(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("v", [IdentifierNode("a")])
        # act
        unit = manager._infer_unit(node, {})
        # assert
        assert unit == "V"

    def test_function_call_i_returns_amps(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("i", [IdentifierNode("r1")])
        # act
        unit = manager._infer_unit(node, {})
        # assert
        assert unit == "A"

    def test_function_call_id_returns_amps(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("id", [IdentifierNode("m1")])
        # act
        unit = manager._infer_unit(node, {})
        # assert
        assert unit == "A"

    def test_function_call_v_uses_context_unit_when_available(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("v", [IdentifierNode("a")])
        # act — context has a matching probe key
        unit = manager._infer_unit(node, {"v(a)": "mV"})
        # assert
        assert unit == "mV"

    def test_function_call_network_param_z11_not_in_context_returns_empty(self):
        # arrange — network param probe only routes to _unit_for_probe when the key is already in unit_context
        manager = self._manager()
        node = FunctionCallNode("z11", [NumberNode("1"), NumberNode("1")])
        # act
        unit = manager._infer_unit(node, {})
        # assert — falls through to _function_unit_multi which returns ""
        assert unit == ""

    def test_function_call_network_param_y11_not_in_context_returns_empty(self):
        # arrange — network param probe only routes to _unit_for_probe when the key is already in unit_context
        manager = self._manager()
        node = FunctionCallNode("y11", [NumberNode("1"), NumberNode("1")])
        # act
        unit = manager._infer_unit(node, {})
        # assert — falls through to _function_unit_multi which returns ""
        assert unit == ""

    def test_function_call_network_param_in_context_uses_context_unit(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("s11", [NumberNode("1"), NumberNode("1")])
        unit_context = {"s11(1, 1)": "dB"}
        # act
        unit = manager._infer_unit(node, unit_context)
        # assert
        assert unit == "dB"

    def test_function_call_nullary_is_dimensionless(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("time", [])
        # act
        unit = manager._infer_unit(node, {})
        # assert
        assert unit == ""

    def test_function_call_unary_db_returns_decibels(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("db", [IdentifierNode("v")])
        # act
        unit = manager._infer_unit(node, {"v": "V"})
        # assert
        assert unit == "dB"

    def test_function_call_multi_arg_max_preserves_first_arg_unit(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("max", [IdentifierNode("a"), IdentifierNode("b")])
        # act
        unit = manager._infer_unit(node, {"a": "V", "b": "V"})
        # assert
        assert unit == "V"

    def test_ternary_node_matching_branch_units_returns_unit(self):
        # arrange
        manager = self._manager()
        node = TernaryOperationNode(NumberNode("1"), IdentifierNode("v1"), IdentifierNode("v2"))
        # act
        unit = manager._infer_unit(node, {"v1": "V", "v2": "V"})
        # assert
        assert unit == "V"

    def test_ternary_node_mismatched_branch_units_returns_empty(self):
        # arrange
        manager = self._manager()
        node = TernaryOperationNode(NumberNode("1"), IdentifierNode("v"), IdentifierNode("i"))
        # act
        unit = manager._infer_unit(node, {"v": "V", "i": "A"})
        # assert
        assert unit == ""

    def test_step_selector_node_returns_base_unit(self):
        # arrange
        manager = self._manager()
        node = StepSelectorNode(IdentifierNode("v"), 1)
        # act
        unit = manager._infer_unit(node, {"v": "V"})
        # assert
        assert unit == "V"

    def test_unknown_node_type_returns_empty(self):
        # arrange
        manager = self._manager()

        class _UnknownNode:
            pass

        # act
        unit = manager._infer_unit(_UnknownNode(), {})  # type: ignore[arg-type]
        # assert
        assert unit == ""


# ---------------------------------------------------------------------------
# TestFormatExpression
# ---------------------------------------------------------------------------


class TestFormatExpression:

    def _manager(self) -> ExpressionManager:
        return _make_manager()

    def test_number_node_returns_text(self):
        # arrange
        manager = self._manager()
        node = NumberNode("3.14")
        # act / assert
        assert manager._format_expression(node) == "3.14"

    def test_identifier_node_returns_name(self):
        # arrange
        manager = self._manager()
        node = IdentifierNode("v_out")
        # act / assert
        assert manager._format_expression(node) == "v_out"

    def test_unary_node_prepends_operator(self):
        # arrange
        manager = self._manager()
        node = UnaryOperationNode(UnaryOperator.NEG, IdentifierNode("x"))
        # act / assert
        assert manager._format_expression(node) == "-x"

    def test_unary_pos_node(self):
        # arrange
        manager = self._manager()
        node = UnaryOperationNode(UnaryOperator.POS, IdentifierNode("x"))
        # act / assert
        assert manager._format_expression(node) == "+x"

    def test_binary_node_wraps_in_parens(self):
        # arrange
        manager = self._manager()
        node = BinaryOperationNode(IdentifierNode("a"), BinaryOperator.ADD, IdentifierNode("b"))
        # act / assert
        assert manager._format_expression(node) == "(a+b)"

    def test_binary_node_mul(self):
        # arrange
        manager = self._manager()
        node = BinaryOperationNode(IdentifierNode("v"), BinaryOperator.MUL, IdentifierNode("i"))
        # act / assert
        assert manager._format_expression(node) == "(v*i)"

    def test_function_call_no_args(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("time", [])
        # act / assert
        assert manager._format_expression(node) == "time()"

    def test_function_call_one_arg(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("abs", [IdentifierNode("x")])
        # act / assert
        assert manager._format_expression(node) == "abs(x)"

    def test_function_call_multiple_args(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("v", [IdentifierNode("a"), IdentifierNode("b")])
        # act / assert
        assert manager._format_expression(node) == "v(a,b)"

    def test_ternary_node_formats_with_question_colon(self):
        # arrange
        manager = self._manager()
        node = TernaryOperationNode(IdentifierNode("c"), IdentifierNode("a"), IdentifierNode("b"))
        # act / assert
        assert manager._format_expression(node) == "(c?a:b)"

    def test_step_selector_formats_as_base_at_n(self):
        # arrange
        manager = self._manager()
        node = StepSelectorNode(IdentifierNode("v"), 3)
        # act / assert
        assert manager._format_expression(node) == "v@3"

    def test_unknown_node_type_returns_empty_string(self):
        # arrange
        manager = self._manager()

        class _UnknownNode:
            pass

        # act / assert
        assert manager._format_expression(_UnknownNode()) == ""  # type: ignore[arg-type]


# ---------------------------------------------------------------------------
# TestUnitForIdentifier
# ---------------------------------------------------------------------------


class TestUnitForIdentifier:

    def _manager(self) -> ExpressionManager:
        return _make_manager()

    def test_identifier_found_in_context_returns_unit(self):
        # arrange
        manager = self._manager()
        # act
        unit = manager._unit_for_identifier("V_out", {"v_out": "V"})
        # assert — lookup is case-folded
        assert unit == "V"

    def test_identifier_not_in_context_but_is_builtin_mho_returns_S(self):
        # arrange
        manager = self._manager()
        # act
        unit = manager._unit_for_identifier("mho", {})
        # assert
        assert unit == "S"

    def test_identifier_built_in_s_returns_seconds(self):
        # arrange
        manager = self._manager()
        # act
        unit = manager._unit_for_identifier("s", {})
        # assert
        assert unit == "s"

    def test_identifier_built_in_pi_returns_empty(self):
        # arrange
        manager = self._manager()
        # act
        unit = manager._unit_for_identifier("pi", {})
        # assert
        assert unit == ""

    def test_unknown_identifier_returns_empty(self):
        # arrange
        manager = self._manager()
        # act
        unit = manager._unit_for_identifier("completely_unknown", {})
        # assert
        assert unit == ""

    def test_context_takes_precedence_over_builtin(self):
        # arrange
        manager = self._manager()
        # act — "s" is a builtin constant (unit "s"), but here context overrides it
        unit = manager._unit_for_identifier("s", {"s": "A"})
        # assert
        assert unit == "A"


# ---------------------------------------------------------------------------
# TestUnitForProbe
# ---------------------------------------------------------------------------


class TestUnitForProbe:

    def _manager(self) -> ExpressionManager:
        return _make_manager()

    def test_v_probe_returns_volts(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("v", [IdentifierNode("out")])
        # act
        unit = manager._unit_for_probe(node, {})
        # assert
        assert unit == "V"

    def test_i_probe_returns_amps(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("i", [IdentifierNode("r1")])
        # act
        unit = manager._unit_for_probe(node, {})
        # assert
        assert unit == "A"

    def test_id_probe_returns_amps(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("id", [IdentifierNode("m1")])
        # act
        unit = manager._unit_for_probe(node, {})
        # assert
        assert unit == "A"

    def test_context_unit_takes_precedence_over_family_default(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("v", [IdentifierNode("out")])
        # act
        unit = manager._unit_for_probe(node, {"v(out)": "mV"})
        # assert
        assert unit == "mV"

    def test_z_network_param_returns_ohm(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("z11", [NumberNode("1"), NumberNode("1")])
        # act
        unit = manager._unit_for_probe(node, {})
        # assert
        assert unit == "Ω"

    def test_y_network_param_returns_siemens(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("y21", [NumberNode("2"), NumberNode("1")])
        # act
        unit = manager._unit_for_probe(node, {})
        # assert
        assert unit == "S"

    def test_s_network_param_not_in_context_returns_empty(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("s11", [NumberNode("1"), NumberNode("1")])
        # act
        unit = manager._unit_for_probe(node, {})
        # assert — s-params have no implicit unit fallback
        assert unit == ""

    def test_h_network_param_not_in_context_returns_empty(self):
        # arrange
        manager = self._manager()
        node = FunctionCallNode("h11", [NumberNode("1"), NumberNode("1")])
        # act
        unit = manager._unit_for_probe(node, {})
        # assert
        assert unit == ""


# ---------------------------------------------------------------------------
# TestProbeKey
# ---------------------------------------------------------------------------


class TestProbeKey:

    def test_identifier_args_joined_with_comma_space(self):
        # arrange
        node = FunctionCallNode("v", [IdentifierNode("a"), IdentifierNode("b")])
        # act
        key = ExpressionManager._probe_key(node)
        # assert
        assert key == "v(a, b)"

    def test_number_args(self):
        # arrange
        node = FunctionCallNode("s11", [NumberNode("1"), NumberNode("1")])
        # act
        key = ExpressionManager._probe_key(node)
        # assert
        assert key == "s11(1, 1)"

    def test_no_args(self):
        # arrange
        node = FunctionCallNode("v", [])
        # act
        key = ExpressionManager._probe_key(node)
        # assert
        assert key == "v()"

    def test_unsupported_arg_type_renders_empty_string(self):
        # arrange — nested function call as argument
        node = FunctionCallNode("v", [FunctionCallNode("x", [])])
        # act
        key = ExpressionManager._probe_key(node)
        # assert
        assert key == "v()"


# ---------------------------------------------------------------------------
# TestProbeArgText
# ---------------------------------------------------------------------------


class TestProbeArgText:

    def test_identifier_node_returns_name(self):
        # arrange
        node = IdentifierNode("out")
        # act / assert
        assert ExpressionManager._probe_arg_text(node) == "out"

    def test_number_node_returns_text(self):
        # arrange
        node = NumberNode("42")
        # act / assert
        assert ExpressionManager._probe_arg_text(node) == "42"

    def test_unsupported_node_returns_empty_string(self):
        # arrange
        node = FunctionCallNode("nested", [])
        # act / assert
        assert ExpressionManager._probe_arg_text(node) == ""


# ---------------------------------------------------------------------------
# TestPropagateBinaryUnit
# ---------------------------------------------------------------------------


class TestPropagateBinaryUnit:

    def test_add_same_units_returns_unit(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.ADD, "V") == "V"

    def test_add_different_units_returns_empty(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.ADD, "A") == ""

    def test_sub_same_units_returns_unit(self):
        assert ExpressionManager._propagate_binary_unit("A", BinaryOperator.SUB, "A") == "A"

    def test_sub_different_units_returns_empty(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.SUB, "A") == ""

    def test_mul_v_times_a_returns_watts(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.MUL, "A") == "W"

    def test_mul_a_times_v_returns_watts(self):
        assert ExpressionManager._propagate_binary_unit("A", BinaryOperator.MUL, "V") == "W"

    def test_mul_s_times_v_returns_amps(self):
        assert ExpressionManager._propagate_binary_unit("S", BinaryOperator.MUL, "V") == "A"

    def test_mul_v_times_s_returns_amps(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.MUL, "S") == "A"

    def test_mul_dimensionless_left_returns_right_unit(self):
        assert ExpressionManager._propagate_binary_unit("", BinaryOperator.MUL, "V") == "V"

    def test_mul_left_unit_wins_when_no_special_case(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.MUL, "Ω") == "V"

    def test_div_same_units_returns_dimensionless(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.DIV, "V") == ""

    def test_div_v_over_a_returns_ohm(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.DIV, "A") == "Ω"

    def test_div_a_over_v_returns_siemens(self):
        assert ExpressionManager._propagate_binary_unit("A", BinaryOperator.DIV, "V") == "S"

    def test_div_left_unit_over_dimensionless_preserves_left(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.DIV, "") == "V"

    def test_div_dimensionless_over_s_returns_ohm(self):
        assert ExpressionManager._propagate_binary_unit("", BinaryOperator.DIV, "S") == "Ω"

    def test_div_dimensionless_over_ohm_returns_siemens(self):
        assert ExpressionManager._propagate_binary_unit("", BinaryOperator.DIV, "Ω") == "S"

    def test_div_dimensionless_over_seconds_returns_hertz(self):
        assert ExpressionManager._propagate_binary_unit("", BinaryOperator.DIV, "s") == "Hz"

    def test_div_dimensionless_over_hz_returns_seconds(self):
        assert ExpressionManager._propagate_binary_unit("", BinaryOperator.DIV, "Hz") == "s"

    def test_div_other_combination_returns_empty(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.DIV, "W") == ""

    def test_pow_returns_dimensionless(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.POW, "V") == ""

    def test_mod_returns_dimensionless(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.MOD, "V") == ""

    def test_logical_or_returns_dimensionless(self):
        assert ExpressionManager._propagate_binary_unit("V", BinaryOperator.LOGICAL_OR, "V") == ""


# ---------------------------------------------------------------------------
# TestFunctionUnit
# ---------------------------------------------------------------------------


class TestFunctionUnit:

    def test_db_returns_decibels(self):
        assert ExpressionManager._function_unit("db", "V") == "dB"

    def test_angle_returns_degrees(self):
        assert ExpressionManager._function_unit("angle", "V") == "°"

    def test_ph_returns_degrees(self):
        assert ExpressionManager._function_unit("ph", "V") == "°"

    def test_phase_returns_degrees(self):
        assert ExpressionManager._function_unit("phase", "V") == "°"

    def test_abs_preserves_arg_unit(self):
        assert ExpressionManager._function_unit("abs", "V") == "V"

    def test_real_preserves_arg_unit(self):
        assert ExpressionManager._function_unit("real", "A") == "A"

    def test_imag_preserves_arg_unit(self):
        assert ExpressionManager._function_unit("imag", "A") == "A"

    def test_mag_preserves_arg_unit(self):
        assert ExpressionManager._function_unit("mag", "V") == "V"

    def test_conj_preserves_arg_unit(self):
        assert ExpressionManager._function_unit("conj", "V") == "V"

    def test_uramp_preserves_arg_unit(self):
        assert ExpressionManager._function_unit("uramp", "V") == "V"

    def test_round_preserves_arg_unit(self):
        assert ExpressionManager._function_unit("round", "V") == "V"

    def test_floor_preserves_arg_unit(self):
        assert ExpressionManager._function_unit("floor", "V") == "V"

    def test_ceil_preserves_arg_unit(self):
        assert ExpressionManager._function_unit("ceil", "V") == "V"

    def test_int_preserves_arg_unit(self):
        assert ExpressionManager._function_unit("int", "V") == "V"

    def test_sqrt_returns_dimensionless(self):
        assert ExpressionManager._function_unit("sqrt", "V") == ""

    def test_sin_returns_dimensionless(self):
        assert ExpressionManager._function_unit("sin", "V") == ""

    def test_log_returns_dimensionless(self):
        assert ExpressionManager._function_unit("log", "V") == ""

    def test_unknown_function_returns_dimensionless(self):
        assert ExpressionManager._function_unit("unknown_fn", "V") == ""


# ---------------------------------------------------------------------------
# TestFunctionUnitMulti
# ---------------------------------------------------------------------------


class TestFunctionUnitMulti:

    def test_min_preserves_first_arg_unit(self):
        assert ExpressionManager._function_unit_multi("min", "V") == "V"

    def test_max_preserves_first_arg_unit(self):
        assert ExpressionManager._function_unit_multi("max", "A") == "A"

    def test_limit_preserves_first_arg_unit(self):
        assert ExpressionManager._function_unit_multi("limit", "V") == "V"

    def test_other_function_returns_dimensionless(self):
        assert ExpressionManager._function_unit_multi("atan2", "V") == ""

    def test_unknown_function_returns_dimensionless(self):
        assert ExpressionManager._function_unit_multi("custom", "A") == ""
