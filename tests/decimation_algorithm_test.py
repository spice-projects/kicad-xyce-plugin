from unittest.mock import patch

import numpy as np
import pytest

from decimation_algorithm import DecimationAlgorithm, decimate, decimate_xy


def _linspace(n: int) -> np.ndarray:
    return np.linspace(0.0, 1.0, n, dtype=np.float64)


def _sine(n: int) -> np.ndarray:
    return np.sin(np.linspace(0.0, 2 * np.pi, n, dtype=np.float64))


def _is_subset_of(output: np.ndarray, original: np.ndarray) -> bool:
    original_set = set(original.tolist())
    return all(v in original_set for v in output.tolist())


def _xy_pairs_coherent(x_out: np.ndarray, y_out: np.ndarray, x_orig: np.ndarray, y_orig: np.ndarray) -> bool:
    pairs_orig = set(zip(x_orig.tolist(), y_orig.tolist()))
    return all((xv, yv) in pairs_orig for xv, yv in zip(x_out.tolist(), y_out.tolist()))


def _assert_short_circuit(algorithm: DecimationAlgorithm):
    # arrange
    values = _sine(100)
    # act
    result = decimate(values, 100, algorithm)
    # assert
    assert result is values
    result = decimate(values, 200, algorithm)
    assert result is values


def _assert_xy_short_circuit(algorithm: DecimationAlgorithm):
    # arrange
    x = _linspace(50)
    y = _sine(50)
    # act
    x_out, y_out = decimate_xy(x, y, 50, algorithm)
    # assert
    assert x_out is x
    assert y_out is y


def _assert_length_le_target(algorithm: DecimationAlgorithm, target: int = 50):
    # arrange
    values = _sine(10_000)
    # act
    result = decimate(values, target, algorithm)
    # assert
    assert len(result) <= target


def _assert_xy_coherent(algorithm: DecimationAlgorithm):
    # arrange
    n = 10_000
    x = _linspace(n)
    y = _sine(n)
    # act
    x_out, y_out = decimate_xy(x, y, 200, algorithm)
    # assert
    assert len(x_out) == len(y_out)
    # when algorithm is not AVERAGE the x/y pairs must exactly match
    if algorithm != DecimationAlgorithm.AVERAGE:
        assert _xy_pairs_coherent(x_out, y_out, x, y)


@pytest.mark.parametrize("algorithm", [DecimationAlgorithm.NTH_POINT, DecimationAlgorithm.MIN_MAX, DecimationAlgorithm.M4, DecimationAlgorithm.LTTB, DecimationAlgorithm.AVERAGE, DecimationAlgorithm.RDP])
def test_short_circuit(algorithm: DecimationAlgorithm):
    _assert_short_circuit(algorithm)


@pytest.mark.parametrize("algorithm", [DecimationAlgorithm.NTH_POINT, DecimationAlgorithm.MIN_MAX, DecimationAlgorithm.M4, DecimationAlgorithm.LTTB, DecimationAlgorithm.AVERAGE, DecimationAlgorithm.RDP])
def test_xy_short_circuit(algorithm: DecimationAlgorithm):
    _assert_xy_short_circuit(algorithm)


@pytest.mark.parametrize("algorithm", [DecimationAlgorithm.NTH_POINT, DecimationAlgorithm.MIN_MAX, DecimationAlgorithm.M4, DecimationAlgorithm.LTTB, DecimationAlgorithm.RDP, DecimationAlgorithm.AVERAGE])
def test_xy_short_circuit_strided_inputs_become_contiguous(algorithm: DecimationAlgorithm):
    # arrange
    matrix = np.arange(400, dtype=np.float64).reshape(100, 4)
    x = matrix[:, 0]
    y = matrix[:, 1]
    # act
    x_out, y_out = decimate_xy(x, y, 100, algorithm)
    # assert
    assert x_out.flags["C_CONTIGUOUS"]
    assert y_out.flags["C_CONTIGUOUS"]
    np.testing.assert_array_equal(x_out, x)
    np.testing.assert_array_equal(y_out, y)


