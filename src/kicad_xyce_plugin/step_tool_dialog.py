import logging
from pathlib import Path

from PySide6.QtCore import QUrl, Slot
from PySide6.QtGui import QColor, QPalette
from PySide6.QtQml import qmlRegisterSingletonInstance
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QApplication, QDialog, QVBoxLayout, QWidget

from .xyce_raw_file import StepInformation

logger = logging.getLogger(__name__)

_QML_FILE = Path(__file__).parent / "step_tool_dialog.qml"


class _ThemeColors:

    def __init__(self):
        palette = QApplication.palette()
        self.background = palette.color(QPalette.ColorRole.Window).name()
        self.alternate_base = palette.color(QPalette.ColorRole.AlternateBase).name()
        self.text = palette.color(QPalette.ColorRole.WindowText).name()
        self.button = palette.color(QPalette.ColorRole.Button).name()
        self.button_text = palette.color(QPalette.ColorRole.ButtonText).name()
        self.highlight = palette.color(QPalette.ColorRole.Highlight).name()
        self.highlighted_text = palette.color(QPalette.ColorRole.HighlightedText).name()
        self.mid = palette.color(QPalette.ColorRole.Mid).name()
        self.midlight = palette.color(QPalette.ColorRole.Midlight).name()
        self.dark = palette.color(QPalette.ColorRole.Dark).name()
        self.mid = palette.color(QPalette.ColorRole.Mid).name()
        self.disabled_text = palette.color(QPalette.ColorRole.Disabled, QPalette.ColorGroup.Disabled).name()
        self.tool_tip_base = palette.color(QPalette.ColorRole.ToolTipBase).name()


class StepToolDialog(QDialog):

    def __init__(self, parent: QWidget, step_information: StepInformation, selected_steps: set[int]):
        super().__init__(parent)
        # parameter names from stepped simulation metadata
        parameter_names = list(step_information.keys)
        # tabular rows consumed by QML
        step_rows = []
        # build one row per step with display-ready string values
        for step_index, row_values in enumerate(step_information.values):
            # append row payload
            step_rows.append({"stepIndex": step_index, "values": [str(value) for value in row_values]})
        # store selected steps
        self._selected_steps: set[int] = selected_steps
        # compute system theme colors
        theme = _ThemeColors()
        # context properties consumed by QML
        self._ctx_properties = {
            "parameterNames": parameter_names,
            "stepRows": step_rows,
            "initialSelectedSteps": sorted(selected_steps),
            "themeColors": {
                "background": theme.background,
                "alternateBase": theme.alternate_base,
                "text": theme.text,
                "button": theme.button,
                "buttonText": theme.button_text,
                "highlight": theme.highlight,
                "highlightedText": theme.highlighted_text,
                "mid": theme.mid,
                "midlight": theme.midlight,
                "dark": theme.dark,
                "disabledText": theme.disabled_text,
                "toolTipBase": theme.tool_tip_base,
            },
        }
        # window setup
        self.setWindowTitle("Step Tool")
        self.setWindowModality(Qt.WindowModality.WindowModal)
        self.resize(640, 430)
        self.setMinimumHeight(400)
        self.setMinimumWidth(560)
        # create QML view
        self._qml_view = QQuickView()
        self._qml_view.statusChanged.connect(self._on_qml_ready)
        self._qml_view.setResizeMode(QQuickView.ResizeMode.SizeRootObjectToView)
        self._qml_view.setColor(QColor(theme.background))
        self._qml_view.setSource(QUrl.fromLocalFile(str(_QML_FILE)))
        # embed QML view into dialog
        container = QWidget.createWindowContainer(self._qml_view, self)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(container)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status):
        # only proceed once QML has finished loading successfully
        if status != QQuickView.Status.Ready:
            return
        # root object
        root = self._qml_view.rootObject()
        # inject context properties
        for key, value in self._ctx_properties.items():
            root.setProperty(key, value)
        # initialize state in QML after properties are available
        root.initialize()
        # connect QML signals
        root.selectionChanged.connect(self._on_selection_changed)
        root.dialogAccepted.connect(self._on_dialog_accepted)
        root.dialogRejected.connect(self.reject)

    @Slot(int, bool)
    def _on_selection_changed(self, step_index: int, selected: bool):
        # check if step was selected or deselected
        if selected:
            # append index to set
            self._selected_steps.add(step_index)
            # exit
            return
        # remove index from set
        self._selected_steps.discard(step_index)

    @Slot(list)
    def _on_dialog_accepted(self, selected_steps):
        # accept dialog
        self.accept()

    @property
    def selected_steps(self) -> set[int]:
        return self._selected_steps
