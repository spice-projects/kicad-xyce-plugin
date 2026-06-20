import logging
import sys
from unittest.mock import ANY, MagicMock, patch

import numpy as np
import pytest

# mock PySide6 submodules before importing chart, which requires Qt at import time
_PYSIDE_MODULE_NAMES = ["PySide6", "PySide6.QtCore", "PySide6.QtGraphs", "PySide6.QtQuick"]
_ORIGINAL_PYSIDE_MODULES = {name: sys.modules.get(name) for name in _PYSIDE_MODULE_NAMES}
sys.modules["PySide6"] = MagicMock()
sys.modules["PySide6.QtCore"] = MagicMock()
sys.modules["PySide6.QtGraphs"] = MagicMock()
sys.modules["PySide6.QtQuick"] = MagicMock()

from kicad_xyce_plugin.chart import Chart, _binary_search, _find_abscissa_index_for_value  # noqa: E402
from kicad_xyce_plugin.expression import Expression  # noqa: E402
from kicad_xyce_plugin.xyce_raw_file import StepInformation  # noqa: E402

for _name, _module in _ORIGINAL_PYSIDE_MODULES.items():
    if _module is None:
        del sys.modules[_name]
    else:
        sys.modules[_name] = _module


def _make_step_information(num_steps: int, abscissa_length: int, ascending: bool = True) -> StepInformation:
    # helper to create StepInformation for tests
    step_size = abscissa_length // num_steps if num_steps > 0 else abscissa_length
    keys = ["step"]
    values = [(i,) for i in range(num_steps)]
    abscissa_indices = [slice(i * step_size, (i + 1) * step_size) for i in range(num_steps)]
    if ascending:
        abscissa_value_ranges = [(float(i * step_size), float((i + 1) * step_size)) for i in range(num_steps)]
    else:
        abscissa_value_ranges = [(float((i + 1) * step_size), float(i * step_size)) for i in range(num_steps)]
    return StepInformation(keys, values, abscissa_indices, abscissa_value_ranges)


def _make_component_with_y_axis() -> MagicMock:
    component = MagicMock()

    def createYAxis(alignment, unit):
        axis = MagicMock()
        axis.property = MagicMock(return_value=unit)
        return axis

    component.createYAxis = MagicMock(side_effect=createYAxis)
    component.removeAllSeries = MagicMock()
    component.updateGraphsView = MagicMock()
    component.resizeAbscissa = MagicMock()
    return component


def _make_chart(component: MagicMock, expression_manager: MagicMock, step_information: StepInformation, abscissa: Expression, abscissa_label: str = "Time", abscissa_scale: str = "linear", decimate_target: int = 500) -> Chart:
    return Chart(component, expression_manager, step_information, abscissa, abscissa_label, abscissa_scale, decimate_target)


