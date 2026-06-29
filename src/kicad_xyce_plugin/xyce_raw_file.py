import logging
import mmap
import re
import time
from pathlib import Path

import numpy as np

from .expression import Expression, ExpressionManager
from .xyce_output_file import AbscissaScale, StepInformation, VariableType, XyceOutputFile

# regex to extract step parameter name and value from a Xyce step-analysis Plotname header
_STEP_PLOTNAME_RE = re.compile(r"^Step Analysis: Step \d+ of \d+ params:\s+name\s*=\s*(\S+)\s+value\s*=\s*(\S+)")

logger = logging.getLogger(__name__)


def _steps_have_consistent_abscissa_direction(abscissa_data: np.ndarray, abscissa_indices: list[slice]) -> bool:
    # infer global direction from first non-flat segment across all data
    global_direction = 0
    for step_slice in abscissa_indices:
        # step abscissa values
        step_data = abscissa_data[step_slice]
        # per-step deltas
        step_delta = np.diff(step_data)
        # ignore flat deltas
        non_zero_delta = step_delta[step_delta != 0]
        # skip flat-only slices
        if len(non_zero_delta) == 0:
            continue
        # infer step direction
        step_direction = 1 if non_zero_delta[0] > 0 else -1
        # set global direction if not set
        if global_direction == 0:
            global_direction = step_direction
        # reject mixed step directions
        if step_direction != global_direction:
            return False
        # reject non-monotonic shape within a step
        if step_direction > 0 and np.any(non_zero_delta < 0):
            return False
        if step_direction < 0 and np.any(non_zero_delta > 0):
            return False
    # all non-flat steps are consistent
    return True


