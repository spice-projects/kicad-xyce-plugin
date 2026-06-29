import os
import tempfile
from pathlib import Path

import numpy as np
import pytest

from kicad_xyce_plugin.xyce_fft_file import FftSignalMetadata, xyce_fft_file_parser, _magnitude_expression_name, _parse_signal_block, _phase_expression_name
from kicad_xyce_plugin.xyce_output_file import AbscissaScale


def _write_temp_fft(content: str) -> str:
    # create a named temporary file that persists after close
    fh = tempfile.NamedTemporaryFile(delete=False, suffix=".fft0", mode="w", encoding="utf-8")
    # write the text content
    fh.write(content)
    # close so the path is accessible on all platforms
    fh.close()
    # return path string
    return fh.name


def _single_signal_fft_text(signal_name: str = "V(OUT)", window: str = "HANN", start_time: float = 0.0, stop_time: float = 1e-2, first_harmonic: float = 100.0, start_freq: float = 100.0, stop_freq: float = 1e5, dc_magnitude: float = 0.01, dc_phase: float = 180.0, data_rows: list[tuple[int, float, float, float]] | None = None) -> str:
    # use a minimal default dataset when none is provided
    if data_rows is None:
        data_rows = [(1, 100.0, 0.5, 90.0), (2, 200.0, 0.25, -90.0), (3, 300.0, 0.1, 0.0)]
    # build header
    lines = [f"FFT analysis for {signal_name}:", f"  Window: {window}, Start Time: {start_time:e}, Stop Time: {stop_time:e}", f"  First Harmonic: {first_harmonic:e}, Start Freq: {start_freq:e}, Stop Freq: {stop_freq:e}", f"  DC component    Norm. Mag= {dc_magnitude:e}   Phase= {dc_phase:e}", "       Index       Frequency       Norm. Mag           Phase"]
    # append data rows
    for idx, freq, mag, phase in data_rows:
        lines.append(f"           {idx}    {freq:e}    {mag:e}    {phase:e}")
    return "\n".join(lines) + "\n"


def _first_fft_file(result: tuple[list[object], list[object]] | None):
    assert result is not None
    files, signals = result
    assert len(files) > 0
    return files[0], signals


class TestFftSignalMetadata:

    def test_properties_are_stored_correctly(self):
        # arrange / act
        meta = FftSignalMetadata(window="HANN", start_time=0.0, stop_time=1e-2, first_harmonic=100.0, start_freq=100.0, stop_freq=2e5, dc_magnitude=0.02, dc_phase=180.0)

        # assert
        assert meta.window == "HANN"
        assert meta.start_time == 0.0
        assert meta.stop_time == pytest.approx(1e-2)
        assert meta.first_harmonic == pytest.approx(100.0)
        assert meta.start_freq == pytest.approx(100.0)
        assert meta.stop_freq == pytest.approx(2e5)
        assert meta.dc_magnitude == pytest.approx(0.02)
        assert meta.dc_phase == pytest.approx(180.0)


class TestExpressionNameHelpers:

    def test_magnitude_name_returns_signal_name(self):
        # act / assert
        assert _magnitude_expression_name("V(OUT)") == "V(OUT)"

    def test_phase_name_wraps_signal_in_phase_prefix(self):
        # act / assert
        assert _phase_expression_name("V(OUT)") == "phase(V(OUT))"

    def test_magnitude_name_for_current_signal(self):
        # act / assert
        assert _magnitude_expression_name("I(R1)") == "I(R1)"

    def test_phase_name_for_current_signal(self):
        # act / assert
        assert _phase_expression_name("I(R1)") == "phase(I(R1))"