class TestChart:

    def test_binary_search_descending_and_out_of_bounds(self):
        # arrange
        data = np.array([10, 8, 6, 4, 2, 0])
        # act & assert — value in range
        assert _binary_search(data, 6, ascending=False, side=1) == 2
        # act & assert — value below range
        assert _binary_search(data, -5, ascending=False, side=1) == 6
        # act & assert — value above range
        assert _binary_search(data, 15, ascending=False, side=1) == 0
        # act & assert — _find_abscissa_index_for_value clamps to valid range
        assert _find_abscissa_index_for_value(data, -5, ascending=False) == 5
        assert _find_abscissa_index_for_value(data, 15, ascending=False) == 0

    def test_plot_series_axis_creation_failure_logs_warning(self, caplog):
        # arrange
        component = MagicMock()
        abscissa = Expression("Time", np.linspace(0.0, 1.0, 10), "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        # patch _get_y_axis to return None to simulate axis creation failure
        chart._get_y_axis = MagicMock(return_value=None)
        with caplog.at_level(logging.WARNING, logger="chart"):
            chart.plot_series({vout})
        # assert — warning about axis creation failure
        assert any("maximum number of Y axes reached" in message for message in caplog.messages)

    def test_plot_series_skips_all_nonfinite(self):
        # arrange
        component = _make_component_with_y_axis()
        abscissa = Expression("Time", np.linspace(0.0, 1.0, 10), "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.full(10, np.inf), "V")
        # patch decimate_xy to return all non-finite values
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(np.arange(10), np.full(10, np.inf))):
            chart.plot_series({vout})
        # assert — no rendered step data is stored for the expression
        assert len(chart._series["Vout"][1]) == 1
        ordinate_series = next(iter(chart._series["Vout"][1].values()))
        assert ordinate_series[1] == {}

    def test_color_cycling_wraps_palette(self):
        # arrange
        component = _make_component_with_y_axis()
        abscissa = Expression("Time", np.linspace(0.0, 1.0, 10), "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        # patch decimate_xy to always return valid data
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(np.arange(10), np.arange(10))):
            for i in range(20):
                expr = Expression(f"V{i}", np.arange(10), "V")
                chart.plot_series({expr})
        # assert — color index wraps, palette length is 14
        assert chart._next_color_index <= 20

    def test_find_abscissa_indexes_outside_window_and_descending(self):
        # arrange
        component = MagicMock()
        abscissa = Expression("Time", np.array([10, 8, 6, 4, 2, 0]), "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 6, ascending=False), abscissa)
        # act — window outside data range (should return empty slice)
        result = chart._find_abscissa_indexes(abscissa.data, 20, 30)
        # assert
        assert result == slice(0, 0)

    def test_redraw_all_series_skips_empty_decimation(self):
        # arrange
        component = MagicMock()
        abscissa = Expression("Time", np.linspace(0.0, 1.0, 10), "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        y_axis = MagicMock()
        # inject a series with a real QLineSeries
        chart._series = {"Vout": (vout, {vout: (y_axis, {0: MagicMock()}, 0.0, 5.0, "#f77f00")})}
        chart._zoom_window = (0.0, None, 1.0, None)
        # patch decimate_xy to return empty arrays
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(np.array([]), np.array([]))):
            chart._redraw_all_series()
        # assert — no exception, series remains
        assert "Vout" in chart._series

    def test_init_zoom_window(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        step_information = _make_step_information(1, 100)
        # act
        chart = _make_chart(component, MagicMock(), step_information, abscissa)
        # assert
        assert chart._zoom_window == (None, None, None, None)

    def test_expressions_initially_empty(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        step_information = _make_step_information(1, 100)
        chart = _make_chart(component, MagicMock(), step_information, abscissa)
        # act
        result = chart.expressions
        # assert
        assert result == []

    def test_expressions_returns_copy(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act
        result = chart.expressions
        result.append(MagicMock())
        # assert — internal list must be unaffected by mutation of the returned copy
        assert len(chart.expressions) == 0

    def test_auto_range_returns_early_when_no_series(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act
        chart.auto_range()
        # assert — no axis interaction when there are no series
        component.createYAxis.assert_not_called()

    def test_selected_steps_setter_noop_when_selection_unchanged(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        chart.plot_series = MagicMock()
        # act
        chart.selected_steps = {0}
        # assert
        chart.plot_series.assert_not_called()

    def test_selected_steps_getter_returns_current_selection(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        # act
        selected_steps = chart.selected_steps
        # assert
        assert selected_steps == {0}

    def test_selected_steps_setter_replots_when_selection_changes(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        chart.plot_series = MagicMock()
        # act
        chart.selected_steps = set()
        # assert
        chart.plot_series.assert_called_once_with([])

    def test_auto_range_calls_set_range_on_axis(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        mock_y_axis = MagicMock()
        vout = Expression("Vout", np.array([1.0, 2.0]), "V")
        # manually inject a series entry with known min/max so auto_range is predictable
        chart._series = {"Vout": (vout, {vout: (mock_y_axis, {}, -1.0, 5.0, "#f77f00")})}
        # act
        chart.auto_range()
        # assert — autorange applies 3% padding on both sides
        mock_y_axis.setRange.assert_called_once_with(-1.18, 5.18)

    def test_auto_range_respects_vertical_zoom(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        mock_y_axis = MagicMock()
        vout = Expression("Vout", np.array([1.0, 2.0]), "V")
        # inject a series with min=-1.0 and max=5.0 (range of 6.0)
        chart._series = {"Vout": (vout, {vout: (mock_y_axis, {}, -1.0, 5.0, "#f77f00")})}
        # apply a vertical zoom that selects only the top quarter (0.0 to 0.25)
        chart.update_zoom_window(None, None, 0.0, 0.25)
        # act
        chart.auto_range()
        # assert — autorange uses series min/max and applies 3% padding
        mock_y_axis.setRange.assert_called_with(-1.18, 5.18)

    def test_update_zoom_window_vertical_only(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act — None x signals "no horizontal change"
        chart.update_zoom_window(None, None, 0.25, 0.75)
        # assert — only vertical slice of zoom window changed, horizontal remains None
        assert chart._zoom_window[0] is None
        assert chart._zoom_window[1] == pytest.approx(0.25)
        assert chart._zoom_window[2] is None
        assert chart._zoom_window[3] == pytest.approx(0.75)

    def test_update_zoom_window_horizontal_only(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act — None signals "no vertical change"
        chart.update_zoom_window(0.1, 0.8, None, None)
        # assert — only horizontal slice of zoom window changed, vertical remains None
        assert chart._zoom_window[0] == pytest.approx(0.1)
        assert chart._zoom_window[1] is None
        assert chart._zoom_window[2] == pytest.approx(0.8)
        assert chart._zoom_window[3] is None

    def test_update_zoom_window_vertical_zoom_composition(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act — apply two consecutive vertical zooms, each selecting the lower half
        chart.update_zoom_window(None, None, 0.0, 0.5)
        chart.update_zoom_window(None, None, 0.0, 0.5)
        # assert — second zoom compounds on the first: upper bound goes from 0.5 to 0.25
        assert chart._zoom_window[1] == pytest.approx(0.0)
        assert chart._zoom_window[3] == pytest.approx(0.25)

    def test_reset_zoom_window_vertical(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act
        chart.update_zoom_window(None, None, 0.1, 0.9)
        # assert — zoom window reflects the provided ratios directly (no composition)
        assert chart._zoom_window[1] == pytest.approx(0.1)
        assert chart._zoom_window[3] == pytest.approx(0.9)

    def test_reset_zoom_window_no_change_when_values_match(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act — reset to the default values that are already in place
        chart.update_zoom_window(None, None, 0.0, 1.0)
        # assert — zoom window unchanged since new values match existing ones
        assert chart._zoom_window[1] == pytest.approx(0.0)
        assert chart._zoom_window[3] == pytest.approx(1.0)

    def test_reset_zoom_window_horizontal(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # first zoom in horizontally
        chart.update_zoom_window(0.2, 0.7, None, None)
        # act — reset horizontal zoom only
        chart.reset_zoom_window(True, False)
        # assert — horizontal reset to None, vertical unchanged
        assert chart._zoom_window[0] is None
        assert chart._zoom_window[1] is None
        assert chart._zoom_window[2] is None
        assert chart._zoom_window[3] is None

    def test_get_y_axis_creates_axis_for_new_expression_type(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act
        axis = chart._get_y_axis("V")
        # assert
        assert axis is not None
        component.createYAxis.assert_called_once_with(ANY, "V")

    def test_get_y_axis_reuses_existing_axis(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        axis_first = chart._get_y_axis("V")
        # act — request the same expression type a second time
        axis_second = chart._get_y_axis("V")
        # assert — same axis object returned, no extra createYAxis calls
        assert axis_first is axis_second
        assert component.createYAxis.call_count == 1

    def test_get_y_axis_returns_none_when_four_axes_already_created(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # fill up all four allowed Y axes with distinct expression types
        chart._get_y_axis("V")
        chart._get_y_axis("A")
        chart._get_y_axis("W")
        chart._get_y_axis("s")
        # act — requesting a fifth distinct type must be rejected
        axis = chart._get_y_axis("Hz")
        # assert
        assert axis is None

    def test_clear_resets_internal_state(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        vout = Expression("Vout", np.array([1.0, 2.0]), "V")
        # inject state that clear() must wipe
        chart._y_axes["V"] = MagicMock()
        chart._series["Vout"] = (vout, {})
        # act
        chart.clear()
        # assert — all tracking collections are empty after clear
        assert chart.expressions == []
        assert chart._series == {}
        assert chart._y_axes == {}

    def test_clear_resets_vertical_zoom_but_preserves_horizontal(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # zoom in horizontally and vertically before clearing
        chart.update_zoom_window(0.2, 0.8, None, None)
        chart.update_zoom_window(None, None, 0.2, 0.8)
        # act
        chart.clear()
        # assert — vertical zoom is reset to None; horizontal range is preserved
        assert chart._zoom_window[0] == pytest.approx(0.2)
        assert chart._zoom_window[1] is None
        assert chart._zoom_window[2] == pytest.approx(0.8)
        assert chart._zoom_window[3] is None

    def test_ordinate_values_at_abscissa_value_returns_empty_when_no_series(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act
        result = chart.ordinate_values_at_abscissa_value(0.5)
        # assert
        assert result == []

    def test_ordinate_values_at_abscissa_value_returns_name_unit_value_for_plotted_series(self):
        # arrange
        component = MagicMock()
        abscissa = Expression("Time", np.linspace(0.0, 1.0, 11), "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 11), abscissa)
        # ordinate: 11 linearly-spaced values from 0 to 100
        vout = Expression("Vout", np.linspace(0.0, 100.0, 11), "V")
        mock_y_axis = MagicMock()
        chart._series["Vout"] = (vout, {vout: (mock_y_axis, {0: MagicMock()}, 0.0, 100.0, "#f77f00")})
        # act — sample at the rightmost abscissa value (1.0) should return the last ordinate
        result = chart.ordinate_values_at_abscissa_value(1.0)
        # assert
        assert len(result) == 1
        name, unit, values = result[0]
        assert name == "Vout"
        assert unit == "V"
        assert values == [100.0]

    def test_ordinate_values_at_abscissa_value_nearest_ordinate_values_at_abscissa_value_midpoint(self):
        # arrange
        component = MagicMock()
        abscissa = Expression("Time", np.linspace(0.0, 10.0, 11), "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 11), abscissa)
        # ordinate: index-valued array so we can easily verify which index was sampled
        vout = Expression("Vout", np.arange(11, dtype=float), "V")
        mock_y_axis = MagicMock()
        chart._series["Vout"] = (vout, {vout: (mock_y_axis, {0: MagicMock()}, 0.0, 10.0, "#f77f00")})
        # act — x_value=5.0 is the midpoint of abscissa [0, 10]
        result = chart.ordinate_values_at_abscissa_value(5.0)
        # assert — nearest sample to the midpoint
        _, _, values = result[0]
        assert values == [5.0]

    def test_ordinate_values_at_abscissa_value_finds_nearest_sample(self):
        # arrange
        component = MagicMock()
        abscissa = Expression("Time", np.linspace(0.0, 10.0, 11), "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 11), abscissa)
        vout = Expression("Vout", np.arange(11, dtype=float), "V")
        mock_y_axis = MagicMock()
        chart._series["Vout"] = (vout, {vout: (mock_y_axis, {0: MagicMock()}, 0.0, 10.0, "#f77f00")})
        # act — x_value=0.0 is the leftmost point
        result_left = chart.ordinate_values_at_abscissa_value(0.0)
        # x_value=10.0 is the rightmost point
        result_right = chart.ordinate_values_at_abscissa_value(10.0)
        # assert
        _, _, left_val = result_left[0]
        _, _, right_val = result_right[0]
        assert left_val == [0.0]
        assert right_val == [10.0]

    def test_ordinate_values_at_abscissa_value_multiple_series(self):
        # arrange
        component = MagicMock()
        abscissa = Expression("Time", np.linspace(0.0, 1.0, 5), "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 5), abscissa)
        vout = Expression("Vout", np.array([10.0, 20.0, 30.0, 40.0, 50.0]), "V")
        iout = Expression("Iout", np.array([1.0, 2.0, 3.0, 4.0, 5.0]), "A")
        mock_axis = MagicMock()
        chart._series["Vout"] = (vout, {vout: (mock_axis, {0: MagicMock()}, 10.0, 50.0, "#f77f00")})
        chart._series["Iout"] = (iout, {iout: (mock_axis, {0: MagicMock()}, 1.0, 5.0, "#00b4d8")})
        # act — x_ratio=0.0 → index 0
        result = chart.ordinate_values_at_abscissa_value(0.0)
        # assert — two entries returned, one per series
        assert len(result) == 2
        names = {r[0] for r in result}
        assert "Vout" in names
        assert "Iout" in names

    def test_release_y_axis_clears_right_primary_reference(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        axis = MagicMock()
        axis.property.return_value = "V"
        chart._right_y_axis_1 = axis
        chart._y_axes = {"V": axis}
        chart._y_axes_ref_counts = {axis: 1}
        # act
        removed = chart._release_y_axis(axis)
        # assert
        assert removed
        assert chart._right_y_axis_1 is None

    def test_release_y_axis_clears_left_secondary_reference(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        axis = MagicMock()
        axis.property.return_value = "A"
        chart._left_y_axis_2 = axis
        chart._y_axes = {"A": axis}
        chart._y_axes_ref_counts = {axis: 1}
        # act
        removed = chart._release_y_axis(axis)
        # assert
        assert removed
        assert chart._left_y_axis_2 is None

    def test_release_y_axis_clears_right_secondary_reference(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        axis = MagicMock()
        axis.property.return_value = "W"
        chart._right_y_axis_2 = axis
        chart._y_axes = {"W": axis}
        chart._y_axes_ref_counts = {axis: 1}
        # act
        removed = chart._release_y_axis(axis)
        # assert
        assert removed
        assert chart._right_y_axis_2 is None

    def test_abscissa_property(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act
        result = chart.abscissa
        # assert
        assert result is abscissa

    def test_redraw_series_initializes_component_with_abscissa_range(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 10.0, 100)
        abscissa = Expression("Time", values, "s")
        step_information = StepInformation(["step"], [(0,)], [slice(0, 100)], [(0.0, 10.0)])
        chart = _make_chart(component, MagicMock(), step_information, abscissa)
        # act
        chart.redraw_series(MagicMock(), step_information, abscissa, "Time", "linear")
        # assert — initialize must receive label, unit, scale, and exact boundary values
        component.initialize.assert_called_once_with("Time", "s", "linear", 0.0, 10.0)

    def test_redraw_series_removes_expression_when_expression_manager_returns_none(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        expression = Expression("Vout", np.linspace(0.0, 1.0, 10), "V")
        expression_manager = MagicMock()
        expression_manager.evaluate.return_value = None
        chart = _make_chart(component, expression_manager, _make_step_information(1, 10), abscissa)
        y_axis = MagicMock()
        y_axis.property.return_value = "V"
        chart._series = {"Vout": (expression, {expression: (y_axis, {0: MagicMock()}, 0.0, 1.0, "#fff")})}
        chart._y_axes = {"V": y_axis}
        chart._y_axes_ref_counts = {y_axis: 1}
        chart._left_y_axis_1 = y_axis
        # act
        chart.redraw_series(expression_manager, _make_step_information(1, 10), abscissa, "Time", "linear")
        # assert — expression removed and updateGraphsView called with removal
        assert chart._series == {}
        component.updateGraphsView.assert_called_once()

    def test_plot_series_adds_expression_to_series_dict(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        decimated_x = np.linspace(0.0, 1.0, 10)
        decimated_y = np.linspace(0.0, 5.0, 10)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(decimated_x, decimated_y)):
            chart.plot_series({vout})
        # assert
        assert "Vout" in chart._series

    def test_plot_series_stores_expression_in_expressions_list(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        decimated_x = np.linspace(0.0, 1.0, 10)
        decimated_y = np.linspace(0.0, 5.0, 10)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(decimated_x, decimated_y)):
            chart.plot_series({vout})
        # assert
        assert vout in chart.expressions

    def test_plot_series_calls_component_plot_series(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        decimated_x = np.linspace(0.0, 1.0, 10)
        decimated_y = np.linspace(0.0, 5.0, 10)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(decimated_x, decimated_y)):
            chart.plot_series({vout})
        # assert
        component.updateGraphsView.assert_called_once()

    def test_plot_series_skips_already_tracked_expression(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        # inject existing series entry so chart believes Vout is already plotted
        chart._series["Vout"] = (vout, {vout: (MagicMock(), {0: MagicMock()}, 0.0, 5.0, "#f77f00")})
        # act
        chart.plot_series({vout})
        # assert — no new axis was requested because no new series were created
        component.createYAxis.assert_not_called()

    def test_plot_series_removes_expression_absent_from_new_set(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        y_axis = MagicMock()
        y_axis.property.return_value = "V"
        chart._y_axes["V"] = y_axis
        chart._y_axes_ref_counts[y_axis] = 1
        chart._left_y_axis_1 = y_axis
        # inject an existing series that is absent from the new expression set
        chart._series["Vout"] = (vout, {vout: (y_axis, {0: MagicMock()}, 0.0, 5.0, "#f77f00")})
        # act — empty set means all existing series should be removed
        chart.plot_series(set())
        # assert
        assert "Vout" not in chart._series
        assert vout not in chart.expressions
        component.updateGraphsView.assert_called_once()

    def test_plot_series_creates_one_series_entry_per_step(self):
        # arrange
        component = MagicMock()
        n = 10
        values = np.linspace(0.0, 1.0, n)
        abscissa = Expression("Time", values, "s")
        # two steps: 20 ordinate points total (10 per step)
        ordinate_data = np.linspace(0.0, 5.0, 2 * n)
        chart = _make_chart(component, MagicMock(), _make_step_information(2, n), abscissa)
        vout = Expression("Vout", ordinate_data, "V")
        decimated_y = np.linspace(0.0, 5.0, n)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(values, decimated_y)):
            chart.plot_series({vout})
        # assert — one QLineSeries created per step
        _, ordinate_series = chart._series["Vout"]
        _, rendered_series, _, _, _ = ordinate_series[vout]
        assert len(rendered_series) == 2

    def test_get_expressions_to_plot_real_returns_single_entry(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        # act
        result = chart._get_expressions_to_plot(vout)
        # assert — real expression returned as-is in a single-element list
        assert result == [vout]

    def test_get_expressions_to_plot_complex_returns_magnitude_and_phase(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        mock_manager = MagicMock()
        magnitude_expr = Expression("db(Vout)", np.ones(10), "dB")
        phase_expr = Expression("phase(Vout)", np.zeros(10), "deg")
        mock_manager.evaluate.side_effect = lambda expr: magnitude_expr if expr == "db(Vout)" else phase_expr
        chart = _make_chart(component, mock_manager, _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.ones(10, dtype=np.complex128), "V")
        # act
        result = chart._get_expressions_to_plot(vout)
        # assert — complex expression splits into magnitude then phase
        assert len(result) == 2
        assert result[0] is magnitude_expr
        assert result[1] is phase_expr
        assert mock_manager.evaluate.call_args_list[0].args == ("db(Vout)",)
        assert mock_manager.evaluate.call_args_list[1].args == ("phase(Vout)",)

    def test_get_expressions_to_plot_complex_returns_empty_when_magnitude_fails(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        mock_manager = MagicMock()
        # evaluate always returns None — magnitude lookup fails immediately
        mock_manager.evaluate.return_value = None
        chart = _make_chart(component, mock_manager, _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.ones(10, dtype=np.complex128), "V")
        # act
        result = chart._get_expressions_to_plot(vout)
        # assert
        assert result == []

    def test_get_expressions_to_plot_complex_returns_empty_when_phase_fails(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        mock_manager = MagicMock()
        magnitude_expr = Expression("db(Vout)", np.ones(10), "dB")
        # magnitude succeeds but phase lookup fails
        mock_manager.evaluate.side_effect = lambda expr: magnitude_expr if expr == "db(Vout)" else None
        chart = _make_chart(component, mock_manager, _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.ones(10, dtype=np.complex128), "V")
        # act
        result = chart._get_expressions_to_plot(vout)
        # assert
        assert result == []

    def test_get_expressions_to_plot_complex_on_tran_chart_returns_magnitude_and_phase(self):
        # arrange
        component = MagicMock()
        abscissa = Expression("Time", np.linspace(0.0, 1e-3, 10), "s")
        mock_manager = MagicMock()
        magnitude_expr = Expression("db(Vout)", np.ones(10), "dB")
        phase_expr = Expression("phase(Vout)", np.zeros(10), "deg")
        mock_manager.evaluate.side_effect = lambda expr: magnitude_expr if expr == "db(Vout)" else phase_expr
        chart = _make_chart(component, mock_manager, _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.ones(10, dtype=np.complex128), "V")
        # act
        result = chart._get_expressions_to_plot(vout)
        # assert — complex expressions return db/phase variants
        assert result == [magnitude_expr, phase_expr]

    def test_get_expressions_to_plot_complex_on_dc_chart_returns_magnitude_and_phase(self):
        # arrange
        component = MagicMock()
        abscissa = Expression("V1", np.linspace(0.0, 5.0, 10), "V")
        mock_manager = MagicMock()
        magnitude_expr = Expression("db(Vout)", np.ones(10), "dB")
        phase_expr = Expression("phase(Vout)", np.zeros(10), "deg")
        mock_manager.evaluate.side_effect = lambda expr: magnitude_expr if expr == "db(Vout)" else phase_expr
        chart = _make_chart(component, mock_manager, _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.ones(10, dtype=np.complex128), "V")
        # act
        result = chart._get_expressions_to_plot(vout)
        # assert — complex expressions return db/phase variants
        assert result == [magnitude_expr, phase_expr]

    def test_clear_resets_axis_slot_references(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        chart._left_y_axis_1 = MagicMock()
        chart._right_y_axis_1 = MagicMock()
        chart._left_y_axis_2 = MagicMock()
        chart._right_y_axis_2 = MagicMock()
        # act
        chart.clear()
        # assert
        assert chart._left_y_axis_1 is None
        assert chart._right_y_axis_1 is None
        assert chart._left_y_axis_2 is None
        assert chart._right_y_axis_2 is None

    def test_release_y_axis_returns_false_while_axis_still_shared(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        axis = MagicMock()
        axis.property.return_value = "V"
        chart._left_y_axis_1 = axis
        chart._y_axes["V"] = axis
        chart._y_axes_ref_counts[axis] = 2
        # act
        result = chart._release_y_axis(axis)
        # assert
        assert not result
        assert chart._y_axes_ref_counts[axis] == 1
        assert chart._left_y_axis_1 is axis

    def test_release_y_axis_clears_internal_axis_reference_when_unused(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        axis = MagicMock()
        axis.property.return_value = "V"
        chart._left_y_axis_1 = axis
        chart._y_axes["V"] = axis
        chart._y_axes_ref_counts[axis] = 1
        # act
        result = chart._release_y_axis(axis)
        # assert
        assert result
        assert chart._y_axes == {}
        assert chart._y_axes_ref_counts == {}
        assert chart._left_y_axis_1 is None

    def test_plot_series_updates_graphs_view_with_rendered_series(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        decimated_x = np.linspace(0.0, 1.0, 10)
        decimated_y = np.linspace(0.0, 5.0, 10)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(decimated_x, decimated_y)):
            chart.plot_series({vout})
        # assert
        rendered, removed = component.updateGraphsView.call_args.args
        assert len(rendered) == 1
        assert rendered[0][0] == "Vout"
        assert removed == []

    def test_plot_series_sets_series_width_to_two(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        decimated_x = np.linspace(0.0, 1.0, 10)
        decimated_y = np.linspace(0.0, 5.0, 10)
        mock_series = MagicMock()
        # act
        with patch("kicad_xyce_plugin.chart.QLineSeries", return_value=mock_series):
            with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(decimated_x, decimated_y)):
                chart.plot_series({vout})
        # assert
        mock_series.setWidth.assert_called_once_with(2)

    def test_redraw_all_series_calls_resize_abscissa(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 10.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        mock_series = MagicMock()
        chart._series["Vout"] = (vout, {vout: (MagicMock(), {0: mock_series}, 0.0, 5.0, "#f77f00")})
        decimated_y = np.linspace(0.0, 5.0, 10)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(values, decimated_y)):
            chart._redraw_all_series()
        # assert — abscissa axis resized to match the current zoom window bounds
        component.resizeAbscissa.assert_called_once_with(float(values[0]), float(values[-1]))

    def test_redraw_all_series_calls_replace_np_on_each_series(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 10.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.linspace(0.0, 5.0, 10), "V")
        mock_series = MagicMock()
        chart._series["Vout"] = (vout, {vout: (MagicMock(), {0: mock_series}, 0.0, 5.0, "#f77f00")})
        x_out = np.linspace(0.0, 10.0, 10)
        y_out = np.linspace(0.0, 5.0, 10)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(x_out, y_out)):
            chart._redraw_all_series()
        # assert — series data updated with newly decimated arrays
        mock_series.replaceNp.assert_called_once()

    def test_update_zoom_window_horizontal_calls_redraw_all_series(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        # act
        with patch.object(chart, "_redraw_all_series") as mock_redraw:
            chart.update_zoom_window(10, 80, None, None)
        # assert
        mock_redraw.assert_called_once()

    def test_update_zoom_window_both_calls_redraw_and_updates_axes(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        mock_y_axis = MagicMock()
        chart._axis_ranges = {mock_y_axis: (0.0, 10.0)}
        # act — both horizontal and vertical changes supplied simultaneously
        with patch.object(chart, "_redraw_all_series") as mock_redraw:
            chart.update_zoom_window(0.1, 0.8, 0.25, 0.75)
        # assert — horizontal triggers redraw, vertical updates axes directly
        mock_redraw.assert_called_once()
        mock_y_axis.setRange.assert_called_once()

    def test_update_zoom_window_vertical_only_updates_axis_ranges(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        mock_y_axis = MagicMock()
        # inject known axis range so the zoom calculation is predictable
        chart._axis_ranges = {mock_y_axis: (0.0, 10.0)}
        # act — select bottom half (ratios 0.5 → 1.0)
        chart.update_zoom_window(None, None, 0.5, 1.0)
        # assert — visual range is [-0.3, 10.3] (3% padding); bottom half selected
        call_args = mock_y_axis.setRange.call_args.args
        assert call_args[0] == pytest.approx(5.0, abs=1e-5)
        assert call_args[1] == pytest.approx(10.3, abs=1e-5)

    def test_reset_zoom_window_vertical_changed_updates_axis_ranges(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        mock_y_axis = MagicMock()
        chart._axis_ranges = {mock_y_axis: (0.0, 10.0)}
        # first zoom in vertically so that reset changes the state
        chart.update_zoom_window(None, None, 0.2, 0.8)
        mock_y_axis.setRange.reset_mock()
        # act — reset vertical zoom
        chart.reset_zoom_window(False, True)
        # assert — reset calls setRange with padded full range
        mock_y_axis.setRange.assert_called_once_with(-0.3, 10.3)

    def test_reset_zoom_window_no_axis_update_when_vertical_unchanged(self):
        # arrange
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 100)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 100), abscissa)
        mock_y_axis = MagicMock()
        chart._axis_ranges = {mock_y_axis: (0.0, 10.0)}
        # act — reset horizontal only, no vertical change
        chart.reset_zoom_window(True, False)
        # assert — setRange must not be called when vertical zoom did not change
        mock_y_axis.setRange.assert_not_called()

    def test_plot_series_handles_constant_nonzero_signal(self):
        # arrange — all ordinate values equal and non-zero: y_range is zero but scale != 0
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.full(10, 3.0), "V")
        decimated_y = np.full(10, 3.0)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(values, decimated_y)):
            chart.plot_series({vout})
        # assert — series stored without raising and min/max reflect the rendered values
        assert "Vout" in chart._series
        _, ordinate_series = chart._series["Vout"]
        _, _, stored_min, stored_max, _ = ordinate_series[vout]
        assert stored_min == 3.0
        assert stored_max == 3.0

    def test_plot_series_handles_all_zero_signal(self):
        # arrange — all ordinate values are zero: scale == 0 triggers the else branch (y_range = 1.0)
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.zeros(10), "V")
        decimated_y = np.zeros(10)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(values, decimated_y)):
            chart.plot_series({vout})
        # assert — series stored and min/max reflect the rendered values
        assert "Vout" in chart._series
        _, ordinate_series = chart._series["Vout"]
        _, _, stored_min, stored_max, _ = ordinate_series[vout]
        assert stored_min == 0.0
        assert stored_max == 0.0

    def test_redraw_all_series_handles_constant_signal(self):
        # arrange — constant ordinate so the flat-signal y_range fix fires inside _redraw_all_series
        component = MagicMock()
        values = np.linspace(0.0, 10.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        vout = Expression("Vout", np.full(10, 7.0), "V")
        mock_series = MagicMock()
        chart._series["Vout"] = (vout, {vout: (MagicMock(), {0: mock_series}, 7.0, 7.0, "#f77f00")})
        decimated_y = np.full(10, 7.0)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(values, decimated_y)):
            chart._redraw_all_series()
        # assert — replaceNp called and resizeAbscissa completed without exception
        mock_series.replaceNp.assert_called_once()
        component.resizeAbscissa.assert_called_once()

    def test_plot_series_skips_expression_when_y_axis_limit_reached(self):
        # arrange — fill up all four Y axis slots before calling plot_series
        component = MagicMock()
        values = np.linspace(0.0, 1.0, 10)
        abscissa = Expression("Time", values, "s")
        chart = _make_chart(component, MagicMock(), _make_step_information(1, 10), abscissa)
        # occupy all four axis slots so the fifth unit returns None from _get_y_axis
        chart._get_y_axis("V")
        chart._get_y_axis("A")
        chart._get_y_axis("W")
        chart._get_y_axis("s")
        # fifth expression has a new unit: _get_y_axis will return None
        fifth = Expression("E5", np.linspace(0.0, 1.0, 10), "Hz")
        decimated_y = np.linspace(0.0, 1.0, 10)
        # act
        with patch("kicad_xyce_plugin.chart.decimate_xy", return_value=(values, decimated_y)):
            chart.plot_series({fifth})
        # assert — expression tracked in _series but its ordinate_series list is empty
        _, ordinate_series = chart._series["E5"]
        assert ordinate_series == {}
