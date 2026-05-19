from __future__ import annotations

from dataclasses import dataclass

from netlist_parser import NetlistTopology
from .print_parameters import PrintParameters


@dataclass(frozen=True)
class LinSimulationParameters:
    """S/Y/Z-parameter (LIN) analysis.

    .LIN drives its own internal sweep via an embedded .AC directive.
    Both .AC and .LIN are emitted from to_xyce_directives so that the class
    is fully self-contained.  Because of this, LinSimulationParameters MUST
    be registered before AcSimulationParameters in ALL_SIMULATION_PARAMETERS_TYPES
    so the dispatch logic resolves .LIN netlists to this class and not to
    AcSimulationParameters.
    """

    # .LIN keyword arguments
    sparcalc: bool = True
    format: str = "TOUCHSTONE2"
    lintype: str = "S"
    dataformat: str = "RI"
    file: str = ""
    width: str = ""
    precision: str = ""
    # embedded AC sweep fields
    sweep_mode: str = "LIN"
    points: str = ""
    start: str = ""
    end: str = ""
    data_table_name: str = ""
    replace_ground: bool = True
    print_parameters: PrintParameters | None = None

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "LinSimulationParameters" | None:
        # init defaults
        sparcalc = True
        lin_format = "TOUCHSTONE2"
        lintype = "S"
        dataformat = "RI"
        lin_file = ""
        width = ""
        precision = ""
        sweep_mode = "LIN"
        points = ""
        start = ""
        end = ""
        data_table_name = ""
        replace_ground = False
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
            # parse print directives and retain lin-specific output config
            if cmd == ".PRINT":
                # parse the print statement from the directive
                print_statement = PrintParameters.from_xyce_statement(directive)
                # retain ac print parameters when found
                if print_statement and print_statement.print_type == "AC":
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
            # parse the embedded ac sweep directive
            if cmd == ".AC":
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
                continue
            # skip non-LIN directives
            if cmd != ".LIN":
                continue
            # flag indicating a valid LIN directive was found
            found = True
            # parse keyword=value pairs from the .LIN directive
            for token in tokens[1:]:
                # normalize the token for case-insensitive key detection
                upper = token.upper()
                # skip tokens without an equals sign
                if "=" not in upper:
                    continue
                # split key and value at the first equals sign
                key, _, val = token.partition("=")
                key_upper = key.upper()
                if key_upper == "SPARCALC":
                    # set sparcalc from the value
                    sparcalc = val.upper() in ("1", "TRUE", "YES")
                elif key_upper == "FORMAT":
                    # set the output format
                    lin_format = val.upper()
                elif key_upper == "TYPE":
                    # set the s-parameter type
                    lintype = val.upper()
                elif key_upper == "DATAFORMAT":
                    # set the data format
                    dataformat = val.upper()
                elif key_upper == "FILE":
                    # set the output file name
                    lin_file = val
                elif key_upper == "WIDTH":
                    # set the column width
                    width = val
                elif key_upper == "PRECISION":
                    # set the output precision
                    precision = val
        # return instance if a valid directive was found
        return cls(sparcalc=sparcalc, format=lin_format, lintype=lintype, dataformat=dataformat, file=lin_file, width=width, precision=precision, sweep_mode=sweep_mode, points=points, start=start, end=end, data_table_name=data_table_name, replace_ground=replace_ground, print_parameters=print_parameters) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # prepend replaceground preprocessing when enabled
        preprocess = [".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []
        # build the embedded ac sweep directive
        if self.sweep_mode == "DATA":
            lines = [f".AC DATA={self.data_table_name}"]
        elif self.sweep_mode in ("DEC", "OCT"):
            lines = [f".AC {self.sweep_mode} {self.points} {self.start} {self.end}"]
        else:
            # lin sweep (explicit)
            lines = [f".AC LIN {self.points} {self.start} {self.end}"]
        # build and append the .LIN directive
        lines.append(self._build_lin_directive())
        # append ac print directive when configured
        if self.print_parameters and self.print_parameters.print_type == "AC":
            # append the print statement
            lines.append(self.print_parameters.to_xyce_statement())
        # return the full directive list
        return preprocess + lines

    def _build_lin_directive(self) -> str:
        # start with the base .LIN keyword
        parts = [".LIN"]
        # append sparcalc keyword when disabled
        if not self.sparcalc:
            # disable s-parameter calculation
            parts.append("SPARCALC=0")
        # append format keyword when non-default
        if self.format != "TOUCHSTONE2":
            # set the output file format
            parts.append(f"FORMAT={self.format}")
        # append parameter type keyword when non-default
        if self.lintype != "S":
            # set the parameter type
            parts.append(f"TYPE={self.lintype}")
        # append data format keyword when non-default
        if self.dataformat != "RI":
            # set the data representation format
            parts.append(f"DATAFORMAT={self.dataformat}")
        # append output file name when specified
        if self.file:
            # set the output file path
            parts.append(f"FILE={self.file}")
        # append column width when specified
        if self.width:
            # set the output column width
            parts.append(f"WIDTH={self.width}")
        # append precision when specified
        if self.precision:
            # set the output precision digits
            parts.append(f"PRECISION={self.precision}")
        # combine all parts into a single directive string
        return " ".join(parts)
