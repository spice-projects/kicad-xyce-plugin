from unittest import TestCase
from unittest.mock import patch

import numpy as np

from decimation_algorithm import DecimationAlgorithm, decimate, decimate_xy


def _linspace(n: int) -> np.ndarray:
    return np.linspace(0.0, 1.0, n, dtype=np.float64)


def _sine(n: int) -> np.ndarray:
    return np.sin(np.linspace(0.0, 2 * np.pi, n, dtype=np.float64))


def _is_subset_of(output: np.ndarray, original: np.ndarray) -> bool:
    original_set = set(original.tolist())
    return all(v in original_set for v in output.tolist())


def _xy_pairs_coherent(x_out: np.ndarray, y_out: np.ndarray,
                       x_orig: np.ndarray, y_orig: np.ndarray) -> bool:
    pairs_orig = set(zip(x_orig.tolist(), y_orig.tolist()))
    return all((xv, yv) in pairs_orig for xv, yv in zip(x_out.tolist(), y_out.tolist()))


class TestDecimationAlgorithm(TestCase):

    def _assert_short_circuit(self, algorithm: DecimationAlgorithm):
        # arrange
        values = _sine(100)
        # act
        result = decimate(values, 100, algorithm)
        # assert
        self.assertIs(result, values)
        result = decimate(values, 200, algorithm)
        self.assertIs(result, values)

    def _assert_xy_short_circuit(self, algorithm: DecimationAlgorithm):
        # arrange
        x = _linspace(50)
        y = _sine(50)
        # act
        x_out, y_out = decimate_xy(x, y, 50, algorithm)
        # assert
        self.assertIs(x_out, x)
        self.assertIs(y_out, y)

    def test_nth_point_short_circuit(self):
        self._assert_short_circuit(DecimationAlgorithm.NTH_POINT)

    def test_min_max_short_circuit(self):
        self._assert_short_circuit(DecimationAlgorithm.MIN_MAX)

    def test_m4_short_circuit(self):
        self._assert_short_circuit(DecimationAlgorithm.M4)

    def test_lttb_short_circuit(self):
        self._assert_short_circuit(DecimationAlgorithm.LTTB)

    def test_average_short_circuit(self):
        self._assert_short_circuit(DecimationAlgorithm.AVERAGE)

    def test_rdp_short_circuit(self):
        self._assert_short_circuit(DecimationAlgorithm.RDP)

    def test_xy_nth_point_short_circuit(self):
        self._assert_xy_short_circuit(DecimationAlgorithm.NTH_POINT)

    def test_xy_min_max_short_circuit(self):
        self._assert_xy_short_circuit(DecimationAlgorithm.MIN_MAX)

    def test_xy_m4_short_circuit(self):
        self._assert_xy_short_circuit(DecimationAlgorithm.M4)

    def test_xy_lttb_short_circuit(self):
        self._assert_xy_short_circuit(DecimationAlgorithm.LTTB)

    def test_xy_average_short_circuit(self):
        self._assert_xy_short_circuit(DecimationAlgorithm.AVERAGE)

    def test_xy_rdp_short_circuit(self):
        self._assert_xy_short_circuit(DecimationAlgorithm.RDP)

    def test_xy_short_circuit_strided_inputs_become_contiguous(self):
        # arrange
        matrix = np.arange(400, dtype=np.float64).reshape(100, 4)
        x = matrix[:, 0]
        y = matrix[:, 1]
        for algorithm in [DecimationAlgorithm.NTH_POINT, DecimationAlgorithm.MIN_MAX, DecimationAlgorithm.M4, DecimationAlgorithm.LTTB, DecimationAlgorithm.RDP, DecimationAlgorithm.AVERAGE]:
            # test for algo
            with self.subTest(algorithm=algorithm):
                # act
                x_out, y_out = decimate_xy(x, y, 100, algorithm)
                # assert
                self.assertTrue(x_out.flags["C_CONTIGUOUS"])
                self.assertTrue(y_out.flags["C_CONTIGUOUS"])
                np.testing.assert_array_equal(x_out, x)
                np.testing.assert_array_equal(y_out, y)

    def test_xy_none_strided_inputs_become_contiguous(self):
        # arrange
        matrix = np.arange(400, dtype=np.float64).reshape(100, 4)
        x = matrix[:, 2]
        y = matrix[:, 3]
        # act
        x_out, y_out = decimate_xy(x, y, 10, DecimationAlgorithm.NONE)
        # assert
        self.assertTrue(x_out.flags["C_CONTIGUOUS"])
        self.assertTrue(y_out.flags["C_CONTIGUOUS"])
        np.testing.assert_array_equal(x_out, x)
        np.testing.assert_array_equal(y_out, y)

    def _assert_length_le_target(self, algorithm: DecimationAlgorithm, target: int = 50):
        # arrange
        values = _sine(10_000)
        # act
        result = decimate(values, target, algorithm)
        # assert
        self.assertLessEqual(len(result), target)

    def test_nth_point_length(self):
        # arrange
        values = _sine(10_000)
        # act
        result = decimate(values, 50, DecimationAlgorithm.NTH_POINT)
        # assert
        self.assertLessEqual(len(result), 50)

    def test_min_max_length(self):
        self._assert_length_le_target(DecimationAlgorithm.MIN_MAX)

    def test_m4_length(self):
        self._assert_length_le_target(DecimationAlgorithm.M4)

    def test_lttb_length(self):
        # arrange
        values = _sine(10_000)
        target = 50
        # act
        result = decimate(values, target, DecimationAlgorithm.LTTB)
        # assert
        self.assertEqual(len(result), target)

    def test_average_length(self):
        self._assert_length_le_target(DecimationAlgorithm.AVERAGE)

    def test_rdp_length(self):
        self._assert_length_le_target(DecimationAlgorithm.RDP)

    def test_last_point_always_included(self):
        # arrange
        values = _sine(1001)
        # act
        result = decimate(values, 100, DecimationAlgorithm.NTH_POINT)
        # assert
        self.assertAlmostEqual(float(result[-1]), float(values[-1]))

    def test_first_point_always_included(self):
        # arrange
        values = _sine(1001)
        # act
        result = decimate(values, 100, DecimationAlgorithm.NTH_POINT)
        # assert
        self.assertAlmostEqual(float(result[0]), float(values[0]))

    def test_output_is_subset_of_input(self):
        # arrange
        values = np.arange(1000, dtype=np.float64)
        # act
        result = decimate(values, 50, DecimationAlgorithm.NTH_POINT)
        # assert
        self.assertTrue(_is_subset_of(result, values))

    def test_uniform_stride(self):
        # arrange
        values = np.arange(1000, dtype=np.float64)
        # act
        result = decimate(values, 100, DecimationAlgorithm.NTH_POINT)
        # assert: indices should match the linspace-based implementation used
        # internally and must include the first & last points.
        expected_indices = np.unique(np.linspace(0, 999, num=100, dtype=np.int64))
        np.testing.assert_array_equal(result, values[expected_indices])

    def test_min_max_output_contains_bucket_extremes(self):
        # arrange
        values = np.zeros(1000, dtype=np.float64)
        values[250] = 99.0
        values[750] = -99.0
        # act
        result = decimate(values, 100, DecimationAlgorithm.MIN_MAX)
        # assert
        self.assertIn(99.0, result.tolist())
        self.assertIn(-99.0, result.tolist())

    def test_min_max_output_is_subset_of_input(self):
        # arrange
        values = _sine(10_000)
        # act
        result = decimate(values, 200, DecimationAlgorithm.MIN_MAX)
        # assert
        self.assertTrue(_is_subset_of(result, values))

    def test_min_max_flat_signal_deduplication(self):
        # arrange
        values = np.ones(10_000, dtype=np.float64)
        target = 200
        # act
        result = decimate(values, target, DecimationAlgorithm.MIN_MAX)
        # assert
        self.assertLessEqual(len(result), target)
        np.testing.assert_array_equal(result, np.ones(len(result)))

    def test_min_max_output_sorted(self):
        # arrange
        values = _sine(5_000)
        # act
        result = decimate(values, 100, DecimationAlgorithm.MIN_MAX)
        # assert
        self.assertFalse(np.any(np.isnan(result)))

    def test_m4_output_contains_bucket_extremes(self):
        # arrange
        values = np.zeros(1000, dtype=np.float64)
        values[99] = 50.0
        values[900] = -50.0
        # act
        result = decimate(values, 100, DecimationAlgorithm.M4)
        # assert
        self.assertIn(50.0, result.tolist())
        self.assertIn(-50.0, result.tolist())

    def test_m4_first_and_last_of_input(self):
        # arrange
        values = _sine(10_000)
        # act
        result = decimate(values, 200, DecimationAlgorithm.M4)
        # assert
        self.assertAlmostEqual(float(result[0]), float(values[0]))
        self.assertAlmostEqual(float(result[-1]), float(values[-1]))

    def test_m4_output_is_subset_of_input(self):
        # arrange
        values = _sine(10_000)
        # act
        result = decimate(values, 200, DecimationAlgorithm.M4)
        # assert
        self.assertTrue(_is_subset_of(result, values))

    def test_m4_flat_signal_deduplication(self):
        # arrange
        values = np.full(10_000, 3.14, dtype=np.float64)
        target = 200
        # act
        result = decimate(values, target, DecimationAlgorithm.M4)
        # assert
        self.assertLessEqual(len(result), target)
        np.testing.assert_array_almost_equal(result, np.full(len(result), 3.14))

    def test_lttb_output_length_equals_target(self):
        # arrange
        values = _sine(10_000)
        target = 150
        # act
        result = decimate(values, target, DecimationAlgorithm.LTTB)
        # assert
        self.assertEqual(len(result), target)

    def test_lttb_first_and_last_preserved(self):
        # arrange
        values = _sine(10_000)
        # act
        result = decimate(values, 100, DecimationAlgorithm.LTTB)
        # assert
        self.assertAlmostEqual(float(result[0]), float(values[0]))
        self.assertAlmostEqual(float(result[-1]), float(values[-1]))

    def test_lttb_output_is_subset_of_input(self):
        # arrange
        values = _sine(10_000)
        # act
        result = decimate(values, 100, DecimationAlgorithm.LTTB)
        # assert
        self.assertTrue(_is_subset_of(result, values))

    def test_rdp_output_is_subset_of_input(self):
        # arrange
        values = _sine(10_000)
        # act
        result = decimate(values, 100, DecimationAlgorithm.RDP)
        # assert
        self.assertTrue(_is_subset_of(result, values))

    def test_rdp_first_and_last_preserved(self):
        # arrange
        values = _sine(10_000)
        # act
        result = decimate(values, 100, DecimationAlgorithm.RDP)
        # assert
        self.assertAlmostEqual(float(result[0]), float(values[0]))
        self.assertAlmostEqual(float(result[-1]), float(values[-1]))

    def test_lttb_target_one_returns_first(self):
        # arrange
        values = _sine(500)
        # act
        result = decimate(values, 1, DecimationAlgorithm.LTTB)
        # assert we get exactly the first sample
        self.assertEqual(len(result), 1)
        self.assertAlmostEqual(float(result[0]), float(values[0]))

    def test_lttb_target_2_edge_case(self):
        # arrange
        values = _sine(500)
        # act
        result = decimate(values, 2, DecimationAlgorithm.LTTB)
        # assert
        self.assertEqual(len(result), 2)
        self.assertAlmostEqual(float(result[0]), float(values[0]))
        self.assertAlmostEqual(float(result[-1]), float(values[-1]))

    def test_lttb_uses_real_x_in_decimate_xy(self):
        # arrange
        n = 10_000
        x = np.logspace(1, 6, n, dtype=np.float64)
        y = _sine(n)
        target = 100
        # act
        x_out, y_out = decimate_xy(x, y, target, DecimationAlgorithm.LTTB)
        # assert
        self.assertEqual(len(x_out), target)
        self.assertEqual(len(y_out), target)
        self.assertTrue(_xy_pairs_coherent(x_out, y_out, x, y))

    def test_xy_lttb_passes_x_through_unchanged(self):
        # arrange — x is always float64 at this call site (QRAW binary format
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
        self.assertEqual(len(captured_x), 1)
        self.assertIs(captured_x[0], x)

    def test_average_output_values_are_bucket_means(self):
        # arrange
        values = np.arange(100, dtype=np.float64)
        target = 10
        # act
        result = decimate(values, target, DecimationAlgorithm.AVERAGE)
        # assert
        expected = np.array([np.mean(values[i*10:(i+1)*10]) for i in range(10)])
        np.testing.assert_array_almost_equal(result, expected)

    def test_average_output_length(self):
        # arrange
        values = _sine(10_000)
        target = 100
        # act
        result = decimate(values, target, DecimationAlgorithm.AVERAGE)
        # assert
        self.assertLessEqual(len(result), target)

    def test_average_flat_signal_unchanged_values(self):
        # arrange
        values = np.full(1_000, 7.0, dtype=np.float64)
        # act
        result = decimate(values, 50, DecimationAlgorithm.AVERAGE)
        # assert
        np.testing.assert_array_almost_equal(result, np.full(len(result), 7.0))

    def test_average_output_values_not_from_original(self):
        # arrange
        values = np.arange(0, 100, dtype=np.float64)
        # act
        result = decimate(values, 10, DecimationAlgorithm.AVERAGE)
        # assert
        for v in result:
            self.assertNotIn(v, values.tolist())

    def _assert_xy_coherent(self, algorithm: DecimationAlgorithm):
        # arrange
        n = 10_000
        x = _linspace(n)
        y = _sine(n)
        # act
        x_out, y_out = decimate_xy(x, y, 200, algorithm)
        # assert
        self.assertEqual(len(x_out), len(y_out))
        # when algorithm is not AVERAGE the x/y pairs must exactly match
        if algorithm != DecimationAlgorithm.AVERAGE:
            self.assertTrue(_xy_pairs_coherent(x_out, y_out, x, y))

    def test_xy_nth_point_coherent(self):
        self._assert_xy_coherent(DecimationAlgorithm.NTH_POINT)

    def test_xy_min_max_coherent(self):
        self._assert_xy_coherent(DecimationAlgorithm.MIN_MAX)

    def test_xy_m4_coherent(self):
        self._assert_xy_coherent(DecimationAlgorithm.M4)

    def test_xy_lttb_coherent(self):
        self._assert_xy_coherent(DecimationAlgorithm.LTTB)

    def test_xy_rdp_coherent(self):
        self._assert_xy_coherent(DecimationAlgorithm.RDP)

    def test_xy_average_coherent(self):
        self._assert_xy_coherent(DecimationAlgorithm.AVERAGE)

    # ------------------------------------------------------------------
    # invalid parameter tests
    # ------------------------------------------------------------------

    def test_target_zero_raises(self):
        # arrange
        values = _sine(100)
        # act/assert
        with self.assertRaises(ValueError):
            decimate(values, 0, DecimationAlgorithm.NTH_POINT)
        with self.assertRaises(ValueError):
            decimate(values, -1, DecimationAlgorithm.MIN_MAX)

    def test_xy_invalid_target_raises(self):
        # arrange
        x = _linspace(10)
        y = _sine(10)
        # act/assert
        with self.assertRaises(ValueError):
            decimate_xy(x, y, 0, DecimationAlgorithm.NTH_POINT)

    def test_xy_length_mismatch_raises(self):
        # arrange
        x = _linspace(10)
        y = _sine(9)
        # any algorithm should reject mismatched input lengths
        # act/assert: iterate algorithms expecting a ValueError each time
        for alg in DecimationAlgorithm:
            with self.subTest(algorithm=alg):
                with self.assertRaises(ValueError):
                    decimate_xy(x, y, 5, alg)

    def test_invalid_algorithm_raises(self):
        # arrange
        values = _sine(100)
        # act/assert
        with self.assertRaises(ValueError):
            # cast a string to bypass type checker
            decimate(values, 10, "not_an_algorithm")

    def test_value_dependent_small_targets(self):
        # arrange - signal for testing
        values = _sine(500)
        # act/assert - exercise MIN_MAX and M4 with tiny targets
        for alg in (DecimationAlgorithm.MIN_MAX, DecimationAlgorithm.M4):
            # for each algorithm try several small target counts
            for t in (1, 2, 3, 4):
                result = decimate(values, t, alg)
                # assert not more points than requested
                self.assertLessEqual(len(result), t)
                # when only one point requested, this must be the first sample
                if t == 1:
                    self.assertEqual(len(result), 1)
                # M4 also promises to keep endpoints for t>=2
                if alg == DecimationAlgorithm.M4 and t >= 2:
                    # the m4 algorithm guarantees the first/last samples are kept
                    self.assertAlmostEqual(float(result[0]), float(values[0]))
                    self.assertAlmostEqual(float(result[-1]), float(values[-1]))
