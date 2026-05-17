from dataclasses import dataclass
from typing import Any

import numpy as np

from .builtins import BUILTIN_CONSTANTS, BUILTIN_FUNCTIONS, XyceValue
from .nodes import BinaryOperationNode, BinaryOperator, ExpressionNode, FunctionCallNode, FunctionDefinitionNode, IdentifierNode, NumberNode, StepSelectorNode, TernaryOperationNode, UnaryOperationNode, UnaryOperator
from .probe_names import is_network_parameter_probe_name


_NUMBER_SUFFIXES: dict[str, float] = {
    "T": 1e12,
    "G": 1e9,
    "MEG": 1e6,
    "K": 1e3,
    "M": 1e-3,
    "U": 1e-6,
    "N": 1e-9,
    "P": 1e-12,
    "F": 1e-15,
    "MIL": 25.4e-6,
}


@dataclass(frozen=True)
class EvaluationContext:

    variables: dict[str, XyceValue]
    functions: dict[str, FunctionDefinitionNode]
    constants: dict[str, XyceValue]
    # optional step slices for @N selector support; each slice gives the index range for one step
    step_slices: tuple[slice, ...] | None = None


class XyceEvaluator:

    def evaluate(self, expression: ExpressionNode, variables: dict[str, Any] | None = None, functions: dict[str, FunctionDefinitionNode] | None = None, constants: dict[str, Any] | None = None, step_slices: tuple[slice, ...] | None = None) -> Any:
        # build the evaluation context
        context = EvaluationContext(self._normalize_value_mapping(variables), self._normalize_function_mapping(functions), self._normalize_value_mapping(constants, BUILTIN_CONSTANTS), step_slices)
        # evaluate the expression tree
        result = self._evaluate(expression, context, ())
        # convert the internal value to a public result
        return self._to_public_value(result)

    def _evaluate(self, expression: ExpressionNode, context: EvaluationContext, call_stack: tuple[str, ...]) -> XyceValue:
        # evaluate a numeric literal
        if isinstance(expression, NumberNode):
            return self._parse_number(expression.text)
        # evaluate an identifier reference
        if isinstance(expression, IdentifierNode):
            return self._lookup_name(expression.name, context)
        # evaluate a unary operation
        if isinstance(expression, UnaryOperationNode):
            return self._evaluate_unary(expression, context, call_stack)
        # evaluate a binary operation
        if isinstance(expression, BinaryOperationNode):
            return self._evaluate_binary(expression, context, call_stack)
        # evaluate a ternary operation
        if isinstance(expression, TernaryOperationNode):
            return self._evaluate_ternary(expression, context, call_stack)
        # evaluate a function call
        if isinstance(expression, FunctionCallNode):
            return self._evaluate_function_call(expression, context, call_stack)
        # evaluate a step selector (@N)
        if isinstance(expression, StepSelectorNode):
            return self._evaluate_step_selector(expression, context, call_stack)
        # fail on an unsupported node type
        raise ValueError(f"Unsupported expression node: {type(expression).__name__}")

    def _evaluate_unary(self, expression: UnaryOperationNode, context: EvaluationContext, call_stack: tuple[str, ...]) -> XyceValue:
        # evaluate the operand value
        value = self._evaluate(expression.operand, context, call_stack)
        # apply unary plus
        if expression.operator == UnaryOperator.POS:
            return value
        # apply unary minus
        if expression.operator == UnaryOperator.NEG:
            return -value
        # apply logical negation
        if expression.operator == UnaryOperator.NOT:
            return self._boolean_result(~self._truth_mask(value))
        # fail on an unsupported unary operator
        raise ValueError(f"Unsupported unary operator: {expression.operator.value}")

    def _evaluate_binary(self, expression: BinaryOperationNode, context: EvaluationContext, call_stack: tuple[str, ...]) -> XyceValue:
        # short-circuit logical and
        if expression.operator == BinaryOperator.LOGICAL_AND:
            left_value = self._evaluate(expression.left, context, call_stack)
            return self._evaluate_logical_and(left_value, expression.right, context, call_stack)
        # short-circuit logical or
        if expression.operator == BinaryOperator.LOGICAL_OR:
            left_value = self._evaluate(expression.left, context, call_stack)
            return self._evaluate_logical_or(left_value, expression.right, context, call_stack)
        # evaluate the left-hand side
        left_value = self._evaluate(expression.left, context, call_stack)
        # evaluate the right-hand side
        right_value = self._evaluate(expression.right, context, call_stack)
        # apply addition
        if expression.operator == BinaryOperator.ADD:
            return left_value + right_value
        # apply subtraction
        if expression.operator == BinaryOperator.SUB:
            return left_value - right_value
        # apply multiplication
        if expression.operator == BinaryOperator.MUL:
            return left_value * right_value
        # apply division
        if expression.operator == BinaryOperator.DIV:
            return left_value / right_value
        # apply exponentiation
        if expression.operator == BinaryOperator.POW:
            return left_value ** right_value
        # apply equality comparison
        if expression.operator == BinaryOperator.EQUAL:
            return self._boolean_result(left_value == right_value)
        # apply inequality comparison
        if expression.operator == BinaryOperator.NOT_EQUAL:
            return self._boolean_result(left_value != right_value)
        # apply less-than comparison
        if expression.operator == BinaryOperator.LESS:
            return self._boolean_result(np.real(left_value) < np.real(right_value))
        # apply less-than-or-equal comparison
        if expression.operator == BinaryOperator.LESS_EQUAL:
            return self._boolean_result(np.real(left_value) <= np.real(right_value))
        # apply greater-than comparison
        if expression.operator == BinaryOperator.GREATER:
            return self._boolean_result(np.real(left_value) > np.real(right_value))
        # apply greater-than-or-equal comparison
        if expression.operator == BinaryOperator.GREATER_EQUAL:
            return self._boolean_result(np.real(left_value) >= np.real(right_value))
        # apply logical xor (bitwise xor of truth masks)
        if expression.operator == BinaryOperator.LOGICAL_XOR:
            return self._boolean_result(self._truth_mask(left_value) ^ self._truth_mask(right_value))
        # apply modulo
        if expression.operator == BinaryOperator.MOD:
            return np.mod(np.real(left_value), np.real(right_value))
        # fail on an unsupported binary operator
        raise ValueError(f"Unsupported binary operator: {expression.operator.value}")

    def _evaluate_ternary(self, expression: TernaryOperationNode, context: EvaluationContext, call_stack: tuple[str, ...]) -> XyceValue:
        # evaluate the condition value
        condition = self._evaluate(expression.condition, context, call_stack)
        # branch directly for scalar conditions
        if self._is_scalar_value(condition):
            if bool(condition.item()):
                return self._evaluate(expression.if_true, context, call_stack)
            return self._evaluate(expression.if_false, context, call_stack)
        # evaluate the true branch
        if_true = self._evaluate(expression.if_true, context, call_stack)
        # evaluate the false branch
        if_false = self._evaluate(expression.if_false, context, call_stack)
        # select element-wise between both branches
        return np.where(self._truth_mask(condition), if_true, if_false)

    def _evaluate_function_call(self, expression: FunctionCallNode, context: EvaluationContext, call_stack: tuple[str, ...]) -> XyceValue:
        # normalize the function name
        function_name = expression.name.casefold()
        # check for probe-style references stored directly in the variable context
        if self._is_probe_call(expression, context):
            return self._evaluate_probe(expression, context, call_stack)
        # look up a builtin implementation
        builtin = BUILTIN_FUNCTIONS.get(function_name)
        if builtin is not None:
            # evaluate builtin arguments
            args = [self._evaluate(arg, context, call_stack) for arg in expression.args]
            return builtin(args)
        # look up a user-defined function
        definition = context.functions.get(function_name)
        if definition is None:
            raise ValueError(f"Unknown function: {expression.name}")
        # reject recursive calls
        if function_name in call_stack:
            raise ValueError(f"Recursive function call detected: {expression.name}")
        # validate the argument count
        if len(expression.args) != len(definition.params):
            raise ValueError(f"Function {expression.name!r} expects {len(definition.params)} arguments, got {len(expression.args)}")
        # seed the local variable scope
        local_variables = dict(context.variables)
        # bind argument values to parameter names
        for param, arg in zip(definition.params, expression.args):
            local_variables[param.casefold()] = self._evaluate(arg, context, call_stack)
        # build the local evaluation context; propagate step_slices, functions, and constants so @N and nested calls work inside function bodies
        local_context = EvaluationContext(local_variables, context.functions, context.constants, context.step_slices)
        # evaluate the function body
        return self._evaluate(definition.body, local_context, call_stack + (function_name,))

    def _is_probe_call(self, expression: FunctionCallNode, context: EvaluationContext) -> bool:
        # normalize the function name
        function_name = expression.name.casefold()
        # canonical SPICE probe families are always treated as probes
        if function_name in ("v", "i") and len(expression.args) > 0:
            return True
        # only two-digit S/Z/Y/H names are treated as network-parameter probes
        if not is_network_parameter_probe_name(function_name):
            return False
        # probe-style network parameters accept only simple identifier/number arguments
        if not self._has_simple_probe_args(expression):
            return False
        # reconstruct the stored variable key and resolve it directly from the context
        probe_key = self._reconstruct_probe_name(expression).casefold()
        # it must be in the variables to be treated as a probe reference
        return probe_key in context.variables

    @staticmethod
    def _has_simple_probe_args(expression: FunctionCallNode) -> bool:
        # only identifiers and numeric literals are valid probe-node arguments
        for arg in expression.args:
            # arguments must be simple identifiers or numbers; reject complex expressions to avoid ambiguity with function calls and ensure consistent probe naming
            if not isinstance(arg, (IdentifierNode, NumberNode)):
                return False
        # ok
        return True

    def _evaluate_probe(self, expression: FunctionCallNode, context: EvaluationContext, call_stack: tuple[str, ...]) -> XyceValue:
        # reconstruct the probe reference name from arguments
        probe_name = self._reconstruct_probe_name(expression)
        # try to find the probe directly in variables
        probe_key = probe_name.casefold()
        if probe_key in context.variables:
            return context.variables[probe_key]
        # if probe has two arguments, try differential decomposition: V(a, b) = V(a) - V(b)
        if len(expression.args) == 2:
            # extract the two node names
            node_a_name = self._extract_node_name(expression.args[0])
            node_b_name = self._extract_node_name(expression.args[1])
            # handle ground as second argument: V(a, 0) = V(a)
            if node_b_name == "0" or node_b_name.lower() == "0":
                # return the first node directly
                probe_a_key = ("v(" + node_a_name + ")").casefold()
                if probe_a_key in context.variables:
                    return context.variables[probe_a_key]
            # handle ground as first argument: V(0, b) = -V(b)
            if node_a_name == "0":
                probe_b_key = ("v(" + node_b_name + ")").casefold()
                if probe_b_key in context.variables:
                    return -context.variables[probe_b_key]
            # try to find both single-node probes for differential
            probe_a_key = ("v(" + node_a_name + ")").casefold()
            probe_b_key = ("v(" + node_b_name + ")").casefold()
            # look up both values
            value_a = context.variables.get(probe_a_key)
            value_b = context.variables.get(probe_b_key) if node_b_name != "0" else np.asarray(0.0)
            # return differential if both probes exist
            if value_a is not None:
                if value_b is not None:
                    return value_a - value_b
                # probe_a exists but probe_b doesn't (e.g., ground)
                return value_a
        # fail if probe not found
        raise ValueError(f"Unknown probe: {probe_name}")

    def _evaluate_step_selector(self, expression: StepSelectorNode, context: EvaluationContext, call_stack: tuple[str, ...]) -> XyceValue:
        # require step slice information in the evaluation context
        if context.step_slices is None:
            raise ValueError("Step selector @N requires step metadata in the evaluation context")
        # validate the step index against the available slices (1-based)
        num_steps = len(context.step_slices)
        if expression.step_index < 1 or expression.step_index > num_steps:
            raise ValueError(f"Step selector @{expression.step_index} is out of range: file has {num_steps} step(s)")
        # evaluate the base expression using the full data context
        base_value = self._evaluate(expression.base, context, call_stack)
        # extract the step slice (convert from 1-based to 0-based index)
        step_slice = context.step_slices[expression.step_index - 1]
        # extract and return the selected step data
        return base_value[step_slice]

    @staticmethod
    def _reconstruct_probe_name(expression: FunctionCallNode) -> str:
        # reconstruct probe name from function call structure
        probe_prefix = expression.name.upper()
        # rebuild the argument list
        args_str = ", ".join(XyceEvaluator._node_name_from_expr(arg) for arg in expression.args)
        # format as probe reference
        return f"{probe_prefix}({args_str})"

    @staticmethod
    def _node_name_from_expr(expr: ExpressionNode) -> str:
        # extract a node name from an expression (should be simple identifier or number)
        if isinstance(expr, IdentifierNode):
            return expr.name
        elif isinstance(expr, NumberNode):
            return expr.text
        else:
            # fallback for complex expressions
            raise ValueError(f"Cannot extract node name from complex expression: {expr}")

    @staticmethod
    def _extract_node_name(expr: ExpressionNode) -> str:
        # extract just the node identifier
        if isinstance(expr, IdentifierNode):
            return expr.name
        elif isinstance(expr, NumberNode):
            return expr.text
        else:
            raise ValueError(f"Expected identifier or number for node reference, got {type(expr).__name__}")

    def _evaluate_logical_and(self, left_value: XyceValue, right_expression: ExpressionNode, context: EvaluationContext, call_stack: tuple[str, ...]) -> XyceValue:
        # short-circuit a false scalar left operand
        if self._is_scalar_value(left_value) and not bool(left_value.item()):
            return np.asarray(0.0)
        # evaluate the right-hand side
        right_value = self._evaluate(right_expression, context, call_stack)
        # combine both truth masks
        return self._boolean_result(self._truth_mask(left_value) & self._truth_mask(right_value))

    def _evaluate_logical_or(self, left_value: XyceValue, right_expression: ExpressionNode, context: EvaluationContext, call_stack: tuple[str, ...]) -> XyceValue:
        # short-circuit a true scalar left operand
        if self._is_scalar_value(left_value) and bool(left_value.item()):
            return np.asarray(1.0)
        # evaluate the right-hand side
        right_value = self._evaluate(right_expression, context, call_stack)
        # combine both truth masks
        return self._boolean_result(self._truth_mask(left_value) | self._truth_mask(right_value))

    def _lookup_name(self, name: str, context: EvaluationContext) -> XyceValue:
        # normalize the identifier name
        key = name.casefold()
        # resolve a variable binding
        if key in context.variables:
            return context.variables[key]
        # resolve a constant binding
        if key in context.constants:
            return context.constants[key]
        # fail on an unknown identifier
        raise ValueError(f"Unknown identifier: {name}")

    @staticmethod
    def _normalize_value_mapping(mapping: dict[str, Any] | None, defaults: dict[str, XyceValue] | None = None) -> dict[str, XyceValue]:
        # initialize the normalized mapping
        normalized: dict[str, XyceValue] = {}
        # seed the mapping with defaults
        if defaults is not None:
            for key, value in defaults.items():
                normalized[key.casefold()] = value
        # exit early when there is no explicit mapping
        if mapping is None:
            return normalized
        # normalize explicit mapping keys
        for key, value in mapping.items():
            normalized[key.casefold()] = XyceEvaluator._normalize_value(value)
        # exit
        return normalized

    @staticmethod
    def _normalize_function_mapping(mapping: dict[str, FunctionDefinitionNode] | None) -> dict[str, FunctionDefinitionNode]:
        # initialize the normalized mapping
        normalized: dict[str, FunctionDefinitionNode] = {}
        # exit early when there is no explicit mapping
        if mapping is None:
            return normalized
        # normalize explicit mapping keys
        for key, value in mapping.items():
            normalized[key.casefold()] = value
        # exit
        return normalized

    @staticmethod
    def _normalize_value(value: Any) -> XyceValue:
        # preserve ndarray inputs
        if isinstance(value, np.ndarray):
            return value
        # normalize scalars to zero-dimensional ndarrays
        return np.asarray(value)

    @staticmethod
    def _is_scalar_value(value: XyceValue) -> bool:
        return value.ndim == 0

    @staticmethod
    def _truth_mask(value: XyceValue) -> np.ndarray:
        return np.asarray(np.real(value) != 0)

    @staticmethod
    def _boolean_result(value: Any) -> XyceValue:
        return np.asarray(value, dtype=float)

    @staticmethod
    def _to_public_value(value: XyceValue) -> Any:
        # unpack scalar results for the public interface
        if value.ndim == 0:
            return value.item()
        # exit
        return value

    @staticmethod
    def _parse_number(text: str) -> XyceValue:
        # normalize the suffix scan input
        upper_text = text.upper()
        # track the matched suffix
        suffix = ""
        # seed the numeric text with the full token
        number_text = upper_text
        # find the longest recognized suffix
        for candidate in sorted(_NUMBER_SUFFIXES, key=len, reverse=True):
            if upper_text.endswith(candidate):
                suffix = candidate
                number_text = text[:-len(candidate)]
                break
        # parse the numeric prefix
        value = float(number_text)
        # apply the suffix multiplier when present
        if suffix:
            return np.asarray(value * _NUMBER_SUFFIXES[suffix])
        # exit
        return np.asarray(value)


def evaluate_expression(expression: ExpressionNode, variables: dict[str, Any] | None = None, functions: dict[str, FunctionDefinitionNode] | None = None, constants: dict[str, Any] | None = None, step_slices: tuple[slice, ...] | None = None) -> Any:
    # evaluate an expression with a fresh evaluator instance
    return XyceEvaluator().evaluate(expression, variables, functions, constants, step_slices)
