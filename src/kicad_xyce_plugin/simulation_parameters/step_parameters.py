from __future__ import annotations

import re
from dataclasses import dataclass


@dataclass(frozen=True)
class StepParameters:

    sweep_mode: str = "LIN"
    variable: str = ""
    start: str = ""
    stop: str = ""
    step: str = ""
    points: str = ""
    list_values: tuple[str, ...] = ()
    data_table_name: str = ""
    enabled: bool = False

    @classmethod
    def _from_single_directive(cls, directive: str) -> "StepParameters | None":
        # normalize spaces around equals sign
        normalized = re.sub(r"\s*=\s*", "=", directive)
        # tokenize the normalized directive
        tokens = normalized.split()
        # skip empty or malformed directives
        if not tokens:
            # return none
            return None
        # only process .STEP directives
        if tokens[0].upper() != ".STEP":
            # return none
            return None
        # init default values for all possible sweep parameters
        sweep_mode = "LIN"
        # init variable
        variable = ""
        # init start
        start = ""
        # init stop
        stop = ""
        # init step value
        step = ""
        # init points
        points = ""
        # init list values
        list_values: tuple[str, ...] = ()
        # init data table name
        data_table_name = ""
        # handle data-driven sweep syntax: .STEP DATA=<tablename>
        if len(tokens) == 2 and "=" in tokens[1] and tokens[1].upper().startswith("DATA="):
            # set data sweep mode
            sweep_mode = "DATA"
            # extract table name from the assignment token
            data_table_name = tokens[1].split("=", 1)[1]
            # return populated instance
            return cls(sweep_mode=sweep_mode, variable=variable, start=start, stop=stop, step=step, points=points, list_values=list_values, data_table_name=data_table_name, enabled=True)
        # skip processing if only the command was provided
        if len(tokens) < 2:
            # return none for incomplete directive
            return None
        # capture the second token for mode detection
        second = tokens[1].upper()
        # handle decade or octave log sweeps: .STEP DEC|OCT var start stop points
        if second in ("DEC", "OCT"):
            # set the log sweep mode
            sweep_mode = second
            # parse positional parameters when enough tokens are present
            if len(tokens) >= 6:
                # capture sweep variable name
                variable = tokens[2]
                # capture start value
                start = tokens[3]
                # capture stop value
                stop = tokens[4]
                # capture points count
                points = tokens[5]
            # return populated instance
            return cls(sweep_mode=sweep_mode, variable=variable, start=start, stop=stop, step=step, points=points, list_values=list_values, data_table_name=data_table_name, enabled=True)
        # handle explicit list sweeps: .STEP var LIST val [val ...]
        if len(tokens) >= 3 and tokens[2].upper() == "LIST":
            # set list sweep mode
            sweep_mode = "LIST"
            # capture sweep variable name
            variable = tokens[1]
            # capture all subsequent tokens as list values
            list_values = tuple(tokens[3:])
            # return populated instance
            return cls(sweep_mode=sweep_mode, variable=variable, start=start, stop=stop, step=step, points=points, list_values=list_values, data_table_name=data_table_name, enabled=True)
        # handle explicit LIN keyword: .STEP LIN var start stop step
        if second == "LIN":
            # parse parameters from explicit linear syntax
            if len(tokens) >= 6:
                # capture sweep variable name
                variable = tokens[2]
                # capture start value
                start = tokens[3]
                # capture stop value
                stop = tokens[4]
                # capture step value
                step = tokens[5]
            # return populated instance
            return cls(sweep_mode="LIN", variable=variable, start=start, stop=stop, step=step, points=points, list_values=list_values, data_table_name=data_table_name, enabled=True)
        # handle implicit linear syntax: .STEP var start stop step
        if len(tokens) >= 5:
            # capture sweep variable name
            variable = tokens[1]
            # capture start value
            start = tokens[2]
            # capture stop value
            stop = tokens[3]
            # capture step value
            step = tokens[4]
        # return populated instance
        return cls(sweep_mode="LIN", variable=variable, start=start, stop=stop, step=step, points=points, list_values=list_values, data_table_name=data_table_name, enabled=True)

    @classmethod
    def all_from_xyce_directives(cls, directives: list[str]) -> "tuple[StepParameters, ...]":
        # collect all parsed step parameters in order
        results: list[StepParameters] = []
        # iterate all directives to find .STEP statements
        for directive in directives:
            # attempt to parse each directive as a step
            parsed = cls._from_single_directive(directive)
            # add to results if a valid step was found
            if parsed is not None:
                # append to list
                results.append(parsed)
        # return tuple preserving declaration order
        return tuple(results)

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "StepParameters":
        # parse all step directives and return the first, or a disabled default
        all_steps = cls.all_from_xyce_directives(directives)
        # return first step if any are found
        if all_steps:
            # return the first entry
            return all_steps[0]
        # return a disabled default when no step directive is present
        return cls()

    def to_xyce_directives(self) -> list[str]:
        # return empty list when sweep is disabled
        if not self.enabled:
            # return empty list
            return []
        # build the directive string based on the active sweep mode
        if self.sweep_mode == "DATA":
            # format data-driven sweep
            directive = f".STEP DATA={self.data_table_name}"
        # handle list sweep
        elif self.sweep_mode == "LIST":
            # format explicit list sweep
            values_str = " ".join(self.list_values)
            # combine variable and values
            directive = f".STEP {self.variable} LIST {values_str}"
        # handle log sweeps (DEC/OCT)
        elif self.sweep_mode in ("DEC", "OCT"):
            # format log sweep with explicit keyword
            directive = f".STEP {self.sweep_mode} {self.variable} {self.start} {self.stop} {self.points}"
        # handle linear sweep — always emit explicit LIN keyword
        else:
            # format linear sweep with explicit LIN keyword
            directive = f".STEP LIN {self.variable} {self.start} {self.stop} {self.step}"
        # return the directive as a single-item list
        return [directive]
