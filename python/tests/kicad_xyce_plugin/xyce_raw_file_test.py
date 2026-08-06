import os
import tempfile
from pathlib import Path

import numpy as np

from kicad_xyce_plugin.expression import Expression
from kicad_xyce_plugin.xyce_output_file import AbscissaScale, PlotSuggestion, StepInformation, VariableType, VariableTypeInformation
from kicad_xyce_plugin.xyce_raw_file import xyce_raw_file_parser, _parse_binary_variables, _process_abscissa_scale


def _make_raw_bytes(title: str = "Test Circuit", plotname: str = "Transient Analysis", flags: str = "real", variable_defs: list[tuple[int, str, str]] | None = None, data_matrix: np.ndarray | None = None, is_ascii: bool = False, num_points_override: int | None = None) -> bytes:
    # default to a minimal two-variable transient setup
    if variable_defs is None:
        # two variables: time abscissa and one voltage node
        variable_defs = [(0, "time", "time"), (1, "V(1)", "voltage")]
    # default to a zero-filled data matrix
    if data_matrix is None:
        data_matrix = np.zeros((2, len(variable_defs)), dtype=np.float64)
    # derive counts from the inputs
    num_variables = len(variable_defs)
    # honour the override when provided, otherwise infer from the matrix row count
    num_points = num_points_override if num_points_override is not None else data_matrix.shape[0]
    # detect complex flag from the flags string
    is_complex = "complex" in flags.lower()
    # build header lines
    lines = [f"Title: {title}", f"Plotname: {plotname}", f"Flags: {flags}", f"No. Variables: {num_variables}", f"No. Points: {num_points}", "Variables:"]
    # append one variable definition line per variable
    for idx, name, type_str in variable_defs:
        # tab-separated index, name, type
        lines.append(f"\t{idx}\t{name}\t{type_str}")
    if is_ascii:
        # ascii data section
        lines.append("Values:")
        # encode header as utf-8 bytes
        header = ("\n".join(lines) + "\n").encode("utf-8")
        # accumulate formatted value lines
        value_lines = []
        # one line per data point
        for pt_idx in range(data_matrix.shape[0]):
            # current row of values
            row = data_matrix[pt_idx]
            if is_complex:
                # interleaved real/imag pairs for each variable
                parts = []
                # format each complex value as two whitespace-separated tokens
                for val in row:
                    # append real part then imaginary part
                    parts.extend([f"{val.real:.6e}", f"{val.imag:.6e}"])
                # append formatted line with leading index token
                value_lines.append(f" {pt_idx}  " + "  ".join(parts))
            else:
                # append formatted real-valued line with leading index token
                value_lines.append(f" {pt_idx}  " + "  ".join(f"{v:.6e}" for v in row))
        # concatenate header and encoded value lines
        return header + ("\n".join(value_lines) + "\n").encode("utf-8")
    else:
        # binary data section
        lines.append("Binary:")
        # encode header as utf-8 bytes
        header = ("\n".join(lines) + "\n").encode("utf-8")
        if is_complex:
            # complex128 little-endian binary
            return header + data_matrix.astype("<c16").tobytes()
        # float64 little-endian binary
        return header + data_matrix.astype("<f8").tobytes()


def _write_temp_raw(content: bytes) -> str:
    # create a named temporary file that persists after close
    fh = tempfile.NamedTemporaryFile(delete=False, suffix=".raw")
    # write the raw bytes
    fh.write(content)
    # close the file handle so the path is accessible on all platforms
    fh.close()
    # return the path string for use by XyceOutputFile.load
    return fh.name


class TestVariableTypeInformation:

    def test_name(self):
        # arrange
        info = VariableTypeInformation("voltage", "V")
        # act
        result = info.name
        # assert
        assert result == "voltage"

    def test_unit(self):
        # arrange
        info = VariableTypeInformation("current", "A")
        # act
        result = info.unit
        # assert
        assert result == "A"


class TestVariableType:

    def test_frequency_unit(self):
        # arrange / act
        vt = VariableType.FREQUENCY
        # assert
        assert vt.value.unit == "Hz"

    def test_voltage_unit(self):
        # arrange / act
        vt = VariableType.VOLTAGE
        # assert
        assert vt.value.unit == "V"

    def test_current_unit(self):
        # arrange / act
        vt = VariableType.CURRENT
        # assert
        assert vt.value.unit == "A"

    def test_time_unit(self):
        # arrange / act
        vt = VariableType.TIME
        # assert
        assert vt.value.unit == "s"

    def test_power_unit(self):
        # arrange / act
        vt = VariableType.POWER
        # assert
        assert vt.value.unit == "W"

    def test_parameter_unit(self):
        # arrange / act
        vt = VariableType.PARAMETER
        # assert
        assert vt.value.unit == ""

    def test_phase_unit(self):
        # arrange / act
        vt = VariableType.PHASE
        # assert
        assert vt.value.unit == "°"

    def test_frequency_name(self):
        # arrange / act
        vt = VariableType.FREQUENCY
        # assert
        assert vt.value.name == "frequency"

    def test_voltage_name(self):
        # arrange / act
        vt = VariableType.VOLTAGE
        # assert
        assert vt.value.name == "voltage"

    def test_all_types_have_unique_names(self):
        # arrange
        names = [vt.value.name for vt in VariableType]
        # act
        unique_names = set(names)
        # assert — no duplicates
        assert len(names) == len(unique_names)


