import logging

import numpy as np
from PySide6.QtCore import QTimer, Qt
from PySide6.QtGraphs import QAbstractAxis, QLineSeries
from PySide6.QtQuick import QQuickItem

from color_palette import SERIES_COLOR_PALETTE
from decimation_algorithm import DecimationAlgorithm, decimate_xy
from expression import Expression, ExpressionManager
from xyce_raw_file import StepInformation

logger = logging.getLogger(__name__)

# default decimation algorithm used when adding series to charts
_DECIMATION_ALGORITHM = DecimationAlgorithm.M4


def _binary_search(data: np.ndarray, value: float, ascending: bool, side: int) -> int:
    """ O(log n) binary search that repects sort order without array copying or view creation. side == 1 for left bound, -1 for right bound"""
    # initialize lo and hi
    lo, hi = 0, len(data)
    # check data is ascending
    if ascending:
        # loop
        while lo < hi:
            # middle index
            middle = (lo + hi) // 2
            # compare middle value with target value
            if (side == 1 and data[middle] < value) or (side == -1 and data[middle] <= value):
                # move lo up
                lo = middle + 1
            else:
                # move hi down
                hi = middle
        # exit
        return lo
    # data is descending, loop
    while lo < hi:
        # middle index
        middle = (lo + hi) // 2
        # compare middle value with target value
        if (side == 1 and data[middle] > value) or (side == -1 and data[middle] >= value):
            # move lo up
            lo = middle + 1
        else:
            # move hi down
            hi = middle
    # exit
    return lo


def _find_abscissa_index_for_value(data: np.ndarray, value: float, ascending: bool) -> int:
    # find insertion point using binary search
    index = _binary_search(data, value, ascending, side=1)
    # clamp to valid range
    index = min(index, len(data) - 1)
    # check if the previous index is closer to the target value
    if index > 0 and abs(data[index - 1] - value) <= abs(data[index] - value):
        return index - 1
    # otherwise return the found index
    return index


