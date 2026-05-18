from __future__ import annotations

from dataclasses import dataclass
from enum import Enum


class TokenKind(Enum):

    DIRECTIVE = "DIRECTIVE"
    IDENTIFIER = "IDENTIFIER"
    NUMBER = "NUMBER"
    LPAREN = "LPAREN"
    RPAREN = "RPAREN"
    LBRACE = "LBRACE"
    RBRACE = "RBRACE"
    COMMA = "COMMA"
    QUESTION = "QUESTION"
    COLON = "COLON"
    AT = "AT"
    PLUS = "PLUS"
    MINUS = "MINUS"
    STAR = "STAR"
    SLASH = "SLASH"
    PERCENT = "PERCENT"
    CARET = "CARET"
    TILDE = "TILDE"
    BANG = "BANG"
    AMPERSAND = "AMPERSAND"
    PIPE = "PIPE"
    LOGICAL_AND = "LOGICAL_AND"
    LOGICAL_OR = "LOGICAL_OR"
    EQUAL_EQUAL = "EQUAL_EQUAL"
    BANG_EQUAL = "BANG_EQUAL"
    LESS = "LESS"
    LESS_EQUAL = "LESS_EQUAL"
    GREATER = "GREATER"
    GREATER_EQUAL = "GREATER_EQUAL"
    POWER = "POWER"
    EOF = "EOF"


@dataclass(frozen=True)
class Token:

    kind: TokenKind
    text: str
    start: int
    end: int


__all__ = ["Token", "TokenKind"]