def _process_steps(stepped: bool, expressions: list[Expression], abscissa: Expression, num_points: int) -> StepInformation:
    # check this is a stepped analysis
    if not stepped:
        # not a stepped analysis — return a single step covering the entire abscissa range with no parameter values
        return StepInformation(keys=[], values=[], abscissa_indices=[slice(0, num_points)], abscissa_value_ranges=[(float(abscissa.data[0]), float(abscissa.data[-1]))] if num_points > 0 else [(0.0, 0.0)])
    # parameter expressions
    parameters = [expr for expr in expressions if expr.variable_type == "parameter"]
    if len(parameters) == 0:
        # no parameter variables — try to detect steps from abscissa resets (e.g. NoiseFigure-style: repeated sweeps)
        if num_points > 1:
            # abscissa values
            abscissa_data = abscissa.data
            # infer whether the sweep is ascending or descending from global endpoints
            sweep_ascending = bool(abscissa_data[0] <= abscissa_data[-1])
            # consecutive deltas
            abscissa_delta = np.diff(abscissa_data)
            # a new step starts when the direction reverses across the step boundary
            if sweep_ascending:
                boundaries = np.flatnonzero(abscissa_delta < 0) + 1
            else:
                boundaries = np.flatnonzero(abscissa_delta > 0) + 1
            # check boundaries were found
            if len(boundaries) > 0:
                # step start indices
                starts_list = [0] + boundaries.astype(int).tolist()
                # step end indices
                ends_list = boundaries.astype(int).tolist() + [num_points]
                # step slices
                abscissa_indices = [slice(s, e) for s, e in zip(starts_list, ends_list)]
                # inferred no-parameter sweeps must have uniform lengths
                step_lengths = [step_slice.stop - step_slice.start for step_slice in abscissa_indices]
                if len(set(step_lengths)) > 1:
                    # log information
                    logger.warning("Invalid stepped abscissa: inconsistent inferred step lengths")
                    # fallback to a single step covering the entire abscissa range with no parameter values, since the mixed directions violate the expected shape of stepped analyses and would cause confusion in the UI
                    return StepInformation(keys=[], values=[], abscissa_indices=[slice(0, num_points)], abscissa_value_ranges=[(float(abscissa.data[0]), float(abscissa.data[-1]))] if num_points > 0 else [(0.0, 0.0)])
                # per-step abscissa ranges
                abscissa_value_ranges = [(float(abscissa_data[step_slice.start]), float(abscissa_data[step_slice.stop - 1])) for step_slice in abscissa_indices]
                # log information
                logger.debug("Inferred %d steps from abscissa resets at indices: %s", len(abscissa_indices), [slice.start for slice in abscissa_indices])
                # return inferred step information
                return StepInformation(keys=[], values=[], abscissa_indices=abscissa_indices, abscissa_value_ranges=abscissa_value_ranges)
        # no resets detected — treat as unstepped
        return StepInformation(keys=[], values=[], abscissa_indices=[slice(0, num_points)], abscissa_value_ranges=[(float(abscissa.data[0]), float(abscissa.data[-1]))] if num_points > 0 else [(0.0, 0.0)])
    # stack all parameter values into a matrix (num_points, num_parameters)
    stacked = np.column_stack([expression.data for expression in parameters]) if len(parameters) > 1 else parameters[0].data.reshape(-1, 1)
    # detect changes in parameter values (N - 1, )
    changed = np.any(stacked[1:] != stacked[:-1], axis=1)
    # boundaries
    boundaries = np.flatnonzero(changed) + 1
    # start and end indices of each step
    starts = np.concatenate(([0], boundaries))
    ends = np.concatenate((boundaries, [num_points]))
    # convert to Python int lists for performance — .tolist() converts numpy array to native Python list, avoiding np.int64 scalars in slice operations and step_lengths
    starts_list = starts.astype(int, copy=False).tolist()
    ends_list = ends.astype(int, copy=False).tolist()
    # parameter values at the start of each step
    values = [tuple(stacked[int(s)].tolist()) for s in starts_list]
    # calculate slices for each one of the steps
    abscissa_indices = [slice(s, e) for s, e in zip(starts_list, ends_list)]
    # validate all parameter-derived steps share one monotonic direction
    if not _steps_have_consistent_abscissa_direction(abscissa.data, abscissa_indices):
        # log information
        logger.warning("Invalid stepped abscissa: mixed ascending/descending step directions")
        # fallback to a single step covering the entire abscissa range with no parameter values, since the mixed directions violate the expected shape of stepped analyses and would cause confusion in the UI
        return StepInformation(keys=[], values=[], abscissa_indices=[slice(0, num_points)], abscissa_value_ranges=[(float(abscissa.data[0]), float(abscissa.data[-1]))] if num_points > 0 else [(0.0, 0.0)])
    # per-step abscissa value ranges in display space
    abscissa_value_ranges = [(float(abscissa.data[step_slice.start]), float(abscissa.data[step_slice.stop - 1])) for step_slice in abscissa_indices]
    # log information
    logger.debug("Detected %d steps from parameter changes at indices: %s", len(abscissa_indices), [slice.start for slice in abscissa_indices])
    # create step information object
    return StepInformation(keys=[expression.name for expression in parameters], values=values, abscissa_indices=abscissa_indices, abscissa_value_ranges=abscissa_value_ranges)


def _process_scale(abscissa: Expression, scale: AbscissaScale) -> Expression:
    # log10: apply per-step to preserve multi-step structure
    if scale == AbscissaScale.DECADE:
        return Expression(abscissa.name, [np.log10(step) for step in abscissa.steps], abscissa.unit, abscissa.source, abscissa.variable_type)
    # log2: apply per-step to preserve multi-step structure
    if scale == AbscissaScale.OCTAVE:
        return Expression(abscissa.name, [np.log2(step) for step in abscissa.steps], abscissa.unit, abscissa.source, abscissa.variable_type)
    # linear scale doesn't modify the abscissa
    return abscissa


_MODE_TO_CHART: dict[str, str] = {
    "ac": "AC",
    "tran": "TRANSIENT",
    "dc": "DC",
    "op": "DC",
    "noise": "AC",
    "fft": "FFT"
}