class TestAbscissaScale:

    def test_linear_value(self):
        # arrange / act
        scale = AbscissaScale.LINEAR
        # assert
        assert scale.value == "lin"

    def test_decade_value(self):
        # arrange / act
        scale = AbscissaScale.DECADE
        # assert
        assert scale.value == "dec"

    def test_octave_value(self):
        # arrange / act
        scale = AbscissaScale.OCTAVE
        # assert
        assert scale.value == "oct"


class TestPlotSuggestion:

    def test_chart_type(self):
        # arrange
        expr = Expression("V(1)", [np.array([1.0])], "V")
        suggestion = PlotSuggestion("AC", [expr])
        # act
        result = suggestion.chart_type
        # assert
        assert result == "AC"

    def test_expressions(self):
        # arrange
        e1 = Expression("V(1)", [np.array([1.0])], "V")
        e2 = Expression("I(R1)", [np.array([0.1])], "A")
        suggestion = PlotSuggestion("TRANSIENT", [e1, e2])
        # act
        result = suggestion.expressions
        # assert
        assert result == [e1, e2]

    def test_empty_expressions(self):
        # arrange
        suggestion = PlotSuggestion("DC", [])
        # act
        result = suggestion.expressions
        # assert
        assert result == []


class TestStepInformation:

    def test_length_single_step(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_value_ranges=[(0.0, 9.0)])
        # act / assert
        assert info.length == 1

    def test_length_multiple_steps(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_value_ranges=[(0.0, 4.0), (0.0, 4.0)])
        # act / assert
        assert info.length == 2

    def test_keys(self):
        # arrange
        info = StepInformation(keys=["R1", "R2"], values=[(1.0, 2.0)], abscissa_value_ranges=[(0.0, 4.0)])
        # act / assert
        assert info.keys == ["R1", "R2"]

    def test_values(self):
        # arrange
        info = StepInformation(keys=["R1"], values=[(1.0,), (2.0,)], abscissa_value_ranges=[(0.0, 4.0), (0.0, 4.0)])
        # act / assert
        assert info.values == [(1.0,), (2.0,)]

    def test_abscissa_left_right_ascending(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_value_ranges=[(0.0, 3.0), (0.0, 3.0)])
        # act / assert — overall left is the global minimum, right is the global maximum for ascending
        assert info.abscissa_left_value == 0.0
        assert info.abscissa_right_value == 3.0

    def test_abscissa_left_right_descending(self):
        # arrange — descending sweep (first value greater than last)
        info = StepInformation(keys=[], values=[], abscissa_value_ranges=[(10.0, 1.0)])
        # act / assert — descending: left is max, right is min
        assert info.abscissa_left_value == 10.0
        assert info.abscissa_right_value == 1.0

    def test_abscissa_ascending_flag(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_value_ranges=[(0.0, 3.0)])
        # act / assert
        assert info.abscissa_ascending is True

    def test_abscissa_descending_flag(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_value_ranges=[(3.0, 0.0)])
        # act / assert
        assert info.abscissa_ascending is False

    def test_step_abscissa_left_value(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_value_ranges=[(0.0, 4.0), (1.0, 5.0)])
        # act / assert — per-step left value
        assert info.step_abscissa_left_value(0) == 0.0
        assert info.step_abscissa_left_value(1) == 1.0

    def test_step_abscissa_right_value(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_value_ranges=[(0.0, 4.0), (1.0, 5.0)])
        # act / assert — per-step right value
        assert info.step_abscissa_right_value(0) == 4.0
        assert info.step_abscissa_right_value(1) == 5.0

    def test_empty_ranges_defaults_to_zero(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_value_ranges=[])
        # act / assert — no ranges: defaults to zero
        assert info.abscissa_left_value == 0.0
        assert info.abscissa_right_value == 0.0


class TestProcessScale:

    def test_linear_returns_original_expression(self):
        # arrange
        data = np.array([1.0, 10.0, 100.0])
        expr = Expression("frequency", data, "Hz", variable_type="frequency")
        # act
        result = _process_abscissa_scale(expr, AbscissaScale.LINEAR)
        # assert — linear scale returns the original expression unchanged
        assert result is expr

    def test_decade_applies_log10(self):
        # arrange
        expr = Expression("frequency", [np.array([1.0, 10.0, 100.0]), np.array([1.0, 10.0, 100.0])], "Hz", variable_type="frequency")
        # act
        result = _process_abscissa_scale(expr, AbscissaScale.DECADE)
        # assert
        np.testing.assert_array_almost_equal(result.data, np.array([0.0, 1.0, 2.0, 0.0, 1.0, 2.0]))

    def test_decade_preserves_name_and_unit(self):
        # arrange
        expr = Expression("frequency", [np.array([100.0])], "Hz", variable_type="frequency")
        # act
        result = _process_abscissa_scale(expr, AbscissaScale.DECADE)
        # assert
        assert result.name == "frequency"
        assert result.unit == "Hz"

    def test_octave_applies_log2(self):
        # arrange
        expr = Expression("frequency", [np.array([1.0, 2.0, 4.0, 8.0])], "Hz", variable_type="frequency")
        # act
        result = _process_abscissa_scale(expr, AbscissaScale.OCTAVE)
        # assert
        np.testing.assert_array_almost_equal(result.data, np.array([0.0, 1.0, 2.0, 3.0]))

    def test_octave_preserves_metadata(self):
        # arrange
        expr = Expression("frequency", [np.array([8.0])], "Hz", source="src", variable_type="frequency")
        # act
        result = _process_abscissa_scale(expr, AbscissaScale.OCTAVE)
        # assert
        assert result.name == "frequency"
        assert result.unit == "Hz"
        assert result.source == "src"
        assert result.variable_type == "frequency"


