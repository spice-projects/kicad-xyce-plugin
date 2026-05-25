import logging
from pathlib import Path

import numpy as np
from numpy.typing import NDArray
from PySide6.QtCore import Q_ARG, QMetaObject, QPointF, QRect, QRectF, QSize, Qt, QTimer, QUrl, Slot
from PySide6.QtGui import QCloseEvent, QColor, QImage, QPainter, QPainterPath, QPen, QPolygonF
from PySide6.QtQml import QmlElement
from PySide6.QtQuick import QQuickItem, QQuickView, QSGNode, QSGSimpleTextureNode, QSGTexture
from PySide6.QtWidgets import QMainWindow, QWidget

from .add_plot_dialog import AddPlotDialog
from .color_palette import SERIES_COLOR_PALETTE
from .decimation_algorithm import decimate_xy, DecimationAlgorithm
from .expression import Expression
from .xyce_raw_file import XyceRawFile
from .step_tool_dialog import StepToolDialog
from .window import load_app_icon, log_screen_info, unregister_child_window

logger = logging.getLogger(__name__)

_QML_FILE = Path(__file__).parent / "smith_chart_window.qml"

# background color matching the chart dark theme
_BG = "#1a1b1e"

R_VALUES = [0.0, 0.2, 0.5, 1.0, 2.0, 5.0]  # constant-resistance
X_VALUES = [0.2, 0.5, 1.0, 2.0, 5.0]  # constant-reactance (±)
SEGS = 128  # tessellation per circle
GRID_COLOR = QColor(83, 74, 183, 160)  # purple-ish
REACT_COLOR = QColor(15, 110, 86, 160)  # teal


# this is the import name and version that QML uses to identify the module containing the SmithGridItem type; it does not need to match the Python module name or version, but it must match the import statement in the QML file
QML_IMPORT_NAME = "SmithChart"
QML_IMPORT_MAJOR_VERSION = 1


@QmlElement
class SmithGridItem(QQuickItem):

    def __init__(self, parent: QQuickItem | None = None):
        super().__init__(parent)
        # initialize state
        self.setFlag(QQuickItem.Flag.ItemHasContents, True)
        self._texture: QSGTexture | None = None
        self._dirty = True
        self._image_size = (0, 0)

    def geometryChange(self, new: QRectF | QRect, old: QRectF | QRect) -> None:
        super().geometryChange(new, old)
        self._dirty = True
        self.update()

    def updatePaintNode(self, old_node: QSGNode | None, _data: QQuickItem.UpdatePaintNodeData) -> QSGNode | None:
        # check dirty flag
        if not self._dirty:
            return old_node
        # reset dirty flag
        self._dirty = False
        # get dimensions in pixels
        W, H = int(self.width()), int(self.height())
        if W <= 0 or H <= 0:
            return old_node
        # only re-render grid if size changed; otherwise reuse existing texture and node tree (GPU-based rendering is very fast after the initial rasterization)
        if (W, H) != self._image_size:
            # store image size for future change detection
            self._image_size = (W, H)
            # create image
            image = self._render_grid(W, H)
            # delete old texture if it exists to free GPU memory; this is important to avoid leaks when resizing the window multiple times, as each new texture consumes GPU resources until the old one is released
            if self._texture:
                self._texture.deleteLater()
            # upload new texture to GPU; this is a one-time cost that can be expensive for large images, but it allows for very fast rendering in subsequent frames since the GPU can efficiently draw the pre-rasterized grid without needing to execute CPU-based drawing commands each time
            self._texture = self.window().createTextureFromImage(image)
        # textured rectangle, fully GPU-composited
        node = old_node or QSGSimpleTextureNode()
        node.setTexture(self._texture)
        node.setRect(QRectF(0, 0, W, H))
        node.setFiltering(QSGTexture.Filtering.Linear)
        # exit
        return node

    def _render_grid(self, W: int, H: int) -> QImage:
        # create transparent image and paint the grid onto it using QPainter; this is a CPU-based operation that generates a rasterized representation of the Smith chart grid, which can then be efficiently rendered by the GPU as a texture in subsequent frames
        image = QImage(W, H, QImage.Format.Format_ARGB32_Premultiplied)
        image.fill(Qt.GlobalColor.transparent)
        # calculate center and radius of the unit disk in pixel coordinates; the grid will be drawn within this disk, and the clipping region will ensure that any parts of the circles that extend beyond the unit disk are not rendered, creating the characteristic shape of the Smith chart
        cx, cy = W / 2, H / 2
        R = min(W, H) / 2 * 0.90
        # set up QPainter for anti-aliased drawing; this allows for smoother and visually appealing rendering of the circles and arcs that make up the Smith chart grid, at the cost of increased CPU time during the rasterization process, which is why we want to minimize how often this needs to be done by caching the resulting texture
        painter = QPainter(image)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        # clip everything to the unit disk
        clip = QPainterPath()
        clip.addEllipse(cx - R, cy - R, R * 2, R * 2)
        painter.setClipPath(clip)
        # resistance circles
        pen_r = QPen(QColor(83, 74, 183, 160), 1.0)
        painter.setPen(pen_r)
        for r in R_VALUES:
            cr = R / (r + 1)
            ccx = cx + r / (r + 1) * R
            painter.drawEllipse(QRectF(ccx - cr, cy - cr, cr * 2, cr * 2))
        # reactance arcs (clipping handles the truncation automatically)
        pen_x = QPen(QColor(15, 110, 86, 160), 1.0)
        painter.setPen(pen_x)
        for x in X_VALUES:
            for sign in [1, -1]:
                xv = sign * x
                cr = R / abs(xv)
                ccx = cx + R  # Γ_re=1 → right edge of unit disk
                ccy = cy - R / xv
                painter.drawEllipse(QRectF(ccx - cr, ccy - cr, cr * 2, cr * 2))
        # outer boundary
        painter.setPen(QPen(QColor(120, 120, 120, 200), 1.5))
        painter.drawEllipse(QRectF(cx - R, cy - R, R * 2, R * 2))
        # real axis
        painter.setPen(QPen(QColor(120, 120, 120, 120), 0.7))
        painter.drawLine(int(cx - R), int(cy), int(cx + R), int(cy))
        # end
        painter.end()
        # exit
        return image


