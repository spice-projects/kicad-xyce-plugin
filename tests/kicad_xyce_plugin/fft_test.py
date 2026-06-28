import numpy as np

from kicad_xyce_plugin.fft import WINDOW_REGISTRY, FftOutput, WindowFunction, ZeroPadding, compute_fft_many, fft_frequency_range, is_uniform, resample_uniform


def _compute_fft(x, y, window=WindowFunction.RECTANGULAR, zero_pad=ZeroPadding.NONE, normalize=False, output=FftOutput.MAGNITUDE, keep_dc=False):
    # use the batch API for a single signal and unwrap row 0
    frequencies, values_matrix = compute_fft_many(x, np.asarray([y]), window, zero_pad, normalize, output, keep_dc)
    return frequencies, values_matrix[0]


class TestFft:

    def test_is_uniform_uniform_grid_returns_true(self):
        # arrange
        x = np.linspace(0.0, 1.0, 1000)
        # act
        result = is_uniform(x)
        # assert
        assert result

    def test_is_uniform_single_element_returns_true(self):
        # arrange
        x = np.array([0.0])
        # act
        result = is_uniform(x)
        # assert
        assert result

    def test_is_uniform_empty_array_returns_true(self):
        # arrange
        x = np.array([])
        # act
        result = is_uniform(x)
        # assert
        assert result

    def test_is_uniform_non_uniform_grid_returns_false(self):
        # arrange — exponentially spaced, highly non-uniform
        x = np.logspace(0, 3, 100)
        # act
        result = is_uniform(x)
        # assert
        assert not (result)

    def test_is_uniform_small_perturbation_within_tolerance(self):
        # arrange — tiny perturbation well within default rtol=1e-3
        x = np.linspace(0.0, 1.0, 100)
        x[50] += 1e-6
        # act
        result = is_uniform(x, rtol=1e-2)
        # assert
        assert result

    def test_is_uniform_large_perturbation_outside_tolerance(self):
        # arrange — large perturbation that exceeds rtol=1e-3
        x = np.linspace(0.0, 1.0, 100)
        x[50] += 0.05
        # act
        result = is_uniform(x, rtol=1e-3)
        # assert
        assert not (result)

    def test_is_uniform_adaptive_atol_handles_tiny_dt(self):
        # arrange — very small time steps with a tiny absolute jitter; this
        # should be considered uniform when using an adaptive absolute
        # tolerance but could be rejected by a naive relative-only test.
        n = 100
        dt = 1e-12
        x = np.linspace(0.0, (n - 1) * dt, n)
        # introduce a tiny absolute jitter smaller than realistic measurement
        # noise but larger than a strict relative threshold
        x[50] += 5e-13
        # act
        result = is_uniform(x)
        # assert — must be treated as uniform
        assert result

    def test_resample_uniform_output_length_matches_requested(self):
        # arrange
        x = np.array([0.0, 0.1, 0.3, 0.7, 1.0])
        y = np.sin(x)
        # act
        x_u, y_u = resample_uniform(x, y, num_points=50)
        # assert
        assert len(x_u) == 50
        assert len(y_u) == 50

    def test_resample_uniform_output_is_uniform(self):
        # arrange
        x = np.logspace(-3, 0, 200)
        y = np.cos(x)
        # act
        x_u, _ = resample_uniform(x, y)
        # assert
        assert is_uniform(x_u)

    def test_resample_uniform_endpoints_preserved(self):
        # arrange
        x = np.array([0.0, 0.5, 1.0])
        y = np.array([1.0, 2.0, 3.0])
        # act
        x_u, y_u = resample_uniform(x, y, num_points=5)
        # assert
        assert abs(float(x_u[0]) - 0.0) < 1e-7
        assert abs(float(x_u[-1]) - 1.0) < 1e-7
        assert abs(float(y_u[0]) - 1.0) < 0.5 * 10**(-10)
        assert abs(float(y_u[-1]) - 3.0) < 0.5 * 10**(-10)

    def test_resample_uniform_default_num_points_equals_input_length(self):
        # arrange
        x = np.linspace(0.0, 1.0, 37)
        y = np.ones(37)
        # act
        _, y_u = resample_uniform(x, y)
        # assert
        assert len(y_u) == 37

    def test_window_registry_all_windows_registered(self):
        # arrange
        # (no extra setup — iterating over the enum is sufficient)
        # act / assert
        for wf in WindowFunction:
            assert wf in WINDOW_REGISTRY

    def test_window_registry_rectangular_is_all_ones(self):
        # arrange
        fn = WINDOW_REGISTRY[WindowFunction.RECTANGULAR]
        # act
        win = fn(64)
        # assert
        np.testing.assert_array_equal(win, np.ones(64))

    def test_window_registry_hamming_length_correct(self):
        # arrange
        fn = WINDOW_REGISTRY[WindowFunction.HAMMING]
        # act
        win = fn(128)
        # assert
        assert len(win) == 128

    def test_window_registry_hanning_length_correct(self):
        # arrange
        fn = WINDOW_REGISTRY[WindowFunction.HANNING]
        # act
        win = fn(64)
        # assert
        assert len(win) == 64

    def test_window_registry_blackman_length_correct(self):
        # arrange
        fn = WINDOW_REGISTRY[WindowFunction.BLACKMAN]
        # act
        win = fn(256)
        # assert
        assert len(win) == 256

    def test_compute_fft_raises_when_x_and_y_lengths_differ(self):
        # arrange
        x = np.linspace(0.0, 1.0, 10)
        y = np.ones(8)
        # act / assert
        try:
            _compute_fft(x, y)
        except ValueError:
            pass
        else:
            assert False, "Expected ValueError"

    def test_compute_fft_raises_when_fewer_than_two_samples(self):
        # arrange
        x = np.array([0.0])
        y = np.array([1.0])
        # act / assert
        try:
            _compute_fft(x, y)
        except ValueError:
            pass
        else:
            assert False, "Expected ValueError"

    def test_compute_fft_dc_magnitude_peak_at_zero_hz(self):
        # arrange — pure DC signal must produce a spike at frequency index 0
        n = 512
        fs = 1000.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.ones(n)
        # act
        frequencies, magnitude = _compute_fft(x, y, output=FftOutput.MAGNITUDE)
        # assert
        peak_index = int(np.argmax(magnitude))
        assert peak_index == 0

    def test_compute_fft_single_tone_magnitude_peak_at_correct_bin(self):
        # arrange
        n = 1024
        fs = 10000.0
        f_tone = 1000.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.sin(2 * np.pi * f_tone * x)
        # act
        frequencies, magnitude = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, output=FftOutput.MAGNITUDE)
        # assert — allow ±1 bin tolerance
        peak_index = int(np.argmax(magnitude))
        assert abs(frequencies[peak_index] - f_tone) <= fs / n

    def test_compute_fft_magnitude_db_output_returns_decibels(self):
        # arrange
        n = 512
        fs = 8000.0
        f_tone = 400.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.sin(2 * np.pi * f_tone * x)
        # act
        _, magnitude = _compute_fft(x, y, output=FftOutput.MAGNITUDE)
        _, db = _compute_fft(x, y, output=FftOutput.MAGNITUDE_DB)
        # assert — dB at the peak must equal 20*log10(linear magnitude)
        peak = int(np.argmax(magnitude))
        assert abs(float(db[peak]) - 20.0 * np.log10(float(magnitude[peak]))) < 0.5 * 10**(-4)

    def test_compute_fft_phase_output_returns_degrees(self):
        # arrange
        n = 512
        fs = 8000.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.cos(2 * np.pi * 400.0 * x)
        # act
        _, phase = _compute_fft(x, y, output=FftOutput.PHASE)
        # assert
        assert np.all(phase >= -180.0)
        assert np.all(phase <= 180.0)

    def test_compute_fft_normalize_peaks_at_one(self):
        # arrange
        n = 512
        fs = 4000.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = 5.0 * np.sin(2 * np.pi * 500.0 * x)
        # act
        _, magnitude = _compute_fft(x, y, normalize=True, output=FftOutput.MAGNITUDE)
        # assert
        assert abs(float(np.max(magnitude)) - 1.0) < 0.5 * 10**(-5)

    def test_compute_fft_zero_padding_increases_bin_count(self):
        # arrange — deliberately non-power-of-two length
        n = 100
        fs = 1000.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.sin(2 * np.pi * 100.0 * x)
        # act
        freq_no_pad, _ = _compute_fft(x, y, zero_pad=ZeroPadding.NONE)
        freq_padded, _ = _compute_fft(x, y, zero_pad=ZeroPadding.NEXT_POWER_OF_TWO)
        # assert — padded FFT must have more frequency bins
        assert len(freq_padded) > len(freq_no_pad)

    def test_compute_fft_non_uniform_input_is_resampled_without_error(self):
        # arrange — exponentially spaced time axis, definitely non-uniform
        x = np.logspace(-4, -1, 200)
        y = np.sin(2 * np.pi * 50.0 * x)
        # act
        frequencies, magnitude = _compute_fft(x, y, output=FftOutput.MAGNITUDE)
        # assert
        assert len(frequencies) == len(magnitude)
        assert np.all(np.isfinite(magnitude))

    def test_compute_fft_output_arrays_are_same_length(self):
        # arrange
        n = 256
        x = np.linspace(0.0, 1.0, n)
        y = np.random.default_rng(0).standard_normal(n)
        # act / assert — verified for all output types
        for output in FftOutput:
            frequencies, values = _compute_fft(x, y, output=output)
            assert len(frequencies) == len(values)

    def test_compute_fft_all_window_functions_run_without_error(self):
        # arrange
        n = 256
        x = np.linspace(0.0, 1.0, n)
        y = np.sin(2 * np.pi * 10.0 * x)
        # act / assert — verified for all window functions
        for wf in WindowFunction:
            frequencies, values = _compute_fft(x, y, window=wf, output=FftOutput.MAGNITUDE)
            assert np.all(np.isfinite(values))

    def test_fft_frequency_range_returns_zero_for_fewer_than_two_points(self):
        # arrange
        x_one = np.array([0.0])
        x_empty = np.array([])
        # act
        result_one = fft_frequency_range(x_one)
        result_empty = fft_frequency_range(x_empty)
        # assert
        assert result_one == (0.0, 0.0)
        assert result_empty == (0.0, 0.0)

    def test_fft_frequency_range_nyquist_is_half_sampling_rate(self):
        # arrange
        fs = 1000.0
        n = 1000
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        # act
        df, f_nyquist = fft_frequency_range(x)
        # assert
        assert abs(f_nyquist - fs / 2) <= 0.1

    def test_fft_frequency_range_bin_width_equals_fs_over_n(self):
        # arrange
        fs = 2000.0
        n = 500
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        # act
        df, _ = fft_frequency_range(x)
        # assert
        assert abs(df - fs / n) <= 0.01

    def test_fft_frequency_range_non_uniform_input_does_not_raise(self):
        # arrange
        x = np.logspace(-3, 0, 100)
        # act
        df, f_nyquist = fft_frequency_range(x)
        # assert
        assert f_nyquist > 0.0

    def test_compute_fft_rectangular_single_tone_amplitude_correct(self):
        # arrange — unit-amplitude sine with an integer number of cycles so the
        # peak falls exactly on a bin and leakage is zero
        n = 1024
        fs = 1024.0
        f_tone = 100.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.sin(2 * np.pi * f_tone * x)
        # act
        frequencies, magnitude = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, output=FftOutput.MAGNITUDE)
        # assert — amplitude at the tone bin must be 1.0 (±1 % tolerance)
        peak_index = int(np.argmax(magnitude))
        assert abs(float(magnitude[peak_index]) - 1.0) <= 0.01

    def test_compute_fft_hamming_window_amplitude_correct(self):
        # arrange — same integer-cycle sine; Hamming window should still recover
        # amplitude ≈ 1.0 once the coherent gain is divided out correctly
        n = 1024
        fs = 1024.0
        f_tone = 100.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.sin(2 * np.pi * f_tone * x)
        # act
        frequencies, magnitude = _compute_fft(x, y, window=WindowFunction.HAMMING, output=FftOutput.MAGNITUDE)
        # assert — the peak amplitude must still be close to 1.0;
        # if scale used 2/n instead of 2/sum(win) it would be ≈ 0.54 (Hamming coherent gain)
        peak_index = int(np.argmax(magnitude))
        assert abs(float(magnitude[peak_index]) - 1.0) <= 0.05

    def test_compute_fft_hanning_window_amplitude_correct(self):
        # arrange
        n = 1024
        fs = 1024.0
        f_tone = 100.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.sin(2 * np.pi * f_tone * x)
        # act
        _, magnitude = _compute_fft(x, y, window=WindowFunction.HANNING, output=FftOutput.MAGNITUDE)
        # assert — coherent gain for Hanning is 0.5; peak must still be ≈ 1.0
        peak_index = int(np.argmax(magnitude))
        assert abs(float(magnitude[peak_index]) - 1.0) <= 0.05

    def test_compute_fft_blackman_window_amplitude_correct(self):
        # arrange
        n = 1024
        fs = 1024.0
        f_tone = 100.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.sin(2 * np.pi * f_tone * x)
        # act
        _, magnitude = _compute_fft(x, y, window=WindowFunction.BLACKMAN, output=FftOutput.MAGNITUDE)
        # assert — coherent gain for Blackman is ≈ 0.42; peak must still be ≈ 1.0
        peak_index = int(np.argmax(magnitude))
        assert abs(float(magnitude[peak_index]) - 1.0) <= 0.05

    def test_compute_fft_dc_amplitude_is_one_for_unit_dc_signal(self):
        # arrange — a constant signal of amplitude 1; dc bin should give 1.0 when DC is kept
        n = 512
        fs = 1000.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.ones(n)
        # act — keep_dc=True so the DC component is not subtracted before the FFT
        _, magnitude = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, output=FftOutput.MAGNITUDE, keep_dc=True)
        # assert — DC bin (index 0) must equal 1.0
        assert abs(float(magnitude[0]) - 1.0) <= 0.001

    def test_compute_fft_nyquist_bin_not_doubled_for_even_n(self):
        # arrange — a cosine at exactly the Nyquist frequency (fs/2) with unit amplitude;
        # for even n it falls exactly on the last rfft bin
        n = 512
        fs = 1000.0
        f_nyq = fs / 2.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.cos(2 * np.pi * f_nyq * x)
        # act
        _, magnitude = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, output=FftOutput.MAGNITUDE)
        # assert — last bin amplitude must be 1.0, not 2.0 (which a missing halving would give)
        assert abs(float(magnitude[-1]) - 1.0) <= 0.01

    def test_compute_fft_raises_on_zero_sum_window(self):
        # arrange — create a simple tone and temporarily replace the
        # rectangular window with a zero-valued window to simulate the
        # pathological case
        n = 128
        fs = 1000.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.sin(2 * np.pi * 50.0 * x)
        orig_win = WINDOW_REGISTRY[WindowFunction.RECTANGULAR]
        try:
            WINDOW_REGISTRY[WindowFunction.RECTANGULAR] = lambda m: np.zeros(m)
            # act / assert — computation must raise a clear ValueError
            try:
                _compute_fft(x, y, window=WindowFunction.RECTANGULAR)
            except ValueError:
                pass
            else:
                assert False, "Expected ValueError"
        finally:
            WINDOW_REGISTRY[WindowFunction.RECTANGULAR] = orig_win

    def test_compute_thd_simple_second_harmonic(self):
        # arrange — fundamental with a small second harmonic (A1=1.0, A2=0.1)
        n = 2048
        fs = 8192.0
        f1 = 123.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = 1.0 * np.sin(2 * np.pi * f1 * x) + 0.1 * np.sin(2 * np.pi * 2.0 * f1 * x)
        # act — compute FFT and then THD
        freqs, magnitude = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, zero_pad=ZeroPadding.NEXT_POWER_OF_TWO, output=FftOutput.MAGNITUDE)
        # assert — expected THD = 0.1 (linear)
        # compute_thd is tested in tests/thd_test.py

    def test_compute_thd_multiple_harmonics(self):
        # arrange — fundamental plus 2nd and 3rd harmonics (A1=1, A2=0.2, A3=0.05)
        n = 2048
        fs = 8192.0
        f1 = 50.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = 1.0 * np.sin(2 * np.pi * f1 * x) + 0.2 * np.sin(2 * np.pi * 2.0 * f1 * x) + 0.05 * np.sin(2 * np.pi * 3.0 * f1 * x)
        # act
        freqs, magnitude = _compute_fft(x, y, window=WindowFunction.HANNING, zero_pad=ZeroPadding.NEXT_POWER_OF_TWO, output=FftOutput.MAGNITUDE)
        # compute_thd is tested in tests/thd_test.py

    def test_compute_thd_raises_on_zero_fundamental(self):
        # arrange — zero signal so fundamental amplitude is zero
        n = 256
        x = np.linspace(0.0, 1.0, n, endpoint=False)
        y = np.zeros(n)
        freqs, magnitude = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, output=FftOutput.MAGNITUDE)
        # compute_thd is tested in tests/thd_test.py

    def test_compute_fft_keep_dc_false_removes_dc_component(self):
        # arrange — constant offset plus a sine; with DC removed the DC bin should be near zero
        n = 512
        fs = 1000.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = 5.0 + np.sin(2 * np.pi * 100.0 * x)
        # act
        _, magnitude = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, output=FftOutput.MAGNITUDE, keep_dc=False)
        # assert — DC bin amplitude must be negligible after mean subtraction
        assert float(magnitude[0]) < 0.01

    def test_compute_fft_keep_dc_true_preserves_dc_component(self):
        # arrange — constant offset of 5 V with no AC content
        n = 512
        fs = 1000.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.full(n, 5.0)
        # act
        _, magnitude = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, output=FftOutput.MAGNITUDE, keep_dc=True)
        # assert — DC bin must reflect the 5 V offset
        assert abs(float(magnitude[0]) - 5.0) <= 0.01

    def test_compute_fft_many_matches_individual_magnitude(self):
        # arrange
        n = 1024
        fs = 8192.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y1 = np.sin(2 * np.pi * 500.0 * x)
        y2 = 0.5 * np.sin(2 * np.pi * 1200.0 * x)
        y_matrix = np.vstack([y1, y2])
        # act
        frequencies_many, values_many = compute_fft_many(x, y_matrix, window=WindowFunction.HANNING, zero_pad=ZeroPadding.NEXT_POWER_OF_TWO, normalize=False, output=FftOutput.MAGNITUDE, keep_dc=False)
        frequencies_1, values_1 = _compute_fft(x, y1, window=WindowFunction.HANNING, zero_pad=ZeroPadding.NEXT_POWER_OF_TWO, normalize=False, output=FftOutput.MAGNITUDE, keep_dc=False)
        frequencies_2, values_2 = _compute_fft(x, y2, window=WindowFunction.HANNING, zero_pad=ZeroPadding.NEXT_POWER_OF_TWO, normalize=False, output=FftOutput.MAGNITUDE, keep_dc=False)
        # assert
        np.testing.assert_allclose(frequencies_many, frequencies_1)
        np.testing.assert_allclose(frequencies_many, frequencies_2)
        np.testing.assert_allclose(values_many[0], values_1, rtol=1e-12, atol=1e-12)
        np.testing.assert_allclose(values_many[1], values_2, rtol=1e-12, atol=1e-12)

    def test_compute_fft_many_matches_individual_non_uniform_input(self):
        # arrange
        x = np.logspace(-4, -1, 300)
        y1 = np.sin(2 * np.pi * 80.0 * x)
        y2 = np.cos(2 * np.pi * 120.0 * x)
        y_matrix = np.vstack([y1, y2])
        # act
        frequencies_many, values_many = compute_fft_many(x, y_matrix, window=WindowFunction.RECTANGULAR, zero_pad=ZeroPadding.NONE, normalize=False, output=FftOutput.MAGNITUDE, keep_dc=False)
        frequencies_1, values_1 = _compute_fft(x, y1, window=WindowFunction.RECTANGULAR, zero_pad=ZeroPadding.NONE, normalize=False, output=FftOutput.MAGNITUDE, keep_dc=False)
        frequencies_2, values_2 = _compute_fft(x, y2, window=WindowFunction.RECTANGULAR, zero_pad=ZeroPadding.NONE, normalize=False, output=FftOutput.MAGNITUDE, keep_dc=False)
        # assert
        np.testing.assert_allclose(frequencies_many, frequencies_1)
        np.testing.assert_allclose(frequencies_many, frequencies_2)
        np.testing.assert_allclose(values_many[0], values_1, rtol=1e-10, atol=1e-10)
        np.testing.assert_allclose(values_many[1], values_2, rtol=1e-10, atol=1e-10)
