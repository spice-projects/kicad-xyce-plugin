import glob
import logging
import mmap
import re
import time
from pathlib import Path
from typing import Any

import numpy as np

from .expression import Expression
from .expression import ExpressionManager
from .xyce_output_file import AbscissaScale, StepInformation, VariableType, XyceOutputFile

logger = logging.getLogger(__name__)

# regex patterns for header lines
_RE_SIGNAL_HEADER = re.compile(r"^FFT analysis for (.+):$")
_RE_WINDOW_LINE = re.compile(r"Window:\s*(\S+),\s*Start Time:\s*([0-9eE+\-.]+),\s*Stop Time:\s*([0-9eE+\-.]+)")
_RE_HARMONIC_LINE = re.compile(r"First Harmonic:\s*([0-9eE+\-.]+),\s*Start Freq:\s*([0-9eE+\-.]+),\s*Stop Freq:\s*([0-9eE+\-.]+)")
_RE_DC_LINE = re.compile(r"DC component\s+(.*)\s+Mag=\s*([0-9eE+\-.]+)\s+Phase=\s*([0-9eE+\-.]+)")


def xyce_fft_file_parser(file_pattern: str | Path, step_information: StepInformation, expression_manager: ExpressionManager | None = None) -> list[XyceOutputFile] | None:
    # find all files matching the pattern
    pattern_str = str(file_pattern)
    matching_files = glob.glob(pattern_str)
    if not matching_files:
        # log error
        logger.error("No Xyce FFT files found matching pattern: %s", pattern_str)
        # exit
        return None

    # helper function to extract numerical suffix for sorting
    def get_fft_index(filepath: str) -> int:
        match = re.search(r'fft(\d+)$', filepath)
        return int(match.group(1)) if match else -1

    # sort files by suffix (fft0, fft1, ..., fftN)
    matching_files.sort(key=get_fft_index)

    # measure time taken to load all files
    overall_perf_counter = time.perf_counter()
    try:
        # dict[key, [frequency_list, dict[signal_name, list[dc_magnitude, dc_phase, magnitude_list, phase_list]]]]
        signals: dict[tuple, tuple[list[float], dict[str, list[tuple[dict[str, Any], float, float, list[float], list[float]]]]]] = {}
        # loop through each matching file
        for filepath in matching_files:
            # create a Path object for the file
            path = Path(filepath)
            # measure time taken to load this file
            perf_counter = time.perf_counter()
            try:
                # log information
                logger.info("Loading Xyce FFT file: %s", path)
                # memory-map the file
                with open(path, "rb") as _file:
                    data = mmap.mmap(_file.fileno(), 0, access=mmap.ACCESS_READ)
                # initialize position for scanning
                pos = 0
                # data structure for storing parsed signal values
                frequency: list[float] = []
                magnitude: list[float] = []
                phase: list[float] = []
                # header section data
                signal_name: str = ""
                window: str = ""
                first_harmonic: float = 0.0
                start_freq: float = 0.0
                stop_freq: float = 0.0
                normalized: bool = False
                dc_magnitude: float = 0.0
                dc_phase: float = 0.0
                # metadata
                metadata: dict[str, Any] = {}
                # flags to track if we have found the header lines
                found_signal = False
                found_window = False
                found_harmonic = False
                found_dc = False
                # data point expected index
                expected_index = 0
                # line number for logging
                line_number = -1
                # scan line by line until the end of the file
                while pos < len(data):
                    try:
                        # increment line number
                        line_number += 1
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
                        # check we should try to read data points
                        if expected_index > 0:
                            # check end of data points (empty line)
                            if line == "":
                                # reset expected index
                                expected_index = 0
                                # next line
                                continue
                            # split the line into columns
                            columns = line.split()
                            # validate the number of columns
                            if len(columns) != 4:
                                # log error
                                logger.error("invalid Xyce FFT file: unexpected number of columns (%d) in data point line '%s'", len(columns), line)
                                # exit
                                return None
                            # append the data points to the lists
                            index = int(columns[0])
                            if index != expected_index:
                                # log error
                                logger.error("invalid Xyce FFT file: unexpected data point index %d (expected %d) in '%s'", index, expected_index, path)
                                # exit
                                return None
                            # append the data points
                            frequency.append(float(columns[1]))
                            magnitude.append(float(columns[2]))
                            phase.append(float(columns[3]))
                            # increment expected index for next data point
                            expected_index += 1
                            # next line
                            continue
                        # check line: FFT analysis for ...
                        if not found_signal:
                            m = _RE_SIGNAL_HEADER.match(line)
                            if m:
                                signal_name = m.group(1).strip()
                                found_signal = True
                                # reset metadata
                                metadata = {}
                                # next
                                continue
                        # check line: Window: ..., Start Time: ..., Stop Time: ...
                        if not found_window:
                            m = _RE_WINDOW_LINE.search(line)
                            if m:
                                window = m.group(1)
                                found_window = True
                                # next
                                continue
                        # check line: First Harmonic: ..., Start Freq: ..., Stop Freq: ...
                        if not found_harmonic:
                            m = _RE_HARMONIC_LINE.search(line)
                            if m:
                                first_harmonic = float(m.group(1))
                                start_freq = float(m.group(2))
                                stop_freq = float(m.group(3))
                                found_harmonic = True
                                # next
                                continue
                        # check line: DC component Norm. Mag=..., Phase=...
                        if not found_dc:
                            m = _RE_DC_LINE.search(line)
                            if m:
                                normalized = m.group(1) == "Norm."
                                dc_magnitude = float(m.group(2))
                                dc_phase = float(m.group(3))
                                found_dc = True
                                # next
                                continue
                        # check line: Index ...
                        if line.strip().startswith("Index"):
                            # validate header lines were found
                            if not (found_signal and found_window and found_harmonic and found_dc):
                                # log error
                                logger.error("invalid Xyce FFT file: missing header lines before data points in '%s'", path)
                                # exit
                                return None
                            # reset flags
                            found_signal = False
                            found_window = False
                            found_harmonic = False
                            found_dc = False
                            # initialize expected index for the data points
                            expected_index = 1
                            # reset data lists
                            frequency = []
                            magnitude = []
                            phase = []
                            # generate key based on the abscissa (frequency) and plot window, and normalized flag
                            abscissa_key = (round(first_harmonic, 9), round(start_freq, 6), round(stop_freq, 6), window, normalized)
                            # lookup key in signals dict
                            abscissa_entry: tuple[list[float], dict] = signals.get(abscissa_key)
                            if abscissa_entry is None:
                                # create dictionary entry
                                abscissa_entry = (frequency, {})
                                signals[abscissa_key] = abscissa_entry
                            # lookup signal key in signals dictionary for this abscissa
                            steps: list[tuple[dict[str, Any], float, float, list[float], list[float]]] = abscissa_entry[1].get(signal_name)
                            if steps is None:
                                # create dictionary entry
                                steps = []
                                abscissa_entry[1][signal_name] = steps
                            # append the current step data to the list
                            steps.append((metadata, dc_magnitude, dc_phase, magnitude, phase))
                            # next line
                            continue
                        # check line: THD = ...
                        if line.startswith("THD"):
                            # metadata
                            metadata["THD"] = line[5:].strip()
                            # next
                            continue
                        # check line: SNDR = ...
                        if line.startswith("SNDR"):
                            # metadata
                            metadata["SNDR"] = line[6:].strip()
                            # next
                            continue
                        # check line: ENOB = ...
                        if line.startswith("ENOB"):
                            # metadata
                            metadata["ENOB"] = line[6:].strip()
                            # next
                            continue
                        # check line: SNR = ...
                        if line.startswith("SNR"):
                            # metadata
                            metadata["SNR"] = line[5:].strip()
                            # next
                            continue
                        # check line: SFDR = ...
                        if line.startswith("SFDR"):
                            # metadata
                            metadata["SFDR"] = line[6:].strip()
                            # next
                            continue

                        if line != "":
                            # unexpected line, log warning
                            logger.warning("unexpected line in Xyce FFT file '%s' at line %d: '%s'", path, line_number, line)
                    except Exception:
                        # log information
                        logger.exception("Error processing Xyce FFT file: %s, line: %d", path, line_number, exc_info=True)
                        # exit
                        return None
            finally:
                logger.info("Finished loading Xyce FFT file: %s, latency: %f seconds", path, time.perf_counter() - perf_counter)

        # result
        output_files: list[XyceOutputFile] = []
        # now we can generate the output files and signals list
        for (first_harmonic, _, _, window, normalized), (frequency_list, signal_dict) in signals.items():
            # abscissa data (for one step)
            abscissa_data = np.array([0] + frequency_list, dtype=np.float64)
            # expressions
            expressions: list[Expression] = []
            # create abscissa for file, repeat abscissa_data for each step in the step_information
            abscissa = Expression("frequency", [abscissa_data for _ in range(step_information.length)], VariableType.FREQUENCY.value.unit, source="FFT", variable_type=VariableType.FREQUENCY.value.name)
            # append abscissa expression to the list
            expressions.append(abscissa)
            # process each signal for this abscissa
            for signal_name, steps in signal_dict.items():
                # validate the step count, it should be the same as in the RAW file, otherwise the data is inconsistent
                if len(steps) != step_information.length:
                    # log error
                    logger.error("invalid Xyce FFT file: inconsistent step count for signal '%s' in '%s': expected %d, found %d", signal_name, pattern_str, step_information.length, len(steps))
                    # exit
                    return None
                # signal data
                magnitude_steps: list[np.ndarray] = []
                phase_steps: list[np.ndarray] = []
                # metadata, an entry for each step, with the same order as in the steps list
                metadata_steps: list[dict[str, Any]] = []
                # loop steps
                for (metadata, dc_magnitude, dc_phase, magnitude_list, phase_list) in steps:
                    # append the DC component to the magnitude and phase lists
                    magnitude_steps.append(np.array([dc_magnitude] + magnitude_list, dtype=np.float64))
                    phase_steps.append(np.array([dc_phase] + phase_list, dtype=np.float64))
                    # append metadata for this step
                    metadata_steps.append(metadata)
                # strip signal name, remove `{}`
                signal_name = signal_name.strip("{}")
                # infer magnitude expression unit from signal name
                magnitude_expression_unit = expression_manager.infer_unit(signal_name) if expression_manager else VariableType.UNKNOWN.value.unit
                # create expressions for magnitude and phase
                expressions.append(Expression(f"FFT({signal_name})", magnitude_steps, magnitude_expression_unit, source="FFT", metadata=metadata_steps))
                expressions.append(Expression(f"FFT(phase({signal_name}))", phase_steps, VariableType.PHASE.value.unit, source="FFT", metadata=metadata_steps))
            # abscissa indexes
            abscissa_indices = [slice(idx * len(abscissa_data), (idx + 1) * len(abscissa_data)) for idx in range(step_information.length)]
            # abscissa value ranges
            abscissa_value_ranges = [(abscissa_data[0], abscissa_data[-1]) for _ in range(step_information.length)]
            # re-create step information
            fft_step_information = StepInformation(step_information.keys, step_information.values, abscissa_indices, abscissa_value_ranges)
            # create expressions manager
            fft_expression_manager = ExpressionManager(expressions)
            # file metadata
            metadata = {
                "Window": window,
                "Normalized": normalized,
                "First Harmonic": first_harmonic
            }
            # create output file object
            output_files.append(XyceOutputFile(filename=Path(matching_files[0]), title="FFT analysis", complex=False, step_information=fft_step_information, abscissa=abscissa, abscissa_scale=AbscissaScale.LINEAR, expression_manager=fft_expression_manager, metadata=metadata))
        # exit
        return output_files
    finally:
        # log information
        logger.info("Finished loading Xyce FFT files matching: %s, total latency: %f seconds", pattern_str, time.perf_counter() - overall_perf_counter)
