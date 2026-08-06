import numpy as np
import pytest

from kicad_xyce_plugin.fft import compute_fft_many2, FftOutput, WindowFunction


def _max_frequency_from_target_samples(x, x_left_index, x_right_index, target_sample_count):
    # derive max_frequency so compute_fft_many2 creates approximately target_sample_count interpolation samples
    left_index = int(x_left_index)
    right_index = int(x_right_index)
    sample_count = int(target_sample_count)
    if sample_count < 2:
        raise ValueError("target_sample_count must be at least 2")
    interval_duration = float(x[right_index - 1]) - float(x[left_index])
    if interval_duration <= 0.0:
        raise ValueError("selected interval duration must be positive")
    return (sample_count - 1) / (10.0 * interval_duration)


def _compute_fft(x, y, window=WindowFunction.RECTANGULAR, normalize=False, output=FftOutput.MAGNITUDE, keep_dc=False, max_frequency=None, np_points=None, x_left_index=0, x_right_index=None):
    # default right interval index to the end of x
    if x_right_index is None:
        x_right_index = len(x)
    # infer default maximum frequency from the selected interval when omitted
    if max_frequency is None:
        selected_sample_count = int(x_right_index) - int(x_left_index)
        if np_points is None:
            np_points = selected_sample_count
        max_frequency = _max_frequency_from_target_samples(x, x_left_index, x_right_index, np_points)
    # use the batch API for a single signal
    freqs, mat = compute_fft_many2(x, np.asarray([y]), max_frequency=max_frequency, window=window, normalize=normalize, x_left_index=x_left_index, x_right_index=x_right_index, output=output, keep_dc=keep_dc)
    # unwrap row 0
    return freqs, mat[0]


class TestKnownSignals:

    def test_integer_cycle_sine_has_single_bin_peak(self):
        # arrange — integer number of cycles so tone falls exactly on FFT bin
        n = 2048
        fs = 2048.0
        f_tone = 123.0
        # compute integer cycle count that aligns the tone to an FFT bin
        cycles = int(f_tone * (n / fs))
        # ensure at least one full cycle
        if cycles == 0:
            # reset to the minimum
            cycles = 1
        # recompute sample count so the tone sits exactly on a bin
        n = int(fs / f_tone * cycles)
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = np.sin(2 * np.pi * f_tone * x)

        # act
        freqs, magnitude = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, output=FftOutput.MAGNITUDE)

        # assert — primary peak magnitude ≈ 1 and all other bins are much smaller
        # locate the peak bin
        peak_idx = int(np.argmax(magnitude))
        assert freqs[peak_idx] == pytest.approx(f_tone, abs=fs / n)
        assert float(magnitude[peak_idx]) == pytest.approx(1.0, abs=0.01)
        # zero out the peak and verify all remaining bins are negligible
        mag_copy = magnitude.copy()
        mag_copy[peak_idx] = 0.0
        assert float(np.max(mag_copy)) < 1e-3

    def test_two_tone_amplitudes_preserved(self):
        # arrange — two tones with known amplitudes
        n = 4096
        fs = 8192.0
        f1 = 300.0
        f2 = 1200.0
        a1 = 1.0
        a2 = 0.5
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        y = a1 * np.sin(2 * np.pi * f1 * x) + a2 * np.sin(2 * np.pi * f2 * x)

        # act
        freqs, magnitude = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, output=FftOutput.MAGNITUDE)

        # assert — peaks near expected frequencies with amplitudes a1 and a2
        # find the bin closest to each tone frequency
        idx1 = int(np.argmin(np.abs(freqs - f1)))
        idx2 = int(np.argmin(np.abs(freqs - f2)))
        assert float(magnitude[idx1]) == pytest.approx(a1, abs=0.02)
        assert float(magnitude[idx2]) == pytest.approx(a2, abs=0.02)

    def test_parseval_energy_conservation(self):
        # arrange — random signal: time-domain energy equals frequency-domain energy within tolerance
        n = 2048
        fs = 1000.0
        x = np.linspace(0.0, n / fs, n, endpoint=False)
        rng = np.random.default_rng(0)
        y = rng.standard_normal(n)

        # act
        freqs, mag = _compute_fft(x, y, window=WindowFunction.RECTANGULAR, output=FftOutput.MAGNITUDE)
        # compute time-domain energy
        time_energy = np.sum(y * y)
        # compute full DFT for reference energy calculation
        spectrum = np.fft.fft(y, n=len(y))
        # compute frequency-domain energy via Parseval's theorem
        freq_energy = np.sum(np.abs(spectrum) ** 2) / len(y)

        # assert — energies are close
        assert float(time_energy) == pytest.approx(float(freq_energy), abs=1e-6 * float(time_energy + 1.0))
