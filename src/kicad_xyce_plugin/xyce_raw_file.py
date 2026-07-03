import logging
import mmap
import re
import time
from pathlib import Path

import numpy as np

from .expression import Expression, ExpressionManager
from .xyce_output_file import AbscissaScale, StepInformation, VariableType, XyceOutputFile

logger = logging.getLogger(__name__)

_PAIR_RE = re.compile(r'name\s*=\s*(\S+)\s+value\s*=\s*([\d.eE+-]+)')


def _process_abscissa_scale(abscissa: Expression, scale: AbscissaScale) -> Expression:
    # log10: apply per-step to preserve multi-step structure
    if scale == AbscissaScale.DECADE:
        return Expression(abscissa.name, [np.log10(step) for step in abscissa.steps], abscissa.unit, abscissa.source, abscissa.variable_type)
    # log2: apply per-step to preserve multi-step structure
    if scale == AbscissaScale.OCTAVE:
        return Expression(abscissa.name, [np.log2(step) for step in abscissa.steps], abscissa.unit, abscissa.source, abscissa.variable_type)
    # linear scale doesn't modify the abscissa
    return abscissa


def _parse_binary_variables(data: mmap.mmap, offset: int, variable_definitions: list[tuple[int, str, VariableType | None]], is_complex: bool, num_variables: int, num_points: int) -> tuple[int, list[tuple[str, np.ndarray, str | None, str | None]]]:
    # xyce binary format: all variables stored as float64 (real) or complex128 (complex), row-major order; the point count may be unknown (0) when No. Points is not specified
    if is_complex:
        # each row contains num_variables complex128 values (16 bytes each); infer row count from available bytes when num_points is 0
        if num_points == 0:
            # calculate number of complete rows from available bytes after the offset
            num_points = (len(data) - offset) // (num_variables * 16)
        # read exactly num_points * num_variables complex128 values; the count cap prevents reading trailing non-data bytes
        count = num_points * num_variables
        flat = np.frombuffer(data, dtype="<c16", offset=offset, count=count)
        # reshape to (num_points, num_variables) matrix
        matrix = flat.reshape(num_points, num_variables)
        # build result
        result: list[tuple[str, np.ndarray, str | None, str | None]] = []
        # loop variable definitions in block
        for idx, name, vt in variable_definitions:
            # unit and variable type derived from VariableType when known
            unit = vt.value.unit if vt is not None else ""
            vtype = vt.value.name if vt is not None else None
            # append variable to result
            result.append((name, matrix[:, idx].real if idx == 0 else matrix[:, idx], unit, vtype))
        # exit
        return num_points, result
    # each row contains num_variables float64 values (8 bytes each); infer row count when num_points is 0
    if num_points == 0:
        # calculate number of complete rows from available bytes after the offset
        num_points = (len(data) - offset) // (num_variables * 8)
    # read exactly num_points * num_variables float64 values; the count cap prevents reading trailing non-data bytes
    flat = np.frombuffer(data, dtype="<f8", offset=offset, count=num_points * num_variables)
    # reshape to (num_points, num_variables) matrix
    matrix = flat.reshape(num_points, num_variables)
    # build result
    result: list[tuple[str, np.ndarray, str | None, str | None]] = []
    # loop variable definitions in block
    for idx, name, vt in variable_definitions:
        # unit and variable type derived from VariableType when known
        unit = vt.value.unit if vt is not None else ""
        vtype = vt.value.name if vt is not None else None
        # append variable to result
        result.append((name, matrix[:, idx], unit, vtype))
    # exit
    return num_points, result


