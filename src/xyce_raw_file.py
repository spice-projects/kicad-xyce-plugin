import logging
import mmap
import time
from enum import Enum
from pathlib import Path

import numpy as np

from expression import Expression
from expression_manager import ExpressionManager

logger = logging.getLogger(__name__)


class AbscissaScale(Enum):
    LINEAR = "lin"
    DECADE = "dec"
    OCTAVE = "oct"


class VariableTypeInformation:

    def __init__(self, name: str, unit: str):
        self._name = name
        self._unit = unit

    @property
    def name(self) -> str:
        return self._name

    @property
    def unit(self) -> str:
        return self._unit


class VariableType(Enum):
    FREQUENCY = VariableTypeInformation("frequency", "Hz")
    VOLTAGE = VariableTypeInformation("voltage", "V")
    CURRENT = VariableTypeInformation("current", "A")
    TIME = VariableTypeInformation("time", "s")
    POWER = VariableTypeInformation("power", "W")
    PARAMETER = VariableTypeInformation("parameter", "")
    PHASE = VariableTypeInformation("phase", "°")


class PlotSuggestion:

    def __init__(self, chart_type: str, expressions: list[Expression]):
        # fields
        self._chart_type = chart_type
        self._expressions = expressions

    @property
    def chart_type(self) -> str:
        return self._chart_type

    @property
    def expressions(self) -> list[Expression]:
        return self._expressions


class StepInformation:

    def __init__(self, keys: list[str], values: list[tuple], abscissa_indices: list[slice], abscissa_value_ranges: list[tuple[float, float]]):
        # fields
        self._keys = keys
        self._values = values
        self._abscissa_indices = abscissa_indices
        self._abscissa_value_ranges = abscissa_value_ranges
        # number of steps
        self._step_count = len(abscissa_indices)
        # determine if abscissa is ascending or descending based on the first step's abscissa value range
        self._abscissa_ascending = self._abscissa_value_ranges[0][0] <= self._abscissa_value_ranges[0][1] if len(self._abscissa_value_ranges) > 0 else True
        # check abscissa direction
        if self._abscissa_ascending:
            # left and right values
            self._abscissa_left_value = float(min((value_range[0] for value_range in self._abscissa_value_ranges), default=0.0))
            self._abscissa_right_value = float(max((value_range[1] for value_range in self._abscissa_value_ranges), default=0.0))
        else:
            # left and right values
            self._abscissa_left_value = float(max((value_range[0] for value_range in self._abscissa_value_ranges), default=0.0))
            self._abscissa_right_value = float(min((value_range[1] for value_range in self._abscissa_value_ranges), default=0.0))

    @property
    def keys(self) -> list[str]:
        return self._keys

    @property
    def values(self) -> list[tuple]:
        return self._values

    @property
    def abscissa_indices(self) -> list[slice]:
        return self._abscissa_indices

    @property
    def length(self) -> int:
        return self._step_count

    @property
    def abscissa_left_value(self) -> float:
        return self._abscissa_left_value

    @property
    def abscissa_right_value(self) -> float:
        return self._abscissa_right_value

    @property
    def abscissa_ascending(self) -> bool:
        return self._abscissa_ascending

    def step_abscissa_left_value(self, step_index: int) -> float:
        return self._abscissa_value_ranges[step_index][0]

    def step_abscissa_right_value(self, step_index: int) -> float:
        return self._abscissa_value_ranges[step_index][1]


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
    # log10
    if scale == AbscissaScale.DECADE:
        return Expression(abscissa.name, np.log10(abscissa.data), abscissa.unit, abscissa.source, abscissa.variable_type)
    # log2
    if scale == AbscissaScale.OCTAVE:
        return Expression(abscissa.name, np.log2(abscissa.data), abscissa.unit, abscissa.source, abscissa.variable_type)
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


