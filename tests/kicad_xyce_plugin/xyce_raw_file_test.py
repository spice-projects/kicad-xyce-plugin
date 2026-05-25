import os
import tempfile
from pathlib import Path

import numpy as np

from kicad_xyce_plugin.expression import Expression
from kicad_xyce_plugin.xyce_raw_file import AbscissaScale, PlotSuggestion, StepInformation, VariableType, VariableTypeInformation, XyceRawFile, _parse_ascii_variables, _parse_binary_variables, _process_scale, _process_steps, _steps_have_consistent_abscissa_direction


def _make_raw_bytes(title: str = "Test Circuit", date: str = "Mon Jan  1 00:00:00 2024", plotname: str = "Transient Analysis", flags: str = "real", variable_defs: list[tuple[int, str, str]] | None = None, data_matrix: np.ndarray | None = None, is_ascii: bool = False, num_points_override: int | None = None) -> bytes:
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
    lines = [f"Title: {title}", f"Date: {date}", f"Plotname: {plotname}", f"Flags: {flags}", f"No. Variables: {num_variables}", f"No. Points: {num_points}", "Variables:"]
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
    # return the path string for use by XyceRawFile.load
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
        expr = Expression("V(1)", np.array([1.0]), "V")
        suggestion = PlotSuggestion("AC", [expr])
        # act
        result = suggestion.chart_type
        # assert
        assert result == "AC"

    def test_expressions(self):
        # arrange
        e1 = Expression("V(1)", np.array([1.0]), "V")
        e2 = Expression("I(R1)", np.array([0.1]), "A")
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
        info = StepInformation(keys=[], values=[], abscissa_indices=[slice(0, 10)], abscissa_value_ranges=[(0.0, 9.0)])
        # act / assert
        assert info.length == 1

    def test_length_multiple_steps(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_indices=[slice(0, 5), slice(5, 10)], abscissa_value_ranges=[(0.0, 4.0), (0.0, 4.0)])
        # act / assert
        assert info.length == 2

    def test_keys(self):
        # arrange
        info = StepInformation(keys=["R1", "R2"], values=[(1.0, 2.0)], abscissa_indices=[slice(0, 5)], abscissa_value_ranges=[(0.0, 4.0)])
        # act / assert
        assert info.keys == ["R1", "R2"]

    def test_values(self):
        # arrange
        info = StepInformation(keys=["R1"], values=[(1.0,), (2.0,)], abscissa_indices=[slice(0, 5), slice(5, 10)], abscissa_value_ranges=[(0.0, 4.0), (0.0, 4.0)])
        # act / assert
        assert info.values == [(1.0,), (2.0,)]

    def test_abscissa_indices(self):
        # arrange
        slices = [slice(0, 5), slice(5, 10)]
        info = StepInformation(keys=[], values=[], abscissa_indices=slices, abscissa_value_ranges=[(0.0, 4.0), (0.0, 4.0)])
        # act / assert
        assert info.abscissa_indices == slices

    def test_abscissa_left_right_ascending(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_indices=[slice(0, 4), slice(4, 8)], abscissa_value_ranges=[(0.0, 3.0), (0.0, 3.0)])
        # act / assert — overall left is the global minimum, right is the global maximum for ascending
        assert info.abscissa_left_value == 0.0
        assert info.abscissa_right_value == 3.0

    def test_abscissa_left_right_descending(self):
        # arrange — descending sweep (first value greater than last)
        info = StepInformation(keys=[], values=[], abscissa_indices=[slice(0, 4)], abscissa_value_ranges=[(10.0, 1.0)])
        # act / assert — descending: left is max, right is min
        assert info.abscissa_left_value == 10.0
        assert info.abscissa_right_value == 1.0

    def test_abscissa_ascending_flag(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_indices=[slice(0, 4)], abscissa_value_ranges=[(0.0, 3.0)])
        # act / assert
        assert info.abscissa_ascending is True

    def test_abscissa_descending_flag(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_indices=[slice(0, 4)], abscissa_value_ranges=[(3.0, 0.0)])
        # act / assert
        assert info.abscissa_ascending is False

    def test_step_abscissa_left_value(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_indices=[slice(0, 5), slice(5, 10)], abscissa_value_ranges=[(0.0, 4.0), (1.0, 5.0)])
        # act / assert — per-step left value
        assert info.step_abscissa_left_value(0) == 0.0
        assert info.step_abscissa_left_value(1) == 1.0

    def test_step_abscissa_right_value(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_indices=[slice(0, 5), slice(5, 10)], abscissa_value_ranges=[(0.0, 4.0), (1.0, 5.0)])
        # act / assert — per-step right value
        assert info.step_abscissa_right_value(0) == 4.0
        assert info.step_abscissa_right_value(1) == 5.0

    def test_empty_ranges_defaults_to_zero(self):
        # arrange
        info = StepInformation(keys=[], values=[], abscissa_indices=[], abscissa_value_ranges=[])
        # act / assert — no ranges: defaults to zero
        assert info.abscissa_left_value == 0.0
        assert info.abscissa_right_value == 0.0