class Chart:

    def __init__(self, component: QQuickItem, expression_manager: ExpressionManager, abscissa: Expression, step_information: StepInformation, decimate_target: int):
        # store component
        self._component = component
        # store expression manager
        self._expression_manager = expression_manager
        # store variables
        self._abscissa = abscissa
        # current zoom window (x: in abscissa percentages, y: in ordinate percentages)
        self._zoom_window = (None, None, None, None)
        # steps
        self._step_information = step_information
        self._selected_steps: set[int] = set(range(self._step_information.length))
        # store decimation target for later use when adding series
        self._decimate_target = decimate_target
        # track active series
        self._series: dict[str, tuple[Expression, dict[Expression, tuple[QAbstractAxis, dict[int, QLineSeries], float, float, str]]]] = {}
        # axis tracking for measurement types, e.g. {"V": <QAbstractAxis>, "I": <QAbstractAxis>}
        self._y_axes: dict[str, QAbstractAxis] = {}
        self._y_axes_ref_counts: dict[QAbstractAxis, int] = {}
        # axis ranges
        self._axis_ranges: dict[QAbstractAxis, tuple[float, float]] = {}
        # y axes instances
        self._left_y_axis_1: QAbstractAxis | None = None
        self._right_y_axis_1: QAbstractAxis | None = None
        self._left_y_axis_2: QAbstractAxis | None = None
        self._right_y_axis_2: QAbstractAxis | None = None
        # next color index for new series
        self._next_color_index = 0

    @property
    def expressions(self) -> list[Expression]:
        return [expression for expression, _ in self._series.values()]

    @property
    def abscissa(self) -> Expression:
        return self._abscissa

    @property
    def selected_steps(self) -> set[int]:
        return self._selected_steps

    @property
    def zoom_window(self) -> tuple[float | None, float | None, float | None, float | None]:
        return self._zoom_window

    @selected_steps.setter
    def selected_steps(self, selected_steps: set[int]) -> None:
        # check selection changed
        if selected_steps == self._selected_steps:
            return
        # update selected steps
        self._selected_steps = selected_steps
        # force step processing, adding/removing series as needed based on the new step selection and the current expressions plotted in the chart
        self.plot_series(self.expressions)

    def render(self, abscissa_label: str, abscissa_scale: str, initial_expressions: set[Expression]):
        # initialize chart component
        self._component.initialize(abscissa_label, self._abscissa.unit, abscissa_scale, self._step_information.abscissa_left_value, self._step_information.abscissa_right_value)
        # render all expressions as series
        self.plot_series(initial_expressions)
        # auto range axes based on the added series
        self.auto_range()

    def plot_series(self, expressions: set[Expression]):
        # series to render and remove from the chart
        series_to_render: list[tuple[str, str, list[QLineSeries]]] = []
        series_to_remove: list[tuple[str | None, list[QLineSeries]]] = []
        # labels to remove from the chart
        labels_to_remove: list[str] = []
        # axis to remove, prevent GC until Qt finishes its async processing of the series removals that triggered these axis removals
        axes_to_remove: list[QAbstractAxis] = []
        # loop existing series to find those that need to be removed (those whose expression is not in the new expressions list)
        for label, (expression, ordinate_series) in self._series.items():
            # check expression should be removed
            if expression not in expressions:
                # enqueue series for removal
                for ordinate_variant, (axis, rendered_series, _, _, _) in ordinate_series.items():
                    # release axis if no longer in use
                    if self._release_y_axis(axis):
                        axes_to_remove.append(axis)
                    # log information
                    logger.debug("Removing series for expression [%s] from chart, steps: %s", ordinate_variant.name, list(rendered_series.keys()))
                    # append to list for later removal from chart
                    series_to_remove.append([ordinate_variant.name, list(rendered_series.values())])
                # remove from tracked series so we don't try to update it later
                labels_to_remove.append(label)
        # update dictionary outside loop
        for label in labels_to_remove:
            del self._series[label]
        # current zoom window in abscissa values, None if not set
        x_left_ratio, _, x_right_ratio, _ = self._zoom_window
        # x0 and x1
        abscissa_left_value = self._ratio_to_abscissa_value(x_left_ratio) if x_left_ratio is not None else self._step_information.abscissa_left_value
        abscissa_right_value = self._ratio_to_abscissa_value(x_right_ratio) if x_right_ratio is not None else self._step_information.abscissa_right_value
        # loop expressions that should be plotted
        for ordinate in expressions:
            # lookup ordinate in series
            _, ordinate_series = self._series.get(ordinate.name, (ordinate, {}))
            # lookup expressions to plot for this ordinate, e.g. magnitude and phase for complex expressions when in AC chart
            for ordinate_variant in self._get_expressions_to_plot(ordinate):
                # looup ordinate variant in series
                y_axis, rendered_series, min_value, max_value, color = ordinate_series.get(ordinate_variant, (None, {}, float("inf"), float("-inf"), None))
                # loop rendered steps
                for step in list(rendered_series.keys()):
                    # check step should be removed
                    if step not in self._selected_steps:
                        # log information
                        logger.debug("Removing series for expression [%s] from chart, step: %d", ordinate_variant.name, step)
                        # append to list for later removal from chart
                        series_to_remove.append([None, [rendered_series[step]]])
                        # remove from dictionary so we don't try to update it later
                        del rendered_series[step]
                # process axis as needed
                if y_axis is None:
                    # find y axis for measurement type
                    y_axis = self._get_y_axis(ordinate_variant.unit)
                    if y_axis is None:
                        # log information
                        logger.warning(f"Cannot add series '{ordinate_variant.name}' of measurement type {ordinate_variant.unit} to chart — maximum number of Y axes reached")
                        # exit loop
                        break
                # check we need to generate a color for this expression
                if color is None:
                    # assign next color in palette
                    color = SERIES_COLOR_PALETTE[self._next_color_index % len(SERIES_COLOR_PALETTE)]
                    # update index
                    self._next_color_index += 1
                # ordinate series to render
                ordinate_series_to_render: list[QLineSeries] = []
                # loop steps
                for step in self._selected_steps:
                    # check step is already rendered
                    if step in rendered_series:
                        continue
                    # step slice
                    step_slice = self._step_information.abscissa_indices[step]
                    # step abscissa & ordinate values
                    abscissa_values = self._abscissa.data[step_slice]
                    ordinate_values_at_abscissa_value = ordinate_variant.data[step_slice]
                    # check we have a zoom window to apply
                    if x_left_ratio is not None and x_right_ratio is not None:
                        # find indexes for the new zoom window
                        indexes = self._find_abscissa_indexes(abscissa_values, abscissa_left_value, abscissa_right_value)
                        # abscissa values
                        abscissa_values = abscissa_values[indexes]
                        # ordinate variant values for this step & zoom window
                        ordinate_values_at_abscissa_value = ordinate_values_at_abscissa_value[indexes]
                    # skip inconsistent slices to protect decimation input contracts
                    if abscissa_values.size == 0:
                        continue
                    # decimate x and y jointly so every plotted (x, y) pair maps to the same original sample
                    x_np, y_np = decimate_xy(abscissa_values, ordinate_values_at_abscissa_value, self._decimate_target, _DECIMATION_ALGORITHM)
                    # remove Inf values
                    inf_mask = np.isinf(y_np)
                    if inf_mask.any():
                        # mask for finite values
                        keep_mask = ~inf_mask
                        # update x and y with finite values only
                        x_np = x_np[keep_mask]
                        y_np = y_np[keep_mask]
                    # check all values were non-finite after filtering
                    if x_np.size == 0 or y_np.size == 0:
                        continue
                    # log information
                    logger.debug("Adding series for expression [%s], step: %d, original size: %d, decimated size: %d", ordinate_variant.name, step, abscissa_values.size, x_np.size)
                    # create series and hand buffers directly to Qt — no Python loop
                    series = QLineSeries()
                    series.setColor(color)
                    series.setWidth(2)
                    series.replaceNp(x_np, y_np)
                    series.setAxisY(y_axis)
                    # stroke style for stepped variants
                    if step > 0:
                        # change stroke style
                        series.setStrokeStyle(QLineSeries.StrokeStyle.DashLine)
                        series.setDashPattern([3, step + 1])
                    # append to lists
                    rendered_series[step] = series
                    # append to list for later addition to chart
                    ordinate_series_to_render.append(series)
                    # update min and max values
                    min_value = min(min_value, float(np.min(y_np)))
                    max_value = max(max_value, float(np.max(y_np)))
                # render new series
                series_to_render.append([ordinate_variant.name if ordinate_variant not in ordinate_series else None, color, ordinate_series_to_render])
                # store series with min and max values for later use when auto-ranging axes
                ordinate_series[ordinate_variant] = (y_axis, rendered_series, min_value, max_value, color)
            # store reference to allow removal later
            self._series[ordinate.name] = (ordinate, ordinate_series)
        # check changes are required in qml
        if len(series_to_render) > 0 or len(series_to_remove) > 0:
            # add/remove series from chart
            self._component.updateGraphsView(series_to_render, series_to_remove)
            # release stash after Qt finishes its async processing
            QTimer.singleShot(2000, lambda: (series_to_remove.clear(), axes_to_remove.clear()))

    def auto_range(self):
        # skip if no series are currently plotted
        if not self._series:
            return
        # min and max values axis index, reset them
        self._axis_ranges = {}
        # loop visible series
        for _, (_, ordinate_series) in self._series.items():
            # process series
            for y_axis, _, min_value, max_value, _ in ordinate_series.values():
                # current min and max for this variable type
                current_min, current_max = self._axis_ranges.get(y_axis, (float("inf"), float("-inf")))
                # compute Y values for this variable index
                self._axis_ranges[y_axis] = (min(current_min, min_value), max(current_max, max_value))
        # update axis ranges based on collected min and max values for each variable type
        for y_axis, (y_min, y_max) in self._axis_ranges.items():
            # range
            y_range = y_max - y_min
            # delta
            delta = 0.03 * y_range
            # set y axis range
            y_axis.setRange(y_min - delta, y_max + delta)

    def update_zoom_window(self, x_left_ratio: float | None, x_right_ratio: float | None, y_top_ratio: float | None, y_bottom_ratio: float | None):
        # check horizontal zoom ratios were provided
        if x_left_ratio is not None and x_right_ratio is not None:
            # current zoom window
            current_x_left_ratio, _, current_x_right_ratio, _ = self._zoom_window
            # calculate new ratios based on the position of the mouse within the chart panel and the current zoom window
            x_left_ratio = (current_x_left_ratio or 0.0) + x_left_ratio * ((current_x_right_ratio or 1.0) - (current_x_left_ratio or 0.0))
            x_right_ratio = (current_x_left_ratio or 0.0) + x_right_ratio * ((current_x_right_ratio or 1.0) - (current_x_left_ratio or 0.0))
            # update zoom window
            self._zoom_window = (x_left_ratio, self._zoom_window[1], x_right_ratio, self._zoom_window[3])
            # process all series to apply the new zoom window, full redraw if horizontal zoom changed
            self._redraw_all_series()
        # check vertical zoom ratios were provided
        if y_top_ratio is not None and y_bottom_ratio is not None:
            # current zoom window
            _, current_y_top_ratio, _, current_y_bottom_ratio = self._zoom_window
            # calculate new ratios based on the position of the mouse within the chart panel and the current zoom window
            y_top_ratio = (current_y_top_ratio or 0.0) + y_top_ratio * ((current_y_bottom_ratio or 1.0) - (current_y_top_ratio or 0.0))
            y_bottom_ratio = (current_y_top_ratio or 0.0) + y_bottom_ratio * ((current_y_bottom_ratio or 1.0) - (current_y_top_ratio or 0.0))
            # update zoom window
            self._zoom_window = (self._zoom_window[0], y_top_ratio, self._zoom_window[2], y_bottom_ratio)
            # update axis ranges based on collected min and max values for each variable type
            for y_axis, (y_min, y_max) in self._axis_ranges.items():
                # range (from data)
                y_range = y_max - y_min
                # delta
                delta = 0.03 * y_range
                # actual axis min/max values (see auto_range)
                visual_y_min = y_min - delta
                visual_y_max = y_max + delta
                # calculate visual axis range
                visual_y_range = visual_y_max - visual_y_min
                # set y axis range
                y_axis.setRange(visual_y_min + y_top_ratio * visual_y_range, visual_y_min + y_bottom_ratio * visual_y_range)

    def reset_zoom_window(self, horizontal: bool, vertical: bool):
        # check horizontal reset
        if horizontal:
            # update zoom window
            self._zoom_window = (None, self._zoom_window[1], None, self._zoom_window[3])
            # process all series to apply the new zoom window, full redraw if horizontal zoom changed
            self._redraw_all_series()
        # check vertical reset
        if vertical:
            # update zoom window
            self._zoom_window = (self._zoom_window[0], None, self._zoom_window[2], None)
            # update axis ranges based on collected min and max values for each variable type
            for y_axis, (y_min, y_max) in self._axis_ranges.items():
                # range
                y_range = y_max - y_min
                # delta
                delta = 0.03 * y_range
                # set y axis range
                y_axis.setRange(y_min - delta, y_max + delta)

    def clear(self):
        # Qt enqueues the visual removal of series asynchronously. Python owns the QLineSeries, so we must NOT let Python GC them until Qt has finished processing the removal queue.
        old_series = self._series
        old_y_axes = self._y_axes
        # reset internal state
        self._series = {}
        self._y_axes = {}
        self._y_axes_ref_counts = {}
        # reset color index for new series
        self._next_color_index = 0
        # reset zoom window (vertical axes only, keep horizontal range)
        self._zoom_window = (self._zoom_window[0], None, self._zoom_window[2], None)
        # enqueue Qt-side removal
        self._component.removeAllSeries()
        # remove axis references
        self._left_y_axis_1 = None
        self._right_y_axis_1 = None
        self._left_y_axis_2 = None
        self._right_y_axis_2 = None
        # release stash after Qt finishes its async processing
        QTimer.singleShot(1000, lambda: (old_series.clear(), old_y_axes.clear()))

    def ordinate_values_at_abscissa_value(self, x_value: float) -> list[tuple[str, str, list[float]]]:
        # check series are plotted
        if not self._series:
            return []
        # ascending or descending abscissa
        ascending = self._step_information.abscissa_ascending
        # collect one (name, unit, value) tuple per plotted variant (magnitude/phase counted separately)
        result: list[tuple[str, str, list[float]]] = []
        # loop series
        for _, (_, ordinate_series) in self._series.items():
            # loop variants for this series (magnitude/phase)
            for ordinate_variant, (_, rendered_series, _, _, _) in ordinate_series.items():
                # values (per step)
                values: list[float] = []
                # step and series for this step
                for step, _ in rendered_series.items():
                    # step slice
                    step_slice = self._step_information.abscissa_indices[step]
                    # abscissa & ordinate values for this step
                    abscissa_data = self._abscissa.data[step_slice]
                    ordinate_data = ordinate_variant.data[step_slice]
                    # find abscissa index for value
                    index = _find_abscissa_index_for_value(abscissa_data, x_value, ascending)
                    # append ordinate value at this abscissa value
                    values.append(float(ordinate_data[index]))
                # append to result (name, unit, value)
                result.append((ordinate_variant.name, ordinate_variant.unit, values))
        # exit
        return result

    def _ratio_to_abscissa_value(self, x_ratio: float) -> float:
        # make sure ratio is between 0 and 1
        percentage = max(0.0, min(1.0, x_ratio))
        # convert to abscissa value
        return self._step_information.abscissa_left_value + percentage * (self._step_information.abscissa_right_value - self._step_information.abscissa_left_value)

    def abscissa_value_at_cursor(self, cursor: float) -> float:
        # make sure cursor is between 0 and 1
        percentage = max(0.0, min(1.0, cursor))
        # apply zoom window if it exists
        if self._zoom_window[0] is not None and self._zoom_window[2] is not None:
            # recalculate percentage based on zoom window
            percentage = self._zoom_window[0] + percentage * (self._zoom_window[2] - self._zoom_window[0])
        # convert to abscissa value
        return self._step_information.abscissa_left_value + percentage * (self._step_information.abscissa_right_value - self._step_information.abscissa_left_value)

    def _find_abscissa_indexes(self, abscissa: np.ndarray, left_value: float, right_value: float) -> slice:
        # ascending or descending abscissa
        ascending = self._step_information.abscissa_ascending
        # check abscissa is not within the zoom window
        if (ascending and (abscissa[0] > right_value or abscissa[-1] < left_value)) or (not ascending and (abscissa[0] < right_value or abscissa[-1] > left_value)):
            return slice(0, 0)
        # find left and right indexes using binary search
        left_index = _binary_search(abscissa, left_value, ascending, side=1)
        right_index = _binary_search(abscissa, right_value, ascending, side=-1)
        # return slice for values within the zoom window
        return slice(left_index, right_index)

    def _get_expressions_to_plot(self, expression: Expression) -> list[Expression]:
        # check we can plot expression as is
        if not expression.complex:
            return [expression]
        # magnitude
        magnitude_expression = self._expression_manager.evaluate(f"db({expression.name})")
        if not magnitude_expression:
            return []
        # phase
        phase_expression = self._expression_manager.evaluate(f"phase({expression.name})")
        if not phase_expression:
            return []
        # exit
        return [magnitude_expression, phase_expression]

    def _redraw_all_series(self):
        # current zoom window in abscissa values, None if not set
        x_left_ratio, _, x_right_ratio, _ = self._zoom_window
        # x0 and x1
        abscissa_left_value = self._ratio_to_abscissa_value(x_left_ratio) if x_left_ratio is not None else self._step_information.abscissa_left_value
        abscissa_right_value = self._ratio_to_abscissa_value(x_right_ratio) if x_right_ratio is not None else self._step_information.abscissa_right_value
        try:
            # loop existing series
            for _, (_, ordinate_series) in self._series.items():
                # loop series data (actual data visible in chart)
                for ordinate_variant, (y_axis, rendered_series, _, _, color) in ordinate_series.items():
                    # min and max value recalculation for the new zoom window
                    min_value = float("inf")
                    max_value = float("-inf")
                    # loop steps
                    for step, series in rendered_series.items():
                        # step slice
                        step_slice = self._step_information.abscissa_indices[step]
                        # step abscissa & ordinate values
                        abscissa_values = self._abscissa.data[step_slice]
                        ordinate_values_at_abscissa_value = ordinate_variant.data[step_slice]
                        # check we have a zoom window to apply
                        if x_left_ratio is not None and x_right_ratio is not None:
                            # find indexes for the new zoom window
                            indexes = self._find_abscissa_indexes(abscissa_values, abscissa_left_value, abscissa_right_value)
                            # abscissa values
                            abscissa_values = abscissa_values[indexes]
                            # ordinate variant values for this step & zoom window
                            ordinate_values_at_abscissa_value = ordinate_values_at_abscissa_value[indexes]
                        # decimate x and y jointly so every plotted (x, y) pair maps to the same original sample
                        x_np, y_np = decimate_xy(abscissa_values, ordinate_values_at_abscissa_value, self._decimate_target, _DECIMATION_ALGORITHM)
                        # remove Inf values
                        inf_mask = np.isinf(y_np)
                        if inf_mask.any():
                            # mask for finite values
                            keep_mask = ~inf_mask
                            # update x and y with finite values only
                            x_np = x_np[keep_mask]
                            y_np = y_np[keep_mask]
                        # log information
                        logger.debug("Updating series for expression [%s], step: %d, original size: %d, decimated size: %d", ordinate_variant.name, step, abscissa_values.size, x_np.size)
                        # update series with decimated data
                        series.replaceNp(x_np, y_np)
                        # skip empty series to protect min/max calculations
                        if y_np.size == 0:
                            continue
                        # update min and max values
                        min_value = min(min_value, float(np.min(y_np)))
                        max_value = max(max_value, float(np.max(y_np)))
                    # update dictionary entry
                    ordinate_series[ordinate_variant] = (y_axis, rendered_series, min_value, max_value, color)
        finally:
            # resize abscissa axis
            self._component.resizeAbscissa(abscissa_left_value, abscissa_right_value)

    def _get_y_axis(self, unit: str) -> QAbstractAxis | None:
        # existing axis for measurement type
        axis = self._y_axes.get(unit)
        if axis is not None:
            # increase reference count for this axis
            self._y_axes_ref_counts[axis] += 1
            # use axis
            return axis
        # log information
        logger.debug("Creating Y axis for measurement type: %s", unit or "<no unit>")
        # left (main)
        if self._left_y_axis_1 is None:
            # create axis
            self._left_y_axis_1 = self._component.createYAxis(Qt.AlignmentFlag.AlignLeft, unit)
            # register axis
            self._y_axes[unit] = self._left_y_axis_1
            self._y_axes_ref_counts[self._left_y_axis_1] = 1
            # use axis
            return self._left_y_axis_1
        # right (main)
        if self._right_y_axis_1 is None:
            # create axis
            self._right_y_axis_1 = self._component.createYAxis(Qt.AlignmentFlag.AlignRight, unit)
            # register axis
            self._y_axes[unit] = self._right_y_axis_1
            self._y_axes_ref_counts[self._right_y_axis_1] = 1
            # use axis
            return self._right_y_axis_1
        # left (secondary)
        if self._left_y_axis_2 is None:
            # create axis
            self._left_y_axis_2 = self._component.createYAxis(Qt.AlignmentFlag.AlignLeft, unit)
            # register axis
            self._y_axes[unit] = self._left_y_axis_2
            self._y_axes_ref_counts[self._left_y_axis_2] = 1
            # use axis
            return self._left_y_axis_2
        # right (secondary)
        if self._right_y_axis_2 is None:
            # create axis
            self._right_y_axis_2 = self._component.createYAxis(Qt.AlignmentFlag.AlignRight, unit)
            # register axis
            self._y_axes[unit] = self._right_y_axis_2
            self._y_axes_ref_counts[self._right_y_axis_2] = 1
            # use axis
            return self._right_y_axis_2
        # no more axes available
        return None

    def _release_y_axis(self, axis: QAbstractAxis) -> bool:
        # decrease reference count for this axis
        self._y_axes_ref_counts[axis] -= 1
        # check if axis is now unused and can be released
        if self._y_axes_ref_counts[axis] == 0:
            # unit
            unit = axis.property("yUnit")
            # log information
            logger.debug("Releasing Y axis for measurement type: %s", unit or "<no unit>")
            # remove from tracked axes
            del self._y_axes_ref_counts[axis]
            del self._y_axes[unit]
            # release the internal reference
            if axis == self._left_y_axis_1:
                self._left_y_axis_1 = None
            elif axis == self._right_y_axis_1:
                self._right_y_axis_1 = None
            elif axis == self._left_y_axis_2:
                self._left_y_axis_2 = None
            elif axis == self._right_y_axis_2:
                self._right_y_axis_2 = None
            # remove from chart
            return True
        # exit
        return False
