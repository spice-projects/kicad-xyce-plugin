"""FFT computation for time-domain waveform analysis.

Uses ``numpy.fft`` (``rfft``/``rfftfreq``) so that existing ``ndarray``
waveform data can be passed directly without any C++ integration.

Window functions are stored in ``WINDOW_REGISTRY`` — a plain ``dict``
mapping each ``WindowFunction`` enum member to a callable that accepts a
length *n* and returns a 1-D numpy weight array.  Adding a new window
requires only a single entry in that dict.

Public API
----------
compute_fft_many(x, y_matrix, np_points, window, normalize, x_left_index, x_right_index, output, keep_dc)
    Batch computation for multiple signals sharing the same *x* axis.
"""

import collections.abc
import enum

import numpy as np


class WindowFunction(enum.Enum):
    RECTANGULAR = "Rectangular"
    HAMMING = "Hamming"
    HANNING = "Hanning"
    BLACKMAN = "Blackman"


class FftOutput(enum.Enum):
    MAGNITUDE = "Magnitude"
    MAGNITUDE_DB = "Magnitude (dB)"
    PHASE = "Phase"


# ---------------------------------------------------------------------------
# Window function registry — pluggable; add new windows here with one entry.
# Each callable accepts a length n and returns a 1-D numpy weight array.
# ---------------------------------------------------------------------------

WINDOW_REGISTRY: dict[WindowFunction, collections.abc.Callable[[int], np.ndarray]] = {
    WindowFunction.RECTANGULAR: np.ones,
    WindowFunction.HAMMING: np.hamming,
    WindowFunction.HANNING: np.hanning,
    WindowFunction.BLACKMAN: np.blackman,
}