class TestProcessScale:

    def test_linear_returns_original_expression(self):
        # arrange
        data = np.array([1.0, 10.0, 100.0])
        expr = Expression("frequency", data, "Hz", variable_type="frequency")
        # act
        result = _process_scale(expr, AbscissaScale.LINEAR)
        # assert — linear scale returns the original expression unchanged
        assert result is expr

    def test_decade_applies_log10(self):
        # arrange
        data = np.array([1.0, 10.0, 100.0])
        expr = Expression("frequency", data, "Hz", variable_type="frequency")
        # act
        result = _process_scale(expr, AbscissaScale.DECADE)
        # assert
        np.testing.assert_array_almost_equal(result.data, np.array([0.0, 1.0, 2.0]))

    def test_decade_preserves_name_and_unit(self):
        # arrange
        expr = Expression("frequency", np.array([100.0]), "Hz", variable_type="frequency")
        # act
        result = _process_scale(expr, AbscissaScale.DECADE)
        # assert
        assert result.name == "frequency"
        assert result.unit == "Hz"

    def test_octave_applies_log2(self):
        # arrange
        data = np.array([1.0, 2.0, 4.0, 8.0])
        expr = Expression("frequency", data, "Hz", variable_type="frequency")
        # act
        result = _process_scale(expr, AbscissaScale.OCTAVE)
        # assert
        np.testing.assert_array_almost_equal(result.data, np.array([0.0, 1.0, 2.0, 3.0]))

    def test_octave_preserves_metadata(self):
        # arrange
        expr = Expression("frequency", np.array([8.0]), "Hz", source="src", variable_type="frequency")
        # act
        result = _process_scale(expr, AbscissaScale.OCTAVE)
        # assert
        assert result.name == "frequency"
        assert result.unit == "Hz"
        assert result.source == "src"
        assert result.variable_type == "frequency"