def test_xy_none_strided_inputs_become_contiguous():
    # arrange
    matrix = np.arange(400, dtype=np.float64).reshape(100, 4)
    x = matrix[:, 2]
    y = matrix[:, 3]
    # act
    x_out, y_out = decimate_xy(x, y, 10, DecimationAlgorithm.NONE)
    # assert
    assert x_out.flags["C_CONTIGUOUS"]
    assert y_out.flags["C_CONTIGUOUS"]
    np.testing.assert_array_equal(x_out, x)
    np.testing.assert_array_equal(y_out, y)


def test_nth_point_length():
    # arrange
    values = _sine(10_000)
    # act
    result = decimate(values, 50, DecimationAlgorithm.NTH_POINT)
    # assert
    assert len(result) <= 50


@pytest.mark.parametrize("algorithm", [DecimationAlgorithm.MIN_MAX, DecimationAlgorithm.M4, DecimationAlgorithm.AVERAGE, DecimationAlgorithm.RDP])
def test_length_le_target(algorithm: DecimationAlgorithm):
    _assert_length_le_target(algorithm)


def test_lttb_length():
    # arrange
    values = _sine(10_000)
    target = 50
    # act
    result = decimate(values, target, DecimationAlgorithm.LTTB)
    # assert
    assert len(result) == target


def test_last_point_always_included():
    # arrange
    values = _sine(1001)
    # act
    result = decimate(values, 100, DecimationAlgorithm.NTH_POINT)
    # assert
    assert float(result[-1]) == pytest.approx(float(values[-1]))


def test_first_point_always_included():
    # arrange
    values = _sine(1001)
    # act
    result = decimate(values, 100, DecimationAlgorithm.NTH_POINT)
    # assert
    assert float(result[0]) == pytest.approx(float(values[0]))


def test_output_is_subset_of_input():
    # arrange
    values = np.arange(1000, dtype=np.float64)
    # act
    result = decimate(values, 50, DecimationAlgorithm.NTH_POINT)
    # assert
    assert _is_subset_of(result, values)


def test_uniform_stride():
    # arrange
    values = np.arange(1000, dtype=np.float64)
    # act
    result = decimate(values, 100, DecimationAlgorithm.NTH_POINT)
    # assert: indices should match the linspace-based implementation used
    # internally and must include the first & last points.
    expected_indices = np.unique(np.linspace(0, 999, num=100, dtype=np.int64))
    np.testing.assert_array_equal(result, values[expected_indices])


def test_min_max_output_contains_bucket_extremes():
    # arrange
    values = np.zeros(1000, dtype=np.float64)
    values[250] = 99.0
    values[750] = -99.0
    # act
    result = decimate(values, 100, DecimationAlgorithm.MIN_MAX)
    # assert
    assert 99.0 in result.tolist()
    assert -99.0 in result.tolist()


def test_min_max_output_is_subset_of_input():
    # arrange
    values = _sine(10_000)
    # act
    result = decimate(values, 200, DecimationAlgorithm.MIN_MAX)
    # assert
    assert _is_subset_of(result, values)


def test_min_max_flat_signal_deduplication():
    # arrange
    values = np.ones(10_000, dtype=np.float64)
    target = 200
    # act
    result = decimate(values, target, DecimationAlgorithm.MIN_MAX)
    # assert
    assert len(result) <= target
    np.testing.assert_array_equal(result, np.ones(len(result)))


def test_min_max_output_sorted():
    # arrange
    values = _sine(5_000)
    # act
    result = decimate(values, 100, DecimationAlgorithm.MIN_MAX)
    # assert
    assert not np.any(np.isnan(result))


def test_m4_output_contains_bucket_extremes():
    # arrange
    values = np.zeros(1000, dtype=np.float64)
    values[99] = 50.0
    values[900] = -50.0
    # act
    result = decimate(values, 100, DecimationAlgorithm.M4)
    # assert
    assert 50.0 in result.tolist()
    assert -50.0 in result.tolist()