def _parse_ascii_variables(data: mmap.mmap, offset: int, variable_definitions: list[tuple[int, str, VariableType | None]], is_complex: bool, num_variables: int, num_points: int) -> tuple[int, list[tuple[str, np.ndarray, str | None, str | None]]]:
    # decode the values section as utf-8 text; replace unrecognised bytes rather than raising
    text = data[offset:].decode("utf-8", errors="replace")
    # for complex analysis each variable contributes a real and imaginary float; for real analysis each variable contributes one float
    floats_per_point = num_variables * 2 if is_complex else num_variables
    # number of tokens per line (index + floats)
    tokens_per_line = 1 + floats_per_point
    # accumulated float tokens across all lines; the point index tokens (integers) are stripped
    all_floats: list[float] = []
    # expected integer index of the next data point — used to identify and skip leading index tokens
    expected_index = 0
    try:
        # loop lines in file
        for line in text.splitlines():
            # skip blank lines
            stripped = line.strip()
            if not stripped:
                continue
            # split line into whitespace-separated tokens
            tokens = stripped.split()
            # validate line, it must have [index real] or [index real imag] tokens
            if len(tokens) != tokens_per_line:
                # log warning
                logger.warning("invalid Xyce RAW file: unexpected number of tokens in values section line: %s", line)
                # exit
                return 0, []
            # validate line number
            if int(tokens[0]) != expected_index:
                # log warning
                logger.warning("invalid Xyce RAW file: unexpected point index in values section line: %s", line)
                # exit
                return 0, []
            # append float values from the line
            all_floats.extend(float(token) for token in tokens[1:])
            # increase expected index for the next line
            expected_index += 1
    except Exception:
        # log error
        logger.error("invalid Xyce RAW file: error parsing values section: %s", )
        # exit
        return 0, []
    # validate the number of points, if num_points was specified in the header; otherwise, infer it from the number of floats collected
    actual_points = len(all_floats) // floats_per_point
    if num_points > 0 and actual_points != num_points:
        # log error
        logger.error("invalid Xyce RAW file: unexpected number of points in values section: expected %d, found %d", num_points, actual_points)
        # exit
        return 0, []
    # build a numpy array from the collected floats
    flat = np.array(all_floats, dtype=np.float64)
    # check whether the data is complex or real
    if is_complex:
        # interleaved real/imag layout: reshape to (actual_points, num_variables, 2) then combine into complex128
        pairs = flat.reshape(actual_points, num_variables, 2)
        complex_matrix = pairs[:, :, 0] + 1j * pairs[:, :, 1]
        # build result
        result: list[tuple[str, np.ndarray, str | None, str | None]] = []
        # loop variable definitions in block
        for idx, name, vt in variable_definitions:
            # unit and variable type derived from VariableType when known
            unit = vt.value.unit if vt is not None else ""
            vtype = vt.value.name if vt is not None else None
            # append variable to result
            result.append((name, complex_matrix[:, idx].real if idx == 0 else complex_matrix[:, idx], unit, vtype))
        # exit
        return actual_points, result
    # reshape to (actual_points, num_variables) float64 matrix
    matrix = flat.reshape(actual_points, num_variables)
    # build result
    result: list[tuple[str, np.ndarray, str | None, str | None]] = []
    # loop variable definitions in block
    for idx, name, vt in variable_definitions:
        # unit and variable type derived from VariableType when known
        unit = vt.value.unit if vt is not None else ""
        vtype = vt.value.name if vt is not None else None
        # append variable to result
        result.append((name, matrix[:, idx], unit, vtype))
    # exit
    return actual_points, result