def _parse_binary_variables(data: mmap.mmap, offset: int, variable_definitions: list[tuple[int, str, VariableType | None]], is_complex: bool, num_variables: int, num_points: int) -> list[Expression] | None:
    # xyce binary format: all variables stored as float64 (real) or complex128 (complex), row-major order; the point count may be unknown (0) when No. Points is not specified
    if is_complex:
        # each row contains num_variables complex128 values (16 bytes each); infer row count from available bytes when num_points is 0
        if num_points == 0:
            # calculate number of complete rows from available bytes after the offset
            available_bytes = len(data) - offset
            num_points = available_bytes // (num_variables * 16)
        # read exactly num_points * num_variables complex128 values; the count cap prevents reading trailing non-data bytes
        count = num_points * num_variables
        flat = np.frombuffer(data, dtype="<c16", offset=offset, count=count)
        # reshape to (num_points, num_variables) matrix
        matrix = flat.reshape(num_points, num_variables)
        # build expression list; abscissa (index 0) is frequency — stored as complex but only the real part is meaningful
        variables: list[Expression] = []
        for idx, name, vt in variable_definitions:
            # unit and variable type derived from VariableType when known
            unit = vt.value.unit if vt is not None else ""
            vtype = vt.value.name if vt is not None else None
            # frequency abscissa: take real component only; other variables remain complex
            column_data = matrix[:, idx].real if idx == 0 else matrix[:, idx]
            variables.append(Expression(name, column_data, unit, source=None, variable_type=vtype))
    else:
        # each row contains num_variables float64 values (8 bytes each); infer row count when num_points is 0
        if num_points == 0:
            # calculate number of complete rows from available bytes after the offset
            available_bytes = len(data) - offset
            num_points = available_bytes // (num_variables * 8)
        # read exactly num_points * num_variables float64 values; the count cap prevents reading trailing non-data bytes
        count = num_points * num_variables
        flat = np.frombuffer(data, dtype="<f8", offset=offset, count=count)
        # reshape to (num_points, num_variables) matrix
        matrix = flat.reshape(num_points, num_variables)
        # build expression list
        variables = []
        for idx, name, vt in variable_definitions:
            # unit and variable type derived from VariableType when known
            unit = vt.value.unit if vt is not None else ""
            vtype = vt.value.name if vt is not None else None
            variables.append(Expression(name, matrix[:, idx], unit, source=None, variable_type=vtype))
    return variables


def _parse_ascii_variables(data: mmap.mmap, offset: int, variable_definitions: list[tuple[int, str, VariableType | None]], is_complex: bool, num_variables: int, num_points: int) -> list[Expression] | None:
    # decode the values section as utf-8 text; replace unrecognised bytes rather than raising
    text = data[offset:].decode("utf-8", errors="replace")
    # for complex analysis each variable contributes a real and imaginary float; for real analysis each variable contributes one float
    floats_per_point = num_variables * 2 if is_complex else num_variables
    # accumulated float tokens across all lines; the point index tokens (integers) are stripped
    all_floats: list[float] = []
    # expected integer index of the next data point — used to identify and skip leading index tokens
    expected_index = 0
    for line in text.splitlines():
        # skip blank lines
        stripped = line.strip()
        if not stripped:
            continue
        tokens = stripped.split()
        start = 0
        # check whether the first token is the point index and strip it
        try:
            if int(tokens[0]) == expected_index:
                start = 1
                expected_index += 1
        except (ValueError, IndexError):
            pass
        # parse remaining tokens as floating-point values; skip tokens that are not numeric
        for token in tokens[start:]:
            try:
                all_floats.append(float(token))
            except ValueError:
                pass
        # stop early once the expected number of complete points has been collected
        if num_points > 0 and len(all_floats) >= floats_per_point * num_points:
            break
    # determine how many complete points are available
    actual_points = len(all_floats) // floats_per_point
    if actual_points == 0:
        # log error
        logger.error("invalid Xyce RAW file: no data points parsed from values section")
        return None
    # build a numpy array from the collected floats, trimming any incomplete trailing point
    flat = np.array(all_floats[: actual_points * floats_per_point], dtype=np.float64)
    if is_complex:
        # interleaved real/imag layout: reshape to (actual_points, num_variables, 2) then combine into complex128
        pairs = flat.reshape(actual_points, num_variables, 2)
        complex_matrix = pairs[:, :, 0] + 1j * pairs[:, :, 1]
        # build expression list; abscissa (index 0) is frequency — only real part is meaningful
        variables: list[Expression] = []
        for idx, name, vt in variable_definitions:
            unit = vt.value.unit if vt is not None else ""
            vtype = vt.value.name if vt is not None else None
            column_data = complex_matrix[:, idx].real if idx == 0 else complex_matrix[:, idx]
            variables.append(Expression(name, column_data, unit, source=None, variable_type=vtype))
    else:
        # reshape to (actual_points, num_variables) float64 matrix
        matrix = flat.reshape(actual_points, num_variables)
        # build expression list
        variables = []
        for idx, name, vt in variable_definitions:
            unit = vt.value.unit if vt is not None else ""
            vtype = vt.value.name if vt is not None else None
            variables.append(Expression(name, matrix[:, idx], unit, source=None, variable_type=vtype))
    return variables


