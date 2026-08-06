from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class FourParameters:

    fundamental_frequency: str
    output_variables: tuple[str, ...]

    @classmethod
    def from_xyce_statement(cls, four_statement: str) -> "FourParameters" | None:
        # tokenize the directive (simple split for now)
        tokens = four_statement.split()
        # reject non-four statements
        if len(tokens) < 3 or tokens[0].upper() != ".FOUR":
            return None
        # fundamental frequency
        fundamental_frequency = tokens[1]
        # output variables
        output_variables = tuple(tokens[2:])
        # return model
        return cls(fundamental_frequency=fundamental_frequency, output_variables=output_variables)

    def to_xyce_statement(self) -> str:
        # init tokens
        tokens = [".FOUR", self.fundamental_frequency]
        # add output variables
        tokens.extend(self.output_variables)
        # return joined statement
        return " ".join(tokens)
