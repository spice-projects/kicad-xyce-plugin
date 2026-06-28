import logging
import re
import time
from pathlib import Path

import numpy as np

from .expression import Expression
from .expression import ExpressionManager
from .xyce_raw_file import AbscissaScale, StepInformation

logger = logging.getLogger(__name__)

# regex patterns for header lines
_RE_SIGNAL_HEADER = re.compile(r"^FFT analysis for (.+):$")
_RE_WINDOW_LINE = re.compile(r"Window:\s*(\S+),\s*Start Time:\s*([0-9eE+\-.]+),\s*Stop Time:\s*([0-9eE+\-.]+)")
_RE_HARMONIC_LINE = re.compile(r"First Harmonic:\s*([0-9eE+\-.]+),\s*Start Freq:\s*([0-9eE+\-.]+),\s*Stop Freq:\s*([0-9eE+\-.]+)")
_RE_DC_LINE = re.compile(r"DC component\s+Norm\. Mag=\s*([0-9eE+\-.]+)\s+Phase=\s*([0-9eE+\-.]+)")
_RE_DATA_LINE = re.compile(r"^\s+(\d+)\s+([0-9eE+\-.]+)\s+([0-9eE+\-.]+)\s+([0-9eE+\-.]+)\s*$")


class FftSignalMetadata:

    def __init__(self, window: str, start_time: float, stop_time: float, first_harmonic: float, start_freq: float, stop_freq: float, dc_magnitude: float, dc_phase: float):
        # fields
        self._window = window
        self._start_time = start_time
        self._stop_time = stop_time
        self._first_harmonic = first_harmonic
        self._start_freq = start_freq
        self._stop_freq = stop_freq
        self._dc_magnitude = dc_magnitude
        self._dc_phase = dc_phase

    @property
    def window(self) -> str:
        return self._window

    @property
    def start_time(self) -> float:
        return self._start_time

    @property
    def stop_time(self) -> float:
        return self._stop_time

    @property
    def first_harmonic(self) -> float:
        return self._first_harmonic

    @property
    def start_freq(self) -> float:
        return self._start_freq

    @property
    def stop_freq(self) -> float:
        return self._stop_freq

    @property
    def dc_magnitude(self) -> float:
        return self._dc_magnitude

    @property
    def dc_phase(self) -> float:
        return self._dc_phase


class FftSignal:

    def __init__(self, name: str, metadata: FftSignalMetadata, frequency: Expression, magnitude: Expression, phase: Expression):
        # fields
        self._name = name
        self._metadata = metadata
        self._frequency = frequency
        self._magnitude = magnitude
        self._phase = phase

    @property
    def name(self) -> str:
        return self._name

    @property
    def metadata(self) -> FftSignalMetadata:
        return self._metadata

    @property
    def frequency(self) -> Expression:
        return self._frequency

    @property
    def magnitude(self) -> Expression:
        return self._magnitude

    @property
    def phase(self) -> Expression:
        return self._phase

    @property
    def num_points(self) -> int:
        return len(self._frequency.data)


def _magnitude_expression_name(signal_name: str) -> str:
    # magnitude expression name is the signal name itself
    return signal_name


def _phase_expression_name(signal_name: str) -> str:
    # phase expression name uses the "phase(<signal>)" convention
    return f"phase({signal_name})"


def _parse_signal_block(signal_name: str, lines: list[str]) -> FftSignal | None:
    # parse metadata: window line
    window = ""
    start_time = 0.0
    stop_time = 0.0
    first_harmonic = 0.0
    start_freq = 0.0
    stop_freq = 0.0
    dc_magnitude = 0.0
    dc_phase = 0.0
    # accumulated data columns
    index_list: list[int] = []
    freq_list: list[float] = []
    mag_list: list[float] = []
    phase_list: list[float] = []
    # track header section
    found_window = False
    found_harmonic = False
    found_dc = False
    for line in lines:
        # try window line
        if not found_window:
            m = _RE_WINDOW_LINE.search(line)
            if m:
                window = m.group(1)
                start_time = float(m.group(2))
                stop_time = float(m.group(3))
                found_window = True
                continue
        # try harmonic line
        if not found_harmonic:
            m = _RE_HARMONIC_LINE.search(line)
            if m:
                first_harmonic = float(m.group(1))
                start_freq = float(m.group(2))
                stop_freq = float(m.group(3))
                found_harmonic = True
                continue
        # try DC component line
        if not found_dc:
            m = _RE_DC_LINE.search(line)
            if m:
                dc_magnitude = float(m.group(1))
                dc_phase = float(m.group(2))
                found_dc = True
                continue
        # try data line (skip header row with "Index")
        if "Index" in line:
            continue
        m = _RE_DATA_LINE.match(line)
        if m:
            index_list.append(int(m.group(1)))
            freq_list.append(float(m.group(2)))
            mag_list.append(float(m.group(3)))
            phase_list.append(float(m.group(4)))
    # validate that we got data
    if len(freq_list) == 0:
        # log error
        logger.error("no data points parsed for signal '%s'", signal_name)
        return None
    # build numpy arrays
    freq_array = np.array(freq_list, dtype=np.float64)
    mag_array = np.array(mag_list, dtype=np.float64)
    phase_array = np.array(phase_list, dtype=np.float64)
    # build expressions
    frequency_expr = Expression("frequency", freq_array, "Hz", source=None, variable_type="frequency")
    magnitude_expr = Expression(_magnitude_expression_name(signal_name), mag_array, "", source=None, variable_type="voltage")
    phase_expr = Expression(_phase_expression_name(signal_name), phase_array, "°", source=None, variable_type="phase")
    # build metadata
    metadata = FftSignalMetadata(window=window, start_time=start_time, stop_time=stop_time, first_harmonic=first_harmonic, start_freq=start_freq, stop_freq=stop_freq, dc_magnitude=dc_magnitude, dc_phase=dc_phase)
    # return the parsed signal
    return FftSignal(name=signal_name, metadata=metadata, frequency=frequency_expr, magnitude=magnitude_expr, phase=phase_expr)