class TestStepsHaveConsistentAbscissaDirection:

    def test_single_ascending_step_returns_true(self):
        # arrange
        data = np.array([0.0, 1.0, 2.0, 3.0])
        slices = [slice(0, 4)]
        # act
        result = _steps_have_consistent_abscissa_direction(data, slices)
        # assert
        assert result is True

    def test_single_descending_step_returns_true(self):
        # arrange
        data = np.array([3.0, 2.0, 1.0, 0.0])
        slices = [slice(0, 4)]
        # act
        result = _steps_have_consistent_abscissa_direction(data, slices)
        # assert
        assert result is True

    def test_two_ascending_steps_returns_true(self):
        # arrange — two ascending sweeps concatenated
        data = np.array([0.0, 1.0, 2.0, 0.0, 1.0, 2.0])
        slices = [slice(0, 3), slice(3, 6)]
        # act
        result = _steps_have_consistent_abscissa_direction(data, slices)
        # assert
        assert result is True

    def test_mixed_directions_returns_false(self):
        # arrange — first step ascending, second step descending
        data = np.array([0.0, 1.0, 2.0, 2.0, 1.0, 0.0])
        slices = [slice(0, 3), slice(3, 6)]
        # act
        result = _steps_have_consistent_abscissa_direction(data, slices)
        # assert
        assert result is False

    def test_non_monotonic_within_step_returns_false(self):
        # arrange — ascending then dip within a single step
        data = np.array([0.0, 1.0, 0.5, 2.0])
        slices = [slice(0, 4)]
        # act
        result = _steps_have_consistent_abscissa_direction(data, slices)
        # assert
        assert result is False

    def test_non_monotonic_within_descending_step_returns_false(self):
        # arrange — descending then bump within a single step
        data = np.array([4.0, 3.0, 3.5, 2.0])
        slices = [slice(0, 4)]
        # act
        result = _steps_have_consistent_abscissa_direction(data, slices)
        # assert
        assert result is False

    def test_all_flat_steps_treated_as_consistent(self):
        # arrange — flat (constant) data has no direction
        data = np.array([1.0, 1.0, 1.0, 1.0])
        slices = [slice(0, 4)]
        # act
        result = _steps_have_consistent_abscissa_direction(data, slices)
        # assert
        assert result is True


class TestProcessSteps:

    def test_not_stepped_returns_single_step(self):
        # arrange
        time_data = np.array([0.0, 1.0, 2.0, 3.0])
        abscissa = Expression("time", time_data, "s", variable_type="time")
        expressions = [abscissa]
        # act
        info = _process_steps(False, expressions, abscissa, len(time_data))
        # assert
        assert info.length == 1
        assert info.abscissa_indices == [slice(0, 4)]
        assert info.abscissa_left_value == 0.0
        assert info.abscissa_right_value == 3.0

    def test_not_stepped_empty_keys_and_values(self):
        # arrange
        abscissa = Expression("time", np.array([0.0, 1.0]), "s", variable_type="time")
        # act
        info = _process_steps(False, [abscissa], abscissa, 2)
        # assert
        assert info.keys == []
        assert info.values == []

    def test_stepped_no_parameters_infers_from_abscissa_resets(self):
        # arrange — two identical ascending sweeps, step boundary detected at reset
        time_data = np.array([0.0, 1.0, 2.0, 0.0, 1.0, 2.0])
        abscissa = Expression("time", time_data, "s", variable_type="time")
        # act
        info = _process_steps(True, [abscissa], abscissa, len(time_data))
        # assert — two steps inferred
        assert info.length == 2
        assert info.abscissa_indices[0] == slice(0, 3)
        assert info.abscissa_indices[1] == slice(3, 6)

    def test_stepped_no_parameters_descending_sweeps_detected(self):
        # arrange — two identical descending sweeps; step boundary at ascending reversal
        time_data = np.array([3.0, 2.0, 1.0, 3.0, 2.0, 1.0])
        abscissa = Expression("time", time_data, "s", variable_type="time")
        # act
        info = _process_steps(True, [abscissa], abscissa, len(time_data))
        # assert — two descending steps inferred
        assert info.length == 2
        assert info.abscissa_indices[0] == slice(0, 3)
        assert info.abscissa_indices[1] == slice(3, 6)

    def test_stepped_with_parameter_variable_detects_step_boundaries(self):
        # arrange — two steps: parameter changes from 1.0 to 2.0 at index 3
        time_data = np.array([0.0, 1.0, 2.0, 0.0, 1.0, 2.0])
        param_data = np.array([1.0, 1.0, 1.0, 2.0, 2.0, 2.0])
        abscissa = Expression("time", time_data, "s", variable_type="time")
        param = Expression("R1", param_data, "", variable_type="parameter")
        # act
        info = _process_steps(True, [abscissa, param], abscissa, len(time_data))
        # assert
        assert info.length == 2
        assert info.keys == ["R1"]
        assert info.values[0] == (1.0,)
        assert info.values[1] == (2.0,)

    def test_not_stepped_zero_points_returns_single_step(self):
        # arrange
        abscissa = Expression("time", np.array([]), "s", variable_type="time")
        # act
        info = _process_steps(False, [abscissa], abscissa, 0)
        # assert
        assert info.length == 1
        assert info.step_abscissa_left_value(0) == 0.0
        assert info.step_abscissa_right_value(0) == 0.0

    def test_stepped_inconsistent_direction_falls_back_to_single_step(self):
        # arrange — mixed direction prevents valid step detection
        time_data = np.array([0.0, 1.0, 2.0, 2.0, 1.0, 0.0])
        param_data = np.array([1.0, 1.0, 1.0, 2.0, 2.0, 2.0])
        abscissa = Expression("time", time_data, "s", variable_type="time")
        param = Expression("R1", param_data, "", variable_type="parameter")
        # act
        info = _process_steps(True, [abscissa, param], abscissa, len(time_data))
        # assert — fallback to single step
        assert info.length == 1

    def test_stepped_inferred_non_uniform_lengths_falls_back(self):
        # arrange — non-uniform step lengths for inferred steps: fallback expected
        time_data = np.array([0.0, 1.0, 2.0, 3.0, 0.0, 1.0])
        abscissa = Expression("time", time_data, "s", variable_type="time")
        # act
        info = _process_steps(True, [abscissa], abscissa, len(time_data))
        # assert — fallback to single step because step lengths differ
        assert info.length == 1

    def test_stepped_no_parameters_no_resets_treated_as_single_step(self):
        # arrange — strictly ascending time with stepped flag but no resets; no step boundaries detected
        time_data = np.array([0.0, 1.0, 2.0, 3.0])
        abscissa = Expression("time", time_data, "s", variable_type="time")
        # act
        info = _process_steps(True, [abscissa], abscissa, len(time_data))
        # assert — no resets found: treated as a single step
        assert info.length == 1
        assert info.abscissa_indices == [slice(0, 4)]