@QmlElement
class SmithTraceItem(QQuickItem):

    def __init__(self, parent: QQuickItem | None = None) -> None:
        super().__init__(parent)
        # initialize state
        self.setFlag(QQuickItem.Flag.ItemHasContents, True)
        self._traces = []
        self._texture = None
        self._dirty = True

    def geometryChange(self, new: QRectF | QRect, old: QRectF | QRect) -> None:
        super().geometryChange(new, old)
        self._dirty = True
        self.update()

    def updatePaintNode(self, old_node: QSGNode | None, _data: QQuickItem.UpdatePaintNodeData) -> QSGNode | None:
        # get dimensions in pixels
        W, H = int(self.width()), int(self.height())
        if W <= 0 or H <= 0:
            return old_node
        # check dirty flag
        if self._dirty:
            # reset dirty flag
            self._dirty = False
            # create transparent image and paint the grid onto it using QPainter
            image = QImage(W, H, QImage.Format.Format_ARGB32_Premultiplied)
            image.fill(Qt.GlobalColor.transparent)
            # calculate center and radius of the unit disk in pixel coordinates
            cx, cy = W / 2, H / 2
            R = min(W, H) / 2 * 0.90
            # set up QPainter for anti-aliased drawing
            painter = QPainter(image)
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)
            # clip traces to the unit disk
            clip = QPainterPath()
            clip.addEllipse(cx - R, cy - R, R * 2, R * 2)
            painter.setClipPath(clip)
            # draw traces
            for pts_gamma, _, color in self._traces:
                # skip if not enough points to draw a trace
                if pts_gamma is None or len(pts_gamma) < 2:
                    continue
                # convert trace points from complex Γ coordinates to pixel coordinates
                px = self._get_pixel_pts(pts_gamma, cx, cy, R)
                n = len(px)
                # set up pen for drawing the trace with the specified color and width
                pen = QPen(color, 2.0)
                pen.setCapStyle(Qt.PenCapStyle.RoundCap)
                painter.setPen(pen)
                # create a QPolygonF from the pixel coordinates of the trace points
                poly = QPolygonF([QPointF(float(px[i, 0]), float(px[i, 1])) for i in range(n)])
                # draw the trace as a connected series of line segments
                painter.drawPolyline(poly)
            # end QPainter
            painter.end()
            # delete old texture
            if self._texture:
                self._texture.deleteLater()
            # upload new texture to GPU
            self._texture = self.window().createTextureFromImage(image)
        # reuse existing node and texture since nothing changed
        node = old_node or QSGSimpleTextureNode()
        node.setTexture(self._texture)
        node.setRect(QRectF(0, 0, W, H))
        node.setFiltering(QSGTexture.Filtering.Linear)
        # exit
        return node

    def _get_pixel_pts(self, pts_gamma: NDArray[np.float64], cx: float, cy: float, R: float) -> NDArray[np.float64]:
        # convert trace points from complex Γ coordinates to pixel coordinates
        px = np.empty_like(pts_gamma)
        px[:, 0] = cx + pts_gamma[:, 0] * R
        px[:, 1] = cy - pts_gamma[:, 1] * R
        # decimation using Ramer-Douglas-Peucker algorithm to reduce the number of points while preserving the overall shape of the trace
        x_dec, y_dec = decimate_xy(px[:, 0], px[:, 1], target=2000, algorithm=DecimationAlgorithm.RDP)
        # exit
        return np.column_stack((x_dec, y_dec))

    @Slot("QVariant")
    def plot(self, traces: list[tuple[NDArray[np.float64], str, QColor]]) -> None:
        # update the list of traces and request a re-render
        self._traces = traces
        self._dirty = True
        self.update()


