from __future__ import annotations

from dataclasses import dataclass

from netlist_parser import NetlistTopology
from .measure_parameters import MeasureEntry
from .print_parameters import PrintParameters
from .sens_parameter import SensParameter


@dataclass(frozen=True)
class AcSimulationParameters:

    sweep_mode: str
    points: str = ""
    start: str = ""
    end: str = ""
    data_table_name: str = ""
    replace_ground: bool = True
    print_parameters: PrintParameters | None = None
    measure_parameters: tuple[MeasureEntry, ...] = ()
    sensitivity: SensParameter | None = None

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "AcSimulationParameters" | None:
        # init defaults
        sweep_mode = "LIN"
        points = ""
        start = ""
        end = ""
        data_table_name = ""
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
            # parse print directives and retain ac-specific output config
            if cmd == ".PRINT":
                # parse the print statement from the directive
                print_statement = PrintParameters.from_xyce_statement(directive)
                # retain ac print parameters when found
                if print_statement and print_statement.print_type in ("AC", "AC_IC"):
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
                if measure_statement and measure_statement.analysis_type in ("AC", "AC_CONT"):
                    # append the parsed measure parameters
                    measure_parameters.append(measure_statement)
                # next
                continue
            # skip non-AC directives
            if cmd != ".AC":
                continue
            # flag indicating a valid AC directive was found
            found = True
            if len(tokens) < 2:
                continue
            second = tokens[1].upper()
            # handle DATA sweep: .AC DATA=<tablename>
            if second.startswith("DATA="):
                # set sweep mode and data table name
                sweep_mode = "DATA"
                data_table_name = tokens[1].split("=", 1)[1]
                # next
                continue
            # detect decade or octave log sweep: .AC DEC|OCT <points> <start> <end>
            if second in ("DEC", "OCT"):
                sweep_mode = second
                if len(tokens) >= 5:
                    points = tokens[2]
                    start = tokens[3]
                    end = tokens[4]
                # next
                continue
            # linear sweep: .AC [LIN] <points> <start> <end>
            sweep_mode = "LIN"
            if second == "LIN":
                # explicit LIN keyword
                if len(tokens) >= 5:
                    points = tokens[2]
                    start = tokens[3]
                    end = tokens[4]
            else:
                # implicit LIN
                if len(tokens) >= 4:
                    points = tokens[1]
                    start = tokens[2]
                    end = tokens[3]
        # parse sensitivity as a companion directive before analysis detection
        sensitivity = SensParameter.from_xyce_directives(directives)
        # return instance if a valid directive was found
        return cls(sweep_mode=sweep_mode, points=points, start=start, end=end, data_table_name=data_table_name, replace_ground=replace_ground, print_parameters=print_parameters, measure_parameters=tuple(measure_parameters), sensitivity=sensitivity) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # prepend replaceground preprocessing when enabled
        preprocess = [".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []
        # build the core ac directive
        if self.sweep_mode == "DATA":
            lines = [f".AC DATA={self.data_table_name}"]
        elif self.sweep_mode in ("DEC", "OCT"):
            lines = [f".AC {self.sweep_mode} {self.points} {self.start} {self.end}"]
        else:
            # lin sweep (explicit)
            lines = [f".AC LIN {self.points} {self.start} {self.end}"]
        # append ac print directive when configured
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
