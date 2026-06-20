from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from ..netlist_parser import NetlistTopology
from .measure_parameters import MeasureEntry
from .print_parameters import PrintParameters


def _parse_output_node(token: str) -> tuple[str, str]:
    # match V(node) or V(node,ref)
    m = re.fullmatch(r"[Vv]\(([^,)]+)(?:,([^)]+))?\)", token)
    if m:
        return m.group(1).strip(), (m.group(2) or "").strip()
    # fallback — return the token itself as the output node
    return token, ""


def _parse_device_noise_operator(variable: str) -> DeviceNoiseOperator | None:
    # match DNI(device) or DNO(device,source)
    m = re.fullmatch(r"D(NI|NO)\(([^,)]+)(?:,([^)]+))?\)", variable.upper())
    if m:
        # extract operator type, device name, and optional noise source
        operator_type = f"D{m.group(1)}"
        device_name = m.group(2).strip()
        noise_source = (m.group(3) or "").strip()
        # return parsed operator
        return DeviceNoiseOperator(device_name=device_name, operator_type=operator_type, noise_source=noise_source)
    # return none for non-matching variables
    return None


def _format_device_noise_operator(operator: DeviceNoiseOperator) -> str:
    """Format DeviceNoiseOperator as DNI(device) or DNO(device,source)."""
    # check if noise source is present
    if operator.noise_source:
        # return two-parameter form
        return f"{operator.operator_type}({operator.device_name},{operator.noise_source})"
    # return single-parameter form
    return f"{operator.operator_type}({operator.device_name})"


@dataclass(frozen=True)
class DeviceNoiseOperator:
    device_name: str
    operator_type: str
    noise_source: str = ""


@dataclass(frozen=True)
class NoiseSimulationParameters:

    output_node: str
    ref_node: str = ""
    source_name: str = ""
    sweep_mode: str = "LIN"
    points: str = ""
    start: str = ""
    end: str = ""
    data_table_name: str = ""
    replace_ground: bool = True
    print_parameters: PrintParameters | None = None
    device_noise_operators: tuple[DeviceNoiseOperator, ...] = ()
    measure_parameters: tuple[MeasureEntry, ...] = ()

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "NoiseSimulationParameters" | None:
        # init defaults
        output_node = ""
        ref_node = ""
        source_name = ""
        sweep_mode = "LIN"
        points = ""
        start = ""
        end = ""
        data_table_name = ""
        replace_ground = True
        print_parameters = None
        device_noise_operators: list[DeviceNoiseOperator] = []
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
            # parse print directives and retain noise-specific output config
            if cmd == ".PRINT":
                # parse the print statement from the directive
                print_statement = PrintParameters.from_xyce_statement(directive)
                # retain noise print parameters when found
                if print_statement and print_statement.print_type == "NOISE":
                    # store the parsed print parameters
                    print_parameters = print_statement
                    # extract device noise operators from output variables
                    for variable in print_statement.output_variables:
                        # attempt to parse as device noise operator
                        operator = _parse_device_noise_operator(variable)
                        if operator:
                            # add to list if parsing succeeded
                            device_noise_operators.append(operator)
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
                if measure_statement and measure_statement.analysis_type in ("NOISE", "NOISE_CONT"):
                    # append the parsed measure parameters
                    measure_parameters.append(measure_statement)
                # next
                continue
            # skip non-NOISE directives
            if cmd != ".NOISE":
                continue
            # flag indicating a valid NOISE directive was found
            found = True
            if len(tokens) < 3:
                continue
            # parse output node: tokens[1] is V(out) or V(out,ref)
            output_node, ref_node = _parse_output_node(tokens[1])
            # parse source name: tokens[2]
            source_name = tokens[2]
            if len(tokens) < 4:
                continue
            sweep_token = tokens[3].upper()
            # handle DATA sweep: .NOISE V(...) SRC DATA=<tablename>
            if sweep_token.startswith("DATA="):
                sweep_mode = "DATA"
                data_table_name = tokens[3].split("=", 1)[1]
                continue
            # detect log sweep type
            if sweep_token in ("DEC", "OCT"):
                sweep_mode = sweep_token
                if len(tokens) >= 7:
                    points = tokens[4]
                    start = tokens[5]
                    end = tokens[6]
                continue
            # linear sweep
            sweep_mode = "LIN"
            if sweep_token == "LIN":
                if len(tokens) >= 7:
                    points = tokens[4]
                    start = tokens[5]
                    end = tokens[6]
            else:
                # implicit LIN (no keyword)
                if len(tokens) >= 6:
                    points = tokens[3]
                    start = tokens[4]
                    end = tokens[5]
        # return instance if a valid directive was found
        return cls(output_node=output_node, ref_node=ref_node, source_name=source_name, sweep_mode=sweep_mode, points=points, start=start, end=end, data_table_name=data_table_name, replace_ground=replace_ground, print_parameters=print_parameters, device_noise_operators=tuple(device_noise_operators), measure_parameters=tuple(measure_parameters)) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # prepend replaceground preprocessing when enabled
        preprocess = [".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []
        # build the v(out[,ref]) token
        if self.ref_node:
            out_token = f"V({self.output_node},{self.ref_node})"
        else:
            out_token = f"V({self.output_node})"
        # build the sweep portion
        if self.sweep_mode == "DATA":
            sweep = f"DATA={self.data_table_name}"
        elif self.sweep_mode in ("DEC", "OCT"):
            sweep = f"{self.sweep_mode} {self.points} {self.start} {self.end}"
        else:
            sweep = f"LIN {self.points} {self.start} {self.end}"
        # build the noise directive line
        lines = [f".NOISE {out_token} {self.source_name} {sweep}"]
        # append noise print directive when configured
        if self.print_parameters and self.print_parameters.print_type == "NOISE":
            # start with existing output variables
            output_vars = list(self.print_parameters.output_variables)
            # add device noise operators in formatted form
            for operator in self.device_noise_operators:
                # format operator as string
                formatted = _format_device_noise_operator(operator)
                # add if not already present
                if formatted not in output_vars:
                    output_vars.append(formatted)
            # only emit print directive if there are output variables
            if output_vars:
                # create updated print parameters with merged output variables
                updated_print = PrintParameters(print_type="NOISE", print_format=self.print_parameters.print_format, print_file=self.print_parameters.print_file, output_variables=tuple(output_vars), extra_options=self.print_parameters.extra_options)
                # append the updated print statement
                lines.append(updated_print.to_xyce_statement())
        # append measure directives when configured
        for measure in self.measure_parameters:
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
