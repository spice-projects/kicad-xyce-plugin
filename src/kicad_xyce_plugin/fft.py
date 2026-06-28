"""FFT computation for time-domain waveform analysis.

Uses ``numpy.fft`` (``rfft``/``rfftfreq``) so that existing ``ndarray``
waveform data can be passed directly without any C++ integration.

Window functions are stored in ``WINDOW_REGISTRY`` — a plain ``dict``
mapping each ``WindowFunction`` enum member to a callable that accepts a
length *n* and returns a 1-D numpy weight array.  Adding a new window
requires only a single entry in that dict.

Public API
----------
compute_fft_many(x, y_matrix, window, zero_pad, normalize, output, keep_dc)
    Batch computation for multiple signals sharing the same *x* axis.

is_uniform(x, rtol, atol)
    Returns *True* when *x* is sampled on a uniform grid.

resample_uniform(x, y, num_points)
    Re-samples *(x, y)* onto a uniform grid via linear interpolation.

fft_frequency_range(x)
    Returns *(bin_width_hz, nyquist_hz)* for a preview before running the FFT.
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


class ZeroPadding(enum.Enum):
    NONE = "None"
    NEXT_POWER_OF_TWO = "Next Power of Two"


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


def is_uniform(x: np.ndarray, rtol: float = 1e-3, atol: float | None = None) -> bool:
    """Return *True* when *x* is sampled on a uniform grid within *rtol* and *atol*.

    The function uses an adaptive absolute tolerance when *atol* is not
    provided so that very small or very large time-step scales are handled
    robustly without forcing callers to compute a scale-aware tolerance.

    Parameters
    ----------
    x    : 1-D array of sample positions (time, etc.).
    rtol : relative tolerance used for the step-size comparison.
    atol : absolute tolerance used for the step-size comparison. When
           ``None`` an adaptive default based on machine epsilon and the
           data scale is used.
    """
    # degenerate inputs with fewer than two points cannot be non-uniform
    if len(x) < 2:
        return True
    # compute pairwise differences and check all are within rtol/atol of the first step
    diffs = np.diff(x)
    # compute an adaptive absolute tolerance when not supplied
    if atol is None:
        # epsilon for the floating-point type of the input data
        eps = np.finfo(float).eps
        # scale is the largest step magnitude, but never less than 1.0 to
        # avoid overly-small eps-derived tolerances for sub-unit steps
        scale = max(float(np.max(np.abs(diffs))), 1.0)
        # multiply by a small factor to produce a sensible floor
        atol = max(1e-12, eps * scale * 10.0)
    # check that all step differences are close to the first step within the specified tolerances
    return bool(np.all(np.isclose(diffs, diffs[0], rtol=rtol, atol=atol)))


def resample_uniform(x: np.ndarray, y: np.ndarray, num_points: int | None = None) -> tuple[np.ndarray, np.ndarray]:
    """Re-sample *(x, y)* onto a uniform time grid using linear interpolation.

    Parameters
    ----------
    x          : original (possibly non-uniform) time array.
    y          : corresponding signal array.
    num_points : desired number of output points; defaults to ``len(x)``.

    Returns
    -------
    Tuple *(x_uniform, y_uniform)* on an evenly-spaced grid spanning ``[x[0], x[-1]]``.
    """
    # default to same number of points as the input
    if num_points is None:
        num_points = len(x)
    # build an evenly-spaced time grid spanning the same interval as the input
    x_uniform = np.linspace(float(x[0]), float(x[-1]), num_points)
    # interpolate signal values onto the new uniform grid
    y_uniform = np.interp(x_uniform, x, y)
    # return new axis data
    return x_uniform, y_uniform


def compute_fft_many(x: np.ndarray, y_matrix: np.ndarray, window: WindowFunction = WindowFunction.RECTANGULAR, zero_pad: ZeroPadding = ZeroPadding.NONE, normalize: bool = False, output: FftOutput = FftOutput.MAGNITUDE, keep_dc: bool = False) -> tuple[np.ndarray, np.ndarray]:
    """Compute FFT for many real-valued signals that share the same *x* axis.

    Parameters
    ----------
    x         : time values in seconds (1-D, real, at least 2 elements).
    y_matrix  : signal matrix with shape *(signals, samples)*.
    window    : window function applied to each signal before the FFT.
    zero_pad  : zero-padding strategy (``NONE`` or ``NEXT_POWER_OF_TWO``).
    normalize : when *True*, each signal is scaled so its peak magnitude equals 1.
    output    : which quantity to return (magnitude, dB or phase).
    keep_dc   : when *False* (default), per-signal mean is subtracted before windowing.

    Returns
    -------
    Tuple *(frequencies, values_matrix)* where *frequencies* is in **Hz** and
    *values_matrix* has shape *(signals, bins)*.

    Raises
    ------
    ValueError
        If dimensions are invalid, sample counts do not match *x*, fewer than
        2 samples are provided, or an unrecognised enum value is passed.
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
    # validate sample count compatibility between x and each signal
    if y_arr.shape[1] != len(x_arr):
        raise ValueError("x and y_matrix sample counts must match")
    # validate minimum number of samples required for FFT
    if y_arr.shape[1] < 2:
        raise ValueError("at least 2 samples are required for FFT")
    # ensure real-valued matrix input
    y_arr = np.real(y_arr) if np.iscomplexobj(y_arr) else np.asarray(y_arr, dtype=np.float64)
    # resample all signals to a shared uniform grid when necessary
    if not is_uniform(x_arr):
        # create a uniform grid spanning the same interval as the input
        x_uniform = np.linspace(float(x_arr[0]), float(x_arr[-1]), len(x_arr))
        # interpolate each signal onto the new uniform grid
        y_resampled = np.empty_like(y_arr, dtype=np.float64)
        for row_index in range(y_arr.shape[0]):
            y_resampled[row_index] = np.interp(x_uniform, x_arr, y_arr[row_index])
        # switch to the new uniform grid and resampled signals for the rest of the computation
        x_arr = x_uniform
        y_arr = y_resampled
    # sampling interval and rate
    dt = float(x_arr[1] - x_arr[0])
    # validate that the time step is positive
    if dt <= 0:
        raise ValueError("time step dt must be positive")
    # number of input samples
    n = y_arr.shape[1]
    # subtract per-signal mean to remove DC component when keep_dc is False
    if not keep_dc:
        y_arr = y_arr - np.mean(y_arr, axis=1, keepdims=True)
    # apply window function
    win = WINDOW_REGISTRY[window](n)
    # defensive guard: ensure the coherent gain denominator is non-zero
    win_sum = float(np.sum(win))
    if win_sum == 0.0:
        raise ValueError("sum of window weights is zero")
    # apply window to all signals via broadcasting
    y_windowed = y_arr * win
    # zero-padding
    if zero_pad == ZeroPadding.NEXT_POWER_OF_TWO:
        n_fft = int(2 ** np.ceil(np.log2(n)))
    else:
        n_fft = n
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


def fft_frequency_range(x: np.ndarray) -> tuple[float, float]:
    """Return *(bin_width_hz, nyquist_hz)* for a preview.

    Uses the average sampling interval so non-uniform inputs are handled
    gracefully (the result is only approximate for highly non-uniform data).

    Parameters
    ----------
    x : time array (1-D, at least 2 elements).

    Returns
    -------
    Tuple *(bin_width_hz, nyquist_hz)*.  Both values are 0 when the input
    has fewer than 2 elements or the total duration is zero.
    """
    # number of input samples
    n = len(x)
    # degenerate input — cannot compute a meaningful range
    if n < 2:
        return 0.0, 0.0
    # total time span of the signal
    total_duration = float(x[-1] - x[0])
    # degenerate duration — cannot compute a meaningful range
    if total_duration <= 0:
        return 0.0, 0.0
    # average sampling interval (works for non-uniform grids as an approximation)
    dt_avg = total_duration / (n - 1)
    # sampling rate derived from the average interval
    fs = 1.0 / dt_avg
    # frequency bin width
    df = fs / n
    # nyquist frequency is half the sampling rate
    f_nyquist = fs / 2.0
    return df, f_nyquist
