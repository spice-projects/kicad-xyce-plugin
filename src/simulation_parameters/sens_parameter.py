from __future__ import annotations

from dataclasses import dataclass

from netlist_parser import NetlistTopology
from .print_parameters import PrintParameters


@dataclass(frozen=True)
class SensParameter:
    # analysis context for the sensitivity directive
    analysis_context: str
    # objective specification mode
    objective_mode: str
    # output objective values
    objective_values: tuple[str, ...]
    # list of device parameters
    parameter_list: tuple[str, ...]
    # direct method flag
    direct: bool = False
    # adjoint method flag
    adjoint: bool = False
    # optional print parameters
    print_parameters: PrintParameters | None = None

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> SensParameter | None:
        # init directive found flag
        found = False
        # init parsed parameters
        analysis_context = ""
        # init objective mode
        objective_mode = ""
        # init objective values
        objective_values: list[str] = []
        # init parameter list
        parameter_list: list[str] = []
        # init direct method flag
        direct = False
        # init adjoint method flag
        adjoint = False
        # init print parameters
        print_parameters = None
        # iterate provided directives
        for directive in directives:
            # tokenize directive
            tokens = directive.split()
            # skip empty directives
            if not tokens:
                # continue to next iteration
                continue
            # extract command keyword
            cmd = tokens[0].upper()
            # check for sens command
            if cmd == ".SENS":
                # set found flag
                found = True
                # iterate over tokens
                for token in tokens[1:]:
                    # skip if no equals sign
                    if "=" not in token:
                        # continue iteration
                        continue
                    # split key and value
                    key, val = token.split("=", 1)
                    # normalize key
                    norm_key = key.lower()
                    # check for objective modes
                    if norm_key in ("objfunc", "objvars", "acobjfunc"):
                        # set objective mode
                        objective_mode = norm_key
                        # parse and clean values
                        objective_values = [v.strip("{}") for v in val.split(",")]
                    # check for parameters
                    elif norm_key == "param":
                        # set parameter list
                        parameter_list = [p.strip() for p in val.split(",")]
            # check for sensitivity options
            if cmd == ".OPTIONS" and len(tokens) > 1 and tokens[1].upper() == "SENSITIVITY":
                # iterate over tokens
                for token in tokens[2:]:
                    # check direct method
                    if token.lower().startswith("direct="):
                        # set direct flag
                        direct = token.split("=")[1] == "1"
                    # check adjoint method
                    if token.lower().startswith("adjoint="):
                        # set adjoint flag
                        adjoint = token.split("=")[1] == "1"
            # check for print directive
            if cmd == ".PRINT" and len(tokens) > 1 and tokens[1].upper() == "SENS":
                # create print parameters
                print_parameters = PrintParameters.from_xyce_statement(directive)
        # check if directive was found
        if not found:
            # return none
            return None
        # return new instance
        return cls(analysis_context, objective_mode, tuple(objective_values), tuple(parameter_list), direct, adjoint, print_parameters)

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # init line list
        lines: list[str] = []
        # build objective directive string
        obj_str = ",".join(self.objective_values)
        # build parameter string
        param_str = ",".join(self.parameter_list)
        # format objective line for objfunc
        if self.objective_mode == "objfunc":
            # add directive line
            lines.append(f".SENS objfunc={{{obj_str}}} param={param_str}")
        # format options string
        options_line = f".OPTIONS SENSITIVITY direct={1 if self.direct else 0} adjoint={1 if self.adjoint else 0}"
        # add options line
        lines.append(options_line)
        # check for print parameters
        if self.print_parameters:
            # add print directive
            lines.append(self.print_parameters.to_xyce_statement())
        # return directive lines
        return lines