class TestParseBinaryVariables:

    def test_real_binary_correct_values(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0, 2.0], [1e-9, 1.1, 2.1]], dtype="<f8")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE), (2, "I(R1)", VariableType.CURRENT)]
        # act
        num_points, variables = _parse_binary_variables(raw_bytes, 0, variable_defs, False, 3, 2)
        # assert
        assert num_points == 2
        assert len(variables) == 3
        np.testing.assert_array_almost_equal(variables[0][1], [0.0, 1e-9])
        np.testing.assert_array_almost_equal(variables[1][1], [1.0, 1.1])
        np.testing.assert_array_almost_equal(variables[2][1], [2.0, 2.1])

    def test_real_binary_variable_names_and_units(self):
        # arrange
        data_matrix = np.zeros((1, 2), dtype="<f8")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(out)", VariableType.VOLTAGE)]
        # act
        num_points, variables = _parse_binary_variables(raw_bytes, 0, variable_defs, False, 2, 1)
        # assert
        assert num_points == 1
        assert len(variables) == 2
        assert variables[0][0] == "time"
        assert variables[0][2] == "s"
        assert variables[0][3] == "time"
        assert variables[1][0] == "V(out)"
        assert variables[1][2] == "V"
        assert variables[1][3] == "voltage"

    def test_real_binary_num_points_zero_infers_from_data(self):
        # arrange — three points, num_points passed as 0
        data_matrix = np.array([[0.0, 1.0], [1.0, 2.0], [2.0, 3.0]], dtype="<f8")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        num_points, variables = _parse_binary_variables(raw_bytes, 0, variable_defs, False, 2, 0)
        # assert
        assert num_points == 3
        assert len(variables) == 2

    def test_real_binary_with_trailing_content_stops_at_num_points(self):
        # arrange — write two data points then append junk bytes
        data_matrix = np.array([[0.0, 1.0], [1.0, 2.0]], dtype="<f8")
        junk = b"\xff" * 64
        raw_bytes = data_matrix.tobytes() + junk
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        num_points, variables = _parse_binary_variables(raw_bytes, 0, variable_defs, False, 2, 2)
        # assert
        assert num_points == 2
        assert len(variables) == 2

    def test_complex_binary_abscissa_is_real(self):
        # arrange — frequency variable stored as complex128; real part is the frequency
        data_matrix = np.array([[1000.0 + 0j, 0.5 + 0.5j], [10000.0 + 0j, 0.7 + 0.3j]], dtype="<c16")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "frequency", VariableType.FREQUENCY), (1, "V(out)", VariableType.VOLTAGE)]
        # act
        num_points, variables = _parse_binary_variables(raw_bytes, 0, variable_defs, True, 2, 2)
        # assert
        assert num_points == 2
        assert len(variables) == 2
        assert variables[0][1].dtype == np.float64
        assert variables[1][1].dtype == np.complex128

    def test_complex_binary_num_points_zero_infers_from_data(self):
        # arrange
        data_matrix = np.array([[1e3 + 0j, 1.0 + 0.5j], [1e4 + 0j, 0.9 + 0.4j], [1e5 + 0j, 0.7 + 0.3j]], dtype="<c16")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "frequency", VariableType.FREQUENCY), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        num_points, variables = _parse_binary_variables(raw_bytes, 0, variable_defs, True, 2, 0)
        # assert
        assert num_points == 3
        assert len(variables) == 2

    def test_unknown_variable_type_uses_empty_unit(self):
        # arrange — variable definition with None type (unknown)
        data_matrix = np.array([[0.0, 5.0]], dtype="<f8")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "time", VariableType.TIME), (1, "CUSTOM_VAR", None)]
        # act
        num_points, variables = _parse_binary_variables(raw_bytes, 0, variable_defs, False, 2, 1)
        # assert — unknown type variable included with empty unit and no variable_type
        assert num_points == 1
        assert len(variables) == 2
        assert variables[1][2] == ""
        assert variables[1][3] is None

    def test_real_binary_with_data_offset(self):
        # arrange — header bytes prepended; offset points past them
        header = b"Header bytes here\n"
        data_matrix = np.array([[1.0, 2.0]], dtype="<f8")
        raw_bytes = header + data_matrix.tobytes()
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        num_points, variables = _parse_binary_variables(raw_bytes, len(header), variable_defs, False, 2, 1)
        # assert
        assert num_points == 1
        assert len(variables) == 2
        np.testing.assert_array_almost_equal(variables[0][1], [1.0])
        np.testing.assert_array_almost_equal(variables[1][1], [2.0])


# class TestParseAsciiVariables:

#     def test_real_ascii_correct_values(self):
#         # arrange — space-separated ascii with leading index
#         text = " 0  0.000000e+00  1.000000e+00\n 1  1.000000e-09  1.100000e+00\n"
#         raw_bytes = text.encode("utf-8")
#         variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
#         # act
#         variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 2)
#         # assert
#         assert variables is not None
#         assert len(variables) == 2
#         np.testing.assert_array_almost_equal(variables[0].data, [0.0, 1e-9])
#         np.testing.assert_array_almost_equal(variables[1].data, [1.0, 1.1])

