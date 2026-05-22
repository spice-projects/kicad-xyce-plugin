from __future__ import annotations

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
    def from_xyce_directives(cls, directives: list[str]) -> "StepParameters":
        # init default values for all possible sweep parameters
        sweep_mode = "LIN"
        variable = ""
        start = ""
        stop = ""
        step = ""
        points = ""
        list_values: tuple[str, ...] = ()
        data_table_name = ""
        enabled = False
        # iterate all directives to find the .STEP statement
        for directive in directives:
            # tokenize the directive for parsing
            tokens = directive.split()
            # skip empty or malformed directives
            if not tokens:
                continue
            # only process .STEP directives
            if tokens[0].upper() != ".STEP":
                continue
            # mark the sweep as enabled when a directive is found
            enabled = True
            # handle data-driven sweep syntax: .STEP DATA=<tablename>
            if len(tokens) == 2 and "=" in tokens[1] and tokens[1].upper().startswith("DATA="):
                # set data sweep mode
                sweep_mode = "DATA"
                # extract table name from the assignment token
                data_table_name = tokens[1].split("=", 1)[1]
                # stop searching once found
                continue
            # skip processing if only the command was provided
            if len(tokens) < 2:
                continue
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
                # stop searching
                continue
            # handle explicit list sweeps: .STEP var LIST val [val ...]
            if len(tokens) >= 3 and tokens[2].upper() == "LIST":
                # set list sweep mode
                sweep_mode = "LIST"
                # capture sweep variable name
                variable = tokens[1]
                # capture all subsequent tokens as list values
                list_values = tuple(tokens[3:])
                # stop searching
                continue
            # handle linear sweeps: .STEP [LIN] var start stop step
            sweep_mode = "LIN"
            # check for explicit LIN keyword
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
            else:
                # parse parameters from implicit linear syntax
                if len(tokens) >= 5:
                    # capture sweep variable name
                    variable = tokens[1]
                    # capture start value
                    start = tokens[2]
                    # capture stop value
                    stop = tokens[3]
                    # capture step value
                    step = tokens[4]
        # return the populated parameter model
        return cls(sweep_mode=sweep_mode, variable=variable, start=start, stop=stop, step=step, points=points, list_values=list_values, data_table_name=data_table_name, enabled=enabled)

    def to_xyce_directives(self) -> list[str]:
        # return empty list when sweep is disabled
        if not self.enabled:
            return []
        # build the directive string based on the active sweep mode
        if self.sweep_mode == "DATA":
            # format data-driven sweep
            directive = f".STEP DATA={self.data_table_name}"
        elif self.sweep_mode == "LIST":
            # format explicit list sweep
            values_str = " ".join(self.list_values)
            # combine variable and values
            directive = f".STEP {self.variable} LIST {values_str}"
        elif self.sweep_mode == "LIN":
            # format linear sweep
            directive = f".STEP {self.variable} {self.start} {self.stop} {self.step}"
        else:
            # format log sweep (DEC/OCT)
            directive = f".STEP {self.sweep_mode} {self.variable} {self.start} {self.stop} {self.points}"
        # return the directive as a single-item list
        return [directive]