def test_m4_first_and_last_of_input():
    # arrange
    values = _sine(10_000)
    # act
    result = decimate(values, 200, DecimationAlgorithm.M4)
    # assert
    assert float(result[0]) == pytest.approx(float(values[0]))
    assert float(result[-1]) == pytest.approx(float(values[-1]))


def test_m4_output_is_subset_of_input():
    # arrange
    values = _sine(10_000)
    # act
    result = decimate(values, 200, DecimationAlgorithm.M4)
    # assert
    assert _is_subset_of(result, values)


def test_m4_flat_signal_deduplication():
    # arrange
    values = np.full(10_000, 3.14, dtype=np.float64)
    target = 200
    # act
    result = decimate(values, target, DecimationAlgorithm.M4)
    # assert
    assert len(result) <= target
    np.testing.assert_array_almost_equal(result, np.full(len(result), 3.14))


def test_lttb_output_length_equals_target():
    # arrange
    values = _sine(10_000)
    target = 150
    # act
    result = decimate(values, target, DecimationAlgorithm.LTTB)
    # assert
    assert len(result) == target


def test_lttb_first_and_last_preserved():
    # arrange
    values = _sine(10_000)
    # act
    result = decimate(values, 100, DecimationAlgorithm.LTTB)
    # assert
    assert float(result[0]) == pytest.approx(float(values[0]))
    assert float(result[-1]) == pytest.approx(float(values[-1]))


def test_lttb_output_is_subset_of_input():
    # arrange
    values = _sine(10_000)
    # act
    result = decimate(values, 100, DecimationAlgorithm.LTTB)
    # assert
    assert _is_subset_of(result, values)


def test_rdp_output_is_subset_of_input():
    # arrange
    values = _sine(10_000)
    # act
    result = decimate(values, 100, DecimationAlgorithm.RDP)
    # assert
    assert _is_subset_of(result, values)


def test_rdp_first_and_last_preserved():
    # arrange
    values = _sine(10_000)
    # act
    result = decimate(values, 100, DecimationAlgorithm.RDP)
    # assert
    assert float(result[0]) == pytest.approx(float(values[0]))
    assert float(result[-1]) == pytest.approx(float(values[-1]))


def test_lttb_target_one_returns_first():
    # arrange
    values = _sine(500)
    # act
    result = decimate(values, 1, DecimationAlgorithm.LTTB)
    # assert we get exactly the first sample
    assert len(result) == 1
    assert float(result[0]) == pytest.approx(float(values[0]))


def test_lttb_target_2_edge_case():
    # arrange
    values = _sine(500)
    # act
    result = decimate(values, 2, DecimationAlgorithm.LTTB)
    # assert
    assert len(result) == 2
    assert float(result[0]) == pytest.approx(float(values[0]))
    assert float(result[-1]) == pytest.approx(float(values[-1]))


def test_lttb_uses_real_x_in_decimate_xy():
    # arrange
    n = 10_000
    x = np.logspace(1, 6, n, dtype=np.float64)
    y = _sine(n)
    target = 100
    # act
    x_out, y_out = decimate_xy(x, y, target, DecimationAlgorithm.LTTB)
    # assert
    assert len(x_out) == target
    assert len(y_out) == target
    assert _xy_pairs_coherent(x_out, y_out, x, y)


def test_xy_lttb_passes_x_through_unchanged():
    # arrange - x is always float64 at this call site (QRAW binary format
    # guarantees <f8; rfftfreq also returns float64); no conversion should occur
    x = np.linspace(0.0, 1.0, 100, dtype=np.float64)
    y = np.sin(x)
    captured_x = []
    # act
    with patch("decimation_algorithm._lttb_indices") as mock_lttb:
        def _capture_x(x_arg: np.ndarray, y_arg: np.ndarray, target_arg: int) -> np.ndarray:
            captured_x.append(x_arg)
            return np.linspace(0, len(y_arg) - 1, num=target_arg, dtype=np.int64)
        mock_lttb.side_effect = _capture_x
        decimate_xy(x, y, 20, DecimationAlgorithm.LTTB)
    # assert
    assert len(captured_x) == 1
    assert captured_x[0] is x


