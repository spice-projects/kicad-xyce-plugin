from __future__ import annotations

from .lexer import tokenize
from .nodes import BinaryOperationNode, BinaryOperator, ExpressionNode, FunctionCallNode, FunctionDefinitionNode, IdentifierNode, NumberNode, StepSelectorNode, TernaryOperationNode, UnaryOperationNode, UnaryOperator
from .tokens import Token, TokenKind


class XyceParser:

    def parse_expression(self, text: str) -> ExpressionNode:
        # tokenize the input
        self._tokens = tokenize(text)
        # reset the token cursor
        self._pos = 0
        # parse the full expression
        expression = self._parse_ternary()
        # require end-of-input after the expression
        self._consume(TokenKind.EOF)
        # exit
        return expression

    def parse_function_definition(self, text: str) -> FunctionDefinitionNode:
        # tokenize the input
        self._tokens = tokenize(text)
        # reset the token cursor
        self._pos = 0
        # parse the directive token
        directive = self._consume(TokenKind.DIRECTIVE)
        # require a .func directive
        if directive.text.casefold() != ".func":
            raise ValueError(f"Expected .func directive at offset {directive.start}")
        # parse the function name
        name = self._consume(TokenKind.IDENTIFIER).text
        # consume the parameter list opener
        self._consume(TokenKind.LPAREN)
        # parse the parameter list
        params = self._parse_parameter_list()
        # consume the parameter list closer
        self._consume(TokenKind.RPAREN)
        # consume the function body opener
        self._consume(TokenKind.LBRACE)
        # parse the function body
        body = self._parse_ternary()
        # consume the function body closer
        self._consume(TokenKind.RBRACE)
        # require end-of-input after the definition
        self._consume(TokenKind.EOF)
        # exit
        return FunctionDefinitionNode(name, tuple(params), body)

    def _parse_parameter_list(self) -> list[str]:
        # collect parameter names
        params: list[str] = []
        # exit early for an empty parameter list
        if self._peek().kind == TokenKind.RPAREN:
            return params
        # parse comma-separated parameters
        while True:
            # append the next parameter name
            params.append(self._consume(TokenKind.IDENTIFIER).text)
            # exit when the parameter list ends
            if self._peek().kind != TokenKind.COMMA:
                return params
            # consume the parameter separator
            self._consume(TokenKind.COMMA)

    def _parse_ternary(self) -> ExpressionNode:
        # parse the ternary condition
        condition = self._parse_logical_or()
        # exit when there is no ternary operator
        if self._peek().kind != TokenKind.QUESTION:
            return condition
        # consume the ternary marker
        self._consume(TokenKind.QUESTION)
        # parse the true branch
        if_true = self._parse_ternary()
        # consume the false-branch separator
        self._consume(TokenKind.COLON)
        # parse the false branch
        if_false = self._parse_ternary()
        # exit
        return TernaryOperationNode(condition, if_true, if_false)

    def _parse_logical_or(self) -> ExpressionNode:
        # parse the left-hand side
        expression = self._parse_logical_xor()
        # fold any chained logical-or operators (single | or double ||)
        while self._peek().kind in (TokenKind.LOGICAL_OR, TokenKind.PIPE):
            # consume the operator token
            self._consume(self._peek().kind)
            # parse and fold the right-hand side
            expression = BinaryOperationNode(expression, BinaryOperator.LOGICAL_OR, self._parse_logical_xor())
        # exit
        return expression

    def _parse_logical_xor(self) -> ExpressionNode:
        # parse the left-hand side
        expression = self._parse_logical_and()
        # fold any chained logical-xor operators (^)
        while self._peek().kind == TokenKind.CARET:
            # consume the operator token
            self._consume(TokenKind.CARET)
            # parse and fold the right-hand side
            expression = BinaryOperationNode(expression, BinaryOperator.LOGICAL_XOR, self._parse_logical_and())
        # exit
        return expression

    def _parse_logical_and(self) -> ExpressionNode:
        # parse the left-hand side
        expression = self._parse_equality()
        # fold any chained logical-and operators (single & or double &&)
        while self._peek().kind in (TokenKind.LOGICAL_AND, TokenKind.AMPERSAND):
            # consume the operator token
            self._consume(self._peek().kind)
            # parse and fold the right-hand side
            expression = BinaryOperationNode(expression, BinaryOperator.LOGICAL_AND, self._parse_equality())
        # exit
        return expression

    def _parse_equality(self) -> ExpressionNode:
        # parse the left-hand side
        expression = self._parse_relational()
        # fold any equality operators
        while self._peek().kind in (TokenKind.EQUAL_EQUAL, TokenKind.BANG_EQUAL):
            # consume the operator token
            operator = self._consume(self._peek().kind)
            # fold an equality comparison
            if operator.kind == TokenKind.EQUAL_EQUAL:
                expression = BinaryOperationNode(expression, BinaryOperator.EQUAL, self._parse_relational())
            # fold an inequality comparison
            else:
                expression = BinaryOperationNode(expression, BinaryOperator.NOT_EQUAL, self._parse_relational())
        # exit
        return expression

    def _parse_relational(self) -> ExpressionNode:
        # parse the left-hand side
        expression = self._parse_additive()
        # fold any relational operators
        while self._peek().kind in (TokenKind.LESS, TokenKind.LESS_EQUAL, TokenKind.GREATER, TokenKind.GREATER_EQUAL):
            # consume the operator token
            operator = self._consume(self._peek().kind)
            # fold a less-than comparison
            if operator.kind == TokenKind.LESS:
                expression = BinaryOperationNode(expression, BinaryOperator.LESS, self._parse_additive())
            # fold a less-than-or-equal comparison
            elif operator.kind == TokenKind.LESS_EQUAL:
                expression = BinaryOperationNode(expression, BinaryOperator.LESS_EQUAL, self._parse_additive())
            # fold a greater-than comparison
            elif operator.kind == TokenKind.GREATER:
                expression = BinaryOperationNode(expression, BinaryOperator.GREATER, self._parse_additive())
            # fold a greater-than-or-equal comparison
            else:
                expression = BinaryOperationNode(expression, BinaryOperator.GREATER_EQUAL, self._parse_additive())
        # exit
        return expression

    def _parse_additive(self) -> ExpressionNode:
        # parse the left-hand side
        expression = self._parse_multiplicative()
        # fold any additive operators
        while self._peek().kind in (TokenKind.PLUS, TokenKind.MINUS):
            # consume the operator token
            operator = self._consume(self._peek().kind)
            # fold an addition
            if operator.kind == TokenKind.PLUS:
                expression = BinaryOperationNode(expression, BinaryOperator.ADD, self._parse_multiplicative())
            # fold a subtraction
            else:
                expression = BinaryOperationNode(expression, BinaryOperator.SUB, self._parse_multiplicative())
        # exit
        return expression

    def _parse_multiplicative(self) -> ExpressionNode:
        # parse the left-hand side
        expression = self._parse_unary()
        # fold explicit operators (* / %) and implicit SPICE suffix multiplication (e.g. 1K, 10MEG)
        while True:
            # handle explicit multiplicative operators
            if self._peek().kind in (TokenKind.STAR, TokenKind.SLASH, TokenKind.PERCENT):
                # consume the operator token
                operator = self._consume(self._peek().kind)
                # parse the right-hand side
                right = self._parse_unary()
                # fold the operator
                if operator.kind == TokenKind.STAR:
                    expression = BinaryOperationNode(expression, BinaryOperator.MUL, right)
                elif operator.kind == TokenKind.SLASH:
                    expression = BinaryOperationNode(expression, BinaryOperator.DIV, right)
                else:
                    expression = BinaryOperationNode(expression, BinaryOperator.MOD, right)
            # handle implicit multiplication: a primary followed directly by an identifier (e.g. 1K, 1MEG)
            elif self._peek().kind == TokenKind.IDENTIFIER and self._last_was_primary(expression):
                # consume the identifier token and treat it as the right-hand side
                right_token = self._consume(TokenKind.IDENTIFIER)
                expression = BinaryOperationNode(expression, BinaryOperator.MUL, IdentifierNode(right_token.text))
            else:
                # no more operators; exit
                break
        # exit
        return expression

    @staticmethod
    def _last_was_primary(node: ExpressionNode) -> bool:
        # return true if the node is a simple primary (number or identifier)
        if isinstance(node, (NumberNode, IdentifierNode)):
            return True
        # a binary expression ending in a primary also qualifies — handles e.g. "a * 1" followed by "s"
        if isinstance(node, BinaryOperationNode):
            return XyceParser._last_was_primary(node.right)
        return False

    def _parse_unary(self) -> ExpressionNode:
        # parse unary plus
        if self._peek().kind == TokenKind.PLUS:
            self._consume(TokenKind.PLUS)
            return UnaryOperationNode(UnaryOperator.POS, self._parse_unary())
        # parse unary minus
        if self._peek().kind == TokenKind.MINUS:
            self._consume(TokenKind.MINUS)
            return UnaryOperationNode(UnaryOperator.NEG, self._parse_unary())
        # parse logical negation — Xyce uses ~ (TILDE); also accept ! for compatibility
        if self._peek().kind in (TokenKind.TILDE, TokenKind.BANG):
            self._consume(self._peek().kind)
            return UnaryOperationNode(UnaryOperator.NOT, self._parse_unary())
        # exit
        return self._parse_power()

    def _parse_power(self) -> ExpressionNode:
        # parse the base expression
        expression = self._parse_primary()
        # fold a right-associative power operator — Xyce uses ** only (^ is XOR)
        if self._peek().kind == TokenKind.POWER:
            self._consume(TokenKind.POWER)
            return BinaryOperationNode(expression, BinaryOperator.POW, self._parse_unary())
        # exit
        return expression

    def _parse_primary(self) -> ExpressionNode:
        # inspect the current token
        token = self._peek()
        # parse a number literal
        if token.kind == TokenKind.NUMBER:
            return NumberNode(self._consume(TokenKind.NUMBER).text)
        # parse an identifier or function call
        if token.kind == TokenKind.IDENTIFIER:
            # consume the identifier token
            identifier = self._consume(TokenKind.IDENTIFIER).text
            # exit early for a bare identifier
            if self._peek().kind != TokenKind.LPAREN:
                return self._parse_postfix_reference(IdentifierNode(identifier))
            # consume the argument list opener
            self._consume(TokenKind.LPAREN)
            # V() and I() probe functions receive raw node-name arguments that may contain hyphens
            if identifier.casefold() in ("v", "i"):
                args = self._parse_probe_argument_list()
            else:
                args = self._parse_argument_list()
            # consume the argument list closer
            self._consume(TokenKind.RPAREN)
            # parse an optional step-selector suffix such as V(INOISE)@1
            return self._parse_postfix_reference(FunctionCallNode(identifier, args))
        # parse a parenthesized sub-expression
        if token.kind == TokenKind.LPAREN:
            self._consume(TokenKind.LPAREN)
            expression = self._parse_ternary()
            self._consume(TokenKind.RPAREN)
            return expression
        # fail on an unexpected token
        raise ValueError(f"Unexpected token {token.text!r} at offset {token.start}")

    def _parse_postfix_reference(self, expression: ExpressionNode) -> ExpressionNode:
        # support step selectors such as V(OUT)@1 for stepped analyses
        if self._peek().kind != TokenKind.AT:
            return expression
        self._consume(TokenKind.AT)
        suffix = self._peek()
        # require a numeric step index after @
        if suffix.kind != TokenKind.NUMBER:
            raise ValueError(f"Expected numeric step index after @ at offset {suffix.start}")
        selector_text = self._consume(TokenKind.NUMBER).text
        # parse the step index as a positive integer
        try:
            step_index = int(selector_text)
        except ValueError:
            raise ValueError(f"Step selector @{selector_text!r} is not a valid integer")
        # reject zero and negative step indices (1-based convention)
        if step_index < 1:
            raise ValueError(f"Step selector @{step_index} is invalid: indices must be >= 1")
        return StepSelectorNode(expression, step_index)

    def _parse_argument_list(self) -> list[ExpressionNode]:
        # collect argument expressions
        args: list[ExpressionNode] = []
        # exit early for an empty argument list
        if self._peek().kind == TokenKind.RPAREN:
            return args
        # parse comma-separated arguments
        while True:
            # append the next argument expression
            args.append(self._parse_ternary())
            # exit when the argument list ends
            if self._peek().kind != TokenKind.COMMA:
                return args
            # consume the argument separator
            self._consume(TokenKind.COMMA)

    def _parse_probe_argument_list(self) -> list[ExpressionNode]:
        # collect probe node-name arguments
        args: list[ExpressionNode] = []
        # exit early for an empty argument list
        if self._peek().kind == TokenKind.RPAREN:
            return args
        # parse comma-separated probe node names
        while True:
            # append the next raw node name
            args.append(self._parse_probe_node_name())
            # exit when the argument list ends
            if self._peek().kind != TokenKind.COMMA:
                return args
            # consume the argument separator
            self._consume(TokenKind.COMMA)

    def _parse_probe_node_name(self) -> ExpressionNode:
        # token kinds that are valid constituents of a SPICE/KiCad net name:
        # - IDENTIFIER: alphabetic start for named nodes
        # - NUMBER: digit-only segments like "0" for ground or numeric node labels
        # - MINUS: hyphens in KiCad auto-generated names like "net-_u304a-g2_"
        # - PLUS: plus signs in node names like "/POWER_SUPPLY/HB+"
        # - SLASH: hierarchy separators in Xyce subcircuit paths like "x1:y"
        # - COLON: component pin references like "XL201:L1"
        _NODE_NAME_TOKENS = frozenset({
            TokenKind.IDENTIFIER, TokenKind.NUMBER,
            TokenKind.MINUS, TokenKind.PLUS, TokenKind.SLASH, TokenKind.COLON,
        })
        # collect raw text fragments that form the node name
        parts: list[str] = []
        while self._peek().kind in _NODE_NAME_TOKENS:
            parts.append(self._consume(self._peek().kind).text)
        # require at least one fragment to form a valid node name
        if not parts:
            raise ValueError(f"Expected probe node name at offset {self._peek().start}")
        # reconstruct the full net name string and wrap it as an identifier
        return IdentifierNode("".join(parts))

    def _peek(self) -> Token:
        # return the current token
        return self._tokens[self._pos]

    def _consume(self, kind: TokenKind) -> Token:
        # read the current token
        token = self._peek()
        # fail on a token kind mismatch
        if token.kind != kind:
            raise ValueError(f"Expected {kind.name} at offset {token.start}, got {token.kind.name}")
        # advance the token cursor
        self._pos += 1
        # exit
        return token


def parse_expression(text: str) -> ExpressionNode:
    # parse a standalone expression with a fresh parser instance
    return XyceParser().parse_expression(text)


def parse_function_definition(text: str) -> FunctionDefinitionNode:
    # parse a .func definition with a fresh parser instance
    return XyceParser().parse_function_definition(text)
