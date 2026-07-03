import logging
from pathlib import Path

import numpy as np
from PySide6.QtCore import Qt, QUrl, Slot
from PySide6.QtGui import QColor
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QDialog, QVBoxLayout, QWidget

from .expression import Expression
from .fft import FftOutput, WindowFunction

logger = logging.getLogger(__name__)

_QML_FILE = Path(__file__).parent / "fft_dialog.qml"
_BG = "#1a1b1e"
_MAX_FREQUENCY_OPTIONS: tuple[tuple[str, float], ...] = (
    ("100 Hz", 100.0),
    ("1 kHz", 1e3),
    ("10 kHz", 1e4),
    ("100 kHz", 1e5),
    ("1 MHz", 1e6),
    ("10 MHz", 1e7),
    ("100 MHz", 1e8),
    ("1 GHz", 1e9),
    ("10 GHz", 1e10),
    ("100 GHz", 1e11),
)
_DEFAULT_MAX_FREQUENCY = 1e5


def _max_frequency_default_index(max_frequency: float) -> int:
    # pick the closest preset so the dropdown default reflects the current data scale
    target = float(max_frequency)
    option_values = [value for _, value in _MAX_FREQUENCY_OPTIONS]
    return min(range(len(option_values)), key=lambda idx: abs(option_values[idx] - target))


