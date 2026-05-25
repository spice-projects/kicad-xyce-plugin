from __future__ import annotations

from dataclasses import dataclass

from netlist_parser import NetlistTopology
from .measure_parameters import MeasureEntry
from .print_parameters import PrintParameters
from .sens_parameter import SensParameter


@dataclass(frozen=True)
class DCSimulationParameters:

    sweep_mode: str
    primary_variable: str = ""
    start: str = ""
    stop: str = ""
    step: str = ""
    points: str = ""
    list_values: tuple[str, ...] = ()
    data_table_name: str = ""
    secondary_variable: str = ""
    secondary_start: str = ""
    secondary_stop: str = ""
    secondary_step: str = ""
    secondary_points: str = ""
    replace_ground: bool = True
    print_parameters: PrintParameters | None = None
    measure_parameters: tuple[MeasureEntry, ...] = ()
    sensitivity: SensParameter | None = None

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "DCSimulationParameters" | None:
        # init defaults
        sweep_mode = "LIN"
        primary_variable = ""
        start = ""
        stop = ""
        step = ""
        points = ""
        list_values: tuple[str, ...] = ()
        data_table_name = ""
        secondary_variable = ""
        secondary_start = ""
        secondary_stop = ""
        secondary_step = ""
        secondary_points = ""
        replace_ground = True
        print_parameters = None
        # init measure parameters
        measure_parameters: list[MeasureEntry] = []
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
            # parse print directives and retain dc-specific output config
            if cmd == ".PRINT":
                # parse the print statement from the directive
                print_statement = PrintParameters.from_xyce_statement(directive)
                # retain dc print parameters when found
                if print_statement and print_statement.print_type in ("DC", "HOMOTOPY"):
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
            # parse measure directives
            if cmd in (".MEASURE", ".MEAS"):
                # parse the measure statement from the directive
                measure_statement = MeasureEntry.from_xyce_statement(directive)
                # retain measure parameters when found and analysis type matches
                if measure_statement and measure_statement.analysis_type in ("DC", "DC_CONT"):
                    # append the parsed measure parameters
                    measure_parameters.append(measure_statement)
                # next
                continue
            # skip non-DC directives
            if cmd != ".DC":
                continue
            # flag indicating a valid DC directive was found
            found = True
            # handle DATA sweep: .DC DATA=<tablename>
            if len(tokens) == 2 and "=" in tokens[1] and tokens[1].upper().startswith("DATA="):
                # set sweep mode and data table name
                sweep_mode = "DATA"
                data_table_name = tokens[1].split("=", 1)[1]
                # next
                continue
            if len(tokens) < 2:
                continue
            second = tokens[1].upper()
            # detect decade or octave log sweep: .DC DEC|OCT var start stop points
            if second in ("DEC", "OCT"):
                sweep_mode = second
                # primary sweep tokens: MODE var start stop points
                if len(tokens) >= 6:
                    primary_variable = tokens[2]
                    start = tokens[3]
                    stop = tokens[4]
                    points = tokens[5]
                # optional secondary sweep: MODE var2 start2 stop2 points2
                if len(tokens) >= 11 and tokens[6].upper() in ("DEC", "OCT"):
                    secondary_variable = tokens[7]
                    secondary_start = tokens[8]
                    secondary_stop = tokens[9]
                    secondary_points = tokens[10]
                # next
                continue
            # detect LIST sweep: .DC var LIST val [val ...]
            if len(tokens) >= 3 and tokens[2].upper() == "LIST":
                # set sweep mode, primary variable, and list values
                sweep_mode = "LIST"
                primary_variable = tokens[1]
                list_values = tuple(tokens[3:])
                # next
                continue
            # linear sweep: .DC [LIN] var start stop step [var2 start2 stop2 step2]
            sweep_mode = "LIN"
            if second == "LIN":
                # explicit LIN keyword
                if len(tokens) >= 6:
                    primary_variable = tokens[2]
                    start = tokens[3]
                    stop = tokens[4]
                    step = tokens[5]
                # optional secondary sweep tokens: var2 start2 stop2 step2
                if len(tokens) >= 10:
                    secondary_variable = tokens[6]
                    secondary_start = tokens[7]
                    secondary_stop = tokens[8]
                    secondary_step = tokens[9]
            else:
                # implicit LIN
                if len(tokens) >= 5:
                    primary_variable = tokens[1]
                    start = tokens[2]
                    stop = tokens[3]
                    step = tokens[4]
                # optional secondary sweep tokens: var2 start2 stop2 step2
                if len(tokens) >= 9:
                    secondary_variable = tokens[5]
                    secondary_start = tokens[6]
                    secondary_stop = tokens[7]
                    secondary_step = tokens[8]
        # parse sensitivity as a companion directive before analysis detection
        sensitivity = SensParameter.from_xyce_directives(directives)
        # return instance if a valid directive was found
        return cls(sweep_mode=sweep_mode, primary_variable=primary_variable, start=start, stop=stop, step=step, points=points, list_values=list_values, data_table_name=data_table_name, secondary_variable=secondary_variable, secondary_start=secondary_start, secondary_stop=secondary_stop, secondary_step=secondary_step, secondary_points=secondary_points, replace_ground=replace_ground, print_parameters=print_parameters, measure_parameters=tuple(measure_parameters), sensitivity=sensitivity) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # prepend replaceground preprocessing when enabled
        preprocess = [".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []
        # build the core dc directive based on the selected sweep mode
        if self.sweep_mode == "DATA":
            lines = [self._build_data_directive()]
        elif self.sweep_mode == "LIST":
            lines = [self._build_list_directive()]
        elif self.sweep_mode == "LIN":
            lines = [self._build_lin_directive()]
        else:
            lines = [self._build_log_directive()]
        # append dc print directive when configured
        if self.print_parameters:
            # append the print statement
            lines.append(self.print_parameters.to_xyce_statement())
        # append sensitivity directives when configured
        if self.sensitivity is not None:
            lines.extend(self.sensitivity.to_xyce_directives(topology))
        # append measure directives
        for measure in self.measure_parameters:
            # append measure statement
            lines.append(measure.to_xyce_statement())
        # return the full directive list
        return preprocess + lines

    def _build_data_directive(self) -> str:
        # data-driven sweep references an existing .DATA table by name
        return f".DC DATA={self.data_table_name}"

    def _build_list_directive(self) -> str:
        # join the explicit sweep values with a single space separator
        values_str = " ".join(self.list_values)
        # combine variable name, LIST keyword, and the value sequence
        return f".DC {self.primary_variable} LIST {values_str}"

    def _build_lin_directive(self) -> str:
        # build the primary linear sweep token sequence
        tokens = [".DC", self.primary_variable, self.start, self.stop, self.step]
        # append secondary sweep tokens when a secondary variable is configured
        if self.secondary_variable:
            # extend with the secondary sweep parameters
            tokens.extend([self.secondary_variable, self.secondary_start, self.secondary_stop, self.secondary_step])
        # combine all tokens into a single directive string
        return " ".join(tokens)

    def _build_log_directive(self) -> str:
        # build the primary decade/octave sweep token sequence
        tokens = [".DC", self.sweep_mode, self.primary_variable, self.start, self.stop, self.points]
        # append secondary sweep tokens when a secondary variable is configured
        if self.secondary_variable:
            # extend with the secondary sweep parameters
            tokens.extend([self.secondary_variable, self.secondary_start, self.secondary_stop, self.secondary_points])
        # combine all tokens into a single directive string
        return " ".join(tokens)
