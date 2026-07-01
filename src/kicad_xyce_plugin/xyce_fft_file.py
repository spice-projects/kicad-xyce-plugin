import logging
import re
import time
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from .expression import Expression
from .expression import ExpressionManager
from .xyce_output_file import AbscissaScale, StepInformation, XyceOutputFile

logger = logging.getLogger(__name__)

# regex patterns for header lines
_RE_SIGNAL_HEADER = re.compile(r"^FFT analysis for (.+):$")
_RE_WINDOW_LINE = re.compile(r"Window:\s*(\S+),\s*Start Time:\s*([0-9eE+\-.]+),\s*Stop Time:\s*([0-9eE+\-.]+)")
_RE_HARMONIC_LINE = re.compile(r"First Harmonic:\s*([0-9eE+\-.]+),\s*Start Freq:\s*([0-9eE+\-.]+),\s*Stop Freq:\s*([0-9eE+\-.]+)")
_RE_DC_LINE = re.compile(r"DC component\s+Norm\. Mag=\s*([0-9eE+\-.]+)\s+Phase=\s*([0-9eE+\-.]+)")
_RE_DATA_LINE = re.compile(r"^\s+(\d+)\s+([0-9eE+\-.]+)\s+([0-9eE+\-.]+)\s+([0-9eE+\-.]+)\s*$")
_RE_THD_LINE = re.compile(r"^\s*THD\s*=\s*([0-9eE+\-.]+)\s*dB\s*\(\s*([0-9eE+\-.]+)\s*\)\s*$")
_RE_SNDR_LINE = re.compile(r"^\s*SNDR\s*=\s*([0-9eE+\-.]+)\s*dB\s*$")
_RE_ENOB_LINE = re.compile(r"^\s*ENOB\s*=\s*([0-9eE+\-.]+)\s*bit\s*$")
_RE_SNR_LINE = re.compile(r"^\s*SNR\s*=\s*([0-9eE+\-.]+)\s*dB\s*$")
_RE_SFDR_LINE = re.compile(r"^\s*SFDR\s*=\s*([0-9eE+\-.]+)\s*dB\s+at\s+frequency\s+([0-9eE+\-.]+)\s*$")


@dataclass(frozen=True)
class FftSignalMeasurements:
    thd_db: float | None = None
    thd_value: float | None = None
    sndr_db: float | None = None
    enob_bits: float | None = None
    snr_db: float | None = None
    sfdr_db: float | None = None
    sfdr_frequency: float | None = None


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

    def __init__(self, name: str, metadata: FftSignalMetadata, frequency: Expression, magnitude: Expression, phase: Expression, measurements: FftSignalMeasurements):
        # fields
        self._name = name
        self._metadata = metadata
        self._frequency = frequency
        self._magnitude = magnitude
        self._phase = phase
        self._measurements = measurements

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
    def measurements(self) -> FftSignalMeasurements:
        return self._measurements

    @property
    def num_points(self) -> int:
        return len(self._frequency.data)


def _magnitude_expression_name(signal_name: str) -> str:
    # magnitude expression name is the signal name itself
    return signal_name


def _phase_expression_name(signal_name: str) -> str:
    # phase expression name uses the "phase(<signal>)" convention
    return f"phase({signal_name})"