class SmithChartWindow(QMainWindow):

    def __init__(self, raw_file: XyceRawFile) -> None:
        super().__init__()
        # load and set the application icon
        icon = load_app_icon()
        if not icon.isNull():
            self.setWindowIcon(icon)
        # keep a reference to the source qraw object for creating secondary windows
        self._raw_file = raw_file
        # extract information from file
        self._abscissa = raw_file.abscissa
        self._abscissa_scale = raw_file.abscissa_scale
        self._expression_manager = raw_file.expression_manager
        self._step_information = raw_file.step_information
        self._selected_steps: set[int] = set(range(self._step_information.length))
        self._plot_suggestions = raw_file.get_plot_suggestions()
        # current visualization state
        self._expressions: dict[Expression, tuple[QColor, dict[int, np.ndarray]]] = {}
        # set window title to include the loaded filename
        self.setWindowTitle(f"{raw_file.chart_type} - {raw_file.filename.name}")
        # apply dark background stylesheet to the window chrome
        self.setStyleSheet(f"QMainWindow {{ background: {_BG}; }}")
        # single QQuickView hosts the entire multi-chart scene — one Metal swap chain
        self._qml_view = QQuickView()
        self._qml_view.statusChanged.connect(self._on_qml_ready)
        self._qml_view.setResizeMode(QQuickView.ResizeMode.SizeRootObjectToView)
        self._qml_view.setColor(QColor(_BG))
        self._qml_view.setSource(QUrl.fromLocalFile(str(_QML_FILE)))
        # embed the single QWindow into the main window's central area
        self._container = QWidget.createWindowContainer(self._qml_view, self)
        self.setCentralWidget(self._container)
        # next color index for new series
        self._next_color_index = 0

    def sizeHint(self) -> QSize:
        return QSize(1200, 800)

    def closeEvent(self, event: QCloseEvent) -> None:
        # release application-level child window ownership when this window closes
        unregister_child_window(self)
        # delegate to the Qt base class when available
        close_event = getattr(super(), "closeEvent", None)
        if close_event is not None:
            close_event(event)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status) -> None:
        # only proceed once QML has finished loading successfully
        if status != QQuickView.Status.Ready:
            return
        # qml view root object
        self._root = self._qml_view.rootObject()
        # set window-level menu capability flags using built-in bool to avoid passing numpy.bool into QML properties
        self._root.setProperty("stepToolEnabled", bool(self._step_information.length > 1))
        # connect signals from QML to Python handlers
        self._root.menuAddRemovePlots.connect(self._on_menu_add_remove_plots)
        self._root.menuDeleteAllPlots.connect(self._on_menu_delete_all_plots)
        self._root.menuStepTool.connect(self._on_menu_step_tool)
        # populate chart after the event loop starts so the window is visible first
        QTimer.singleShot(0, self._populate_charts)
        # log screen information for debugging purposes
        if logger.isEnabledFor(logging.DEBUG):
            QTimer.singleShot(0, lambda: log_screen_info(self.screen()))

    @Slot()
    def _on_menu_add_remove_plots(self) -> None:
        # log information
        logger.debug("User requested adding/removing plots on smith chart")
        # open the add plot dialog, only show complex expressions suitable for smith chart
        dialog = AddPlotDialog(self, self._expression_manager, list(self._expressions.keys()), allow_custom_expressions=False, expression_filter=lambda expression: expression.complex)
        # exit if the user cancelled
        if dialog.exec() != AddPlotDialog.DialogCode.Accepted:
            return
        # plot selected expressions on the chart
        self._add_plots(dialog.selected_expressions)

    @Slot()
    def _on_menu_delete_all_plots(self) -> None:
        # log information
        logger.debug("User requested deleting all plots on smith chart")
        # clear chart
        self._clear()

    @Slot()
    def _on_menu_step_tool(self) -> None:
        # log information
        logger.debug("User requested step tool on smith chart")
        # get selected steps for this chart, make a copy
        selected_steps = set(self._selected_steps)
        # open step tool dialog
        dialog = StepToolDialog(self, self._step_information, selected_steps)
        # exit if the user canceled
        if dialog.exec() != StepToolDialog.DialogCode.Accepted:
            return
        # store selected steps for later filtering phase
        self._selected_steps = dialog.selected_steps
        # refresh plots
        self._add_plots(set(self._expressions.keys()))

    def _populate_charts(self) -> None:
        # loop suggestions — each suggestion carries its own chart type
        for suggestion in self._plot_suggestions:
            # append plot
            self._add_plots(suggestion.expressions)
            # we are visualizing a single Smith Chart, so exit after the first suggestion (more than one expression is supported)
            break

    def _add_plots(self, expressions: set[Expression]) -> None:
        # find the SmithTraceItem instance
        trace_item = self._root.findChild(SmithTraceItem)
        if not trace_item:
            return
        # 1. remove any expressions that are no longer selected for plotting
        for expression in list(self._expressions.keys()):
            if expression not in expressions:
                # log information
                logger.debug("Removing series for expression [%s] from Smith chart", expression.name)
                # delete the expression
                del self._expressions[expression]
        # 2. loop expressions to update their per-step traces
        for expression in expressions:
            # lookup existing state or create new one
            color, rendered_traces = self._expressions.get(expression, (None, {}))
            # assign next color in palette if this is a new expression
            if color is None:
                # select color from palette
                color = QColor(SERIES_COLOR_PALETTE[self._next_color_index % len(SERIES_COLOR_PALETTE)])
                # increment color index
                self._next_color_index += 1
            # remove steps that are no longer selected
            for step in list(rendered_traces.keys()):
                # check step should be visible
                if step not in self._selected_steps:
                    # log information
                    logger.debug("Removing trace for [%s], step: %d", expression.name, step)
                    # remove step trace
                    del rendered_traces[step]
            # add missing steps that are now selected
            for step in self._selected_steps:
                # check step is not already rendered
                if step in rendered_traces:
                    continue
                # step slice
                step_slice = self._step_information.abscissa_indices[step]
                # complex reflection coefficient data
                data = expression.data[step_slice]
                # filter non-finite values (Inf/NaN) to prevent rendering issues
                finite_mask = np.isfinite(data)
                if not np.all(finite_mask):
                    data = data[finite_mask]
                # skip if no valid data points to plot
                if data.size == 0:
                    continue
                # convert to [re, im] columns
                if expression.complex:
                    gamma = np.column_stack((data.real, data.imag))
                else:
                    gamma = np.column_stack((data, np.zeros_like(data)))
                # store rendered trace for this step
                rendered_traces[step] = gamma
            # update state
            self._expressions[expression] = (color, rendered_traces)
        # 3. collect all traces to be rendered in the SmithTraceItem
        traces_to_render = []
        # loop expressions and associated traces
        for expression, (color, rendered_traces) in self._expressions.items():
            # loop steps and associated rendered trace for this expression
            for step, gamma in rendered_traces.items():
                traces_to_render.append((gamma, f"{expression.name} (Step {step})", color))
        # plot all traces on the Smith chart
        QMetaObject.invokeMethod(trace_item, "plot", Q_ARG("QVariant", traces_to_render))

    def _clear(self) -> None:
        # clear internal state
        self._expressions = {}
        self._next_color_index = 0
        # find the SmithTraceItem instance
        trace_item = self._root.findChild(SmithTraceItem)
        if trace_item:
            QMetaObject.invokeMethod(trace_item, "plot", Q_ARG("QVariant", []))