class TestParseBinaryVariables:

    def test_real_binary_correct_values(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0, 2.0], [1e-9, 1.1, 2.1]], dtype="<f8")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE), (2, "I(R1)", VariableType.CURRENT)]
        # act
        variables = _parse_binary_variables(raw_bytes, 0, variable_defs, False, 3, 2)
        # assert
        assert variables is not None
        assert len(variables) == 3
        np.testing.assert_array_almost_equal(variables[0].data, [0.0, 1e-9])
        np.testing.assert_array_almost_equal(variables[1].data, [1.0, 1.1])
        np.testing.assert_array_almost_equal(variables[2].data, [2.0, 2.1])

    def test_real_binary_variable_names_and_units(self):
        # arrange
        data_matrix = np.zeros((1, 2), dtype="<f8")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(out)", VariableType.VOLTAGE)]
        # act
        variables = _parse_binary_variables(raw_bytes, 0, variable_defs, False, 2, 1)
        # assert
        assert variables[0].name == "time"
        assert variables[0].unit == "s"
        assert variables[0].variable_type == "time"
        assert variables[1].name == "V(out)"
        assert variables[1].unit == "V"

    def test_real_binary_num_points_zero_infers_from_data(self):
        # arrange — three points, num_points passed as 0
        data_matrix = np.array([[0.0, 1.0], [1.0, 2.0], [2.0, 3.0]], dtype="<f8")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        variables = _parse_binary_variables(raw_bytes, 0, variable_defs, False, 2, 0)
        # assert — all three points recovered
        assert variables is not None
        assert len(variables[0].data) == 3

    def test_real_binary_with_trailing_content_stops_at_num_points(self):
        # arrange — write two data points then append junk bytes
        data_matrix = np.array([[0.0, 1.0], [1.0, 2.0]], dtype="<f8")
        junk = b"\xff" * 64
        raw_bytes = data_matrix.tobytes() + junk
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act — num_points=2 prevents reading into the junk bytes
        variables = _parse_binary_variables(raw_bytes, 0, variable_defs, False, 2, 2)
        # assert — exactly two points
        assert variables is not None
        assert len(variables[0].data) == 2

    def test_complex_binary_abscissa_is_real(self):
        # arrange — frequency variable stored as complex128; real part is the frequency
        data_matrix = np.array([[1000.0 + 0j, 0.5 + 0.5j], [10000.0 + 0j, 0.7 + 0.3j]], dtype="<c16")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "frequency", VariableType.FREQUENCY), (1, "V(out)", VariableType.VOLTAGE)]
        # act
        variables = _parse_binary_variables(raw_bytes, 0, variable_defs, True, 2, 2)
        # assert — abscissa (index 0) is real, V(out) is complex
        assert variables is not None
        assert variables[0].data.dtype == np.float64
        np.testing.assert_array_almost_equal(variables[0].data, [1000.0, 10000.0])
        assert variables[1].data.dtype == np.complex128

    def test_complex_binary_num_points_zero_infers_from_data(self):
        # arrange
        data_matrix = np.array([[1e3 + 0j, 1.0 + 0.5j], [1e4 + 0j, 0.9 + 0.4j], [1e5 + 0j, 0.7 + 0.3j]], dtype="<c16")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "frequency", VariableType.FREQUENCY), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        variables = _parse_binary_variables(raw_bytes, 0, variable_defs, True, 2, 0)
        # assert — three points inferred
        assert variables is not None
        assert len(variables[0].data) == 3

    def test_unknown_variable_type_uses_empty_unit(self):
        # arrange — variable definition with None type (unknown)
        data_matrix = np.array([[0.0, 5.0]], dtype="<f8")
        raw_bytes = data_matrix.tobytes()
        variable_defs = [(0, "time", VariableType.TIME), (1, "CUSTOM_VAR", None)]
        # act
        variables = _parse_binary_variables(raw_bytes, 0, variable_defs, False, 2, 1)
        # assert — unknown type variable included with empty unit and no variable_type
        assert variables is not None
        assert len(variables) == 2
        assert variables[1].unit == ""
        assert variables[1].variable_type is None

    def test_real_binary_with_data_offset(self):
        # arrange — header bytes prepended; offset points past them
        header = b"Header bytes here\n"
        data_matrix = np.array([[1.0, 2.0]], dtype="<f8")
        raw_bytes = header + data_matrix.tobytes()
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        variables = _parse_binary_variables(raw_bytes, len(header), variable_defs, False, 2, 1)
        # assert
        assert variables is not None
        np.testing.assert_array_almost_equal(variables[0].data, [1.0])
        np.testing.assert_array_almost_equal(variables[1].data, [2.0])


