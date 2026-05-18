"""
Decimation algorithms for downsampling large waveform vectors for screen presentation.

Two public entry points are provided:

    decimate(values, target, algorithm)     -> np.ndarray
    decimate_xy(x, y, target, algorithm)   -> tuple[np.ndarray, np.ndarray]

Always prefer ``decimate_xy`` when both an abscissa (x) and an ordinate (y)
are available.  The value-dependent algorithms (MIN_MAX, M4, LTTB) select
indices based on the **y** values and then apply those same indices to **x**,
which guarantees that plotted x/y pairs always correspond to the same original
sample point.  Decimating x and y independently would produce mismatched pairs
and distort the rendered waveform.

The 'target' parameter is the desired maximum number of output points.  When
the input vector is already shorter than 'target', it is returned unchanged.

Algorithm comparison
--------------------
NTH_POINT  – Keeps every N-th sample.  Fastest possible path (O(1) setup,
              zero arithmetic).  May completely miss narrow transients or
              shift their apparent timing.  Suitable when speed is the only
              priority and the signal is known to be smooth.

MIN_MAX    – Splits the vector into buckets and retains the index of the
              minimum and the maximum inside each bucket.  Guarantees that
              amplitude extremes are never lost, which makes glitches and
              spikes always visible.  Can slightly distort the shape of
              smooth sinusoids because the two extremes are not evenly spaced.

M4         – Extends MIN_MAX by also keeping the first and last sample of
              every bucket (4 points per bucket).  The extra anchors preserve
              the gross waveform shape between extremes, giving better visual
              fidelity at only a small performance cost over MIN_MAX.  Good
              all-round default for SPICE waveforms.

LTTB       – Largest Triangle Three Buckets.  For each output point it picks
              the sample inside the current bucket that forms the triangle
              with the largest area relative to the previously selected point
              and the centroid of the next bucket.  Maximises the perceived
              visual difference between the decimated and the original curve.
              Best choice for smooth continuous signals (AC sweeps, Bode
              plots).  May miss very narrow spikes because it optimises shape
              rather than extremes.

RDP        – Ramer-Douglas-Peucker polyline simplification.  Selects the
              smallest subset of original samples whose piecewise-linear
              interpolation stays within a tolerance of the original curve.
              Here tolerance is auto-tuned (binary search) so output has at
              most *target* points while preserving endpoints.  Works well for
              traces with long nearly-linear stretches and occasional bends.

AVERAGE    – Replaces each bucket with its arithmetic mean.  Smooths
              high-frequency noise and gives a clean envelope view.  Hides
              transients and peak values — useful for noisy measurements where
              the trend matters more than individual extremes.  When used with
              ``decimate_xy``, the x values inside each bucket are also
              averaged so the output x represents the centroid of the bucket
              rather than a specific sample.

NONE       – Passes the original vector through unchanged.  Disables
              decimation entirely; every sample is handed to the renderer.
              Useful for short captures where the point count is already
              within the display resolution, or for debugging.
"""

import enum

import numpy as np


class DecimationAlgorithm(enum.Enum):
    NONE = "none"
    NTH_POINT = "nth_point"
    MIN_MAX = "min_max"
    M4 = "m4"
    LTTB = "lttb"
    RDP = "rdp"
    AVERAGE = "average"


# ---------------------------------------------------------------------------
# Index-selection helpers (value-dependent algorithms)
# Each function returns a sorted int64 index array into the original vector.
# Separating index computation from value selection allows the same indices
# to be reused for the paired abscissa array in decimate_xy().
# The helper ``_trim_indices`` is used to make sure the result never
# contains more than *target* points, which might happen when the bucketed
# algorithms produce two extremes per bucket and then deduplicate.
# ---------------------------------------------------------------------------


def _trim_indices(indices: np.ndarray, target: int) -> np.ndarray:
    """Ensure ``indices`` contains at most *target* entries while keeping
    both endpoints.

    If the array is already short enough it is returned unchanged.  When
    trimming is required we always keep the first and last index and choose
    the remaining slots by sampling the sorted interior evenly.
    """
    if len(indices) <= target:
        return indices
    # keep first and last, sample the interior
    if target <= 2:
        return np.array([indices[0], indices[-1]])[:target]
    # select evenly spaced positions in the interior (excluding endpoints)
    interior = indices[1:-1]
    count = target - 2
    # np.linspace with dtype=int may produce duplicates, so use float and
    # round below
    pos = np.linspace(0, len(interior) - 1, count)
    chosen = interior[np.round(pos).astype(np.int64)]
    return np.concatenate(([indices[0]], chosen, [indices[-1]]))