class XyceRawFile:

    def __init__(self, filename: Path, title: str, date: str, plotname: str, complex: bool, step_information: StepInformation, abscissa: Expression, abscissa_scale: AbscissaScale, command: str, expression_manager: ExpressionManager, _mmap: mmap.mmap | None = None):
        # fields
        self._filename = filename
        self._title = title
        self._date = date
        self._plotname = plotname
        self._complex = complex
        self._step_information = step_information
        self._abscissa = abscissa
        self._abscissa_scale = abscissa_scale
        self._command = command
        self._expression_manager = expression_manager
        # keep the mmap alive for as long as this object exists — Variable._values arrays are zero-copy views into the mmap buffer; closing the mmap would invalidate all of them
        self._mmap = _mmap
        # calculated
        self._abscissa_points = len(abscissa.data)

    @property
    def filename(self) -> Path:
        return self._filename

    @property
    def title(self) -> str:
        return self._title

    @property
    def date(self) -> str:
        return self._date

    @property
    def plotname(self) -> str:
        return self._plotname

    @property
    def complex(self) -> bool:
        return self._complex

    @property
    def step_information(self) -> StepInformation:
        return self._step_information

    @property
    def steps(self) -> int:
        return self._step_information.length

    @property
    def abscissa(self) -> Expression:
        return self._abscissa

    @property
    def abscissa_scale(self) -> AbscissaScale:
        return self._abscissa_scale

    @property
    def chart_type(self) -> str:
        # abscissa unit unambiguously determines the chart layout
        if self._abscissa.unit == "Hz":
            return "AC"
        if self._abscissa.unit == "s":
            return "TRANSIENT"
        # VOLTAGE (DC transfer sweep) and PARAMETER (operating point sweep) both use the DC layout
        return "DC"

    @property
    def command(self) -> str:
        return self._command

    @property
    def expression_manager(self) -> ExpressionManager:
        return self._expression_manager

    @staticmethod
    def load(filename: str | Path) -> "XyceRawFile | None":
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
            # on POSIX (macOS/Linux) closing the fd after mmap() is safe — the OS keeps the mapping alive independently; the mmap object itself is stored in XyceRawFile to prevent GC
            # single-pass progressive parser: scan line by line until the data section marker is reached
            header: dict[str, str] = {}
            variable_definitions: list[tuple[int, str, VariableType | None]] = []
            data_offset = -1
            is_ascii = False
            in_variables = False
            pos = 0
            # process file bytes
            while pos < len(data):
                # find newline
                newline = data.find(b"\n", pos)
                if newline == -1:
                    break
                # line: decode header text as utf-8; replace unrecognised bytes rather than raising
                line = data[pos:newline].decode("utf-8", errors="replace").strip()
                # log information
                logger.debug(">> %s", line)
                # advance position to next line
                pos = newline + 1
                # state machine to parse header and variables until the data section is reached
                if in_variables:
                    # check for end of variables section — either binary or ascii data marker
                    if line in ("Binary:", "Values:"):
                        # log information
                        logger.debug(">> ...")
                        # data section starts at the current position in the file
                        data_offset = pos
                        # record whether the data is in ascii text format
                        is_ascii = line == "Values:"
                        # exit loop
                        break
                    # parse variable line: expected format is "index\tname\ttype"
                    parts = line.split("\t")
                    # only parse lines with exactly 3 tab-separated parts; skip malformed lines
                    if len(parts) == 3:
                        # find variable type from string by matching name; unknown types are accepted as None rather than skipped, so variable count stays consistent with num_variables
                        variable_type = next((vt for vt in VariableType if vt.value.name == parts[2]), None)
                        if variable_type is None:
                            # log information
                            logger.warning("Unknown variable type '%s' for variable '%s'; treating as untyped", parts[2], parts[1])
                        # append variable to list
                        variable_definitions.append((int(parts[0]), parts[1], variable_type))
                        # next
                        continue
                    # next
                    continue
                # check this is the start of the variables section
                if line == "Variables:":
                    # state machine: next lines will contain variable definitions until "Binary:" or "Values:" is reached
                    in_variables = True
                    # next
                    continue
                # parse header line: expected format is "key: value"
                if ":" in line:
                    # split at the first colon to separate key and value; strip whitespace
                    key, _, value = line.partition(":")
                    # store in header dictionary
                    header[key.strip()] = value.strip()
            # validate that we found the data section
            if data_offset < 0:
                # log error
                logger.error("invalid Xyce RAW file: data section not found")
                # exit
                return None
            # parse header values needed to interpret the data
            is_complex = "complex" in header.get("Flags", "").lower()
            is_stepped = "stepped" in header.get("Flags", "").lower()
            num_variables = int(header.get("No. Variables", 0))
            num_points = int(header.get("No. Points", "0").strip())
            # xyce raw files always use linear scale on the abscissa; the file format does not encode a scale keyword
            abscissa_scale = AbscissaScale.LINEAR
            # parse the data section — ascii (Values:) or binary (Binary:)
            if is_ascii:
                variables = _parse_ascii_variables(data, data_offset, variable_definitions, is_complex, num_variables, num_points)
            else:
                variables = _parse_binary_variables(data, data_offset, variable_definitions, is_complex, num_variables, num_points)
            # validate that variable parsing succeeded
            if variables is None or len(variables) == 0:
                # log error
                logger.error("invalid Xyce RAW file: failed to parse variable data")
                # exit
                return None
            # actual point count after parsing (may differ from header when No. Points was 0)
            actual_num_points = len(variables[0].data)
            # process scale (no-op for linear) before step detection so abscissa values are correct
            abscissa = _process_scale(variables[0], abscissa_scale)
            # process steps using the abscissa values — slices are needed by the expression manager for @N
            step_information = _process_steps(is_stepped, variables, abscissa, actual_num_points)
            # build step slices tuple for @N selector support in expressions
            step_slices: tuple[slice, ...] | None = tuple(step_information.abscissa_indices) if step_information.length > 1 else None
            # create expression manager with all parsed variables
            expression_manager = ExpressionManager(variables, step_slices)
            # command comes from the Command: header; fall back to the Version: line if present (Xyce optionally writes a Version: line)
            command = header.get("Command", header.get("Version", ""))
            # create XyceRawFile instance with parsed header, variables, and data; pass the mmap so it stays alive for the lifetime of the XyceRawFile — Variable arrays are views into it
            return XyceRawFile(filename=path, title=header.get("Title", ""), date=header.get("Date", ""), plotname=header.get("Plotname", ""), complex=is_complex, step_information=step_information, abscissa=abscissa, abscissa_scale=abscissa_scale, command=command, expression_manager=expression_manager, _mmap=data)
        finally:
            # log information
            logger.info("Finished loading Xyce RAW file: %s, latency: %f seconds", path, time.perf_counter() - start_time)
