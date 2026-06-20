from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from ..netlist_parser import NetlistTopology
from .fft_parameters import FftParameters
from .four_parameters import FourParameters
from .measure_parameters import MeasureEntry
from .print_parameters import PrintParameters
from .sens_parameter import SensParameter


@dataclass(frozen=True)
class TransientSchedulePoint:
    time_value: str
    max_time_step_value: str


@dataclass(frozen=True)
class TransientSimulationParameters:

    initial_step_value: str
    final_time_value: str
    start_time_value: str = ""
    step_ceiling_value: str = ""
    op_keyword: str = ""
    schedule_points: tuple[TransientSchedulePoint, ...] = ()
    replace_ground: bool = True
    print_parameters: PrintParameters | None = None
    fft_parameters: tuple[FftParameters, ...] = ()
    four_parameters: tuple[FourParameters, ...] = ()
    measure_parameters: tuple[MeasureEntry, ...] = ()
    sensitivity: SensParameter | None = None

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "TransientSimulationParameters" | None:
        # init defaults
        initial_step_value = ""
        final_time_value = ""
        start_time_value = ""
        step_ceiling_value = ""
        op_keyword = ""
        schedule_points: list[TransientSchedulePoint] = []
        replace_ground = True
        print_parameters = None
        # init fft parameters
        fft_parameters: list[FftParameters] = []
        # init four parameters
        four_parameters: list[FourParameters] = []
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
            # get command
            cmd = tokens[0].upper()
            # parse print directives and retain transient-specific output config
            if cmd == ".PRINT":
                # parse the print statement from the directive
                print_statement = PrintParameters.from_xyce_statement(directive)
                # retain transient print parameters when found
                if print_statement and print_statement.print_type in ("TRAN", "TRANADJOINT"):
                    # store the parsed print parameters
                    print_parameters = print_statement
                    # next
                    continue
            # parse fft directives
            if cmd == ".FFT":
                # parse the fft statement from the directive
                fft_statement = FftParameters.from_xyce_statement(directive)
                # retain fft parameters when found
                if fft_statement:
                    # append the parsed fft parameters
                    fft_parameters.append(fft_statement)
                # next
                continue
            # parse four directives
            if cmd == ".FOUR":
                # parse the four statement from the directive
                four_statement = FourParameters.from_xyce_statement(directive)
                # retain four parameters when found
                if four_statement:
                    # append the parsed four parameters
                    four_parameters.append(four_statement)
                # next
                continue
            # parse measure directives
            if cmd in (".MEASURE", ".MEAS"):
                # parse the measure statement from the directive
                measure_statement = MeasureEntry.from_xyce_statement(directive)
                # retain measure parameters when found and analysis type matches
                if measure_statement and measure_statement.analysis_type in ("TRAN", "TRAN_CONT"):
                    # append the parsed measure parameters
                    measure_parameters.append(measure_statement)
                # next
                continue
            # handle preprocess replaceground
            if cmd == ".PREPROCESS" and len(tokens) > 2 and tokens[1].upper() == "REPLACEGROUND":
                # set flag based on value
                replace_ground = tokens[2].upper() == "TRUE"
                # next
                continue
            # skip non-TRAN directives
            if cmd != ".TRAN":
                continue
            # flag indicating a valid TRAN directive was found
            found = True
            # extract schedule clause from raw directive if present
            schedule_match = re.search(r'\{schedule\s*\(([^}]*)\)\s*\}', directive, re.IGNORECASE)
            if schedule_match:
                # split comma-separated time/step pairs from schedule content
                schedule_content = schedule_match.group(1)
                # strip each value and filter empties
                schedule_values = [v.strip() for v in schedule_content.split(",") if v.strip()]
                # pair consecutive entries as (time, max_step)
                for i in range(0, len(schedule_values) - 1, 2):
                    schedule_points.append(TransientSchedulePoint(time_value=schedule_values[i], max_time_step_value=schedule_values[i + 1],))
                # strip schedule clause so remaining tokens parse cleanly
                directive = directive[:schedule_match.start()].strip()
            # re-tokenize after schedule removal
            tokens = directive.split()
            # parse initial step (required, position 1)
            if len(tokens) >= 2:
                initial_step_value = tokens[1]
            # parse final time (required, position 2)
            if len(tokens) >= 3:
                final_time_value = tokens[2]
            # separate NOOP/UIC keywords from positional arguments
            positional = []
            for tok in tokens[3:]:
                if tok.upper() in ("NOOP", "UIC"):
                    # capture op keyword
                    op_keyword = tok.upper()
                else:
                    # accumulate remaining positional args
                    positional.append(tok)
            # assign optional start time (position 3)
            if len(positional) >= 1:
                start_time_value = positional[0]
            # assign optional step ceiling (position 4)
            if len(positional) >= 2:
                step_ceiling_value = positional[1]
        # parse sensitivity as a companion directive before analysis detection
        sensitivity = SensParameter.from_xyce_directives(directives)
        # return instance if a valid directive was found
        return cls(initial_step_value=initial_step_value, final_time_value=final_time_value, start_time_value=start_time_value, step_ceiling_value=step_ceiling_value, op_keyword=op_keyword, schedule_points=tuple(schedule_points), replace_ground=replace_ground, print_parameters=print_parameters, fft_parameters=tuple(fft_parameters), four_parameters=tuple(four_parameters), measure_parameters=tuple(measure_parameters), sensitivity=sensitivity) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # prepend replaceground preprocessor directive when enabled
        preprocess = [".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []
        # start with the transient analysis directive
        lines = [self._to_xyce_directive()]
        # append transient print directive when configured
        if self.print_parameters:
            # append print statement
            lines.append(self.print_parameters.to_xyce_statement())
        # append sensitivity directives when configured
        if self.sensitivity is not None:
            lines.extend(self.sensitivity.to_xyce_directives(topology))
        # append fft directives
        for fft in self.fft_parameters:
            # append fft statement
            lines.append(fft.to_xyce_statement())
        # append four directives
        for four in self.four_parameters:
            # append four statement
            lines.append(four.to_xyce_statement())
        # append measure directives
        for measure in self.measure_parameters:
            # append measure statement
            lines.append(measure.to_xyce_statement())
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

    def _to_xyce_directive(self) -> str:
        # build the required transient directive fields first
        tokens = [".TRAN", self.initial_step_value, self.final_time_value]
        # include start and step ceiling in positional order when either is provided
        if self.start_time_value or self.step_ceiling_value:
            # insert the default start time when only step ceiling was provided
            tokens.append(self.start_time_value if self.start_time_value else "0")
        # include optional step ceiling value only when provided
        if self.step_ceiling_value:
            tokens.append(self.step_ceiling_value)
        # append the selected operating-point behavior keyword when requested
        if self.op_keyword:
            tokens.append(self.op_keyword)
        # append schedule clause when schedule points are configured
        if self.schedule_points:
            # flatten alternating time and step entries for schedule syntax
            schedule_tokens = []
            # iterate all schedule points in user-provided order
            for schedule_point in self.schedule_points:
                # append the schedule time token
                schedule_tokens.append(schedule_point.time_value)
                # append the max-step token paired with that time
                schedule_tokens.append(schedule_point.max_time_step_value)
            # append the full schedule clause wrapped as a brace expression
            tokens.append("{schedule(" + ", ".join(schedule_tokens) + ")}")
        # return a single directive string that can be placed in a netlist
        return " ".join(tokens)
