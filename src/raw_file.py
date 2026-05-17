import logging
import mmap
import re
import time
from enum import Enum
from pathlib import Path

import numpy as np

from expression import Expression
from expression_manager import ExpressionManager

logger = logging.getLogger(__name__)


def _register_noise_spectrum_aliases(expression_manager: ExpressionManager, plotname: str) -> None:
    # only create synthetic aliases for noise analysis files
    if "noise" not in plotname.casefold():
        return
    # snapshot current expression names so synthetic aliases never override real variables or explicit aliases
    existing_names = {expression.name.casefold() for expression in expression_manager.expressions}
    # collect pending aliases first so iterating expressions remains stable while the manager cache grows
    pending_aliases: list[tuple[str, str]] = []
    # process existing expressions
    for expression in expression_manager.expressions:
        # skip non-spectrum expressions
        if not expression.name.casefold().endswith("_spectrum)"):
            continue
        # derive the synthetic base name from the stored spectrum variable
        alias_name = expression.name[:-10] + ")"
        # only fill gaps; do not replace an existing variable or alias
        if alias_name.casefold() in existing_names:
            continue
        # append new expression
        pending_aliases.append((alias_name, expression.name))
        existing_names.add(alias_name.casefold())
    # register synthetic aliases through the expression manager cache
    for alias_name, alias_expression in pending_aliases:
        # log information
        logger.debug("Registering synthetic alias '%s' for noise spectrum expression '%s'", alias_name, alias_expression)
        # evaluate expression to register it in the manager cache; the actual value of the expression is not used since it's a direct alias, so we can skip the costly evaluation of the aliased expression and just store a reference to it in the cache
        expression_manager.evaluate(alias_expression, alias_name)


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
    # custom, not part of the QRAW specification
    "fft": "FFT"
}