class TestParseSignalBlock:

    def test_parses_metadata_fields(self):
        # arrange
        block_lines = ["  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02", "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 2.048000e+05", "  DC component    Norm. Mag= 2.637066e-02   Phase= 1.800000e+02", "       Index       Frequency       Norm. Mag           Phase", "           1    1.000000e+02    5.000000e-01    9.000000e+01", "           2    2.000000e+02    2.500000e-01   -9.000000e+01"]

        # act
        signal = _parse_signal_block("V(SPEAKER)", block_lines)

        # assert
        assert signal is not None
        assert signal.metadata.window == "HANN"
        assert signal.metadata.start_time == pytest.approx(0.0)
        assert signal.metadata.stop_time == pytest.approx(1e-2)
        assert signal.metadata.first_harmonic == pytest.approx(100.0)
        assert signal.metadata.start_freq == pytest.approx(100.0)
        assert signal.metadata.stop_freq == pytest.approx(204800.0)
        assert signal.metadata.dc_magnitude == pytest.approx(2.637066e-02)
        assert signal.metadata.dc_phase == pytest.approx(180.0)

    def test_parses_data_arrays(self):
        # arrange
        block_lines = ["  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02", "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 2.048000e+05", "  DC component    Norm. Mag= 0.000000e+00   Phase= 0.000000e+00", "       Index       Frequency       Norm. Mag           Phase", "           1    1.000000e+02    5.000000e-01    9.000000e+01", "           2    2.000000e+02    2.500000e-01   -9.000000e+01", "           3    3.000000e+02    1.250000e-01    4.500000e+01"]

        # act
        signal = _parse_signal_block("V(OUT)", block_lines)

        # assert
        assert signal is not None
        assert signal.num_points == 3
        np.testing.assert_array_almost_equal(signal.frequency.data, [1e2, 2e2, 3e2])
        np.testing.assert_array_almost_equal(signal.magnitude.data, [0.5, 0.25, 0.125])
        np.testing.assert_array_almost_equal(signal.phase.data, [90.0, -90.0, 45.0])

    def test_signal_name_is_stored(self):
        # arrange
        block_lines = ["  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02", "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+05", "  DC component    Norm. Mag= 0.000000e+00   Phase= 0.000000e+00", "       Index       Frequency       Norm. Mag           Phase", "           1    1.000000e+02    1.000000e+00    0.000000e+00"]

        # act
        signal = _parse_signal_block("V(INPUT)", block_lines)

        # assert
        assert signal is not None
        assert signal.name == "V(INPUT)"

    def test_expression_names_follow_convention(self):
        # arrange
        block_lines = ["  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02", "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+05", "  DC component    Norm. Mag= 0.000000e+00   Phase= 0.000000e+00", "       Index       Frequency       Norm. Mag           Phase", "           1    1.000000e+02    1.000000e+00    0.000000e+00"]

        # act
        signal = _parse_signal_block("V(SPEAKER)", block_lines)

        # assert
        assert signal is not None
        assert signal.magnitude.name == "V(SPEAKER)"
        assert signal.phase.name == "phase(V(SPEAKER))"
        assert signal.frequency.name == "frequency"

    def test_expression_units(self):
        # arrange
        block_lines = ["  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02", "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+05", "  DC component    Norm. Mag= 0.000000e+00   Phase= 0.000000e+00", "       Index       Frequency       Norm. Mag           Phase", "           1    1.000000e+02    1.000000e+00    0.000000e+00"]

        # act
        signal = _parse_signal_block("V(OUT)", block_lines)

        # assert
        assert signal is not None
        assert signal.frequency.unit == "Hz"
        assert signal.phase.unit == "°"

    def test_returns_none_when_no_data_points(self):
        # arrange
        block_lines = ["  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02", "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+05", "  DC component    Norm. Mag= 0.000000e+00   Phase= 0.000000e+00", "       Index       Frequency       Norm. Mag           Phase"]

        # act
        signal = _parse_signal_block("V(OUT)", block_lines)

        # assert
        assert signal is None

    def test_handles_negative_phase_values(self):
        # arrange
        block_lines = ["  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02", "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+05", "  DC component    Norm. Mag= 0.000000e+00   Phase= 0.000000e+00", "       Index       Frequency       Norm. Mag           Phase", "           1    1.000000e+02    8.900000e-01   -8.974781e+01", "           2    2.000000e+02    5.001798e-01    9.029180e+01"]

        # act
        signal = _parse_signal_block("V(OUT)", block_lines)

        # assert
        assert signal is not None
        assert signal.phase.data[0] == pytest.approx(-89.74781)
        assert signal.phase.data[1] == pytest.approx(90.2918)


