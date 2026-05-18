import logging

import numpy as np

from .expression import Expression
from .evaluator import XyceEvaluator
from .nodes import BinaryOperationNode, BinaryOperator, ExpressionNode, FunctionCallNode, IdentifierNode, NumberNode, StepSelectorNode, TernaryOperationNode, UnaryOperationNode
from .parser import XyceParser
from .probe_names import is_network_parameter_probe_name

logger = logging.getLogger(__name__)


class ExpressionManager:

    _CONSTANT_UNITS: dict[str, str] = {
        "e": "",
        "f": "",
        "g": "",
        "j": "",
        "k": "",
        "m": "",
        "meg": "",
        "mho": "S",
        "mil": "",
        "n": "",
        "p": "",
        "pi": "",
        "s": "s",
        "t": "",
        "u": "",
    }

    def __init__(self, expressions: list[Expression], step_slices: tuple[slice, ...] | None = None):
        # create expression context; keys are lowercased so that evaluate() lookups always match
        self._context: dict[str, Expression] = {expression.name.lower(): expression for expression in expressions}
        # initialize the qspice language parser and evaluator for expression data
        self._parser = XyceParser()
        self._evaluator = XyceEvaluator()
        # store optional step slice metadata for @N selector evaluation
        self._step_slices = step_slices

    @property
    def expressions(self) -> list[Expression]:
        # do not show calculated expressions in the list of expressions
        return list(self._context.values())

    @property
    def step_slices(self) -> tuple[slice, ...] | None:
        return self._step_slices

    def evaluate(self, expression: str, name: str | None = None) -> Expression | None:
        # context key
        key = (name or expression).lower()
        # check expression has been evaluated before
        result = self._context.get(key, None)
        if result is None:
            try:
                # parse the expression string into a qspice AST
                ast = self._parser.parse_expression(expression)
                # build qspice variable context from cached expressions
                data_context = {context_key: context_expression.data for context_key, context_expression in self._context.items()}
                # evaluate qspice AST to get computed numeric data; pass step slice metadata for @N support
                evaluated_data = self._evaluator.evaluate(ast, data_context, step_slices=self._step_slices)
                # re-materialize step-selector results to full stepped layout when necessary
                evaluated_data = self._rematerialize(evaluated_data, ast)
                # build unit lookup context from cached expressions
                unit_context = {context_key: context_expression.unit for context_key, context_expression in self._context.items()}
                # infer propagated unit from the qspice AST
                inferred_unit = self._infer_unit(ast, unit_context)
                # normalize expression name
                result_name = name if name is not None else self._format_expression(ast)
                # build the final expression using qspice data and qspice-based unit propagation
                result = Expression(result_name, np.asarray(evaluated_data), inferred_unit, source="expression manager")
                # update context with the evaluated expression for future reference
                self._context[key] = result
            except ValueError as e:
                # log information
                logger.warning("Failed to evaluate expression %r: %s", expression, e)
        # exit
        return result

    def _rematerialize(self, data: np.ndarray, ast: ExpressionNode) -> np.ndarray:
        """Re-materialize a step-selector result to full stepped layout.

        When an expression contains @N selectors it may produce a step-length
        vector instead of a full total-points array.  This method detects that
        case and tiles the result across all step blocks so that the output
        length matches the total point count expected by charting and FFT code.
        """
        # nothing to do when there are no step slices or when data is scalar
        if self._step_slices is None or data.ndim == 0:
            return data
        # compute the total number of points
        total_points = sum(s.stop - s.start for s in self._step_slices)
        # return data as-is when it already has the expected full length
        if len(data) == total_points:
            return data
        # check whether the expression tree contains any step selector nodes (including inside function bodies)
        if not self._has_step_selector(ast):
            return data
        # determine the step length from the first slice
        step_length = self._step_slices[0].stop - self._step_slices[0].start
        # reject data that does not match any recognizable step length
        if len(data) != step_length:
            return data
        # tile the single-step result across all steps to produce the full-length array
        return np.tile(data, len(self._step_slices))

    @staticmethod
    def _has_step_selector(node: ExpressionNode) -> bool:
        """Return True when the AST contains at least one StepSelectorNode."""
        # direct match
        if isinstance(node, StepSelectorNode):
            return True
        # recurse into unary operands
        if isinstance(node, UnaryOperationNode):
            return ExpressionManager._has_step_selector(node.operand)
        # recurse into binary operands
        if isinstance(node, BinaryOperationNode):
            return ExpressionManager._has_step_selector(node.left) or ExpressionManager._has_step_selector(node.right)
        # recurse into ternary branches
        if isinstance(node, TernaryOperationNode):
            return (ExpressionManager._has_step_selector(node.condition) or ExpressionManager._has_step_selector(node.if_true) or ExpressionManager._has_step_selector(node.if_false))
        # recurse into function call arguments
        if isinstance(node, FunctionCallNode):
            return any(ExpressionManager._has_step_selector(arg) for arg in node.args)
        # base cases: literals and identifiers have no selectors
        return False

    def _infer_unit(self, node: ExpressionNode, unit_context: dict[str, str]) -> str:
        # numeric literals are dimensionless
        if isinstance(node, NumberNode):
            return ""
        # resolve identifier unit from context or built-in constants
        if isinstance(node, IdentifierNode):
            return self._unit_for_identifier(node.name, unit_context)
        # unary operators preserve the operand unit
        if isinstance(node, UnaryOperationNode):
            return self._infer_unit(node.operand, unit_context)
        # infer unit for binary operators from both sides
        if isinstance(node, BinaryOperationNode):
            left_unit = self._infer_unit(node.left, unit_context)
            right_unit = self._infer_unit(node.right, unit_context)
            return self._propagate_binary_unit(left_unit, node.operator, right_unit)
        # infer unit for function calls from the first argument by convention
        if isinstance(node, FunctionCallNode):
            function_name = node.name.casefold()
            # resolve probe units using probe references
            if function_name in ("v", "i", "id"):
                return self._unit_for_probe(node, unit_context)
            if is_network_parameter_probe_name(function_name) and self._probe_key(node).casefold() in unit_context:
                return self._unit_for_probe(node, unit_context)
            # dimensionless fallback for nullary calls
            if len(node.args) == 0:
                return ""
            # infer first argument unit
            first_arg_unit = self._infer_unit(node.args[0], unit_context)
            # infer unit for unary function calls
            if len(node.args) == 1:
                return self._function_unit(function_name, first_arg_unit)
            # infer unit for multi-argument function calls
            return self._function_unit_multi(function_name, first_arg_unit)
        # infer a common unit for ternary branches
        if isinstance(node, TernaryOperationNode):
            true_unit = self._infer_unit(node.if_true, unit_context)
            false_unit = self._infer_unit(node.if_false, unit_context)
            return true_unit if true_unit == false_unit else ""
        # step selector does not change the unit of its base expression
        if isinstance(node, StepSelectorNode):
            return self._infer_unit(node.base, unit_context)
        # default to dimensionless for unsupported nodes
        return ""

    def _format_expression(self, node: ExpressionNode) -> str:
        # format numeric literals
        if isinstance(node, NumberNode):
            return node.text
        # format identifiers
        if isinstance(node, IdentifierNode):
            return node.name
        # format unary expressions
        if isinstance(node, UnaryOperationNode):
            return node.operator.value + self._format_expression(node.operand)
        # format binary expressions with explicit grouping
        if isinstance(node, BinaryOperationNode):
            return "(" + self._format_expression(node.left) + node.operator.value + self._format_expression(node.right) + ")"
        # format function calls
        if isinstance(node, FunctionCallNode):
            return node.name + "(" + ",".join(self._format_expression(arg) for arg in node.args) + ")"
        # format ternary expressions with explicit grouping
        if isinstance(node, TernaryOperationNode):
            return "(" + self._format_expression(node.condition) + "?" + self._format_expression(node.if_true) + ":" + self._format_expression(node.if_false) + ")"
        # format step selectors as base@N
        if isinstance(node, StepSelectorNode):
            return self._format_expression(node.base) + "@" + str(node.step_index)
        # fallback for unknown nodes
        return ""

    def _unit_for_identifier(self, name: str, unit_context: dict[str, str]) -> str:
        # normalize the identifier lookup key
        key = name.casefold()
        # resolve context expression unit
        unit = unit_context.get(key)
        if unit is not None:
            return unit
        # resolve built-in constant unit
        constant_unit = self._CONSTANT_UNITS.get(key)
        if constant_unit is not None:
            return constant_unit
        # fallback to dimensionless
        return ""

    def _unit_for_probe(self, probe: FunctionCallNode, unit_context: dict[str, str]) -> str:
        # reconstruct the probe key used by the context
        probe_key = self._probe_key(probe).casefold()
        # resolve a directly stored probe unit
        direct_unit = unit_context.get(probe_key)
        if direct_unit not in (None, ""):
            return direct_unit
        # infer known probe family units
        lower_name = probe.name.casefold()
        # voltage
        if lower_name == "v":
            return "V"
        # current
        if lower_name in ("i", "id"):
            return "A"
        # network parameters (Z, Y, S, H)
        if is_network_parameter_probe_name(lower_name):
            # impedance
            if lower_name.startswith("z"):
                return "Ω"
            # admittance
            if lower_name.startswith("y"):
                return "S"
        # no unit
        return ""

    @staticmethod
    def _probe_key(probe: FunctionCallNode) -> str:
        # reconstruct probe text with normalized comma spacing
        args = ", ".join(ExpressionManager._probe_arg_text(arg) for arg in probe.args)
        return f"{probe.name}({args})"

    @staticmethod
    def _probe_arg_text(arg: ExpressionNode) -> str:
        # serialize identifier arguments directly
        if isinstance(arg, IdentifierNode):
            return arg.name
        # serialize numeric arguments directly
        if isinstance(arg, NumberNode):
            return arg.text
        # unsupported argument forms are rendered empty
        return ""

    @staticmethod
    def _propagate_binary_unit(left_unit: str, operator: BinaryOperator, right_unit: str) -> str:
        # addition and subtraction require matching units
        if operator in (BinaryOperator.ADD, BinaryOperator.SUB):
            return left_unit if left_unit == right_unit else ""
        # multiplication handles common electrical identities
        if operator == BinaryOperator.MUL:
            if {left_unit, right_unit} == {"V", "A"}:
                return "W"
            if {left_unit, right_unit} == {"S", "V"}:
                return "A"
            if left_unit == "":
                return right_unit
            return left_unit
        # division handles common reciprocal electrical identities
        if operator == BinaryOperator.DIV:
            if left_unit == right_unit:
                return ""
            if left_unit == "V" and right_unit == "A":
                return "Ω"
            if left_unit == "A" and right_unit == "V":
                return "S"
            if right_unit == "":
                return left_unit
            if left_unit == "":
                if right_unit == "S":
                    return "Ω"
                if right_unit == "Ω":
                    return "S"
                if right_unit == "s":
                    return "Hz"
                if right_unit == "Hz":
                    return "s"
            return ""
        # all other operators are treated as dimensionless
        return ""

    @staticmethod
    def _function_unit(function_name: str, arg_unit: str) -> str:
        # db always returns decibels
        if function_name == "db":
            return "dB"
        # angle aliases return degrees
        if function_name in ("angle", "ph", "phase"):
            return "°"
        # these functions preserve their argument unit
        if function_name in ("abs", "real", "imag", "mag", "conj", "uramp", "round", "floor", "ceil", "int"):
            return arg_unit
        # all remaining unary functions are dimensionless
        return ""

    @staticmethod
    def _function_unit_multi(function_name: str, first_arg_unit: str) -> str:
        # min, max, and limit preserve the first argument unit
        if function_name in ("min", "max", "limit"):
            return first_arg_unit
        # all remaining multi-argument functions are dimensionless
        return ""