def _signal_frequency_key(signal: FftSignal) -> tuple[str, tuple[int, ...], bytes]:
    # group signals by exact abscissa match so blocks with different NP values are kept separate
    frequency_data = signal.frequency.data
    return (frequency_data.dtype.str, frequency_data.shape, frequency_data.tobytes())


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
    freq_list: list[float] = []
    mag_list: list[float] = []
    phase_list: list[float] = []
    measurements = FftSignalMeasurements()
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
            freq_list.append(float(m.group(2)))
            mag_list.append(float(m.group(3)))
            phase_list.append(float(m.group(4)))
            continue
        # parse optional FFT measurement lines emitted when FFTOUT=1
        m = _RE_THD_LINE.match(line)
        if m:
            measurements = FftSignalMeasurements(thd_db=float(m.group(1)), thd_value=float(m.group(2)), sndr_db=measurements.sndr_db, enob_bits=measurements.enob_bits, snr_db=measurements.snr_db, sfdr_db=measurements.sfdr_db, sfdr_frequency=measurements.sfdr_frequency)
            continue
        m = _RE_SNDR_LINE.match(line)
        if m:
            measurements = FftSignalMeasurements(thd_db=measurements.thd_db, thd_value=measurements.thd_value, sndr_db=float(m.group(1)), enob_bits=measurements.enob_bits, snr_db=measurements.snr_db, sfdr_db=measurements.sfdr_db, sfdr_frequency=measurements.sfdr_frequency)
            continue
        m = _RE_ENOB_LINE.match(line)
        if m:
            measurements = FftSignalMeasurements(thd_db=measurements.thd_db, thd_value=measurements.thd_value, sndr_db=measurements.sndr_db, enob_bits=float(m.group(1)), snr_db=measurements.snr_db, sfdr_db=measurements.sfdr_db, sfdr_frequency=measurements.sfdr_frequency)
            continue
        m = _RE_SNR_LINE.match(line)
        if m:
            measurements = FftSignalMeasurements(thd_db=measurements.thd_db, thd_value=measurements.thd_value, sndr_db=measurements.sndr_db, enob_bits=measurements.enob_bits, snr_db=float(m.group(1)), sfdr_db=measurements.sfdr_db, sfdr_frequency=measurements.sfdr_frequency)
            continue
        m = _RE_SFDR_LINE.match(line)
        if m:
            measurements = FftSignalMeasurements(thd_db=measurements.thd_db, thd_value=measurements.thd_value, sndr_db=measurements.sndr_db, enob_bits=measurements.enob_bits, snr_db=measurements.snr_db, sfdr_db=float(m.group(1)), sfdr_frequency=float(m.group(2)))
            continue
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
    return FftSignal(name=signal_name, metadata=metadata, frequency=frequency_expr, magnitude=magnitude_expr, phase=phase_expr, measurements=measurements)


def xyce_fft_file_parser(filename: Path) -> tuple[list[XyceOutputFile], list[FftSignal]] | None:
    # load file
    path = Path(filename)
    if not path.exists():
        # log error
        logger.error("Xyce FFT file not found: %s", path)
        # exit
        return None
    # measure time taken to load file
    start_time = time.perf_counter()
    output_files: list[XyceOutputFile] = []
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
        # group signals by exact abscissa so blocks with different NP values become separate output files
        grouped_signals: list[list[FftSignal]] = []
        grouped_signal_map: dict[tuple[str, tuple[int, ...], bytes], list[FftSignal]] = {}
        for signal in signals:
            key = _signal_frequency_key(signal)
            group = grouped_signal_map.get(key)
            if group is None:
                group = []
                grouped_signal_map[key] = group
                grouped_signals.append(group)
            group.append(signal)
        # build one output file per abscissa group
        for group in grouped_signals:
            # use the frequency from the first signal in the group as the shared abscissa
            abscissa = group[0].frequency
            num_points = len(abscissa.data)
            abscissa_range = (float(abscissa.data[0]), float(abscissa.data[-1])) if num_points > 0 else (0.0, 0.0)
            step_information = StepInformation(keys=[], values=[], abscissa_indices=[slice(0, num_points)], abscissa_value_ranges=[abscissa_range])
            # create expression manager with the grouped signal expressions
            all_expressions: list[Expression] = [abscissa]
            for signal in group:
                all_expressions.append(signal.magnitude)
                all_expressions.append(signal.phase)
            fft_expression_manager = ExpressionManager(all_expressions)
            # derive title from parsed signal names
            title = "FFT – " + ", ".join(s.name for s in group)
            # return XyceOutputFile with FFT data
            output_files.append(XyceOutputFile(filename=path, title=title, date="", plotname="FFT", complex=False, step_information=step_information, abscissa=abscissa, abscissa_scale=AbscissaScale.LINEAR, command="", expression_manager=fft_expression_manager))
    finally:
        # log information
        logger.info("Finished loading Xyce FFT file: %s, latency: %f seconds", path, time.perf_counter() - start_time)
    return output_files, signals