def _detect_encoding(data: bytes) -> tuple[str, bytes]:
    # utf-16-le
    if data[1:2] == b'\x00' or data[0:2] == b'\xff\xfe':
        return "utf-16-le", b'\n\x00'
    # utf-16
    if data[0:2] == b'\xfe\xff':
        return "utf-16-be", b'\x00\n'
    # utf-8
    return "utf-8", b'\n'


def _scan_block_header(data: mmap.mmap, start_pos: int, encoding: str, delimiter: bytes) -> tuple[dict[str, str], list[tuple[int, str, VariableType | None]], int, bool] | None:
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
        newline = data.find(delimiter, pos)
        if newline == -1:
            break
        # decode the line; replace unrecognised bytes rather than raising
        line = data[pos:newline].decode(encoding, errors="replace").strip()
        # log header line
        logger.debug(">> %s", line)
        # advance position to the next line
        pos = newline + len(delimiter)
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
    return header, variable_definitions, data_offset, is_ascii


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
        # on POSIX (macOS/Linux) closing the fd after mmap() is safe — the OS keeps the mapping alive independently; the mmap object itself is stored in XyceOutputFile to prevent GC
        # detect encoding from the first few bytes of the file
        encoding, delimiter = _detect_encoding(data[:4])
        # collect all parsed blocks: each entry is (header_dict, expressions, actual_num_points)
        blocks: list[tuple[dict[str, str], list[Expression], int, bool]] = []
        # start scanning from the beginning of the file
        pos = 0
        # multi-block scan loop: each iteration parses one header+data block
        while True:
            # scan the next block header starting at the current position
            block_result = _scan_block_header(data, pos, encoding, delimiter)
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
            if is_ascii:
                block_exprs = _parse_ascii_variables(data, data_offset, block_var_defs, is_complex, num_variables, num_points)
            else:
                block_exprs = _parse_binary_variables(data, data_offset, block_var_defs, is_complex, num_variables, num_points)
            # stop accumulating blocks when parsing fails
            if block_exprs is None or len(block_exprs) == 0:
                break
            # actual number of points after parsing (may differ from header when No. Points was 0)
            actual_num_points = len(block_exprs[0].data)
            # store block result
            blocks.append((block_header, block_exprs, actual_num_points, is_complex))
            # advance past the binary data to the start of the next block; ascii data end is not computable so stop after ascii
            if is_ascii:
                break
            # bytes per data value: float64 for real, complex128 for complex
            bytes_per_value = 16 if is_complex else 8
            # compute the byte position immediately after this block's binary data
            pos = data_offset + actual_num_points * num_variables * bytes_per_value
            # stop when the end of the file has been reached
            if pos >= len(data):
                break
        # validate that at least one block was successfully parsed
        if not blocks:
            # log error
            logger.error("invalid Xyce RAW file: data section not found")
            # exit
            return None
        # extract metadata from the first block
        first_header, first_exprs, first_num_points, first_is_complex = blocks[0]
        # xyce raw files always use linear scale on the abscissa; the file format does not encode a scale keyword
        abscissa_scale = AbscissaScale.LINEAR
        # derive is_stepped from the Flags header (legacy single-block stepped flag)
        is_stepped = "stepped" in first_header.get("Flags", "").lower()
        # single-block file: use existing step detection logic
        if len(blocks) == 1:
            # variables from the single parsed block
            variables = first_exprs
            # apply scale transformation to the abscissa (no-op for linear)
            abscissa = _process_scale(variables[0], abscissa_scale)
            # detect steps from the stepped flag or abscissa resets
            step_information = _process_steps(is_stepped, variables, abscissa, first_num_points)
        else:
            # multi-block file: build per-variable multi-step expressions from accumulated block data
            num_variables = len(first_exprs)
            # per-variable accumulator: each inner list collects one strided view per block
            per_var_steps: list[list[np.ndarray]] = [[] for _ in range(num_variables)]
            # step parameter name extracted from Plotname (shared across all blocks)
            step_param_name: str | None = None
            # parameter value tuple per block (one element per parameter)
            step_param_values: list[tuple] = []
            # flat-layout abscissa slice boundaries for ExpressionManager @N support
            abscissa_indices: list[slice] = []
            # per-step abscissa value ranges for StepInformation
            abscissa_value_ranges: list[tuple[float, float]] = []
            # running byte cursor for flat slice construction
            cursor = 0
            # iterate over all parsed blocks to accumulate per-variable step views
            for block_header, block_exprs, block_num_points, _ in blocks:
                # collect per-variable step views for this block (zero copy: step_data(0) is the strided mmap view)
                for i in range(num_variables):
                    per_var_steps[i].append(block_exprs[i].step_data(0))
                # extract step parameter from Plotname when present
                block_plotname = block_header.get("Plotname", "")
                match = _STEP_PLOTNAME_RE.match(block_plotname)
                if match:
                    # record the parameter name from the first matching block
                    if step_param_name is None:
                        step_param_name = match.group(1)
                    # parse the parameter value as float; fall back to 0.0 on parse error
                    try:
                        param_value: float = float(match.group(2))
                    except ValueError:
                        param_value = 0.0
                    step_param_values.append((param_value,))
                else:
                    # no step metadata available for this block
                    step_param_values.append(())
                # build the flat-layout abscissa slice for this block
                abscissa_indices.append(slice(cursor, cursor + block_num_points))
                # advance the cursor by this block's point count
                cursor += block_num_points
                # record the abscissa value range for this block
                block_abscissa = per_var_steps[0][-1]
                abscissa_value_ranges.append((float(block_abscissa[0]), float(block_abscissa[-1])))
            # build multi-step Expression objects: each variable holds a list of per-block views
            variables = []
            for i in range(num_variables):
                first_expr = first_exprs[i]
                variables.append(Expression(first_expr.name, per_var_steps[i], first_expr.unit, source=None, variable_type=first_expr.variable_type))
            # apply scale transformation to the multi-step abscissa
            abscissa = _process_scale(variables[0], abscissa_scale)
            # determine step keys and values from extracted parameter metadata
            step_keys = [step_param_name] if step_param_name is not None else []
            step_values = step_param_values if step_param_name is not None else [() for _ in blocks]
            # build step information directly from block metadata
            step_information = StepInformation(keys=step_keys, values=step_values, abscissa_indices=abscissa_indices, abscissa_value_ranges=abscissa_value_ranges)
        # build step slices tuple for @N selector support in the expression manager
        step_slices: tuple[slice, ...] | None = tuple(step_information.abscissa_indices) if step_information.length > 1 else None
        # create expression manager with all parsed variables
        expression_manager = ExpressionManager(variables, step_slices)
        # command comes from the Command: header; fall back to the Version: line if present (Xyce optionally writes a Version: line)
        command = first_header.get("Command", first_header.get("Version", ""))
        # create XyceOutputFile instance with parsed header, variables, and data; pass the mmap so it stays alive for the lifetime of the object — variable arrays are views into it
        return XyceOutputFile(filename=path, title=first_header.get("Title", ""), date=first_header.get("Date", ""), plotname=first_header.get("Plotname", ""), complex=first_is_complex, step_information=step_information, abscissa=abscissa, abscissa_scale=abscissa_scale, command=command, expression_manager=expression_manager, _mmap=data)
    finally:
        # log information
        logger.info("Finished loading Xyce RAW file: %s, latency: %f seconds", path, time.perf_counter() - start_time)