class TestParseAsciiVariables:

    def test_real_ascii_correct_values(self):
        # arrange — space-separated ascii with leading index
        text = " 0  0.000000e+00  1.000000e+00\n 1  1.000000e-09  1.100000e+00\n"
        raw_bytes = text.encode("utf-8")
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 2)
        # assert
        assert variables is not None
        assert len(variables) == 2
        np.testing.assert_array_almost_equal(variables[0].data, [0.0, 1e-9])
        np.testing.assert_array_almost_equal(variables[1].data, [1.0, 1.1])

    def test_real_ascii_without_index_tokens(self):
        # arrange — values only, no leading index
        text = " 0.000000e+00  2.000000e+00\n 1.000000e-09  3.000000e+00\n"
        raw_bytes = text.encode("utf-8")
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 2)
        # assert
        assert variables is not None
        np.testing.assert_array_almost_equal(variables[0].data, [0.0, 1e-9])
        np.testing.assert_array_almost_equal(variables[1].data, [2.0, 3.0])

    def test_real_ascii_variable_names_and_units(self):
        # arrange
        text = " 0  0.0  1.0\n 1  1.0  2.0\n"
        raw_bytes = text.encode("utf-8")
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(out)", VariableType.VOLTAGE)]
        # act
        variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 2)
        # assert
        assert variables[0].name == "time"
        assert variables[0].unit == "s"
        assert variables[1].name == "V(out)"
        assert variables[1].unit == "V"

    def test_complex_ascii_abscissa_is_real(self):
        # arrange — interleaved real/imag pairs: freq, then V(out) real+imag
        text = " 0  1.000000e+03  0.000000e+00  5.000000e-01  5.000000e-01\n"
        raw_bytes = text.encode("utf-8")
        variable_defs = [(0, "frequency", VariableType.FREQUENCY), (1, "V(out)", VariableType.VOLTAGE)]
        # act
        variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, True, 2, 1)
        # assert — frequency abscissa is real
        assert variables is not None
        assert variables[0].data.dtype == np.float64
        np.testing.assert_almost_equal(variables[0].data[0], 1e3)
        assert variables[1].data.dtype == np.complex128
        np.testing.assert_almost_equal(variables[1].data[0], 0.5 + 0.5j)

    def test_real_ascii_num_points_zero_reads_all(self):
        # arrange — num_points=0 means read all available lines
        text = " 0  0.0  1.0\n 1  1.0  2.0\n 2  2.0  3.0\n"
        raw_bytes = text.encode("utf-8")
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 0)
        # assert — all three points read
        assert variables is not None
        assert len(variables[0].data) == 3

    def test_real_ascii_stops_at_num_points(self):
        # arrange — four data lines but only two requested
        text = " 0  0.0  1.0\n 1  1.0  2.0\n 2  2.0  3.0\n 3  3.0  4.0\n"
        raw_bytes = text.encode("utf-8")
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 2)
        # assert — only first two points
        assert variables is not None
        assert len(variables[0].data) == 2

    def test_real_ascii_with_data_offset(self):
        # arrange — header bytes prepended; offset points past them
        header = b"Values:\n"
        text = " 0  5.0  6.0\n 1  7.0  8.0\n"
        raw_bytes = header + text.encode("utf-8")
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        variables = _parse_ascii_variables(raw_bytes, len(header), variable_defs, False, 2, 2)
        # assert
        assert variables is not None
        np.testing.assert_array_almost_equal(variables[0].data, [5.0, 7.0])

    def test_empty_text_returns_none(self):
        # arrange
        raw_bytes = b"\n\n\n"
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 2)
        # assert — no parseable data
        assert variables is None

    def test_line_with_no_tokens_is_skipped(self):
        # arrange — blank line then valid data line
        text = "\n 0  1.0  2.0\n"
        raw_bytes = text.encode("utf-8")
        variable_defs = [(0, "time", VariableType.TIME), (1, "V(1)", VariableType.VOLTAGE)]
        # act
        variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 1)
        # assert — blank line caused IndexError on tokens[0]; gracefully skipped
        assert variables is not None
        np.testing.assert_almost_equal(variables[0].data[0], 1.0)

    def test_unknown_variable_type_accepted(self):
        # arrange
        text = " 0  0.0  9.9\n"
        raw_bytes = text.encode("utf-8")
        variable_defs = [(0, "time", VariableType.TIME), (1, "MYSTERY", None)]
        # act
        variables = _parse_ascii_variables(raw_bytes, 0, variable_defs, False, 2, 1)
        # assert — variable with None type is still included
        assert variables is not None
        assert len(variables) == 2
        assert variables[1].variable_type is None
        assert variables[1].unit == ""