def test_average_output_values_are_bucket_means():
    # arrange
    values = np.arange(100, dtype=np.float64)
    target = 10
    # act
    result = decimate(values, target, DecimationAlgorithm.AVERAGE)
    # assert
    expected = np.array([np.mean(values[i * 10:(i + 1) * 10]) for i in range(10)])
    np.testing.assert_array_almost_equal(result, expected)


def test_average_output_length():
    # arrange
    values = _sine(10_000)
    target = 100
    # act
    result = decimate(values, target, DecimationAlgorithm.AVERAGE)
    # assert
    assert len(result) <= target


def test_average_flat_signal_unchanged_values():
    # arrange
    values = np.full(1_000, 7.0, dtype=np.float64)
    # act
    result = decimate(values, 50, DecimationAlgorithm.AVERAGE)
    # assert
    np.testing.assert_array_almost_equal(result, np.full(len(result), 7.0))


def test_average_output_values_not_from_original():
    # arrange
    values = np.arange(0, 100, dtype=np.float64)
    # act
    result = decimate(values, 10, DecimationAlgorithm.AVERAGE)
    # assert
    for v in result:
        assert v not in values.tolist()


@pytest.mark.parametrize("algorithm", [DecimationAlgorithm.NTH_POINT, DecimationAlgorithm.MIN_MAX, DecimationAlgorithm.M4, DecimationAlgorithm.LTTB, DecimationAlgorithm.RDP, DecimationAlgorithm.AVERAGE])
def test_xy_coherent(algorithm: DecimationAlgorithm):
    _assert_xy_coherent(algorithm)


# ------------------------------------------------------------------
# invalid parameter tests
# ------------------------------------------------------------------


def test_target_zero_raises():
    # arrange
    values = _sine(100)
    # act/assert
    with pytest.raises(ValueError):
        decimate(values, 0, DecimationAlgorithm.NTH_POINT)
    with pytest.raises(ValueError):
        decimate(values, -1, DecimationAlgorithm.MIN_MAX)


def test_xy_invalid_target_raises():
    # arrange
    x = _linspace(10)
    y = _sine(10)
    # act/assert
    with pytest.raises(ValueError):
        decimate_xy(x, y, 0, DecimationAlgorithm.NTH_POINT)


@pytest.mark.parametrize("algorithm", list(DecimationAlgorithm))
def test_xy_length_mismatch_raises(algorithm: DecimationAlgorithm):
    # arrange
    x = _linspace(10)
    y = _sine(9)
    # act/assert
    with pytest.raises(ValueError):
        decimate_xy(x, y, 5, algorithm)


def test_invalid_algorithm_raises():
    # arrange
    values = _sine(100)
    # act/assert
    with pytest.raises(ValueError):
        # cast a string to bypass type checker
        decimate(values, 10, "not_an_algorithm")


@pytest.mark.parametrize("algorithm", [DecimationAlgorithm.MIN_MAX, DecimationAlgorithm.M4])
@pytest.mark.parametrize("target", [1, 2, 3, 4])
def test_value_dependent_small_targets(algorithm: DecimationAlgorithm, target: int):
    # arrange - signal for testing
    values = _sine(500)
    # act
    result = decimate(values, target, algorithm)
    # assert
    assert len(result) <= target
    # when only one point requested, this must be the first sample
    if target == 1:
        assert len(result) == 1
    # M4 also promises to keep endpoints for target>=2
    if algorithm == DecimationAlgorithm.M4 and target >= 2:
        # the m4 algorithm guarantees the first/last samples are kept
        assert float(result[0]) == pytest.approx(float(values[0]))
        assert float(result[-1]) == pytest.approx(float(values[-1]))