def compute_fft_many(x: np.ndarray, y_matrix: np.ndarray, np_points: int, window: WindowFunction = WindowFunction.RECTANGULAR, normalize: bool = False, x_left_index: int = 0, x_right_index: int | None = None, output: FftOutput = FftOutput.MAGNITUDE, keep_dc: bool = False) -> tuple[np.ndarray, np.ndarray]:
    """Compute FFT for many real-valued signals that share the same *x* axis.

    Parameters
    ----------
    x         : time values in seconds (1-D, real, at least 2 elements).
    y_matrix  : signal matrix with shape *(signals, samples)*.
    np_points : number of points used by FFT/interpolation; must be a power of 2 and >= 4.
    window    : window function applied to each signal before the FFT.
    normalize : when *True*, each signal is scaled so its peak magnitude equals 1.
    x_left_index  : inclusive left index for the selected abscissa interval.
    x_right_index : exclusive right index for the selected abscissa interval; defaults to ``len(x)``.
    output    : which quantity to return (magnitude, dB or phase).
    keep_dc   : when *False* (default), per-signal mean is subtracted before windowing.

    Returns
    -------
    Tuple *(frequencies, values_matrix)* where *frequencies* is in **Hz** and
    *values_matrix* has shape *(signals, bins)*.

    Raises
    ------
    ValueError
        If dimensions are invalid, sample counts do not match *x*, interval
        indices are invalid, fewer than 2 interval samples are provided,
        ``np_points`` is invalid, or an unrecognised enum value is passed.
    """
    # ensure x is a 1-D float array
    x_arr = np.asarray(x, dtype=np.float64)
    if x_arr.ndim != 1:
        raise ValueError("x must be a 1-D array")
    # ensure y_matrix is a 2-D float array with one row per signal
    y_arr = np.asarray(y_matrix)
    if y_arr.ndim == 1:
        y_arr = y_arr.reshape(1, -1)
    if y_arr.ndim != 2:
        raise ValueError("y_matrix must be a 1-D or 2-D array")
    # validate FFT point count type
    if not isinstance(np_points, (int, np.integer)):
        raise ValueError("np_points must be an integer")
    # convert FFT point count to plain int
    n_fft = int(np_points)
    # validate minimum FFT point count
    if n_fft < 4:
        raise ValueError("np_points must be at least 4")
    # validate power-of-two FFT point count
    if n_fft & (n_fft - 1):
        raise ValueError("np_points must be a power of 2")
    # validate sample count compatibility between x and each signal
    if y_arr.shape[1] != len(x_arr):
        raise ValueError("x and y_matrix sample counts must match")
    # default the right interval index to the end of x
    if x_right_index is None:
        x_right_index = len(x_arr)
    # validate interval index types
    if not isinstance(x_left_index, (int, np.integer)) or not isinstance(x_right_index, (int, np.integer)):
        raise ValueError("x_left_index and x_right_index must be integers")
    # convert interval indices to plain ints
    left_index = int(x_left_index)
    right_index = int(x_right_index)
    # validate interval bounds and ordering
    if left_index < 0 or right_index > len(x_arr) or left_index >= right_index:
        raise ValueError("invalid interval indices")
    # validate minimum number of interval samples required for interpolation
    if right_index - left_index < 2:
        raise ValueError("selected interval must contain at least 2 samples")
    # ensure real-valued matrix input
    y_arr = np.real(y_arr) if np.iscomplexobj(y_arr) else np.asarray(y_arr, dtype=np.float64)
    # select the abscissa interval used for the FFT
    x_interval = x_arr[left_index:right_index]
    # validate strict monotonic increase required by interpolation/FFT sampling interval
    if np.any(np.diff(x_interval) <= 0):
        raise ValueError("x values in selected interval must be strictly increasing")
    # select signal samples for the same abscissa interval
    y_interval = y_arr[:, left_index:right_index]
    # interpolate to a uniform grid with the requested FFT point count
    x_uniform = np.linspace(float(x_interval[0]), float(x_interval[-1]), n_fft)
    # allocate matrix for interpolated signals
    y_resampled = np.empty((y_interval.shape[0], n_fft), dtype=np.float64)
    # interpolate each signal row onto the uniform grid
    for row_index in range(y_interval.shape[0]):
        y_resampled[row_index] = np.interp(x_uniform, x_interval, y_interval[row_index])
    # sampling interval and rate
    dt = float(x_uniform[1] - x_uniform[0])
    # validate that the time step is positive
    if dt <= 0:
        raise ValueError("time step dt must be positive")
    # subtract per-signal mean to remove DC component when keep_dc is False
    if not keep_dc:
        y_resampled = y_resampled - np.mean(y_resampled, axis=1, keepdims=True)
    # apply window function
    win = WINDOW_REGISTRY[window](n_fft)
    # defensive guard: ensure the coherent gain denominator is non-zero
    win_sum = float(np.sum(win))
    if win_sum == 0.0:
        raise ValueError("sum of window weights is zero")
    # apply window to all signals via broadcasting
    y_windowed = y_resampled * win
    # compute one-sided FFT for all rows at once
    spectrum = np.fft.rfft(y_windowed, n=n_fft, axis=1)
    # frequency axis in Hz
    frequencies = np.fft.rfftfreq(n_fft, d=dt)
    # amplitude correction for one-sided FFT
    scale = 2.0 / win_sum
    # output formatting
    if output == FftOutput.MAGNITUDE:
        # magnitude is the absolute value of the complex spectrum, scaled by the window gain correction factor
        values = np.abs(spectrum) * scale
        # halve the DC component which is not doubled in one-sided FFT
        values[:, 0] /= 2.0
        # halve the Nyquist bin for even-length FFTs
        if n_fft % 2 == 0:
            values[:, -1] /= 2.0
        # optionally normalise each row so its peak equals 1
        if normalize:
            # compute the peak magnitude for each row and avoid division by zero for rows with zero peak
            peaks = np.max(values, axis=1, keepdims=True)
            # avoid division by zero for rows with zero peak
            peaks = np.where(peaks > 0.0, peaks, 1.0)
            # normalize each row by its peak value
            values = values / peaks
    elif output == FftOutput.MAGNITUDE_DB:
        # magnitude in dB is 20*log10 of the absolute value of the complex spectrum, scaled by the window gain correction factor
        magnitude = np.abs(spectrum) * scale
        # halve the DC component which is not doubled in one-sided FFT
        magnitude[:, 0] /= 2.0
        # halve the Nyquist bin for even-length FFTs
        if n_fft % 2 == 0:
            magnitude[:, -1] /= 2.0
        # optionally normalise each row so its peak equals 1 before dB conversion
        if normalize:
            # compute the peak magnitude for each row and avoid division by zero for rows with zero peak
            peaks = np.max(magnitude, axis=1, keepdims=True)
            # avoid division by zero for rows with zero peak
            peaks = np.where(peaks > 0.0, peaks, 1.0)
            # normalize each row by its peak value before dB conversion
            magnitude = magnitude / peaks
        # clamp to avoid log(0)
        magnitude = np.maximum(magnitude, 1e-300)
        # convert to dB
        values = 20.0 * np.log10(magnitude)
    elif output == FftOutput.PHASE:
        # phase in degrees is the angle of the complex spectrum converted from radians to degrees
        values = np.angle(spectrum, deg=True)
    else:
        # invalid output type
        raise ValueError(f"Unknown FftOutput: {output}")
    # return frequencies and values
    return frequencies, values


