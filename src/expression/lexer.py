from __future__ import annotations

from .tokens import Token, TokenKind


_SINGLE_CHAR_TOKENS: dict[str, TokenKind] = {
    "(": TokenKind.LPAREN,
    ")": TokenKind.RPAREN,
    "{": TokenKind.LBRACE,
    "}": TokenKind.RBRACE,
    ",": TokenKind.COMMA,
    "?": TokenKind.QUESTION,
    ":": TokenKind.COLON,
    "@": TokenKind.AT,
    "+": TokenKind.PLUS,
    "-": TokenKind.MINUS,
    "*": TokenKind.STAR,
    "/": TokenKind.SLASH,
    "%": TokenKind.PERCENT,
    "^": TokenKind.CARET,
    "~": TokenKind.TILDE,
    "!": TokenKind.BANG,
    "&": TokenKind.AMPERSAND,
    "|": TokenKind.PIPE,
    "<": TokenKind.LESS,
    ">": TokenKind.GREATER,
}

_DOUBLE_CHAR_TOKENS: dict[str, TokenKind] = {
    "&&": TokenKind.LOGICAL_AND,
    "||": TokenKind.LOGICAL_OR,
    "==": TokenKind.EQUAL_EQUAL,
    "!=": TokenKind.BANG_EQUAL,
    "<=": TokenKind.LESS_EQUAL,
    ">=": TokenKind.GREATER_EQUAL,
    "**": TokenKind.POWER,
}


class XyceLexer:

    def tokenize(self, text: str) -> list[Token]:
        # store the input buffer
        self._text = text
        # cache the input length
        self._length = len(text)
        # reset the scan position
        self._pos = 0
        # collect output tokens
        tokens: list[Token] = []
        # scan until the input is exhausted
        while self._pos < self._length:
            # read the current character
            ch = self._text[self._pos]
            # skip whitespace
            if ch.isspace():
                self._pos += 1
                continue
            # scan a directive token
            token = self._scan_directive()
            if token is not None:
                tokens.append(token)
                continue
            # scan a multi-character operator token
            token = self._scan_multi_char_operator()
            if token is not None:
                tokens.append(token)
                continue
            # scan a numeric literal token
            token = self._scan_number()
            if token is not None:
                tokens.append(token)
                continue
            # scan an identifier token
            token = self._scan_identifier()
            if token is not None:
                tokens.append(token)
                continue
            # look up a single-character token kind
            token_kind = _SINGLE_CHAR_TOKENS.get(ch)
            if token_kind is not None:
                # capture the token start offset
                start = self._pos
                # advance past the token
                self._pos += 1
                # append the scanned token
                tokens.append(Token(token_kind, ch, start, self._pos))
                continue
            # fail on an unsupported character
            raise ValueError(f"Unexpected character {ch!r} at offset {self._pos}")
        # append the end-of-file token
        tokens.append(Token(TokenKind.EOF, "", self._pos, self._pos))
        # exit
        return tokens

    def _scan_directive(self) -> Token | None:
        # skip when the current character does not start a directive
        if self._text[self._pos] != ".":
            return None
        # capture the token start offset
        start = self._pos
        # compute the position of the first directive character
        next_pos = self._pos + 1
        # require an identifier head after the dot
        if next_pos >= self._length or not self._is_identifier_start(self._text[next_pos]):
            raise ValueError(f"Unexpected character '.' at offset {self._pos}")
        # advance past the directive head
        self._pos = next_pos + 1
        # consume the rest of the directive name
        while self._pos < self._length and self._is_identifier_part(self._text[self._pos]):
            self._pos += 1
        # exit
        return Token(TokenKind.DIRECTIVE, self._text[start:self._pos], start, self._pos)

    def _scan_multi_char_operator(self) -> Token | None:
        # skip when fewer than two characters remain
        if self._pos + 1 >= self._length:
            return None
        # read the two-character operator candidate
        text = self._text[self._pos:self._pos + 2]
        # look up the operator token kind
        token_kind = _DOUBLE_CHAR_TOKENS.get(text)
        if token_kind is None:
            return None
        # capture the token start offset
        start = self._pos
        # advance past the operator
        self._pos += 2
        # exit
        return Token(token_kind, text, start, self._pos)

    def _scan_number(self) -> Token | None:
        # capture the token start offset
        start = self._pos
        # read the current character
        ch = self._text[self._pos]
        # skip when the current character cannot start a number
        if not (ch.isdigit() or ch == "."):
            return None
        # track whether at least one digit was consumed
        saw_digit = False
        # consume a leading decimal point only when followed by a digit
        if ch == ".":
            if self._pos + 1 >= self._length or not self._text[self._pos + 1].isdigit():
                return None
            self._pos += 1
        # consume digits before the decimal point
        while self._pos < self._length and self._text[self._pos].isdigit():
            self._pos += 1
            saw_digit = True
        # consume the fractional part
        if self._pos < self._length and self._text[self._pos] == ".":
            self._pos += 1
            while self._pos < self._length and self._text[self._pos].isdigit():
                self._pos += 1
                saw_digit = True
        # reject a token that never consumed a digit
        if not saw_digit:
            return None
        # consume an exponent when present
        if self._pos < self._length and self._text[self._pos] in "eE":
            # compute the start of the exponent digits
            exp_pos = self._pos + 1
            # skip an optional exponent sign
            if exp_pos < self._length and self._text[exp_pos] in "+-":
                exp_pos += 1
            # remember the first exponent digit position
            exp_start = exp_pos
            # consume exponent digits
            while exp_pos < self._length and self._text[exp_pos].isdigit():
                exp_pos += 1
            # reject an exponent with no digits
            if exp_pos == exp_start:
                raise ValueError(f"Invalid exponent at offset {self._pos}")
            # commit the exponent scan
            self._pos = exp_pos
        # alphabetic suffixes are tokenized separately as identifiers for implicit multiplication support
        # exit
        return Token(TokenKind.NUMBER, self._text[start:self._pos], start, self._pos)

    def _scan_identifier(self) -> Token | None:
        # skip when the current character cannot start an identifier
        if not self._is_identifier_start(self._text[self._pos]):
            return None
        # capture the token start offset
        start = self._pos
        # advance past the identifier head
        self._pos += 1
        # consume the remaining identifier characters
        while self._pos < self._length and self._is_identifier_part(self._text[self._pos]):
            self._pos += 1
        # exit
        return Token(TokenKind.IDENTIFIER, self._text[start:self._pos], start, self._pos)

    @staticmethod
    def _is_identifier_start(ch: str) -> bool:
        # Xyce identifiers start with a letter or underscore; standard SPICE node names
        return ch.isalpha() or ch == "_"

    @staticmethod
    def _is_identifier_part(ch: str) -> bool:
        # Xyce identifiers continue with alphanumeric characters, underscores, or square brackets
        return ch.isalnum() or ch in "_[]"


def tokenize(text: str) -> list[Token]:
    # tokenize the input with a fresh lexer instance
    return XyceLexer().tokenize(text)