class TestXyceRawFile:

    def test_load_returns_none_when_file_not_found(self):
        # arrange
        path = "/tmp/nonexistent_xyce_raw_file_abc123.raw"
        # act
        result = XyceRawFile.load(path)
        # assert
        assert result is None

    def test_load_real_binary_title(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1], [2e-9, 1.2]], dtype=np.float64)
        content = _make_raw_bytes(title="RC Circuit", data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = XyceRawFile.load(path)
        # assert
        assert raw is not None
        assert raw.title == "RC Circuit"
        # cleanup
        os.unlink(path)

    def test_load_real_binary_date(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(date="Tue Feb  6 12:00:00 2024", data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = XyceRawFile.load(path)
        # assert
        assert raw is not None
        assert raw.date == "Tue Feb  6 12:00:00 2024"
        # cleanup
        os.unlink(path)

    def test_load_real_binary_plotname(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(plotname="Transient Analysis", data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = XyceRawFile.load(path)
        # assert
        assert raw is not None
        assert raw.plotname == "Transient Analysis"
        # cleanup
        os.unlink(path)

    def test_load_real_binary_filename(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
        # assert
        assert raw is not None
        assert raw.chart_type == "DC"
        # cleanup
        os.unlink(path)

    def test_load_command_from_command_header(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        # insert Command: line into header
        content = content.replace(b"Title:", b"Command: Xyce Release 7.9.0\nTitle:", 1)
        path = _write_temp_raw(content)
        # act
        raw = XyceRawFile.load(path)
        # assert
        assert raw is not None
        assert "Xyce" in raw.command
        # cleanup
        os.unlink(path)

    def test_load_command_falls_back_to_version_header(self):
        # arrange — Version: line present, no Command: line
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        content = content.replace(b"Title:", b"Version: Xyce Release 7.9.0\nTitle:", 1)
        path = _write_temp_raw(content)
        # act
        raw = XyceRawFile.load(path)
        # assert
        assert raw is not None
        assert "Xyce" in raw.command
        # cleanup
        os.unlink(path)

    def test_load_command_empty_when_no_command_or_version(self):
        # arrange
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = XyceRawFile.load(path)
        # assert
        assert raw is not None
        assert raw.command == ""
        # cleanup
        os.unlink(path)

    def test_load_binary_with_trailing_content_ignored(self):
        # arrange — extra junk bytes after valid binary data
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1], [2e-9, 1.2]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        content = content + b"\nSome extra CSV junk\n1,2,3\n4,5,6\n"
        path = _write_temp_raw(content)
        # act
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        result = XyceRawFile.load(path)
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
        result = XyceRawFile.load(path)
        # assert — file still loads; malformed line was silently skipped
        assert result is not None
        # cleanup
        os.unlink(path)

    def test_load_returns_none_when_ascii_parse_produces_no_data(self):
        # arrange — Values: section has only non-numeric content; parse returns None
        content = b"Title: Test\nDate: Mon Jan 1 00:00:00 2024\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 0\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\nValues:\nNO_NUMERIC_DATA\n"
        path = _write_temp_raw(content)
        # act
        result = XyceRawFile.load(path)
        # assert — ascii parse returned None; load returns None
        assert result is None
        # cleanup
        os.unlink(path)

    def test_load_returns_none_when_file_has_no_trailing_newline(self):
        # arrange — header truncated with no newline at end; no data section reached
        content = b"Title: Test\nDate: Mon Jan 1 00:00:00 2024"
        path = _write_temp_raw(content)
        # act
        result = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
        # assert — file loads successfully; unknown-typed variable is accessible
        assert raw is not None
        em = raw.expression_manager
        custom = em.evaluate("CUSTOM_SIG")
        assert custom is not None
        # cleanup
        os.unlink(path)

    def test_load_stepped_two_steps_detected(self):
        # arrange — two identical ascending sweeps; stepped flag triggers step detection
        time_data = np.tile(np.array([0.0, 1e-9, 2e-9, 3e-9]), 2)
        v_data = np.tile(np.array([1.0, 1.1, 1.2, 1.3]), 2)
        data_matrix = np.column_stack([time_data, v_data])
        content = _make_raw_bytes(flags="real stepped", data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = XyceRawFile.load(path)
        # assert — two steps inferred from abscissa resets
        assert raw is not None
        assert raw.steps == 2
        # cleanup
        os.unlink(path)

    def test_load_step_information_abscissa_range(self):
        # arrange — single non-stepped analysis; check step info range
        data_matrix = np.array([[0.0, 1.0], [1e-9, 1.1], [2e-9, 1.2]], dtype=np.float64)
        content = _make_raw_bytes(data_matrix=data_matrix)
        path = _write_temp_raw(content)
        # act
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
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
        raw = XyceRawFile.load(path)
        # assert — all three points read
        assert raw is not None
        assert len(raw.abscissa.data) == 3
        # cleanup
        os.unlink(path)