#     def test_real_ascii_without_index_tokens(self):
#         # arrange — values only, no leading index
#         text = " 0.000000e+00  2.000000e+00\n 1.000000e-09  3.000000e+00\n"
#         raw_bytes = text.encode("utf-8")
#         variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
#         # act
#         variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 2)
#         # assert
#         assert variables is not None
#         np.testing.assert_array_almost_equal(variables[0].data, [0.0, 1e-9])
#         np.testing.assert_array_almost_equal(variables[1].data, [2.0, 3.0])

#     def test_real_ascii_variable_names_and_units(self):
#         # arrange
#         text = " 0  0.0  1.0\n 1  1.0  2.0\n"
#         raw_bytes = text.encode("utf-8")
#         variable_defs = [(0, "time", VariableType.TIME), (1, "V(out)", VariableType.VOLTAGE)]
#         # act
#         variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 2)
#         # assert
#         assert variables[0].name == "time"
#         assert variables[0].unit == "s"
#         assert variables[1].name == "V(out)"
#         assert variables[1].unit == "V"

#     def test_complex_ascii_abscissa_is_real(self):
#         # arrange — interleaved real/imag pairs: freq, then V(out) real+imag
#         text = " 0  1.000000e+03  0.000000e+00  5.000000e-01  5.000000e-01\n"
#         raw_bytes = text.encode("utf-8")
#         variable_defs = [(0, "frequency", VariableType.FREQUENCY), (1, "V(out)", VariableType.VOLTAGE)]
#         # act
#         variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, True, 2, 1)
#         # assert — frequency abscissa is real
#         assert variables is not None
#         assert variables[0].data.dtype == np.float64
#         np.testing.assert_almost_equal(variables[0].data[0], 1e3)
#         assert variables[1].data.dtype == np.complex128
#         np.testing.assert_almost_equal(variables[1].data[0], 0.5 + 0.5j)

#     def test_real_ascii_num_points_zero_reads_all(self):
#         # arrange — num_points=0 means read all available lines
#         text = " 0  0.0  1.0\n 1  1.0  2.0\n 2  2.0  3.0\n"
#         raw_bytes = text.encode("utf-8")
#         variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
#         # act
#         variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 0)
#         # assert — all three points read
#         assert variables is not None
#         assert len(variables[0].data) == 3

#     def test_real_ascii_stops_at_num_points(self):
#         # arrange — four data lines but only two requested
#         text = " 0  0.0  1.0\n 1  1.0  2.0\n 2  2.0  3.0\n 3  3.0  4.0\n"
#         raw_bytes = text.encode("utf-8")
#         variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
#         # act
#         variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 2)
#         # assert — only first two points
#         assert variables is not None
#         assert len(variables[0].data) == 2

#     def test_real_ascii_with_data_offset(self):
#         # arrange — header bytes prepended; offset points past them
#         header = b"Values:\n"
#         text = " 0  5.0  6.0\n 1  7.0  8.0\n"
#         raw_bytes = header + text.encode("utf-8")
#         variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
#         # act
#         variables = _parse_ascii_variables(raw_bytes, len(header), variable_defs, False, 2, 2)
#         # assert
#         assert variables is not None
#         np.testing.assert_array_almost_equal(variables[0].data, [5.0, 7.0])

#     def test_empty_text_returns_none(self):
#         # arrange
#         raw_bytes = b"\n\n\n"
#         variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
#         # act
#         variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 2)
#         # assert — no parseable data
#         assert variables is None

#     def test_line_with_no_tokens_is_skipped(self):
#         # arrange — blank line then valid data line
#         text = "\n 0  1.0  2.0\n"
#         raw_bytes = text.encode("utf-8")
#         variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
#         # act
#         variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 1)
#         # assert — blank line caused IndexError on tokens[0]; gracefully skipped
#         assert variables is not None
#         np.testing.assert_almost_equal(variables[0].data[0], 1.0)

#     def test_unknown_variable_type_accepted(self):
#         # arrange
#         text = " 0  0.0  9.9\n"
#         raw_bytes = text.encode("utf-8")
#         variable_defs = [(0, "time", VariableType.TIME), (1, "MYSTERY", None)]
#         # act
#         variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 1)
#         # assert — variable with None type is still included
#         assert variables is not None
#         assert len(variables) == 2
#         assert variables[1].variable_type is None
#         assert variables[1].unit == ""


