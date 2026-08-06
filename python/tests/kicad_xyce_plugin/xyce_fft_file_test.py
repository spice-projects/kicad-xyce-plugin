import tempfile
from pathlib import Path

import numpy as np

from kicad_xyce_plugin.expression import Expression
from kicad_xyce_plugin.expression import ExpressionManager
from kicad_xyce_plugin.xyce_fft_file import xyce_fft_file_parser
from kicad_xyce_plugin.xyce_output_file import StepInformation


class TestXyceFftFileParser:

    def test_returns_none_when_no_files_match_pattern(self):
        # arrange
        # create step information for verification
        step_info = StepInformation([], [], [(0.0, 10.0)])

        # act
        result = xyce_fft_file_parser("/nonexistent/path/*.fft*", step_info)

        # assert
        # verify no files matched and result is none
        assert result is None

    def test_parses_single_fft_file_with_one_signal(self):
        # arrange
        # create a temporary directory for test files
        with tempfile.TemporaryDirectory() as tmpdir:
            # construct temporary file path
            file_path = Path(tmpdir) / "test_sim.fft0"
            # prepare standard fft file content
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
                "           2    2.000000e+02    2.500000e-01   -9.000000e+01\n"
                "\n"
            )
            # write standard content to file
            file_path.write_text(content, encoding="utf-8")
            # create step information with single step
            step_info = StepInformation([], [], [(0.0, 200.0)])

            # act
            # invoke parser with pattern matching the single file
            result = xyce_fft_file_parser(str(Path(tmpdir) / "test_sim.fft*"), step_info)

            # assert
            # verify parsing succeeded and returned one output file
            assert result is not None
            assert len(result) == 1
            output_file = result[0]
            # verify abscissa data matches expected frequencies including dc at index 0
            assert output_file.abscissa is not None
            assert output_file.abscissa.step_count == 1
            assert np.allclose(output_file.abscissa.steps[0], [0.0, 100.0, 200.0])
            # verify expression manager has parsed signals
            assert "FFT(V(OUT))" in output_file.expression_manager.expression_names
            assert "FFT(phase(V(OUT)))" in output_file.expression_manager.expression_names
            # evaluate parsed signals
            mag_expr = output_file.expression_manager.evaluate("FFT(V(OUT))")
            phase_expr = output_file.expression_manager.evaluate("FFT(phase(V(OUT)))")
            assert mag_expr is not None
            assert phase_expr is not None
            # verify magnitude and phase values including dc component
            assert np.allclose(mag_expr.steps[0], [0.01, 0.5, 0.25])
            assert np.allclose(phase_expr.steps[0], [180.0, 90.0, -90.0])

    def test_parses_multiple_files_and_accumulates_steps(self):
        # arrange
        # create temporary directory for multiple steps
        with tempfile.TemporaryDirectory() as tmpdir:
            # construct step 0 file path
            file_path_0 = Path(tmpdir) / "sim.fft0"
            # content for transient step 0
            content_0 = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
                "\n"
            )
            # write step 0 file
            file_path_0.write_text(content_0, encoding="utf-8")
            # construct step 1 file path
            file_path_1 = Path(tmpdir) / "sim.fft1"
            # content for transient step 1
            content_1 = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 2.000000e-02   Phase= 0.000000e+00\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    8.000000e-01   -9.000000e+01\n"
                "\n"
            )
            # write step 1 file
            file_path_1.write_text(content_1, encoding="utf-8")
            # create step information with two steps
            step_info = StepInformation([], [], [(0.0, 100.0), (0.0, 100.0)])

            # act
            # parse using file pattern matching both steps
            result = xyce_fft_file_parser(str(Path(tmpdir) / "sim.fft*"), step_info)

            # assert
            # verify result contains two steps accumulated
            assert result is not None
            assert len(result) == 1
            output_file = result[0]
            # verify step count
            assert output_file.step_information.length == 2
            # evaluate magnitudes and phases
            mag_expr = output_file.expression_manager.evaluate("FFT(V(OUT))")
            phase_expr = output_file.expression_manager.evaluate("FFT(phase(V(OUT)))")
            assert mag_expr is not None
            assert phase_expr is not None
            # verify step count matches number of steps
            assert mag_expr.step_count == 2
            # verify step 0 data
            assert np.allclose(mag_expr.steps[0], [0.01, 0.5])
            assert np.allclose(phase_expr.steps[0], [180.0, 90.0])
            # verify step 1 data
            assert np.allclose(mag_expr.steps[1], [0.02, 0.8])
            assert np.allclose(phase_expr.steps[1], [0.0, -90.0])

    def test_parses_fft_metrics_and_stores_in_metadata(self):
        # arrange
        # create a temporary directory for metrics testing
        with tempfile.TemporaryDirectory() as tmpdir:
            # construct file path
            file_path = Path(tmpdir) / "metrics.fft0"
            # prepare content with thd, sndr, enob, snr, and sfdr metrics
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
                "\n"
                "  THD = 1.687485e+01 dB ( 6.978185e+00 )\n"
                " SNDR = -1.687485e+01 dB\n"
                " ENOB = -3.095490e+00 bit\n"
                "  SNR = 2.000000e+02 dB\n"
                " SFDR = -1.257423e+00 dB at frequency 1.150000e+03\n"
            )
            # write test file
            file_path.write_text(content, encoding="utf-8")
            # create step information
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            # parse the metrics file
            result = xyce_fft_file_parser(str(Path(tmpdir) / "metrics.fft*"), step_info)

            # assert
            # verify metrics are parsed and stored in metadata
            assert result is not None
            output_file = result[0]
            mag_expr = output_file.expression_manager.evaluate("FFT(V(OUT))")
            assert mag_expr is not None
            # check metadata list containing metadata dictionary for step 0
            assert mag_expr.metadata is not None
            assert len(mag_expr.metadata) == 1
            step_metadata = mag_expr.metadata[0]
            # verify parsed metrics fields
            assert step_metadata.get("THD") == "1.687485e+01 dB ( 6.978185e+00 )"
            assert step_metadata.get("SNDR") == "-1.687485e+01 dB"
            assert step_metadata.get("ENOB") == "-3.095490e+00 bit"
            assert step_metadata.get("SNR") == "2.000000e+02 dB"
            assert step_metadata.get("SFDR") == "-1.257423e+00 dB at frequency 1.150000e+03"

    def test_parses_multiple_signals_and_resets_header_flags(self):
        # arrange
        # create temporary directory for multiple signals block
        with tempfile.TemporaryDirectory() as tmpdir:
            # construct file path
            file_path = Path(tmpdir) / "multi_signals.fft0"
            # prepare content containing both V(SPEAKER) and V(INPUT) signal blocks
            content = (
                "FFT analysis for V(SPEAKER):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
                "\n"
                "  THD = 1.000000e+01 dB ( 3.000000e+00 )\n"
                "\n"
                "FFT analysis for V(INPUT):\n"
                "  Window: RECT, Start Time: 1.000000e-02, Stop Time: 2.000000e-02\n"
                "  First Harmonic: 2.000000e+02, Start Freq: 2.000000e+02, Stop Freq: 2.000000e+03\n"
                "  DC component    Norm. Mag= 5.000000e-02   Phase= 0.000000e+00\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    2.000000e+02    8.000000e-01   -9.000000e+01\n"
                "\n"
                "  THD = 2.000000e+01 dB ( 4.000000e+00 )\n"
            )
            # write test file
            file_path.write_text(content, encoding="utf-8")
            # create step information
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            # parse the multi-signals file
            result = xyce_fft_file_parser(str(Path(tmpdir) / "multi_signals.fft*"), step_info)

            # assert
            # verify both signals parsed into separate output files (different abscissas)
            assert result is not None
            # two distinct abscissa groups => two output files
            assert len(result) == 2
            # gather all expression names across all output files
            all_names = [name for of in result for name in of.expression_manager.expression_names]
            assert "FFT(V(SPEAKER))" in all_names
            assert "FFT(V(INPUT))" in all_names
            # locate the output file containing V(SPEAKER)
            speaker_file = next(of for of in result if "FFT(V(SPEAKER))" in of.expression_manager.expression_names)
            speaker_mag = speaker_file.expression_manager.evaluate("FFT(V(SPEAKER))")
            assert speaker_mag is not None
            assert np.allclose(speaker_mag.steps[0], [0.01, 0.5])
            assert speaker_mag.metadata[0].get("THD") == "1.000000e+01 dB ( 3.000000e+00 )"
            # locate the output file containing V(INPUT)
            input_file = next(of for of in result if "FFT(V(INPUT))" in of.expression_manager.expression_names)
            input_mag = input_file.expression_manager.evaluate("FFT(V(INPUT))")
            assert input_mag is not None
            assert np.allclose(input_mag.steps[0], [0.05, 0.8])
            assert input_mag.metadata[0].get("THD") == "2.000000e+01 dB ( 4.000000e+00 )"

    def test_returns_none_when_data_line_has_wrong_column_count(self):
        # arrange
        # create a temporary directory
        with tempfile.TemporaryDirectory() as tmpdir:
            # construct file path with malformed data line (only 3 columns instead of 4)
            file_path = Path(tmpdir) / "bad_cols.fft0"
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "bad_cols.fft*"), step_info)

            # assert
            # wrong column count must cause parser to return none
            assert result is None

    def test_returns_none_when_data_line_index_is_out_of_order(self):
        # arrange
        # create a temporary directory
        with tempfile.TemporaryDirectory() as tmpdir:
            # construct file path with data line index jumping from 1 to 3 (skipping 2)
            file_path = Path(tmpdir) / "bad_idx.fft0"
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
                "           3    3.000000e+02    2.500000e-01   -9.000000e+01\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 300.0)])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "bad_idx.fft*"), step_info)

            # assert
            # non-sequential data index must cause parser to return none
            assert result is None

    def test_returns_none_when_index_header_appears_before_signal_header(self):
        # arrange
        # create a temporary directory
        with tempfile.TemporaryDirectory() as tmpdir:
            # construct file path missing the fft-analysis header before the index line
            file_path = Path(tmpdir) / "bad_order.fft0"
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                # dc line missing; index header appears immediately
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "bad_order.fft*"), step_info)

            # assert
            # missing dc line before index header must cause parser to return none
            assert result is None

    def test_returns_none_when_step_count_mismatches_step_information(self):
        # arrange
        # create a temporary directory
        with tempfile.TemporaryDirectory() as tmpdir:
            # write a single-step fft file but pass step_information expecting 2 steps
            file_path = Path(tmpdir) / "mismatch.fft0"
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
                "\n"
            )
            file_path.write_text(content, encoding="utf-8")
            # step_information expects 2 steps but only 1 file exists with 1 step
            step_info = StepInformation([], [], [(0.0, 100.0), (0.0, 100.0)])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "mismatch.fft*"), step_info)

            # assert
            # step count mismatch must cause parser to return none
            assert result is None

    def test_returns_none_when_file_raises_exception_during_parse(self):
        # arrange
        # create a temporary directory
        with tempfile.TemporaryDirectory() as tmpdir:
            # write a file with non-utf-8 bytes in the data section to trigger a decode error
            file_path = Path(tmpdir) / "bad_encoding.fft0"
            # valid header in utf-8 followed by an invalid byte sequence in the data section
            valid_header = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
            ).encode("utf-8")
            # invalid utf-8 continuation byte triggers a UnicodeDecodeError
            bad_bytes = b"\xff\xfe bad data\n"
            file_path.write_bytes(valid_header + bad_bytes)
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "bad_encoding.fft*"), step_info)

            # assert
            # decode exception must cause parser to return none
            assert result is None

    def test_file_without_trailing_newline_is_handled(self):
        # arrange
        # create a temporary directory
        with tempfile.TemporaryDirectory() as tmpdir:
            # write a valid fft file with no trailing newline after the last data line
            file_path = Path(tmpdir) / "no_newline.fft0"
            # strip the trailing \n from last line to exercise the break-on-eof path
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            # parser must not crash when the last line has no trailing newline
            result = xyce_fft_file_parser(str(Path(tmpdir) / "no_newline.fft*"), step_info)

            # assert
            # even with no trailing newline the parser should return a result (the last line is ignored gracefully)
            assert result is not None

    def test_logs_warning_on_unexpected_line(self, caplog):
        # arrange
        # create a temporary directory for unexpected line test
        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = Path(tmpdir) / "unexpected.fft0"
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "THIS IS AN UNEXPECTED LINE\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            with caplog.at_level("WARNING"):
                result = xyce_fft_file_parser(str(Path(tmpdir) / "unexpected.fft*"), step_info)

            # assert
            assert result is not None
            assert "unexpected line" in caplog.text
            assert "THIS IS AN UNEXPECTED LINE" in caplog.text

    def test_parses_file_without_fft_index_suffix(self):
        # arrange
        # create temporary directory for file named without .fftN suffix
        with tempfile.TemporaryDirectory() as tmpdir:
            # use a filename matching the pattern but without digits at the end
            file_path = Path(tmpdir) / "sim.fft"
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            # parser must handle files without numeric suffix (sorted as -1)
            result = xyce_fft_file_parser(str(Path(tmpdir) / "sim.fft*"), step_info)

            # assert
            assert result is not None
            assert len(result) == 1

    def test_parses_file_with_multiple_abscissas(self):
        # arrange
        # create temporary directory for multiple abscissas test
        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = Path(tmpdir) / "multi_abscissa.fft0"
            # content with two DIFFERENT harmonic settings (creating different abscissas)
            content = (
                "FFT analysis for V(OUT1):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
                "\n"
                "FFT analysis for V(OUT2):\n"
                "  Window: RECT, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 2.000000e+02, Start Freq: 2.000000e+02, Stop Freq: 2.000000e+03\n"
                "  DC component    Norm. Mag= 5.000000e-02   Phase= 0.000000e+00\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    2.000000e+02    8.000000e-01   -9.000000e+01\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "multi_abscissa.fft*"), step_info)

            # assert
            # two different abscissa keys => two output files
            assert result is not None
            assert len(result) == 2

    def test_parses_non_normalized_magnitude(self):
        # arrange
        # create temporary directory for non-normalized magnitude test
        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = Path(tmpdir) / "non_norm.fft0"
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency           Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "non_norm.fft*"), step_info)

            # assert
            assert result is not None
            output_file = result[0]
            # check the metadata in output_file (not in signals)
            # wait, the parser stores 'normalized' in the abscissa key, and then in output_file.metadata
            assert output_file.metadata.get("Normalized") is False

    def test_parses_three_step_fft_files_generated(self):
        # arrange
        with tempfile.TemporaryDirectory() as tmpdir:
            for step, dc_value, thd_value in (
                (0, 0.4323973, "1.687485e+01 dB ( 6.978185e+00 )"),
                (1, 0.3312294, "1.683650e+01 dB ( 6.947444e+00 )"),
                (2, 0.2770515, "1.679928e+01 dB ( 6.917735e+00 )"),
            ):
                file_path = Path(tmpdir) / f"sim_step{step}.fft{step}"
                content = (
                    "FFT analysis for I(L1):\n"
                    "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                    "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                    f"  DC component    Norm. Mag= {dc_value:.7e}   Phase= 1.800000e+02\n"
                    "       Index       Frequency       Norm. Mag           Phase\n"
                    "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
                    "           2    2.000000e+02    2.500000e-01   -9.000000e+01\n"
                    "\n"
                    f"  THD = {thd_value}\n"
                )
                file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 200.0), (0.0, 200.0), (0.0, 200.0)])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "sim_step*.fft*"), step_info)

            # assert
            assert result is not None
            assert len(result) == 1
            output_file = result[0]
            assert output_file.step_information.length == 3
            assert output_file.abscissa.step_count == 3
            assert len(output_file.abscissa.steps[0]) == 3
            assert output_file.abscissa.steps[0][0] == 0.0
            assert np.isclose(output_file.abscissa.steps[0][1], 100.0)
            mag = output_file.expression_manager.evaluate("FFT(I(L1))")
            assert mag is not None
            assert mag.step_count == 3
            assert np.isclose(mag.steps[0][0], 0.4323973, rtol=1e-5)
            assert np.isclose(mag.steps[1][0], 0.3312294, rtol=1e-5)
            assert np.isclose(mag.steps[2][0], 0.2770515, rtol=1e-5)
            assert mag.metadata[0].get("THD") == "1.687485e+01 dB ( 6.978185e+00 )"
            assert mag.metadata[1].get("THD") == "1.683650e+01 dB ( 6.947444e+00 )"
            assert mag.metadata[2].get("THD") == "1.679928e+01 dB ( 6.917735e+00 )"

    def test_parses_synthetic_single_step_fft_file(self):
        # arrange
        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = Path(tmpdir) / "single_step.fft0"
            content = (
                "FFT analysis for I(L1):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 4.323973e-01   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    8.652246e-01    9.000000e+01\n"
                "           2    2.000000e+02    1.234567e-01   -9.000000e+01\n"
                "\n"
                "  THD = 1.687485e+01 dB ( 6.978185e+00 )\n"
                " SNDR = 2.000000e+01 dB\n"
                " ENOB = 3.000000e+00 bit\n"
                "  SNR = 2.000000e+02 dB\n"
                " SFDR = -1.257423e+00 dB at frequency 1.150000e+03\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 200.0)])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "single_step.fft*"), step_info)

            # assert
            assert result is not None
            assert len(result) == 1
            output_file = result[0]
            mag = output_file.expression_manager.evaluate("FFT(I(L1))")
            assert mag is not None
            assert mag.step_count == 1
            assert np.isclose(mag.steps[0][0], 0.4323973, rtol=1e-7)
            assert np.isclose(mag.steps[0][1], 0.8652246, rtol=1e-7)
            assert mag.metadata[0].get("THD") == "1.687485e+01 dB ( 6.978185e+00 )"
            assert mag.metadata[0].get("SNDR") == "2.000000e+01 dB"
            assert mag.metadata[0].get("ENOB") == "3.000000e+00 bit"
            assert mag.metadata[0].get("SNR") == "2.000000e+02 dB"
            assert mag.metadata[0].get("SFDR") == "-1.257423e+00 dB at frequency 1.150000e+03"

    def test_parses_two_signals_with_same_abscissa(self):
        # arrange
        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = Path(tmpdir) / "same_abscissa.fft0"
            content = (
                "FFT analysis for I(L1):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 4.000000e-01   Phase= 0.000000e+00\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    0.000000e+00\n"
                "\n"
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 2.000000e-01   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    6.000000e-01    9.000000e+01\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 100.0)])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "same_abscissa.fft*"), step_info)

            # assert
            assert result is not None
            assert len(result) == 1
            output_file = result[0]
            names = output_file.expression_manager.expression_names
            assert "FFT(I(L1))" in names
            assert "FFT(V(OUT))" in names
            assert np.allclose(output_file.expression_manager.evaluate("FFT(I(L1))").steps[0], [0.4, 0.5])
            assert np.allclose(output_file.expression_manager.evaluate("FFT(V(OUT))").steps[0], [0.2, 0.6])

    def test_infers_magnitude_unit_from_expression_manager(self):
        # arrange
        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = Path(tmpdir) / "infer_unit.fft0"
            content = (
                "FFT analysis for V(OUT):\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 100.0)])
            raw_expression_manager = ExpressionManager([Expression("V(OUT)", [np.array([0.0, 1.0])], "mV")])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "infer_unit.fft*"), step_info, raw_expression_manager)

            # assert
            assert result is not None
            output_file = result[0]
            magnitude_expression = output_file.expression_manager.evaluate("FFT(V(OUT))")
            assert magnitude_expression is not None
            assert magnitude_expression.unit == "mV"

    def test_strips_braces_before_unit_inference(self):
        # arrange
        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = Path(tmpdir) / "strip_braces.fft0"
            content = (
                "FFT analysis for {V(OUT)}:\n"
                "  Window: HANN, Start Time: 0.000000e+00, Stop Time: 1.000000e-02\n"
                "  First Harmonic: 1.000000e+02, Start Freq: 1.000000e+02, Stop Freq: 1.000000e+03\n"
                "  DC component    Norm. Mag= 1.000000e-02   Phase= 1.800000e+02\n"
                "       Index       Frequency       Norm. Mag           Phase\n"
                "           1    1.000000e+02    5.000000e-01    9.000000e+01\n"
            )
            file_path.write_text(content, encoding="utf-8")
            step_info = StepInformation([], [], [(0.0, 100.0)])
            raw_expression_manager = ExpressionManager([Expression("V(OUT)", [np.array([0.0, 1.0])], "V")])

            # act
            result = xyce_fft_file_parser(str(Path(tmpdir) / "strip_braces.fft*"), step_info, raw_expression_manager)

            # assert
            assert result is not None
            output_file = result[0]
            assert "FFT(V(OUT))" in output_file.expression_manager.expression_names
            magnitude_expression = output_file.expression_manager.evaluate("FFT(V(OUT))")
            assert magnitude_expression is not None
            assert magnitude_expression.unit == "V"