def _scan_block_header(data: mmap.mmap, start_pos: int) -> tuple[dict[str, str], list[tuple[int, str, VariableType | None]], int, bool] | None:
    # scan one header+variables block starting at start_pos; returns (header_dict, variable_defs, data_offset, is_ascii) or None when no data section is found
    header: dict[str, str] = {}
    variable_definitions: list[tuple[int, str, VariableType | None]] = []
    data_offset = -1
    is_ascii = False
    in_variables = False
    pos = start_pos
    # scan line by line until the Binary: or Values: marker is reached
    while pos < len(data):
        # find next newline
        newline = data.find(b'\n', pos)
        if newline == -1:
            break
        # decode the line
        line = data[pos:newline].decode("utf-8").strip()
        # log header line
        logger.debug(">> %s", line)
        # advance position to the next line (include the delimiter)
        pos = newline + 1
        # parse the variables section
        if in_variables:
            # check for the data section marker
            if line in ("Binary:", "Values:"):
                # log transition
                logger.debug(">> ...")
                # record the byte offset at which data begins
                data_offset = pos
                # record whether the data is in ascii text format
                is_ascii = line == "Values:"
                # exit the scan loop
                break
            # parse variable definition line: expected format is "index\tname\ttype"
            parts = line.split("\t")
            # only accept lines with exactly 3 tab-separated parts; skip malformed lines
            if len(parts) == 3:
                # match variable type by name; accept None for unknown types so variable count stays consistent
                variable_type = next((vt for vt in VariableType if vt.value.name == parts[2]), None)
                if variable_type is None:
                    # log warning for unknown type
                    logger.warning("Unknown variable type '%s' for variable '%s'; treating as untyped", parts[2], parts[1])
                # append variable definition
                variable_definitions.append((int(parts[0]), parts[1], variable_type))
            continue
        # detect start of the variables section
        if line == "Variables:":
            # switch state to variables parsing
            in_variables = True
            continue
        # parse header key: value line
        if ":" in line:
            # split at first colon to separate key from value
            key, _, value = line.partition(":")
            # store in the header dictionary
            header[key.strip()] = value.strip()
    # return None when no data section was found (e.g. truncated file or end of file)
    if data_offset < 0:
        return None
    # exit with the parsed header, variable definitions, data offset, and ascii flag
    return header, variable_definitions, data_offset, is_ascii


def _parse_step_plotname(plotname: str) -> tuple[list[str], tuple] | None:
    # extract name/value pairs
    pairs = _PAIR_RE.findall(plotname)
    if not pairs:
        return None
    # unpack names and values
    return [name for name, _ in pairs], tuple(float(value) for _, value in pairs)


