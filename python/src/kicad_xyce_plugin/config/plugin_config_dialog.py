from pathlib import Path

from PySide6.QtCore import Qt, QUrl, Slot
from PySide6.QtGui import QColor
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QDialog, QFileDialog, QVBoxLayout, QWidget

from .plugin_config import PluginConfig

_QML_FILE = Path(__file__).parent / "plugin_config_dialog.qml"
_BG = "#efefe8"


class PluginConfigDialog(QDialog):

    def __init__(self, parent: QWidget | None, initial_config: PluginConfig):
        super().__init__(parent)
        # set modal
        self.setWindowModality(Qt.ApplicationModal)
        # capture initial config for form defaults
        self._initial_config = initial_config
        # keep accepted result available to caller
        self._result: PluginConfig | None = None
        # set the native frame title
        self.setWindowTitle("Plugin Configuration")
        # create qml scene for the form UI
        self._qml_view = QQuickView()
        # connect qml load status to setup hook
        self._qml_view.statusChanged.connect(self._on_qml_ready)
        # keep qml root item sized to container
        self._qml_view.setResizeMode(QQuickView.ResizeMode.SizeRootObjectToView)
        # align dialog background with existing UI style
        self._qml_view.setColor(QColor(_BG))
        # load qml source file
        self._qml_view.setSource(QUrl.fromLocalFile(str(_QML_FILE)))
        # host qquickview inside qdialog widget layout
        self._container = QWidget.createWindowContainer(self._qml_view, self)
        # create top-level layout for the container
        self._layout = QVBoxLayout(self)
        # remove margins to avoid double padding around qml content
        self._layout.setContentsMargins(0, 0, 0, 0)
        # insert qml container into dialog layout
        self._layout.addWidget(self._container)
        # apply initial dialog dimensions
        self.resize(640, 320)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status) -> None:
        # wait for qml to fully initialize before property access
        if status != QQuickView.Status.Ready:
            return
        # capture qml root object for signal and property interaction
        self._root = self._qml_view.rootObject()
        # populate path field from current configuration
        self._root.setProperty("xyceExecutablePath", self._initial_config.xyce_executable_path)
        # clear any stale validation message on open
        self._root.setProperty("errorText", "")
        # connect qml browse signal to native file chooser
        self._root.browseRequested.connect(self._on_browse_requested)
        # connect qml submit signal to validation and persistence
        self._root.submit.connect(self._on_submit)
        # connect qml cancel signal to reject dialog
        self._root.cancelRequested.connect(self.reject)

    @Slot()
    def _on_browse_requested(self) -> None:
        # open native file picker for selecting the xyce executable
        selected_file, _ = QFileDialog.getOpenFileName(self, "Select Xyce Executable", self._initial_config.xyce_executable_path or "")
        # keep existing field value when user cancels selection
        if not selected_file:
            return
        # push selected path back into qml field
        self._root.setProperty("xyceExecutablePath", selected_file)
        # clear any previous validation message after a new selection
        self._root.setProperty("errorText", "")

    @Slot(str)
    def _on_submit(self, xyce_executable_path: str) -> None:
        # normalize path text before validation
        normalized_path = xyce_executable_path.strip()
        # require a non-empty path so plugin can launch xyce
        if not normalized_path:
            # render validation feedback in dialog
            self._root.setProperty("errorText", "Xyce executable path is required")
            # keep dialog open for correction
            return
        # create typed configuration object from user input
        config = PluginConfig(xyce_executable_path=normalized_path)
        # reject path values that are not executable files
        if not config.is_xyce_executable_valid():
            # render validation feedback in dialog
            self._root.setProperty("errorText", "Selected path is not an executable file")
            # keep dialog open for correction
            return
        # persist validated configuration
        config.save()
        # capture result for caller after successful save
        self._result = config
        # close dialog with accepted status
        self.accept()

    def get_config(self) -> PluginConfig:
        return self._result
