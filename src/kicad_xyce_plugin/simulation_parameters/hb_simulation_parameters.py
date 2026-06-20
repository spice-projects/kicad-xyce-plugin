from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

from ..netlist_parser import NetlistTopology
from .print_parameters import PrintParameters


@dataclass(frozen=True)
class HbSimulationParameters:

    frequencies: tuple[str, ...]
    harmonics: tuple[int, ...] = ()
    tahb: int | None = None
    selectharms: str | None = None
    startup_periods: int | None = None
    replace_ground: bool = True
    print_parameters: PrintParameters | None = None
    nonlin_options: dict[str, str] = field(default_factory=dict)
    linsol_options: dict[str, str] = field(default_factory=dict)

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "HbSimulationParameters" | None:
        # init defaults
        frequencies: list[str] = []
        harmonics: list[int] = []
        tahb = None
        selectharms = None
        startup_periods = None
        replace_ground = True
        print_parameters = None
        nonlin_options = {}
        linsol_options = {}
        # flag indicating whether a valid directive was found
        found = False
        # parse directives
        for directive in directives:
            # tokenize the directive
            tokens = directive.split()
            # skip empty directives
            if not tokens:
                # next
                continue
            # get command
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
            # handle .OPTIONS
            if cmd == ".OPTIONS" and len(tokens) > 1:
                # get package
                pkg = tokens[1].upper()
                # handle HBINT
                if pkg == "HBINT":
                    # parse options
                    for token in tokens[2:]:
                        # check for equals
                        if "=" in token:
                            # split key and value
                            k, v = token.split("=", 1)
                            # normalize key
                            k = k.upper()
                            # handle NUMFREQ
                            if k == "NUMFREQ":
                                # split comma-separated values
                                try:
                                    # parse integers
                                    harmonics = [int(x) for x in v.split(",") if x.strip()]
                                # catch parse errors
                                except ValueError:
                                    # ignore invalid NUMFREQ
                                    pass
                            # handle TAHB
                            elif k == "TAHB":
                                # parse integer
                                try:
                                    # store tahb
                                    tahb = int(v)
                                # catch parse errors
                                except ValueError:
                                    # ignore
                                    pass
                            # handle SELECTHARMS
                            elif k == "SELECTHARMS":
                                # store selectharms
                                selectharms = v.lower()
                            # handle STARTUPPERIODS
                            elif k == "STARTUPPERIODS":
                                # parse integer
                                try:
                                    # store startup periods
                                    startup_periods = int(v)
                                # catch parse errors
                                except ValueError:
                                    # ignore
                                    pass
                    # next
                    continue
                # handle NONLIN-HB
                if pkg == "NONLIN-HB":
                    # parse options
                    for token in tokens[2:]:
                        # check for equals
                        if "=" in token:
                            # split key and value
                            k, v = token.split("=", 1)
                            # store
                            nonlin_options[k.upper()] = v
                    # next
                    continue
                # handle LINSOL-HB
                if pkg == "LINSOL-HB":
                    # parse options
                    for token in tokens[2:]:
                        # check for equals
                        if "=" in token:
                            # split key and value
                            k, v = token.split("=", 1)
                            # store
                            linsol_options[k.upper()] = v
                    # next
                    continue
            # skip non-HB directives
            if cmd != ".HB":
                # next
                continue
            # flag indicating a valid HB directive was found
            found = True
            # collect all fundamental frequencies from remaining tokens
            frequencies = tokens[1:]
        # return instance if a valid directive was found
        return cls(frequencies=tuple(frequencies), harmonics=tuple(harmonics), tahb=tahb, selectharms=selectharms, startup_periods=startup_periods, replace_ground=replace_ground, print_parameters=print_parameters, nonlin_options=nonlin_options, linsol_options=linsol_options) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # prepend replaceground preprocessing when enabled
        preprocess = [".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []
        # build the hb directive with space-separated fundamental frequencies
        lines = [".HB " + " ".join(self.frequencies)]
        # build hbint options
        hbint_options: list[str] = []
        # append NUMFREQ when provided
        if self.harmonics:
            # format as comma-separated integers
            hbint_options.append("NUMFREQ=" + ",".join(str(h) for h in self.harmonics))
        # append TAHB when provided
        if self.tahb is not None:
            # append option
            hbint_options.append(f"TAHB={self.tahb}")
        # append SELECTHARMS when provided
        if self.selectharms:
            # append option
            hbint_options.append(f"SELECTHARMS={self.selectharms}")
        # append STARTUPPERIODS when provided
        if self.startup_periods is not None:
            # append option
            hbint_options.append(f"STARTUPPERIODS={self.startup_periods}")
        # check if we have hbint options
        if hbint_options:
            # append .OPTIONS HBINT line
            lines.append(".OPTIONS HBINT " + " ".join(hbint_options))
        # append nonlin-hb options
        if self.nonlin_options:
            # format as key=value pairs
            opts = " ".join(f"{k}={v}" for k, v in self.nonlin_options.items())
            # append directive
            lines.append(f".OPTIONS NONLIN-HB {opts}")
        # append linsol-hb options
        if self.linsol_options:
            # format as key=value pairs
            opts = " ".join(f"{k}={v}" for k, v in self.linsol_options.items())
            # append directive
            lines.append(f".OPTIONS LINSOL-HB {opts}")
        # append hb print directive when configured
        if self.print_parameters and self.print_parameters.print_type in ("HB", "HB_FD", "HB_TD"):
            # append the print statement
            lines.append(self.print_parameters.to_xyce_statement())
        # return the full directive list
        return preprocess + lines

    def raw_output_file_path(self, working_directory: Path, netlist_file_path: Path) -> Path | None:
        # check raw format is selected in print parameters and a print file is specified
        if self.print_parameters is None or self.print_parameters.print_format != "RAW":
            return None
        # return the output file path specified in the print parameters if available
        if self.print_parameters.print_file:
            return working_directory / self.print_parameters.print_file
        # otherwise, create raw file from netlist file, adding the ".raw" suffix
        return netlist_file_path.with_suffix(netlist_file_path.suffix + ".raw")