def xyce_raw_file_parser(filename: Path) -> XyceOutputFile | None:
    # load file
    path = Path(filename)
    if not path.exists():
        # log error
        logger.error("Xyce RAW file not found: %s", path)
        # exit
        return None
    # measure time taken to load file
    start_time = time.perf_counter()
    try:
        # log information
        logger.info("Loading Xyce RAW file: %s", path)
        # memory-map the file — the OS pages in only the regions that are actually read, so for a 300-variable file where only a few are displayed, the remaining columns are never loaded into physical RAM
        with open(path, "rb") as _file:
            data = mmap.mmap(_file.fileno(), 0, access=mmap.ACCESS_READ)
        # collect all parsed blocks: each entry is (headers, num_points)
        blocks: list[tuple[dict[str, str], int, bool, list[tuple[str, np.ndarray, str | None, str | None]]]] = []
        # step information
        step_keys: list[str] = []
        step_values: list[tuple] = []
        # start scanning from the beginning of the file
        pos = 0
        # multi-block scan loop: each iteration parses one header+data block
        while True:
            # scan the next block header starting at the current position
            block_result = _scan_block_header(data, pos)
            # stop when no more blocks are found (end of file or truncated header)
            if block_result is None:
                break
            # unpack block header scan result
            block_header, block_var_defs, data_offset, is_ascii = block_result
            # parse header fields needed to interpret this block's data
            is_complex = "complex" in block_header.get("Flags", "").lower()
            num_variables = int(block_header.get("No. Variables", 0))
            num_points = int(block_header.get("No. Points", "0").strip())
            # parse the data section — ascii (Values:) or binary (Binary:)
            num_points, block_variables = _parse_ascii_variables(data, data_offset, block_var_defs, is_complex, num_variables, num_points) if is_ascii else _parse_binary_variables(data, data_offset, block_var_defs, is_complex, num_variables, num_points)
            # stop accumulating blocks when parsing fails
            if num_points == 0 or len(block_variables) == 0:
                break
            # store block result
            blocks.append((block_header, num_points, is_complex, block_variables))
            # advance past the binary data to the start of the next block; ascii data end is not computable so stop after ascii
            if is_ascii:
                break
            # bytes per data value: float64 for real, complex128 for complex
            bytes_per_value = 16 if is_complex else 8
            # compute the byte position immediately after this block's binary data
            pos = data_offset + num_points * num_variables * bytes_per_value
            # parse plotname to extract step parameter name and value
            step_info = _parse_step_plotname(block_header.get("Plotname", ""))
            if step_info is not None:
                # parameter names and values for this step
                param_names, param_values = step_info
                # store step information for this block
                step_keys = param_names
                step_values.append(param_values)
            # stop when the end of the file has been reached
            if pos >= len(data):
                break
        # validate that at least one block was successfully parsed
        if not blocks:
            # log error
            logger.error("invalid Xyce RAW file: data section not found")
            # exit
            return None
        # at least one block is required (no steps)
        block_headers, block_num_points, block_is_complex, block_variables = blocks[0]
        # step information
        step_count = len(blocks)
        # abscissa related data
        abscissa_indices: list[slice] = [slice(0, block_num_points)]
        abscissa_scale = AbscissaScale.LINEAR
        abscissa_value_ranges: list[tuple[float, float]] = [(float(block_variables[0][1][0]), float(block_variables[0][1][-1]))] if block_num_points > 0 else [(0.0, 0.0)]
        # variable data
        variables: list[tuple[str, list[np.ndarray], str | None, str | None]] = []
        # information from first block
        title = block_headers.get("Title", "")
        is_complex = block_is_complex
        # append the first block's variable data to the accumulator
        for name, data_array, unit, vtype in block_variables:
            variables.append((name, [data_array], unit, vtype))
        # abscissa index offset
        abscissa_index_offset = block_num_points
        # process the rest of the blocks
        for block_headers, block_num_points, block_is_complex, block_variables in blocks[1:]:
            # append variables from this step
            for idx, (name, data_array, unit, vtype) in enumerate(block_variables):
                # existing variable from previous steps
                variable_definition = variables[idx] if idx < len(variables) else None
                if variable_definition is None or variable_definition[0] != name:
                    # log error
                    logger.error("Invalid Xyce RAW file, all steps must have the same variables in the same order: missing variable '%s' in step %d", name, len(variables))
                    # exit
                    return None
                # append data for this step
                variable_definition[1].append(data_array)
            # append left and right abscissa values for this step
            abscissa_value_ranges.append((float(block_variables[0][1][0]), float(block_variables[0][1][-1])) if block_num_points > 0 else (0.0, 0.0))
            # append abscissa slice for this step
            abscissa_indices.append(slice(abscissa_index_offset, abscissa_index_offset + block_num_points))
            # advance the abscissa index offset
            abscissa_index_offset += block_num_points
        # construct StepInformation
        step_information = StepInformation(step_keys, step_values, abscissa_indices, abscissa_value_ranges)
        # expressions from file variables
        expressions = [Expression(name, steps, unit, variable_type=vtype) for name, steps, unit, vtype in variables]
        # abscissa expression is the first variable (index 0) across all steps
        abscissa = _process_abscissa_scale(expressions[0], abscissa_scale)
        # update the abscissa in the expressions list to reflect the scaled version
        expressions[0] = abscissa
        # build step slices tuple for @N selector support in the expression manager
        step_slices: tuple[slice, ...] | None = tuple(abscissa_indices) if step_count > 1 else None
        # create expression manager with all parsed variables
        expression_manager = ExpressionManager(expressions, step_slices)
        # create XyceOutputFile instance with parsed header, variables, and data; pass the mmap so it stays alive for the lifetime of the object — variable arrays are views into it
        return XyceOutputFile(filename=path, title=title, complex=is_complex, step_information=step_information, abscissa=abscissa, abscissa_scale=abscissa_scale, expression_manager=expression_manager, _mmap=data)
    finally:
        # log information
        logger.info("Finished loading Xyce RAW file: %s, latency: %f seconds", path, time.perf_counter() - start_time)