def _nth_point_indices(length: int, target: int) -> np.ndarray:
    # generate at most *target* indices evenly spaced through the range
    # [0, length-1].  ``numpy.linspace`` handles the edge cases neatly and
    # always includes the first sample; ``np.unique`` is used afterwards to
    # remove duplicates that can occur when the requested point count is
    # comparable to the input length.
    if length <= target:
        # caller will handle the short-circuit case, but having a quick path
        # here simplifies the logic in ``decimate``.
        return np.arange(length, dtype=np.int64)
    # linspace with integer dtype will floor the generated values; unique
    # keeps the count <= target and preserves monotonic order.
    indices = np.linspace(0, length - 1, num=target, dtype=np.int64)
    return np.unique(indices)


def _min_max_indices(values: np.ndarray, target: int) -> np.ndarray:
    length = len(values)
    # extremely small targets are trivial
    if target <= 1:
        return np.array([0], dtype=np.int64)
    if target == 2:
        return np.array([0, length - 1], dtype=np.int64)
    # determine how many points should be in each bucket so that two points
    # per bucket (min & max) will not exceed *target* before deduplication
    half = max(1, target // 2)
    points_per_bucket = max(1, (length + half - 1) // half)
    number_of_buckets = length // points_per_bucket
    # reshape the portion of the array that fits evenly into buckets
    trimmed = values[: number_of_buckets * points_per_bucket].reshape(number_of_buckets, points_per_bucket)
    # find extrema within each bucket
    min_indices = np.argmin(trimmed, axis=1)
    max_indices = np.argmax(trimmed, axis=1)
    # build the absolute indices, interleaving min and max
    indexes = np.empty((number_of_buckets * 2,), dtype=np.int64)
    indexes[0::2] = min_indices + np.arange(number_of_buckets) * points_per_bucket
    indexes[1::2] = max_indices + np.arange(number_of_buckets) * points_per_bucket
    # sort and deduplicate the result
    indices = np.unique(indexes)
    # final trimming ensures length <= target
    # ensure the count still respects *target* after deduplication
    return _trim_indices(indices, target)


def _m4_indices(values: np.ndarray, target: int) -> np.ndarray:
    length = len(values)
    # trivial cases first
    if target <= 1:
        return np.array([0], dtype=np.int64)
    if target == 2:
        return np.array([0, length - 1], dtype=np.int64)
    # choose bucket size so that four points per bucket stays within target
    quarter = max(1, target // 4)
    points_per_bucket = max(1, (length + quarter - 1) // quarter)
    number_of_buckets = length // points_per_bucket
    bucket_offsets = np.arange(number_of_buckets) * points_per_bucket
    # reshape for bucket-wise calculations
    trimmed = values[: number_of_buckets * points_per_bucket].reshape(number_of_buckets, points_per_bucket)
    # collect first/last/min/max positions within each bucket
    first_indices = np.zeros(number_of_buckets, dtype=np.int64)
    last_indices = np.full(number_of_buckets, points_per_bucket - 1, dtype=np.int64)
    min_indices = np.argmin(trimmed, axis=1)
    max_indices = np.argmax(trimmed, axis=1)
    indexes = np.concatenate([
        first_indices + bucket_offsets,
        last_indices + bucket_offsets,
        min_indices + bucket_offsets,
        max_indices + bucket_offsets,
    ])
    indices = np.unique(indexes)
    # ensure we didn't accidentally exceed the limit
    return _trim_indices(indices, target)


def _lttb_indices(x: np.ndarray, values: np.ndarray, target: int) -> np.ndarray:
    length = len(values)
    # handle very small target values explicitly so we don't overwrite the
    # only slot when target == 1 (previous implementation ended up returning
    # the *last* point, which surprised callers).
    if target <= 0:
        return np.empty(0, dtype=np.int64)
    if target == 1:
        return np.array([0], dtype=np.int64)
    # output buffer for indices; first and last slots are fixed below
    out_indices = np.empty(target, dtype=np.int64)
    out_indices[0] = 0
    out_indices[-1] = length - 1
    # target == 2 is now satisfied, and we can return early without entering
    # the bucket loop that would divide by zero.
    if target == 2:
        return out_indices
    # divide the interior into (target - 2) buckets; each iteration selects one point
    bucket_size = (length - 2) / (target - 2)
    # index of the previously selected point (starts at the first sample)
    prev_index = 0
    # loop over output slots — skip first and last which are already fixed
    for i in range(1, target - 1):
        # boundaries of the current bucket
        bucket_start = int((i - 1) * bucket_size) + 1
        bucket_end = min(int(i * bucket_size) + 1, length - 1)
        # boundaries of the next bucket (used to compute its centroid)
        next_start = bucket_end
        next_end = min(int((i + 1) * bucket_size) + 1, length - 1)
        # centroid of the next bucket — the "far" apex of the triangle
        next_x_avg = np.mean(x[next_start:next_end])
        next_y_avg = np.mean(values[next_start:next_end])
        # point A: the previously selected sample
        ax = float(x[prev_index])
        ay = float(values[prev_index])
        # candidate points in the current bucket
        bx = x[bucket_start:bucket_end]
        by = values[bucket_start:bucket_end]
        # triangle area = 0.5 * |cross product of (A→B) × (A→C)|
        # the 0.5 factor is constant so we maximise the absolute cross product
        area = np.abs((ax - next_x_avg) * (by - ay) - (ax - bx) * (next_y_avg - ay))
        # pick the candidate with the largest triangle area
        best_relative = int(np.argmax(area))
        best_index = bucket_start + best_relative
        out_indices[i] = best_index
        prev_index = best_index
    # exit — indices are already monotonically increasing
    return out_indices


def _rdp_indices_for_epsilon(x: np.ndarray, values: np.ndarray, epsilon: float) -> np.ndarray:
    # vector length
    length = len(values)
    # initialize keep mask and always preserve endpoints
    keep_mask = np.zeros(length, dtype=bool)
    keep_mask[0] = True
    keep_mask[-1] = True
    # stack of segments to process (iterative form avoids recursion limits)
    segment_stack: list[tuple[int, int]] = [(0, length - 1)]
    # process all segments
    while segment_stack:
        # pop one segment
        start_index, end_index = segment_stack.pop()
        # segments with no interior points are already simplified
        if end_index <= start_index + 1:
            continue
        # segment endpoints
        x_start = float(x[start_index])
        y_start = float(values[start_index])
        x_end = float(x[end_index])
        y_end = float(values[end_index])
        # interior candidate points
        x_interior = x[start_index + 1:end_index]
        y_interior = values[start_index + 1:end_index]
        # guard against empty interior slice
        if len(x_interior) == 0:
            continue
        # segment direction vector
        dx = x_end - x_start
        dy = y_end - y_start
        # perpendicular distance from each interior point to the segment line
        if dx == 0.0 and dy == 0.0:
            distances = np.hypot(x_interior - x_start, y_interior - y_start)
        else:
            denominator = np.hypot(dx, dy)
            distances = np.abs((x_interior - x_start) * dy - (y_interior - y_start) * dx) / denominator
        # locate farthest interior point
        farthest_relative_index = int(np.argmax(distances))
        farthest_distance = float(distances[farthest_relative_index])
        # keep and split only when the tolerance is exceeded
        if farthest_distance > epsilon:
            farthest_index = start_index + 1 + farthest_relative_index
            keep_mask[farthest_index] = True
            segment_stack.append((start_index, farthest_index))
            segment_stack.append((farthest_index, end_index))
    # return kept indices in ascending order
    return np.flatnonzero(keep_mask).astype(np.int64)


def _rdp_indices(x: np.ndarray, values: np.ndarray, target: int) -> np.ndarray:
    length = len(values)
    # explicit tiny-target handling keeps behavior aligned with other algorithms
    if target <= 0:
        return np.empty(0, dtype=np.int64)
    if target == 1:
        return np.array([0], dtype=np.int64)
    if target == 2:
        return np.array([0, length - 1], dtype=np.int64)
    if length <= target:
        return np.arange(length, dtype=np.int64)
    # epsilon=0 keeps all points, epsilon=max_distance collapses to endpoints
    epsilon_low = 0.0
    x0 = float(x[0])
    y0 = float(values[0])
    x1 = float(x[-1])
    y1 = float(values[-1])
    dx = x1 - x0
    dy = y1 - y0
    if dx == 0.0 and dy == 0.0:
        epsilon_high = float(np.max(np.hypot(x - x0, values - y0)))
    else:
        denom = float(np.hypot(dx, dy))
        epsilon_high = float(np.max(np.abs((x - x0) * dy - (values - y0) * dx) / denom))
    # binary-search epsilon so output length is as close as possible without exceeding target
    best = _rdp_indices_for_epsilon(x, values, epsilon_high)
    for _ in range(24):
        epsilon_mid = (epsilon_low + epsilon_high) / 2.0
        candidate = _rdp_indices_for_epsilon(x, values, epsilon_mid)
        if len(candidate) > target:
            epsilon_low = epsilon_mid
        else:
            best = candidate
            epsilon_high = epsilon_mid
    # final guard: if binary search lands below target due to discrete jumps, trim safely
    return _trim_indices(best, target)


# ---------------------------------------------------------------------------
# Individual algorithm implementations (single-vector convenience wrappers)
# ---------------------------------------------------------------------------

def _nth_point(values: np.ndarray, target: int) -> np.ndarray:
    # vector length
    length = len(values)
    # if we have fewer points than the target, just return the original array
    if length <= target:
        return values
    return values[_nth_point_indices(length, target)]


def _min_max(values: np.ndarray, target: int) -> np.ndarray:
    # vector length
    length = len(values)
    # if we have fewer points than the target, just return the original array
    if length <= target:
        return values
    return values[_min_max_indices(values, target)]


def _m4(values: np.ndarray, target: int) -> np.ndarray:
    # vector length
    length = len(values)
    # if we have fewer points than the target, just return the original array
    if length <= target:
        return values
    return values[_m4_indices(values, target)]


def _lttb(values: np.ndarray, target: int) -> np.ndarray:
    # vector length
    length = len(values)
    # if we have fewer points than the target, just return the original array
    if length <= target:
        return values
    # build an evenly spaced index axis — only relative spacing matters for area
    x = np.arange(length, dtype=np.float64)
    return values[_lttb_indices(x, values, target)]


def _rdp(values: np.ndarray, target: int) -> np.ndarray:
    # vector length
    length = len(values)
    # if we have fewer points than the target, just return the original array
    if length <= target:
        return values
    # use sample index as x axis for single-vector decimation
    x = np.arange(length, dtype=np.float64)
    return values[_rdp_indices(x, values, target)]


def _average(values: np.ndarray, target: int) -> np.ndarray:
    # vector length
    length = len(values)
    # if we have fewer points than the target, just return the original array
    if length <= target:
        return values
    # ceiling division: ensures number_of_buckets <= target so output <= target
    points_per_bucket = max(1, (length + target - 1) // target)
    # number of buckets
    number_of_buckets = length // points_per_bucket
    # trimmed vector reshaped into (buckets × points_per_bucket), then averaged
    trimmed = values[:number_of_buckets * points_per_bucket].reshape(number_of_buckets, points_per_bucket)
    # exit — return the mean of each bucket as the decimated signal
    return trimmed.mean(axis=1)


def _none(values: np.ndarray, target: int) -> np.ndarray:  # noqa: ARG001
    # pass the original vector through unchanged — decimation disabled
    return values


# ---------------------------------------------------------------------------
# Dispatch table and public entry point
# ---------------------------------------------------------------------------

_ALGORITHM_FN = {
    DecimationAlgorithm.NONE: _none,
    DecimationAlgorithm.NTH_POINT: _nth_point,
    DecimationAlgorithm.MIN_MAX: _min_max,
    DecimationAlgorithm.M4: _m4,
    DecimationAlgorithm.LTTB: _lttb,
    DecimationAlgorithm.RDP: _rdp,
    DecimationAlgorithm.AVERAGE: _average,
}


def decimate(values: np.ndarray, target: int, algorithm: DecimationAlgorithm = DecimationAlgorithm.M4) -> np.ndarray:
    """Decimate *values* to at most *target* points using the chosen algorithm.

    Parameters
    ----------
    values    : 1-D numpy array of real (float64) samples.
    target    : desired maximum number of output points.  Must be >= 1 unless
                ``algorithm`` is ``DecimationAlgorithm.NONE``.
    algorithm : one of the DecimationAlgorithm enum members (default: M4).

    Returns
    -------
    A numpy array with at most *target* elements.  If ``len(values) <= target``
    the original array is returned unchanged regardless of the algorithm.

    Raises
    ------
    ValueError
        If ``target`` is less than 1 (and the algorithm is not ``NONE``) or
        if ``algorithm`` is not a recognised enum member.
    """
    # validate target; ``NONE`` deliberately bypasses decimation, so allow
    # any positive or zero value in that case.
    if target < 1 and algorithm != DecimationAlgorithm.NONE:
        raise ValueError("target must be >= 1")
    # dispatch to algorithm implementation; give a nicer error message instead
    # of letting a KeyError bubble up.
    try:
        fn = _ALGORITHM_FN[algorithm]
    except KeyError:
        raise ValueError(f"Unknown decimation algorithm: {algorithm}")
    return fn(values, target)


def decimate_xy(x: np.ndarray, y: np.ndarray, target: int, algorithm: DecimationAlgorithm = DecimationAlgorithm.M4) -> tuple[np.ndarray, np.ndarray]:
    """Decimate an (x, y) pair jointly so plotted points remain coherent.

    For value-dependent algorithms (MIN_MAX, M4, LTTB) the index set is
    derived from the *y* values — preserving signal extremes — and then
    applied to *x* as well.  This guarantees that every output (x[i], y[i])
    pair corresponds to the same original sample.

    For AVERAGE, both arrays are bucketed with the same bucket boundaries and
    each output point is the arithmetic mean of its bucket, which is
    consistent and correct.

    For NTH_POINT the stride is value-independent, so x and y are naturally
    coherent regardless of which array drives the selection.

    Parameters
    ----------
    x         : 1-D numpy array of abscissa values (time, frequency, …).
    y         : 1-D numpy array of ordinate values (signal amplitude, …).
    target    : desired maximum number of output points.
    algorithm : one of the DecimationAlgorithm enum members (default: M4).

    Raises
    ------
    ValueError
        If ``target`` is less than 1 (and the algorithm is not ``NONE``) or if
        ``len(x) != len(y)``.

    Returns
    -------
    Tuple (x_decimated, y_decimated), each a numpy array with at most *target* elements.  If ``len(y) <= target`` both arrays are returned unchanged.
    """
    # validate input lengths early to avoid confusing index errors later
    if len(x) != len(y):
        raise ValueError("x and y must have the same length")
    if target < 1 and algorithm != DecimationAlgorithm.NONE:
        raise ValueError("target must be >= 1")
    # NONE — pass both arrays through unchanged while ensuring C-contiguous buffers for downstream Qt upload paths
    if algorithm == DecimationAlgorithm.NONE:
        return np.ascontiguousarray(x), np.ascontiguousarray(y)
    # vector length
    length = len(y)
    # if we have fewer points than the target, return arrays directly while ensuring C-contiguous buffers for downstream Qt upload paths
    if length <= target:
        return np.ascontiguousarray(x), np.ascontiguousarray(y)
    # AVERAGE — bucket both x and y with the same bucket boundaries;
    # ceiling division ensures number_of_buckets <= target
    if algorithm == DecimationAlgorithm.AVERAGE:
        points_per_bucket = max(1, (length + target - 1) // target)
        number_of_buckets = length // points_per_bucket
        trim = number_of_buckets * points_per_bucket
        x_out = x[:trim].reshape(number_of_buckets, points_per_bucket).mean(axis=1)
        y_out = y[:trim].reshape(number_of_buckets, points_per_bucket).mean(axis=1)
        return x_out, y_out
    # index-based algorithms — derive indices from y, apply to both x and y
    if algorithm == DecimationAlgorithm.NTH_POINT:
        indices = _nth_point_indices(length, target)
    elif algorithm == DecimationAlgorithm.MIN_MAX:
        indices = _min_max_indices(y, target)
    elif algorithm == DecimationAlgorithm.M4:
        indices = _m4_indices(y, target)
    elif algorithm == DecimationAlgorithm.LTTB:
        # x is always float64 at this call site (binary format guarantees <f8 for
        # both real and complex QRAW files; rfftfreq also returns float64 for the
        # FFT synthetic path) — pass it directly with no conversion
        indices = _lttb_indices(x, y, target)
    elif algorithm == DecimationAlgorithm.RDP:
        # use the real x axis so distance is computed in chart space
        indices = _rdp_indices(x, y, target)
    else:
        raise ValueError(f"Unknown decimation algorithm: {algorithm}")
    # exit
    return x[indices], y[indices]
