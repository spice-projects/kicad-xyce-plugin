from __future__ import annotations

from dataclasses import dataclass
from enum import Enum


class BinaryOperator(Enum):

    LOGICAL_OR = "||"
    LOGICAL_XOR = "^"
    LOGICAL_AND = "&&"
    EQUAL = "=="
    NOT_EQUAL = "!="
    LESS = "<"
    LESS_EQUAL = "<="
    GREATER = ">"
    GREATER_EQUAL = ">="
    ADD = "+"
    SUB = "-"
    MUL = "*"
    DIV = "/"
    MOD = "%"
    POW = "**"


class UnaryOperator(Enum):

    POS = "+"
    NEG = "-"
    NOT = "!"


@dataclass(frozen=True)
class NumberNode:

    text: str


@dataclass(frozen=True)
class IdentifierNode:

    name: str


@dataclass(frozen=True)
class FunctionCallNode:

    name: str
    args: list[ExpressionNode]


@dataclass(frozen=True)
class UnaryOperationNode:

    operator: UnaryOperator
    operand: ExpressionNode


@dataclass(frozen=True)
class BinaryOperationNode:

    left: ExpressionNode
    operator: BinaryOperator
    right: ExpressionNode


@dataclass(frozen=True)
class TernaryOperationNode:

    condition: ExpressionNode
    if_true: ExpressionNode
    if_false: ExpressionNode


@dataclass(frozen=True)
class StepSelectorNode:
    """Postfix step selector: base@N selects the data slice for step N (1-based)."""

    base: "ExpressionNode"
    step_index: int


@dataclass(frozen=True)
class FunctionDefinitionNode:

    name: str
    params: tuple[str, ...]
    body: "ExpressionNode"


ExpressionNode = NumberNode | IdentifierNode | FunctionCallNode | UnaryOperationNode | BinaryOperationNode | TernaryOperationNode | StepSelectorNode


__all__ = [
    "BinaryOperationNode",
    "BinaryOperator",
    "ExpressionNode",
    "FunctionCallNode",
    "FunctionDefinitionNode",
    "IdentifierNode",
    "NumberNode",
    "StepSelectorNode",
    "TernaryOperationNode",
    "UnaryOperationNode",
    "UnaryOperator",
]