class TestXyceOutputFile:

    def test_load_returns_none_when_file_not_found(self):
        # arrange
        path = "/tmp/nonexistent_xyce_raw_file_abc123.raw"
        # act
        result = xyce_raw_file_parser(path)
        # assert
        assert result is None

    def test_load_real_binary_title(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1], [2e-9, 1.2]], dtype=np.float64)
        content = _make_raw_bytes(title="RC Circuit", data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert raw.title == "RC Circuit"
        # cleanup
        os.unlink(path)

    def test_load_real_binary_filename(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — filename stored as Path
        assert raw is not None
        assert raw.filename == Path(path)
        # cleanup
        os.unlink(path)

    def test_load_real_binary_complex_flag_false(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(flags="real", data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert raw.complex is False
        # cleanup
        os.unlink(path)

    def test_load_complex_binary_complex_flag_true(self):
        # arrange
        data_matrix = np.array([[1e3 + 0j, 0.5 + 0.5j], [1e4 + 0j, 0.7 + 0.3j]], dtype="<c16")
        vdefs = [(0, "frequency", "frequency"), (1, "V(out)", "voltage")]
        content = _make_raw_bytes(plotname="AC Analysis", flags="complex", variable_defs=vdefs, data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert raw.complex is True
        # cleanup
        os.unlink(path)

    def test_load_real_binary_abscissa_values(self):
        # arrange
        time_values = np.array([0.0, 1e-9, 2e-9, 3e-9])
        data_matrix = np.column_stack([time_values, np.ones(4)])
        content = _make_raw_bytes(data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        np.testing.assert_array_almost_equal(raw.abscissa.data, time_values)
        # cleanup
        os.unlink(path)

    def test_load_real_binary_abscissa_scale_is_linear(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — xyce always uses linear abscissa scale
        assert raw is not None
        assert raw.abscissa_scale == AbscissaScale.LINEAR
        # cleanup
        os.unlink(path)

    def test_load_real_binary_single_step(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1], [2e-9, 1.2]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert raw.steps == 1
        # cleanup
        os.unlink(path)

    def test_load_chart_type_transient(self):
        # arrange — time abscissa → TRANSIENT
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(plotname="Transient Analysis", flags="real", variable_defs=[(0, "time", "time"), (1, "V(1)", "voltage")], data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert raw.chart_type == "TRANSIENT"
        # cleanup
        os.unlink(path)

    def test_load_chart_type_ac(self):
        # arrange — frequency abscissa → AC
        data_matrix = np.array([[1e3 + 0j, 0.5 + 0.5j], [1e4 + 0j, 0.7 + 0.3j]], dtype="<c16")
        vdefs = [(0, "frequency", "frequency"), (1, "V(out)", "voltage")]
        content = _make_raw_bytes(plotname="AC Analysis", flags="complex", variable_defs=vdefs, data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert raw.chart_type == "AC"
        # cleanup
        os.unlink(path)

    def test_load_chart_type_dc(self):
        # arrange — voltage abscissa → DC
        v_sweep = np.array([0.0, 1.0, 2.0, 3.0, 4.0, 5.0])
        v_out = np.array([0.0, 0.5, 1.0, 1.5, 2.0, 2.5])
        data_matrix = np.column_stack([v_sweep, v_out])
        vdefs = [(0, "v(v-sweep)", "voltage"), (1, "V(out)", "voltage")]
        content = _make_raw_bytes(plotname="DC transfer characteristic", flags="real", variable_defs=vdefs, data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert raw.chart_type == "DC"
        # cleanup
        os.unlink(path)

    def test_load_binary_with_trailing_content_ignored(self):
        # arrange — extra junk bytes after valid binary data
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1], [2e-9, 1.2]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        content = content + b"\nSome extra CSV junk\n1,2,3\n4,5,6\n"
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — three points loaded correctly; trailing content ignored
        assert raw is not None
        assert len(raw.abscissa.data) == 3
        # cleanup
        os.unlink(path)

    def test_load_no_points_zero_infers_from_binary_data(self):
        # arrange — No. Points: 0 in header; actual points inferred from binary size
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1], [2e-9, 1.2]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix, num_points_override=0)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — three points inferred even though header says 0
        assert raw is not None
        assert len(raw.abscissa.data) == 3
        # cleanup
        os.unlink(path)

    def test_load_ascii_values_section(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0, 2.0], [1e-9, 1.1, 2.1], [2e-9, 1.2, 2.2]], dtype=np.float64)
        vdefs = [(0, "time", "time"), (1, "V(1)", "voltage"), (2, "I(R1)", "current")]
        content = _make_raw_bytes(variable_defs=vdefs, data_matrix=data_matrix, is_ascii=True)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert len(raw.abscissa.data) == 3
        np.testing.assert_array_almost_equal(raw.abscissa.data, [0.0, 1e-9, 2e-9])
        # cleanup
        os.unlink(path)

    def test_load_ascii_values_variable_data_correct(self):
        # arrange
        v_data = np.array([1.0, 1.5, 2.0])
        data_matrix = np.column_stack([np.array([0.0, 1e-9, 2e-9]), v_data])
        vdefs = [(0, "time", "time"), (1, "V(out)", "voltage")]
        content = _make_raw_bytes(variable_defs=vdefs, data_matrix=data_matrix, is_ascii=True)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        em = raw.expression_manager
        v_out = em.evaluate("V(out)")
        assert v_out is not None
        np.testing.assert_array_almost_equal(v_out.data, v_data)
        # cleanup
        os.unlink(path)

    def test_load_returns_none_when_data_section_missing(self):
        # arrange — file with no Binary: or Values: line
        content = b"Title: Test\nDate: Mon Jan 1 00:00:00 2024\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 1\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\n"
        path = _write_temp_raw(content)
        # act
        result = xyce_raw_file_parser(path)
        # assert
        assert result is None
        # cleanup
        os.unlink(path)

    def test_load_skips_malformed_variable_lines(self):
        # arrange — variables section has one valid and one malformed (two-field) line
        content = b"Title: Test\nDate: Mon Jan 1 00:00:00 2024\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 1\nVariables:\n\t0\ttime\ttime\n\tBAD LINE\n\t1\tV(1)\tvoltage\nBinary:\n"
        data_row = np.array([[0.0, 1.0]], dtype="<f8")
        content = content + data_row.tobytes()
        path = _write_temp_raw(content)
        # act
        result = xyce_raw_file_parser(path)
        # assert — file still loads; malformed line was silently skipped
        assert result is not None
        # cleanup
        os.unlink(path)

    def test_load_returns_none_when_ascii_parse_produces_no_data(self):
        # arrange — Values: section has only non-numeric content; parse returns None
        content = b"Title: Test\nDate: Mon Jan 1 00:00:00 2024\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 0\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\nValues:\nNO_NUMERIC_DATA\n"
        path = _write_temp_raw(content)
        # act
        result = xyce_raw_file_parser(path)
        # assert — ascii parse returned None; load returns None
        assert result is None
        # cleanup
        os.unlink(path)

    def test_load_returns_none_when_file_has_no_trailing_newline(self):
        # arrange — header truncated with no newline at end; no data section reached
        content = b"Title: Test\nDate: Mon Jan 1 00:00:00 2024"
        path = _write_temp_raw(content)
        # act
        result = xyce_raw_file_parser(path)
        # assert — no data section: returns None
        assert result is None
        # cleanup
        os.unlink(path)

    def test_load_expression_manager_contains_all_variables(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0, 2.0], [1e-9, 1.1, 2.1]], dtype=np.float64)
        vdefs = [(0, "time", "time"), (1, "V(1)", "voltage"), (2, "I(R1)", "current")]
        content = _make_raw_bytes(variable_defs=vdefs, data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        em = raw.expression_manager
        assert em.evaluate("V(1)") is not None
        assert em.evaluate("I(R1)") is not None
        assert em.evaluate("time") is not None
        # cleanup
        os.unlink(path)

    def test_load_unknown_variable_type_still_loaded(self):
        # arrange — variable with type not in VariableType enum
        data_matrix = np.array([[0.0, 5.0], [1.0, 6.0]], dtype=np.float64)
        vdefs = [(0, "time", "time"), (1, "CUSTOM_SIG", "custom_type")]
        content = _make_raw_bytes(variable_defs=vdefs, data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — file loads successfully; unknown-typed variable is accessible
        assert raw is not None
        em = raw.expression_manager
        custom = em.evaluate("CUSTOM_SIG")
        assert custom is not None
        # cleanup
        os.unlink(path)

    def test_load_step_information_abscissa_range(self):
        # arrange — single non-stepped analysis; check step info range
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1], [2e-9, 1.2]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        info = raw.step_information
        assert info.step_abscissa_left_value(0) == 0.0
        np.testing.assert_almost_equal(info.step_abscissa_right_value(0), 2e-9)
        # cleanup
        os.unlink(path)

    def test_load_utf8_encoded_header(self):
        # arrange — title with non-ASCII characters encoded as utf-8
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(title="RC Schéma", data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert "Sch" in raw.title
        # cleanup
        os.unlink(path)

    def test_load_complex_ac_abscissa_is_frequency(self):
        # arrange
        freq_vals = np.array([1e3, 1e4, 1e5])
        v_vals = np.array([1.0 + 0j, 0.7 + 0.7j, 0.0 + 1.0j])
        data_matrix = np.column_stack([freq_vals.astype("<c16"), v_vals.astype("<c16")])
        vdefs = [(0, "frequency", "frequency"), (1, "V(out)", "voltage")]
        content = _make_raw_bytes(plotname="AC Analysis", flags="complex", variable_defs=vdefs, data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — abscissa is real frequency values
        assert raw is not None
        np.testing.assert_array_almost_equal(raw.abscissa.data, [1e3, 1e4, 1e5])
        assert raw.abscissa.data.dtype == np.float64
        # cleanup
        os.unlink(path)

    def test_load_complex_ac_signal_is_complex(self):
        # arrange
        freq_vals = np.array([1e3, 1e4])
        v_vals = np.array([0.5 + 0.5j, 0.7 + 0.3j])
        data_matrix = np.column_stack([freq_vals.astype("<c16"), v_vals.astype("<c16")])
        vdefs = [(0, "frequency", "frequency"), (1, "V(out)", "voltage")]
        content = _make_raw_bytes(plotname="AC Analysis", flags="complex", variable_defs=vdefs, data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — V(out) expression is complex
        assert raw is not None
        v_out = raw.expression_manager.evaluate("V(out)")
        assert v_out is not None
        assert v_out.complex is True
        np.testing.assert_almost_equal(v_out.data[0], 0.5 + 0.5j)
        # cleanup
        os.unlink(path)

    def test_load_ascii_complex_ac(self):
        # arrange
        freq_vals = np.array([1e3, 1e4])
        v_vals = np.array([0.5 + 0.5j, 0.7 + 0.3j])
        data_matrix = np.column_stack([freq_vals.astype("<c16"), v_vals.astype("<c16")])
        vdefs = [(0, "frequency", "frequency"), (1, "V(out)", "voltage")]
        content = _make_raw_bytes(plotname="AC Analysis", flags="complex", variable_defs=vdefs, data_matrix=data_matrix, is_ascii=True)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert raw.complex is True
        assert raw.chart_type == "AC"
        np.testing.assert_array_almost_equal(raw.abscissa.data, [1e3, 1e4])
        # cleanup
        os.unlink(path)

    def test_load_ascii_no_points_zero_reads_all(self):
        # arrange — No. Points: 0 with ASCII format
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1], [2e-9, 1.2]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix, is_ascii=True, num_points_override=0)
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — all three points read
        assert raw is not None
        assert len(raw.abscissa.data) == 3
        # cleanup
        os.unlink(path)


def _make_multi_block_raw_bytes(title: str = "Stepped Circuit", variable_defs: list[tuple[int, str, str]] | None = None, step_matrices: list[np.ndarray] | None = None, param_name: str = "R1", param_values: list[float] | None = None) -> bytes:
    # default to two-variable DC sweep with a single voltage node
    if variable_defs is None:
        variable_defs = [(0, "sweep", "voltage"), (1, "V(2)", "voltage")]
    # default to three steps with four points each
    if step_matrices is None:
        step_matrices = [np.array([[0.0, v * 0.5], [1.0, v * 0.5 + 0.5], [2.0, v * 0.5 + 1.0], [3.0, v * 0.5 + 1.5]], dtype=np.float64) for v in [1.0, 2.0, 3.0]]
    # default parameter values: one per step matrix
    if param_values is None:
        param_values = [float(i + 1) * 1000 for i in range(len(step_matrices))]
    # total number of steps
    num_steps = len(step_matrices)
    # number of variables
    num_variables = len(variable_defs)
    # accumulate raw bytes for all blocks
    result = b""
    # build one header+binary block per step
    for step_index, (data_matrix, param_value) in enumerate(zip(step_matrices, param_values)):
        # number of points for this block
        num_points = data_matrix.shape[0]
        # build the step plotname
        plotname = f"Step Analysis: Step {step_index + 1} of {num_steps} params:  name = {param_name} value = {param_value:.0f}  DC transfer characteristic"
        # build header lines
        lines = [f"Title: {title}", f"Plotname: {plotname}", "Flags: real", f"No. Variables: {num_variables}", f"No. Points: {num_points}", "Variables:"]
        # append variable definitions
        for idx, name, type_str in variable_defs:
            lines.append(f"\t{idx}\t{name}\t{type_str}")
        # binary data section marker
        lines.append("Binary:")
        # encode header as utf-8 bytes
        header = ("\n".join(lines) + "\n").encode("utf-8")
        # append binary data for this block
        result += header + data_matrix.astype("<f8").tobytes()
    return result


class TestXyceOutputFileMultiBlock:

    def test_multi_block_step_count(self):
        # arrange — two-step file with two blocks
        content = _make_multi_block_raw_bytes(param_values=[1000.0, 2000.0], step_matrices=[np.array([[0.0, 1.0], [1.0, 2.0]], dtype=np.float64), np.array([[0.0, 1.5], [1.0, 2.5]], dtype=np.float64)])
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — two steps parsed
        assert raw is not None
        assert raw.steps == 2
        # cleanup
        os.unlink(path)

    def test_multi_block_three_steps(self):
        # arrange — three-step file
        content = _make_multi_block_raw_bytes(param_values=[1000.0, 2000.0, 3000.0])
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert raw.steps == 3
        # cleanup
        os.unlink(path)

    def test_multi_block_step_parameter_keys(self):
        # arrange
        content = _make_multi_block_raw_bytes(param_name="R1_VAL", param_values=[1000.0, 2000.0])
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — parameter name extracted from Plotname
        assert raw is not None
        assert raw.step_information.keys == ["R1_VAL"]
        # cleanup
        os.unlink(path)

    def test_multi_block_step_parameter_values(self):
        # arrange
        content = _make_multi_block_raw_bytes(param_name="R1", param_values=[1000.0, 2000.0])
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — parameter values extracted from each block's Plotname
        assert raw is not None
        assert raw.step_information.values == [(1000.0,), (2000.0,)]
        # cleanup
        os.unlink(path)

    def test_multi_block_expression_step_count(self):
        # arrange — two steps of 3 points each
        m = np.array([[0.0, 1.0], [1.0, 2.0], [2.0, 3.0]], dtype=np.float64)
        content = _make_multi_block_raw_bytes(param_values=[1000.0, 2000.0], step_matrices=[m, m])
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — the abscissa expression has two per-step arrays
        assert raw is not None
        assert raw.abscissa.step_count == 2
        # cleanup
        os.unlink(path)

    def test_multi_block_step_data_correct_values(self):
        # arrange — two steps with distinct data
        m0 = np.array([[0.0, 1.0], [1.0, 2.0], [2.0, 3.0]], dtype=np.float64)
        m1 = np.array([[0.0, 4.0], [1.0, 5.0], [2.0, 6.0]], dtype=np.float64)
        content = _make_multi_block_raw_bytes(param_values=[1000.0, 2000.0], step_matrices=[m0, m1])
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — step_data(0) and step_data(1) return correct per-step values
        assert raw is not None
        v2_expr = raw.expression_manager.evaluate("V(2)")
        assert v2_expr is not None
        np.testing.assert_array_almost_equal(v2_expr.step_data(0), [1.0, 2.0, 3.0])
        np.testing.assert_array_almost_equal(v2_expr.step_data(1), [4.0, 5.0, 6.0])
        # cleanup
        os.unlink(path)

    def test_multi_block_abscissa_step_data_zero_copy(self):
        # arrange — verify step_data returns a numpy view (not a copy) by checking base array
        m0 = np.array([[0.0, 1.0], [1.0, 2.0]], dtype=np.float64)
        m1 = np.array([[0.0, 3.0], [1.0, 4.0]], dtype=np.float64)
        content = _make_multi_block_raw_bytes(param_values=[1000.0, 2000.0], step_matrices=[m0, m1])
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — step_data arrays are views into the mmap (have a non-None base)
        assert raw is not None
        assert raw.abscissa.steps[0].base is not None
        assert raw.abscissa.steps[1].base is not None
        # cleanup
        os.unlink(path)

    def test_multi_block_data_property_concatenates(self):
        # arrange — two steps with distinct ordinate values
        m0 = np.array([[0.0, 1.0], [1.0, 2.0]], dtype=np.float64)
        m1 = np.array([[0.0, 3.0], [1.0, 4.0]], dtype=np.float64)
        content = _make_multi_block_raw_bytes(param_values=[1000.0, 2000.0], step_matrices=[m0, m1])
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — .data returns all steps concatenated
        assert raw is not None
        v2_expr = raw.expression_manager.evaluate("V(2)")
        assert v2_expr is not None
        np.testing.assert_array_almost_equal(v2_expr.data, [1.0, 2.0, 3.0, 4.0])
        # cleanup
        os.unlink(path)

    def test_multi_block_title_from_first_block(self):
        # arrange
        content = _make_multi_block_raw_bytes(title="DC Stepped Test")
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert
        assert raw is not None
        assert raw.title == "DC Stepped Test"
        # cleanup
        os.unlink(path)

    def test_multi_block_abscissa_value_ranges(self):
        # arrange
        m = np.array([[0.0, 1.0], [1.0, 2.0], [2.0, 3.0]], dtype=np.float64)
        content = _make_multi_block_raw_bytes(param_values=[1000.0, 2000.0], step_matrices=[m, m])
        path = _write_temp_raw(content)
        # act
        raw = xyce_raw_file_parser(path)
        # assert — both steps share the same sweep range [0.0, 2.0]
        assert raw is not None
        assert raw.step_information.step_abscissa_left_value(0) == 0.0
        assert raw.step_information.step_abscissa_right_value(0) == 2.0
        assert raw.step_information.step_abscissa_left_value(1) == 0.0
        assert raw.step_information.step_abscissa_right_value(1) == 2.0
        # cleanup
        os.unlink(path)


class TestXyceRawFileParserEdgeCases:

    def test_ascii_values_with_blank_lines(self):
        # Blank line in ASCII values should be skipped (handled by `continue`)
        content = _make_raw_bytes(is_ascii=True)
        # Introduce blank lines in the middle of Values: data
        parts = content.split(b"\n")
        idx = parts.index(b"Values:")
        parts.insert(idx + 2, b"")
        parts.insert(idx + 4, b"   ")
        content = b"\n".join(parts)

        path = _write_temp_raw(content)
        raw = xyce_raw_file_parser(Path(path))
        assert raw is not None
        assert len(raw.abscissa.data) == 2
        os.unlink(path)

    def test_ascii_values_unexpected_index(self):
        # Unexpected point index (e.g. index 2 instead of index 1)
        content = _make_raw_bytes(is_ascii=True)
        content = content.replace(b"\n 1 ", b"\n 2 ")
        path = _write_temp_raw(content)
        raw = xyce_raw_file_parser(Path(path))
        assert raw is None
        os.unlink(path)

    def test_ascii_values_invalid_token_count(self):
        # Modify line to have unexpected number of tokens
        content = _make_raw_bytes(is_ascii=True)
        content = content.replace(b"\n 1 ", b"\n 1 0.000000e+00")
        path = _write_temp_raw(content)
        raw = xyce_raw_file_parser(Path(path))
        assert raw is None
        os.unlink(path)

    def test_ascii_values_parsing_exception(self):
        # Non-numeric token causes a float conversion exception
        content = _make_raw_bytes(is_ascii=True)
        content = content.replace(b" 1  0.000000e+00", b" 1  abc")
        path = _write_temp_raw(content)
        raw = xyce_raw_file_parser(Path(path))
        assert raw is None
        os.unlink(path)

    def test_ascii_values_point_count_mismatch(self):
        # No. Points specifies 3 but only 2 points are provided
        content = _make_raw_bytes(is_ascii=True, num_points_override=3)
        path = _write_temp_raw(content)
        raw = xyce_raw_file_parser(Path(path))
        assert raw is None
        os.unlink(path)

    def test_multi_block_variables_mismatch(self):
        # Second step has different variables than first step
        m0 = np.array([[0.0, 1.0], [1.0, 2.0]], dtype=np.float64)
        m1 = np.array([[0.0, 3.0], [1.0, 4.0]], dtype=np.float64)
        content0 = _make_multi_block_raw_bytes(param_values=[1000.0], step_matrices=[m0])
        content1 = _make_multi_block_raw_bytes(param_values=[2000.0], step_matrices=[m1], variable_defs=[(0, "sweep", "voltage"), (1, "V(3)", "voltage")])
        header_marker = b"Title: Stepped Circuit"
        idx = content1.find(header_marker)
        content = content0 + content1[idx:]

        path = _write_temp_raw(content)
        raw = xyce_raw_file_parser(Path(path))
        assert raw is None
        os.unlink(path)
