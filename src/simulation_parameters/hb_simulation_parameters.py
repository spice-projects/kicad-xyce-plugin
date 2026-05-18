from __future__ import annotations

from dataclasses import dataclass

from netlist_parser import NetlistTopology
from .print_parameters import PrintParameters


@dataclass(frozen=True)
class HbSimulationParameters:

    frequencies: tuple[str, ...]
    replace_ground: bool = True
    print_parameters: PrintParameters | None = None

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "HbSimulationParameters" | None:
        # init defaults
        frequencies: list[str] = []
        replace_ground = True
        print_parameters = None
        # flag indicating whether a valid directive was found
        found = False
        # parse directives
        for directive in directives:
            # tokenize the directive
            tokens = directive.split()
            # skip empty directives
            if not tokens:
                continue
            cmd = tokens[0].upper()
            # parse print directives and retain hb-specific output config
            if cmd == ".PRINT":
                # parse the print statement from the directive
                print_statement = PrintParameters.from_xyce_statement(directive)
                # retain hb print parameters when found
                if print_statement and print_statement.print_type in ("HB", "HB_FD", "HB_TD"):
                    # store the parsed print parameters
                    print_parameters = print_statement
                    # next
                    continue
            # handle preprocess replaceground
            if cmd == ".PREPROCESS" and len(tokens) > 2 and tokens[1].upper() == "REPLACEGROUND":
                # set replace_ground based on the third token
                replace_ground = tokens[2].upper() == "TRUE"
                # next
                continue
            # skip non-HB directives
            if cmd != ".HB":
                continue
            # flag indicating a valid HB directive was found
            found = True
            # collect all fundamental frequencies from remaining tokens
            frequencies = tokens[1:]
        # return instance if a valid directive was found
        return cls(frequencies=tuple(frequencies), replace_ground=replace_ground, print_parameters=print_parameters) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # prepend replaceground preprocessing when enabled
        preprocess = [".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []
        # build the hb directive with space-separated fundamental frequencies
        lines = [".HB " + " ".join(self.frequencies)]
        # append hb print directive when configured
        if self.print_parameters and self.print_parameters.print_type in ("HB", "HB_FD", "HB_TD"):
            # append the print statement
            lines.append(self.print_parameters.to_xyce_statement())
        # return the full directive list
        return preprocess + lines