class FftDialog(QDialog):
    """Dialog for configuring and launching an FFT computation.

    Presents a QML UI for selecting FFT parameters: expressions, data range,
    window, maximum frequency, normalization, output type, and DC option.
    After acceptance, result properties reflect the user's selections.

    Parameters:
        parent: QWidget — parent widget
        expressions: list[Expression] — ordinate expressions for FFT
        min_abscissa_value: float — minimum abscissa value
        max_abscissa_value: float — maximum abscissa value
        min_abscissa_value_zoomed: float — minimum abscissa value in zoom window
        max_abscissa_value_zoomed: float — maximum abscissa value in zoom window
    """

    def __init__(self, parent: QWidget, expressions: list[Expression], min_abscissa_value: float, max_abscissa_value: float, min_abscissa_value_zoomed: float, max_abscissa_value_zoomed: float, default_max_frequency: float = _DEFAULT_MAX_FREQUENCY):
        """Initialize the FFT dialog and set up the QML UI. All expressions are pre-selected by default."""
        super().__init__(parent)
        # store references
        self._expressions = expressions
        # selected expressions tracked via selectionChanged signal; all pre-selected
        self._selected_expressions: set[Expression] = set(expressions)
        # result fields populated when the dialog is accepted
        self._result_expressions: list[Expression] = []
        self._result_from_index: float = float(min_abscissa_value)
        self._result_to_index: float = float(max_abscissa_value)
        self._result_window: WindowFunction = WindowFunction.HANNING
        self._default_max_frequency: float = float(default_max_frequency) if np.isfinite(default_max_frequency) and float(default_max_frequency) > 0.0 else _DEFAULT_MAX_FREQUENCY
        self._default_max_frequency_index: int = _max_frequency_default_index(self._default_max_frequency)
        self._result_max_frequency: float = _MAX_FREQUENCY_OPTIONS[self._default_max_frequency_index][1]
        self._result_normalize: bool = True
        self._result_keep_dc: bool = False
        self._result_output: FftOutput = FftOutput.MAGNITUDE
        # window setup
        self.setWindowTitle("FFT")
        self.setWindowModality(Qt.WindowModality.WindowModal)
        self.resize(480, 650)
        self.setMinimumHeight(650)
        # expose data to QML via context properties
        self._ctx_properties = {
            "windowFunctions": [w.value for w in WindowFunction],
            "outputTypes": [o.value for o in FftOutput],
            "maxFrequencyOptions": [{"label": label, "value": value} for label, value in _MAX_FREQUENCY_OPTIONS],
            "abscissaMin": min_abscissa_value,
            "abscissaMax": max_abscissa_value,
            "zoomFromTime": min_abscissa_value_zoomed,
            "zoomToTime": max_abscissa_value_zoomed,
            "defaultWindowIndex": [w.value for w in WindowFunction].index(WindowFunction.HANNING.value),
            "defaultMaxFrequencyIndex": self._default_max_frequency_index,
        }
        # create QML view
        self._qml_view = QQuickView()
        self._qml_view.statusChanged.connect(self._on_qml_ready)
        self._qml_view.setResizeMode(QQuickView.ResizeMode.SizeRootObjectToView)
        self._qml_view.setColor(QColor(_BG))
        self._qml_view.setSource(QUrl.fromLocalFile(str(_QML_FILE)))
        # embed QML view into dialog
        container = QWidget.createWindowContainer(self._qml_view, self)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(container)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status):
        """Inject context properties and connect QML signals after QML is ready."""
        # only proceed once QML has finished loading successfully
        if status != QQuickView.Status.Ready:
            return
        # set properties directly on the root object
        root = self._qml_view.rootObject()
        for key, value in self._ctx_properties.items():
            root.setProperty(key, value)
        # initialize expression list with all expressions pre-selected
        root.initializeExpressions([[e.name, True] for e in self._expressions])
        # connect QML dialog signals to Python slots
        root.selectionChanged.connect(self._on_expression_selection_changed)
        root.dialogAccepted.connect(self._on_dialog_accepted)
        root.dialogRejected.connect(self.reject)

    @Slot(str, bool)
    def _on_expression_selection_changed(self, name: str, selected: bool):
        """Update selected expressions when user toggles an expression in the QML UI."""
        # find the matching expression and toggle it in the selected set
        expression = next((e for e in self._expressions if e.name == name), None)
        if expression is None:
            return
        if selected:
            self._selected_expressions.add(expression)
        else:
            self._selected_expressions.discard(expression)

    @Slot(str, float, str, bool, str, float, float, bool)
    def _on_dialog_accepted(self, window_fn: str, max_frequency: float, output: str, normalize: bool, range_mode: str, custom_from: float, custom_to: float, keep_dc: bool):
        """Validate selection, store result properties, and close the dialog when accepted from QML UI."""
        # reject if no expressions are selected
        if not self._selected_expressions:
            # log warning and reject dialog when no expressions are selected
            logger.warning("FFT dialog accepted with no expressions selected")
            # reject dialog
            self.reject()
            # exit
            return
        # store resolved expressions preserving their original list order
        self._result_expressions = [e for e in self._expressions if e in self._selected_expressions]
        try:
            # window function
            self._result_window = WindowFunction(window_fn)
        except ValueError:
            # fall back to rectangular when the value is unrecognised
            self._result_window = WindowFunction.RECTANGULAR
        try:
            # maximum frequency
            parsed_max_frequency = float(max_frequency)
            if not np.isfinite(parsed_max_frequency) or parsed_max_frequency <= 0.0:
                raise ValueError()
            self._result_max_frequency = parsed_max_frequency
        except (TypeError, ValueError):
            # fall back to default when the value is unrecognised
            self._result_max_frequency = _MAX_FREQUENCY_OPTIONS[self._default_max_frequency_index][1]
        try:
            # output type
            self._result_output = FftOutput(output)
        except ValueError:
            # fall back to magnitude when the value is unrecognised
            self._result_output = FftOutput.MAGNITUDE
        # normalize flag
        self._result_normalize = normalize
        # keep dc flag
        self._result_keep_dc = keep_dc
        # proceed to accept the dialog and close it
        self.accept()

    @property
    def result_expressions(self) -> list[Expression]:
        """List of expressions selected for FFT (original order)."""
        return self._result_expressions

    @property
    def result_from_index(self) -> float:
        """Left edge of the selected abscissa range."""
        return self._result_from_index

    @property
    def result_to_index(self) -> float:
        """Right edge of the selected abscissa range."""
        return self._result_to_index

    @property
    def result_window(self) -> WindowFunction:
        """Selected window function for FFT."""
        return self._result_window

    @property
    def result_max_frequency(self) -> float:
        """Selected maximum frequency for FFT interpolation/output."""
        return self._result_max_frequency

    @property
    def result_normalize(self) -> bool:
        """Whether normalization is enabled for FFT output."""
        return self._result_normalize

    @property
    def result_keep_dc(self) -> bool:
        """Whether to keep the DC component in FFT output."""
        return self._result_keep_dc

    @property
    def result_output(self) -> FftOutput:
        """Selected FFT output type (e.g., magnitude, phase)."""
        return self._result_output