class QRawFile:

    def __init__(self, filename: Path, title: str, date: str, plotname: str, complex: bool, step_information: StepInformation, abscissa: Expression, abscissa_scale: AbscissaScale, command: str, plot_suggestion: str, expression_manager: ExpressionManager, _mmap: mmap.mmap | None = None, chart_type: str | None = None):
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
        self._plot_suggestion = plot_suggestion
        self._expression_manager = expression_manager
        # keep the mmap alive for as long as this object exists — Variable._values arrays are zero-copy views into the mmap buffer; closing the mmap would invalidate all of them
        self._mmap = _mmap
        # calculated
        self._abscissa_points = len(abscissa.data)
        self._plot_suggestions: list[PlotSuggestion] | None = None
        # pre-determined chart type
        self._chart_type = chart_type

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
        # check initialized chart type
        if self._chart_type is not None:
            return self._chart_type
        # type unambiguously determines the chart layout
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

    def get_plot_suggestions(self) -> list[PlotSuggestion]:
        # empty or whitespace-only suggestions string means no suggestions
        if not self._plot_suggestion.strip():
            return []
        # calculate plot suggestions
        if self._plot_suggestions is None:
            # file chart type
            file_chart_type = self.chart_type
            # plot suggestions
            self._plot_suggestions = []
            # extract each «...» group in order, ``\xab``/``\xbb`` are the «» characters; using a raw string avoids accidental escaping.
            for group_text in re.findall(r"\xab(.*?)\xbb", self._plot_suggestion):
                # split group text into tokens; the first token may be a mode keyword
                tokens: list[str] = group_text.split()
                # chart type, defaults to chart type for file
                chart_type = file_chart_type
                # resolve chart type from an optional leading mode keyword (ac, tran, dc …)
                if tokens and (resolved := _MODE_TO_CHART.get(tokens[0].lower())) is not None:
                    # use chart type
                    chart_type = resolved
                    # remove mode token so the remaining tokens are variable names or expressions
                    tokens = tokens[1:]
                # expressions in group
                expressions: list[Expression] = []
                # avoid duplicate expressions within the same chart
                seen: set[Expression] = set()
                # loop tokens
                for token in tokens:
                    # evaluate expression
                    expression = self._expression_manager.evaluate(token)
                    # skip unmatched or already-added variables for this chart type
                    if expression is None or expression in seen:
                        continue
                    # add this expression to the chart type's list and mark it as seen
                    seen.add(expression)
                    # append expression
                    expressions.append(expression)
                # append to list
                self._plot_suggestions.append(PlotSuggestion(chart_type=chart_type, expressions=expressions))
        # exit
        return self._plot_suggestions

    @staticmethod
    def load(filename: str | Path) -> "QRawFile | None":
        # load file
        path = Path(filename)
        if not path.exists():
            # log error
            logger.error("QRAW file not found: %s", path)
            # exit
            return None
        # measure time taken to load file
        start_time = time.perf_counter()
        try:
            # log information
            logger.info("Loading QRAW file: %s", path)
            # memory-map the file — the OS pages in only the regions that are actually read, so for a 300-variable file where only a few are displayed, the remaining columns are never loaded into physical RAM
            with open(path, "rb") as _file:
                data = mmap.mmap(_file.fileno(), 0, access=mmap.ACCESS_READ)
            # on POSIX (macOS/Linux) closing the fd after mmap() is safe — the OS keeps the mapping alive independently; the mmap object itself is stored in QRawFile to prevent GC
            # single-pass progressive parser: scan line by line until Binary: is reached
            header: dict[str, str] = {}
            variable_definitions: list[tuple[int, str, VariableType]] = []
            binary_offset = -1
            in_variables = False
            pos = 0
            # aliases
            aliasses: dict[str, str] = {}
            # user-defined function definitions (.func directives)
            func_definitions: list[str] = []
            # process file bytes
            while pos < len(data):
                # find \n
                newline = data.find(b"\n", pos)
                if newline == -1:
                    break
                # line: decode header text using Windows-1252
                line = data[pos:newline].decode("cp1252").strip()
                # log information
                logger.debug(">> %s", line)
                # advance position to next line
                pos = newline + 1
                # state machine to parse header and variables until binary section is reached
                if in_variables:
                    # check for end of variables section
                    if line == "Binary:":
                        # log information
                        logger.debug(">> ...")
                        # binary section starts at the current position in the file
                        binary_offset = pos
                        # exit loop
                        break
                    # parse variable line: expected format is "index\tname\ttype"
                    parts = line.split("\t")
                    # only parse lines with exactly 3 parts; skip malformed lines
                    if len(parts) == 3:
                        # find variable type from string by matching name
                        variable_type = next((vt for vt in VariableType if vt.value.name == parts[2]), None)
                        if variable_type is None:
                            # log information
                            logger.warning("Skipping variable with unknown type: %s", parts[2])
                            # skip malformed or unknown variable type
                            continue
                        # append variable to list
                        variable_definitions.append((int(parts[0]), parts[1], variable_type))
                        # next
                        continue
                # check this is the start of the variables section
                if line == "Variables:":
                    # state machine: next lines will contain variable definitions until "Binary:" is reached
                    in_variables = True
                    # next
                    continue
                # parse header line: expected format is "key: value"
                if ":" in line:
                    # split at the first colon to separate key and value; strip whitespace
                    key, _, value = line.partition(":")
                    # store in header dictionary
                    header[key.strip()] = value.strip()
                # alias
                if line.startswith(".alias"):
                    # split line (space)
                    parts = line.split()
                    # should be three parts ".alias Name Expression"
                    if len(parts) == 3:
                        # append to aliasses
                        aliasses[parts[1]] = parts[2]
                # user-defined function directive
                if line.startswith(".func"):
                    # store the raw directive text for the expression manager to parse
                    func_definitions.append(line)
            # validate that we found the binary section
            if binary_offset < 0:
                # log error
                logger.error("invalid QRAW file: Binary section not found")
                # exit
                return None
            # parse header values needed to interpret the binary data
            complex = "complex" in header.get("Flags", "").lower()
            stepped = "stepped" in header.get("Flags", "").lower()
            num_variables = int(header.get("No. Variables", 0))
            num_points = int(header.get("No. Points", "0").strip())
            # parse abscissa range and optional scale keyword (dec / oct / lin)
            abscissa_parts = header.get("Abscissa", "").split()
            abscissa_scale = AbscissaScale(abscissa_parts[2]) if len(abscissa_parts) >= 3 else AbscissaScale.LINEAR
            # check file contains vectors with complex numbers
            if complex:
                # complex files store the abscissa (index 0) as float64 and the remaining variables as complex128 in a structured array
                row_dtype = np.dtype([("abscissa", "<f8"), ("data", "<c16", num_variables - 1)])
                # parse binary data into a structured array with separate fields for abscissa and data variables; the abscissa is stored in a separate field to allow it to be read as float64 while the data variables are stored as complex128
                matrix = np.frombuffer(data, dtype=row_dtype, offset=binary_offset)
                # abscissa definition
                abscissa_definition = variable_definitions[0]
                # abscissa variable expression
                abscissa_expression = Expression(abscissa_definition[1], matrix["abscissa"], abscissa_definition[2].value.unit, source=None, variable_type=abscissa_definition[2].value.name)
                # all variable expressions
                variables = [abscissa_expression] + [Expression(name, matrix["data"][:, idx - 1], var_type.value.unit, source=None, variable_type=var_type.value.name) for idx, name, var_type in variable_definitions[1:]]
            else:
                # real files store all variables as float64 in a uniform layout; parse binary data into a 2D array with shape (num_points, num_variables)
                flat = np.frombuffer(data, dtype="<f8", offset=binary_offset)
                # reshape flat array into a 2D array with shape (num_points, num_variables); the data is stored in row-major order, so each row corresponds to a point and each column corresponds to a variable
                matrix = flat.reshape(num_points, num_variables)
                # extract variables
                variables = [Expression(name, matrix[:, idx], var_type.value.unit, source=None, variable_type=var_type.value.name) for idx, name, var_type in variable_definitions]
            # process scale (x axis) before step detection so abscissa values are correct
            abscissa = _process_scale(variables[0], abscissa_scale)
            # process steps using the scaled abscissa values — slices are needed by the expression manager for @N
            step_information = _process_steps(stepped, variables, abscissa, num_points)
            # build step slices tuple for @N selector support in expressions and .func definitions
            step_slices: tuple[slice, ...] | None = tuple(step_information.abscissa_indices) if step_information.length > 1 else None
            # create expression manager, passing any user-defined .func definitions and step slice metadata
            expression_manager = ExpressionManager(variables, func_definitions if func_definitions else None, step_slices)
            # register noise-specific synthetic aliases such as V(Onoise) -> V(onoise_spectrum) before user aliases are evaluated
            _register_noise_spectrum_aliases(expression_manager, header.get("Plotname", ""))
            # process aliasses
            if len(aliasses) > 0:
                # loop aliasses
                for alias_name, alias_expression in aliasses.items():
                    try:
                        # evaluate expression using the variables we have so far; this allows aliasses to reference previously-defined aliasses as long as there are no circular references
                        expression_manager.evaluate(alias_expression, alias_name)
                    except Exception as ex:
                        # log error but continue processing other aliasses
                        logger.error("Failed to evaluate expression '%s': %s", alias_name, ex)
            # create QRawFile instance with parsed header, variables, and binary data; pass the mmap so it stays alive for the lifetime of the QRawFile — Variable arrays are views into it
            return QRawFile(filename=path, title=header.get("Title", ""), date=header.get("Date", ""), plotname=header.get("Plotname", ""), complex=complex, step_information=step_information, abscissa=abscissa, abscissa_scale=abscissa_scale, command=header.get("Command", ""), plot_suggestion=header.get("Plot Suggestion(s)", ""), expression_manager=expression_manager, _mmap=data)
        finally:
            # log information
            logger.info("Finished loading QRAW file: %s, latency: %f seconds", path, time.perf_counter() - start_time)