def compute_fft_many2(x: np.ndarray, y_matrix: np.ndarray, max_frequency: float, window: WindowFunction = WindowFunction.RECTANGULAR, normalize: bool = False, x_left_index: int = 0, x_right_index: int | None = None, output: FftOutput = FftOutput.MAGNITUDE, keep_dc: bool = False) -> tuple[np.ndarray, np.ndarray]:
    """Compute FFT for many real-valued signals that share the same *x* axis.

    Parameters
    ----------
    x         : time values in seconds (1-D, real, at least 2 elements).
    y_matrix  : signal matrix with shape *(signals, samples)*.
    max_frequency : maximum frequency in Hz used to derive interpolation spacing.
    window    : window function applied to each signal before the FFT.
    normalize : when *True*, each signal is scaled so its peak magnitude equals 1.
    x_left_index  : inclusive left index for the selected abscissa interval.
    x_right_index : exclusive right index for the selected abscissa interval; defaults to ``len(x)``.
    output    : which quantity to return (magnitude, dB or phase).
    keep_dc   : when *False* (default), per-signal mean is subtracted before windowing.

    Returns
    -------
    Tuple *(frequencies, values_matrix)* where *frequencies* is in **Hz** and
    *values_matrix* has shape *(signals, bins)*.
    """
    # ensure x is a 1-D float array
    x_arr = np.asarray(x, dtype=np.float64)
    if x_arr.ndim != 1:
        raise ValueError("x must be a 1-D array")
    # ensure y_matrix is a 2-D float array with one row per signal
    y_arr = np.asarray(y_matrix)
    if y_arr.ndim == 1:
        y_arr = y_arr.reshape(1, -1)
    if y_arr.ndim != 2:
        raise ValueError("y_matrix must be a 1-D or 2-D array")
    # ensure real-valued matrix input
    y_arr = np.real(y_arr) if np.iscomplexobj(y_arr) else np.asarray(y_arr, dtype=np.float64)
    # validate max_frequency and derive interpolation time step
    if not np.isscalar(max_frequency):
        raise ValueError("max_frequency must be a scalar")
    if max_frequency <= 0.0:
        raise ValueError("max_frequency must be positive")
    # validate sample count compatibility between x and each signal
    if y_arr.shape[1] != len(x_arr):
        raise ValueError("x and y_matrix sample counts must match")
    # default the right interval index to the end of x
    if x_right_index is None:
        x_right_index = len(x_arr)
    # validate interval index types
    if not isinstance(x_left_index, (int, np.integer)) or not isinstance(x_right_index, (int, np.integer)):
        raise ValueError("x_left_index and x_right_index must be integers")
    # abscissa indexes
    left_index = int(x_left_index)
    right_index = int(x_right_index)
    # validate interval bounds and ordering
    if left_index < 0 or right_index > len(x_arr) or left_index >= right_index:
        raise ValueError("invalid interval indices")
    # validate minimum number of interval samples required for interpolation
    if right_index - left_index < 2:
        raise ValueError("selected interval must contain at least 2 samples")
    # select the abscissa interval used for the FFT
    x_interval = x_arr[left_index:right_index]
    if np.any(np.diff(x_interval) <= 0):
        raise ValueError("x values in selected interval must be strictly increasing")
    # select signal samples for the same abscissa interval
    y_interval = y_arr[:, left_index:right_index]
    # compute uniform-abscissa point count
    abscissa_left_value = float(x_interval[0])
    abscissa_right_value = float(x_interval[-1])
    # absolute duration of the selected interval
    total_duration = abscissa_right_value - abscissa_left_value
    # derive the number of samples required to satisfy the Nyquist criterion for the requested max_frequency
    delta = 1.0 / (10.0 * max_frequency)
    n_samples = int(np.floor(total_duration / delta)) + 1
    if n_samples < 2:
        raise ValueError("Derived sample count must be at least 2. Decrease max_frequency or expand interval.")
    # interpolate to a uniform grid derived from max_frequency
    x_uniform = np.linspace(abscissa_left_value, abscissa_right_value, n_samples)
    # recalculate the actual time step based on the uniform grid
    dt = float(x_uniform[1] - x_uniform[0])
    # allocate matrix for interpolated signals
    y_resampled = np.empty((y_interval.shape[0], n_samples), dtype=np.float64)
    # interpolate each signal row onto the uniform grid
    for row_index in range(y_interval.shape[0]):
        y_resampled[row_index] = np.interp(x_uniform, x_interval, y_interval[row_index])
    # subtract per-signal mean to remove DC component when keep_dc is False
    if not keep_dc:
        y_resampled = y_resampled - np.mean(y_resampled, axis=1, keepdims=True)
    # calculate the next power of 2 for FFT speed optimization
    n_fft = 2 ** int(np.ceil(np.log2(n_samples)))
    if n_fft < 4:
        n_fft = 4
    # apply window function
    win = WINDOW_REGISTRY[window](n_samples)
    win_sum = float(np.sum(win))
    if win_sum == 0.0:
        raise ValueError("sum of window weights is zero")
    # apply window to all signals via broadcasting
    y_windowed = y_resampled * win
    # compute one-sided FFT for all rows at once
    spectrum = np.fft.rfft(y_windowed, n=n_fft, axis=1)
    frequencies = np.fft.rfftfreq(n_fft, d=dt)
    # Base magnitude processing
    raw_magnitude = np.abs(spectrum)
    # find peaks for normalization before the DC/Nyquist bins are altered
    if normalize:
        # scale factor applies evenly across the row, so we can find raw peaks first
        peaks = np.max(raw_magnitude, axis=1, keepdims=True)
        peaks = np.where(peaks > 0.0, peaks, 1.0)
        raw_magnitude = raw_magnitude / peaks
    # apply physical scale factor (Includes zero-padding correction)
    # Note: If normalize=True, scale factor cancels out during conversion,
    # but we apply it consistently here for uniform data structures.
    if not normalize:
        scale = (2.0 / win_sum) * (float(n_fft) / float(n_samples))
        raw_magnitude = raw_magnitude * scale
    # uniformly halve the non-doubled bins for all magnitude types safely
    raw_magnitude[:, 0] /= 2.0
    if n_fft % 2 == 0:
        raw_magnitude[:, -1] /= 2.0
    # output formatting logic
    if output == FftOutput.MAGNITUDE:
        values = raw_magnitude
    elif output == FftOutput.MAGNITUDE_DB:
        # clamp to avoid log(0)
        raw_magnitude = np.maximum(raw_magnitude, 1e-300)
        # convert to dB
        values = 20.0 * np.log10(raw_magnitude)
    elif output == FftOutput.PHASE:
        # phase in degrees is the angle of the complex spectrum converted from radians to degrees
        values = np.angle(spectrum, deg=True)
    else:
        raise ValueError(f"Unknown FftOutput: {output}")
    # exit
    return frequencies, values