class XyceFftFile:

    def __init__(self, filename: Path, signals: list[FftSignal], expression_manager: ExpressionManager):
        # fields
        self._filename = filename
        self._signals = signals
        self._expression_manager = expression_manager
        # abscissa is the shared frequency axis taken from the first signal
        self._abscissa = signals[0].frequency if signals else Expression("frequency", np.array([]), "Hz", source=None, variable_type="frequency")
        # step information: a single step covering the entire frequency axis
        num_points = len(self._abscissa.data)
        abscissa_range = (float(self._abscissa.data[0]), float(self._abscissa.data[-1])) if num_points > 0 else (0.0, 0.0)
        self._step_information = StepInformation(keys=[], values=[], abscissa_indices=[slice(0, num_points)], abscissa_value_ranges=[abscissa_range])

    @property
    def filename(self) -> Path:
        return self._filename

    @property
    def title(self) -> str:
        # derive title from the list of signal names
        return "FFT – " + ", ".join(s.name for s in self._signals) if self._signals else "FFT"

    @property
    def date(self) -> str:
        return ""

    @property
    def plotname(self) -> str:
        return "FFT"

    @property
    def complex(self) -> bool:
        return False

    @property
    def command(self) -> str:
        return ""

    @property
    def abscissa(self) -> Expression:
        return self._abscissa

    @property
    def abscissa_scale(self) -> AbscissaScale:
        return AbscissaScale.LINEAR

    @property
    def step_information(self) -> StepInformation:
        return self._step_information

    @property
    def chart_type(self) -> str:
        # FFT output always uses a frequency abscissa, mapped to the AC chart layout
        return "AC"

    @property
    def signals(self) -> list[FftSignal]:
        return self._signals

    @property
    def expression_manager(self) -> ExpressionManager:
        return self._expression_manager

    @staticmethod
    def load(filename: str | Path, expression_manager: ExpressionManager | None = None) -> "XyceFftFile | None":
        # load file
        path = Path(filename)
        if not path.exists():
            # log error
            logger.error("Xyce FFT file not found: %s", path)
            return None
        # measure time taken to load file
        start_time = time.perf_counter()
        try:
            # log information
            logger.info("Loading Xyce FFT file: %s", path)
            # read entire file as text
            text = path.read_text(encoding="utf-8", errors="replace")
            # split into lines for processing
            all_lines = text.splitlines()
            # locate the start index of each signal block
            signal_starts: list[tuple[int, str]] = []
            for i, line in enumerate(all_lines):
                m = _RE_SIGNAL_HEADER.match(line.strip())
                if m:
                    signal_starts.append((i, m.group(1).strip()))
            # validate that at least one signal was found
            if len(signal_starts) == 0:
                # log error
                logger.error("invalid Xyce FFT file: no signal blocks found in '%s'", path)
                return None
            # parse each signal block
            signals: list[FftSignal] = []
            for block_idx, (start_line, signal_name) in enumerate(signal_starts):
                # lines belonging to this block end at the start of the next block (or end of file)
                if block_idx + 1 < len(signal_starts):
                    end_line = signal_starts[block_idx + 1][0]
                else:
                    end_line = len(all_lines)
                # extract block lines (excluding the "FFT analysis for ..." header line itself)
                block_lines = all_lines[start_line + 1:end_line]
                # parse the signal block
                signal = _parse_signal_block(signal_name, block_lines)
                if signal is None:
                    # log warning and continue with remaining signals
                    logger.warning("Failed to parse signal block for '%s'; skipping", signal_name)
                    continue
                signals.append(signal)
            # validate that we successfully parsed at least one signal
            if len(signals) == 0:
                # log error
                logger.error("invalid Xyce FFT file: no signals could be parsed from '%s'", path)
                return None
            # build list of expressions for the expression manager
            # use the frequency from the first signal as the shared abscissa
            all_expressions: list[Expression] = [signals[0].frequency]
            for signal in signals:
                # add magnitude and phase expressions for each signal
                all_expressions.append(signal.magnitude)
                all_expressions.append(signal.phase)
            # create or extend the expression manager
            if expression_manager is None:
                # create a new expression manager with the parsed expressions
                fft_expression_manager = ExpressionManager(all_expressions)
            else:
                # merge the parsed expressions into the provided expression manager
                existing = list(expression_manager.expressions)
                merged = existing + all_expressions
                fft_expression_manager = ExpressionManager(merged, expression_manager.step_slices)
            # return the parsed file
            return XyceFftFile(filename=path, signals=signals, expression_manager=fft_expression_manager)
        finally:
            # log information
            logger.info("Finished loading Xyce FFT file: %s, latency: %f seconds", path, time.perf_counter() - start_time)