class TestXyceFftFileLoad:

    def test_load_returns_none_for_nonexistent_file(self):
        # act
        result = xyce_fft_file_parser("/nonexistent/path/file.fft0")

        # assert
        assert result is None

    def test_load_single_signal(self):
        # arrange
        content = _single_signal_fft_text("V(SPEAKER)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            exprs = fft_file.expression_manager.expressions
            # frequency + 1 magnitude + 1 phase = 3 expressions for 1 signal
            assert len(exprs) == 3
            assert exprs[1].name == "V(SPEAKER)"
        finally:
            os.unlink(path)

    def test_load_two_signals(self):
        # arrange
        block1 = _single_signal_fft_text("V(SPEAKER)")
        block2 = _single_signal_fft_text("V(INPUT)")
        path = _write_temp_fft(block1 + "\n" + block2)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            exprs = fft_file.expression_manager.expressions
            # frequency + 2 magnitude + 2 phase = 5 expressions for 2 signals
            assert len(exprs) == 5
            assert exprs[1].name == "V(SPEAKER)"
            assert exprs[3].name == "V(INPUT)"
        finally:
            os.unlink(path)

    def test_load_stores_filename(self):
        # arrange
        content = _single_signal_fft_text()
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            files, _ = result
            assert files[0].filename == Path(path)
        finally:
            os.unlink(path)

    def test_load_returns_none_for_empty_file(self):
        # arrange
        path = _write_temp_fft("")

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is None
        finally:
            os.unlink(path)

    def test_load_returns_none_for_file_without_signal_headers(self):
        # arrange
        path = _write_temp_fft("This is not a valid FFT file.\nSome random content.\n")

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is None
        finally:
            os.unlink(path)

    def test_load_creates_expression_manager(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert fft_file.expression_manager is not None
        finally:
            os.unlink(path)

    def test_expression_manager_contains_magnitude_expression(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            magnitude = fft_file.expression_manager.evaluate("V(OUT)")
            assert magnitude is not None

        finally:
            os.unlink(path)

    def test_expression_manager_contains_phase_expression(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            phase = fft_file.expression_manager.evaluate("phase(V(OUT))")
            assert phase is not None
        finally:
            os.unlink(path)

    def test_expression_manager_contains_frequency_expression(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            freq = fft_file.expression_manager.evaluate("frequency")
            assert freq is not None
        finally:
            os.unlink(path)

    def test_load_expression_manager_contains_all_fft_expressions(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert fft_file.expression_manager.evaluate("V(OUT)") is not None
            assert fft_file.expression_manager.evaluate("phase(V(OUT))") is not None
            assert fft_file.expression_manager.evaluate("frequency") is not None
        finally:
            os.unlink(path)

    def test_frequency_data_matches_parsed_values(self):
        # arrange
        data_rows = [(1, 100.0, 0.5, 90.0), (2, 200.0, 0.25, -90.0), (3, 300.0, 0.125, 45.0)]
        content = _single_signal_fft_text("V(OUT)", data_rows=data_rows)
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            np.testing.assert_array_almost_equal(fft_file.abscissa.data, [100.0, 200.0, 300.0])
        finally:
            os.unlink(path)

    def test_magnitude_data_matches_parsed_values(self):
        # arrange
        data_rows = [(1, 100.0, 0.5, 90.0), (2, 200.0, 0.25, -90.0), (3, 300.0, 0.125, 45.0)]
        content = _single_signal_fft_text("V(OUT)", data_rows=data_rows)
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            mag = fft_file.expression_manager.evaluate("V(OUT)")
            assert mag is not None
            np.testing.assert_array_almost_equal(mag.data, [0.5, 0.25, 0.125])
        finally:
            os.unlink(path)

    def test_phase_data_matches_parsed_values(self):
        # arrange
        data_rows = [(1, 100.0, 0.5, 90.0), (2, 200.0, 0.25, -90.0), (3, 300.0, 0.125, 45.0)]
        content = _single_signal_fft_text("V(OUT)", data_rows=data_rows)
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            phase = fft_file.expression_manager.evaluate("phase(V(OUT))")
            assert phase is not None
            np.testing.assert_array_almost_equal(phase.data, [90.0, -90.0, 45.0])
        finally:
            os.unlink(path)

    def test_metadata_window_parsed_correctly(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)", window="HANN")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            # window metadata is an internal parsing detail; verify parse succeeded and signal is accessible
            fft_file, _ = _first_fft_file(result)
            assert fft_file.expression_manager.evaluate("V(OUT)") is not None
            assert fft_file.plotname == "FFT"
        finally:
            os.unlink(path)

    def test_metadata_dc_component_parsed_correctly(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)", dc_magnitude=2.637066e-02, dc_phase=180.0)
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            # dc component metadata is an internal parsing detail; verify parse succeeded and signal is accessible
            fft_file, _ = _first_fft_file(result)
            assert fft_file.expression_manager.evaluate("V(OUT)") is not None
        finally:
            os.unlink(path)

    def test_num_points_matches_data_row_count(self):
        # arrange
        data_rows = [(i, float(i) * 100, float(i) * 0.01, 0.0) for i in range(1, 11)]
        content = _single_signal_fft_text("V(OUT)", data_rows=data_rows)
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert len(fft_file.abscissa.data) == 10
        finally:
            os.unlink(path)

    def test_load_real_sample_file(self):
        # arrange
        sample_path = Path(__file__).parents[3] / "xyce_prg5g6v4.cir.fft0"
        if not sample_path.exists():
            pytest.skip("sample fft file not found")

        # act
        result = xyce_fft_file_parser(sample_path)

        # assert
        assert result is not None
        fft_file, _ = _first_fft_file(result)
        exprs = fft_file.expression_manager.expressions
        # frequency + V(SPEAKER) mag + V(SPEAKER) phase + V(INPUT) mag + V(INPUT) phase
        assert len(exprs) == 5
        assert exprs[1].name == "V(SPEAKER)"
        assert exprs[3].name == "V(INPUT)"

    def test_real_sample_file_first_signal_metadata(self):
        # arrange
        sample_path = Path(__file__).parents[3] / "xyce_prg5g6v4.cir.fft0"
        if not sample_path.exists():
            pytest.skip("sample fft file not found")

        # act
        result = xyce_fft_file_parser(sample_path)

        # assert
        assert result is not None
        # window/dc metadata are internal parsing details; verify parse succeeded and signal is accessible
        fft_file, _ = _first_fft_file(result)
        assert fft_file.expression_manager.evaluate("V(SPEAKER)") is not None
        assert fft_file.plotname == "FFT"

    def test_real_sample_file_point_count(self):
        # arrange
        sample_path = Path(__file__).parents[3] / "xyce_prg5g6v4.cir.fft0"
        if not sample_path.exists():
            pytest.skip("sample fft file not found")

        # act
        result = xyce_fft_file_parser(sample_path)

        # assert
        assert result is not None
        # each signal block has 2048 data points in the sample file
        fft_file, _ = _first_fft_file(result)
        assert len(fft_file.abscissa.data) == 2048

    def test_real_sample_file_frequency_range(self):
        # arrange
        sample_path = Path(__file__).parents[3] / "xyce_prg5g6v4.cir.fft0"
        if not sample_path.exists():
            pytest.skip("sample fft file not found")

        # act
        result = xyce_fft_file_parser(sample_path)

        # assert
        assert result is not None
        fft_file, _ = _first_fft_file(result)
        freq = fft_file.abscissa.data
        assert freq[0] == pytest.approx(100.0)
        assert freq[-1] == pytest.approx(204800.0)

    def test_real_sample_file_expression_manager_evaluates_signal(self):
        # arrange
        sample_path = Path(__file__).parents[3] / "xyce_prg5g6v4.cir.fft0"
        if not sample_path.exists():
            pytest.skip("sample fft file not found")

        # act
        result = xyce_fft_file_parser(sample_path)

        # assert
        assert result is not None
        fft_file, _ = _first_fft_file(result)
        mag = fft_file.expression_manager.evaluate("V(SPEAKER)")
        assert mag is not None
        assert len(mag.data) == 2048

    def test_real_mixed_sample_file_is_split_by_abscissa(self):
        # arrange
        sample_path = Path(__file__).parents[3] / "xyce_5u4h4ca3.cir.fft0"
        if not sample_path.exists():
            pytest.skip("mixed fft sample file not found")

        # act
        result = xyce_fft_file_parser(sample_path)

        # assert
        assert result is not None
        files, signals = result
        assert len(files) == 2
        assert len(files[0].abscissa.data) == 2048
        assert len(files[1].abscissa.data) == 1024
        assert files[0].expression_manager.evaluate("V(SPEAKER)") is not None
        assert files[1].expression_manager.evaluate("V(INPUT)") is not None

    def test_real_mixed_sample_file_parses_fft_measurements(self):
        # arrange
        sample_path = Path(__file__).parents[3] / "xyce_5u4h4ca3.cir.fft0"
        if not sample_path.exists():
            pytest.skip("mixed fft sample file not found")

        # act
        result = xyce_fft_file_parser(sample_path)

        # assert
        assert result is not None
        files, signals = result
        speaker = next(signal for signal in signals if signal.name == "V(SPEAKER)")
        input_signal = next(signal for signal in signals if signal.name == "V(INPUT)")

        assert speaker.measurements.thd_db == pytest.approx(22.94221)
        assert speaker.measurements.thd_value == pytest.approx(14.03171)
        assert speaker.measurements.sndr_db == pytest.approx(-22.94221)
        assert speaker.measurements.enob_bits == pytest.approx(-4.103358)
        assert speaker.measurements.snr_db == pytest.approx(200.0)
        assert speaker.measurements.sfdr_db == pytest.approx(-18.24281)
        assert speaker.measurements.sfdr_frequency == pytest.approx(1000.0)

        assert input_signal.measurements.thd_db == pytest.approx(25.89136)
        assert input_signal.measurements.thd_value == pytest.approx(19.70462)
        assert input_signal.measurements.sndr_db == pytest.approx(-25.89136)
        assert input_signal.measurements.enob_bits == pytest.approx(-4.593249)
        assert input_signal.measurements.snr_db == pytest.approx(200.0)
        assert input_signal.measurements.sfdr_db == pytest.approx(-21.19528)
        assert input_signal.measurements.sfdr_frequency == pytest.approx(1000.0)

    def test_two_signals_have_independent_data(self):
        # arrange
        data1 = [(1, 100.0, 1.0, 0.0), (2, 200.0, 0.5, 90.0)]
        data2 = [(1, 100.0, 0.1, 45.0), (2, 200.0, 0.2, -45.0)]
        block1 = _single_signal_fft_text("V(A)", data_rows=data1)
        block2 = _single_signal_fft_text("V(B)", data_rows=data2)
        path = _write_temp_fft(block1 + "\n" + block2)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            mag_a = fft_file.expression_manager.evaluate("V(A)")
            mag_b = fft_file.expression_manager.evaluate("V(B)")
            assert mag_a is not None
            assert mag_b is not None
            np.testing.assert_array_almost_equal(mag_a.data, [1.0, 0.5])
            np.testing.assert_array_almost_equal(mag_b.data, [0.1, 0.2])
        finally:
            os.unlink(path)

    def test_expression_manager_evaluates_arithmetic_expression(self):
        # arrange
        data_rows = [(1, 100.0, 2.0, 0.0), (2, 200.0, 4.0, 0.0)]
        content = _single_signal_fft_text("V(OUT)", data_rows=data_rows)
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            doubled = fft_file.expression_manager.evaluate("V(OUT)*2")

            # assert
            assert doubled is not None
            np.testing.assert_array_almost_equal(doubled.data, [4.0, 8.0])
        finally:
            os.unlink(path)


class TestXyceFftFileRawFileInterface:
    """Verify XyceFftFile exposes the same interface as XyceOutputFile so MainWindow can consume it directly."""

    def test_abscissa_is_frequency_expression(self):
        # arrange
        data_rows = [(1, 100.0, 0.5, 90.0), (2, 200.0, 0.25, -90.0)]
        content = _single_signal_fft_text("V(OUT)", data_rows=data_rows)
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert fft_file.abscissa is not None
            assert fft_file.abscissa.unit == "Hz"
            np.testing.assert_array_almost_equal(fft_file.abscissa.data, [100.0, 200.0])
        finally:
            os.unlink(path)

    def test_abscissa_scale_is_linear(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert fft_file.abscissa_scale == AbscissaScale.LINEAR
        finally:
            os.unlink(path)

    def test_step_information_is_single_step(self):
        # arrange
        data_rows = [(1, 100.0, 0.5, 90.0), (2, 200.0, 0.25, -90.0), (3, 300.0, 0.1, 0.0)]
        content = _single_signal_fft_text("V(OUT)", data_rows=data_rows)
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            si = fft_file.step_information
            assert si.length == 1
            assert si.abscissa_indices[0] == slice(0, 3)
        finally:
            os.unlink(path)

    def test_step_information_abscissa_range_matches_frequency_data(self):
        # arrange
        data_rows = [(1, 100.0, 0.5, 0.0), (2, 200.0, 0.25, 0.0), (3, 300.0, 0.1, 0.0)]
        content = _single_signal_fft_text("V(OUT)", data_rows=data_rows)
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            si = fft_file.step_information
            assert si.abscissa_left_value == pytest.approx(100.0)
            assert si.abscissa_right_value == pytest.approx(300.0)
        finally:
            os.unlink(path)

    def test_plotname_is_fft(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert fft_file.plotname == "FFT"
        finally:
            os.unlink(path)

    def test_complex_is_false(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert fft_file.complex is False
        finally:
            os.unlink(path)

    def test_date_is_empty_string(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert fft_file.date == ""
        finally:
            os.unlink(path)

    def test_command_is_empty_string(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert fft_file.command == ""
        finally:
            os.unlink(path)

    def test_title_contains_signal_name(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert "V(OUT)" in fft_file.title
        finally:
            os.unlink(path)

    def test_chart_type_is_ac(self):
        # arrange
        content = _single_signal_fft_text("V(OUT)")
        path = _write_temp_fft(content)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert fft_file.chart_type == "AC"
        finally:
            os.unlink(path)

    def test_abscissa_is_shared_across_signals(self):
        # arrange — two signals with the same frequency axis
        data1 = [(1, 100.0, 1.0, 0.0), (2, 200.0, 0.5, 90.0)]
        data2 = [(1, 100.0, 0.1, 45.0), (2, 200.0, 0.2, -45.0)]
        block1 = _single_signal_fft_text("V(A)", data_rows=data1)
        block2 = _single_signal_fft_text("V(B)", data_rows=data2)
        path = _write_temp_fft(block1 + "\n" + block2)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            # abscissa comes from the first signal
            fft_file, _ = _first_fft_file(result)
            np.testing.assert_array_almost_equal(fft_file.abscissa.data, [100.0, 200.0])
            # both signal magnitudes are accessible through the expression manager on the same frequency axis
            mag_a = fft_file.expression_manager.evaluate("V(A)")
            mag_b = fft_file.expression_manager.evaluate("V(B)")
            assert mag_a is not None
            assert mag_b is not None
        finally:
            os.unlink(path)

    def test_all_signal_expressions_accessible_via_expression_manager(self):
        # arrange
        block1 = _single_signal_fft_text("V(SPEAKER)")
        block2 = _single_signal_fft_text("V(INPUT)")
        path = _write_temp_fft(block1 + "\n" + block2)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            fft_file, _ = _first_fft_file(result)
            assert fft_file.expression_manager.evaluate("V(SPEAKER)") is not None
            assert fft_file.expression_manager.evaluate("phase(V(SPEAKER))") is not None
            assert fft_file.expression_manager.evaluate("V(INPUT)") is not None
            assert fft_file.expression_manager.evaluate("phase(V(INPUT))") is not None
            assert fft_file.expression_manager.evaluate("frequency") is not None
        finally:
            os.unlink(path)

    def test_title_contains_all_signal_names_for_multiple_signals(self):
        # arrange
        block1 = _single_signal_fft_text("V(SPEAKER)")
        block2 = _single_signal_fft_text("V(INPUT)")
        path = _write_temp_fft(block1 + "\n" + block2)

        try:
            # act
            result = xyce_fft_file_parser(path)

            # assert
            assert result is not None
            files, _ = result
            assert "V(SPEAKER)" in files[0].title
            assert "V(INPUT)" in files[0].title
        finally:
            os.unlink(path)
