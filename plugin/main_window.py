import logging
from pathlib import Path

from kipy import KiCad
from PySide6.QtCore import QSize, QTimer, QUrl, Slot, Signal
from PySide6.QtGui import QAction, QColor, QKeySequence
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QFileDialog, QMainWindow, QWidget, QVBoxLayout

from config_dialog import ConfigDialog
from expression import Expression
from kicad_icons import get_kicad_icon, KiCadIcon, load_kicad_icons
from plugin_config import PluginConfig
from run_xyce_simulation import run_xyce_simulation, XyceSimulationRunner
from simulation_dialog import SimulationDialog
from window import load_app_icon, log_screen_info

logger = logging.getLogger(__name__)

_QML_FILE = Path(__file__).parent / "main_window.qml"

# background color matching the KiCad schematic window
_BG = "#efefe8"


class MainWindow(QMainWindow):

    # signals for log panel updates
    logAppendRequested = Signal(str)
    logClearRequested = Signal()

    def __init__(self, kicad_client: KiCad, plugin_config: PluginConfig):
        super().__init__()
        # load kicad icons
        load_kicad_icons()
        # load and set the application icon
        icon = load_app_icon()
        if not icon.isNull():
            self.setWindowIcon(icon)
        # store fields
        self._kicad_client = kicad_client
        self._plugin_config = plugin_config

        # initialize data structures
        self._charts = []  # : list[Chart] = []
        self._runner: XyceSimulationRunner | None = None
        # store currently selected simulation parameters from the dialog
        self._simulation_parameters = None

        # set window title to include the loaded filename
        self.setWindowTitle("Xyce Simulation - No file loaded")
        # apply dark background stylesheet to the window chrome
        self.setStyleSheet(f"QMainWindow {{ background: {_BG}; }}")
        # single QQuickView hosts the entire multi-chart scene — one Metal swap chain
        self._qml_view = QQuickView()
        # connect qml-ready lifecycle hook before loading qml
        self._qml_view.statusChanged.connect(self._on_qml_ready)
        # keep the qml root sized to the embedded window container
        self._qml_view.setResizeMode(QQuickView.ResizeMode.SizeRootObjectToView)
        # match the window background to the rest of the application
        self._qml_view.setColor(QColor(_BG))
        # load the simulation dialog qml component
        self._qml_view.setSource(QUrl.fromLocalFile(str(_QML_FILE)))
        # wrap the qml view in a widget so it can be hosted by qdialog
        self._container = QWidget.createWindowContainer(self._qml_view, self)
        # set up a simple one-widget dialog layout
        self._layout = QVBoxLayout(self)
        # remove margins so qml controls align edge-to-edge
        self._layout.setContentsMargins(0, 0, 0, 0)
        # insert the qml container into the dialog layout
        self._layout.addWidget(self._container)
        # set an initial size that fits both tab forms on first open
        self.resize(640, 680)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status):
        # only proceed once QML has finished loading successfully
        if status != QQuickView.Status.Ready:
            return
        # qml view root object
        self._root = self._qml_view.rootObject()
        # analyze expressions to enable/disable Smith Chart support
        # smith_chart_expressions = [expression for expression in self._expression_manager.expressions if expression.name.startswith(("S11", "S22")) and expression.variable_type == "parameter"]
        # set window-level menu capability flags using built-in bool to avoid passing numpy.bool into QML properties
        self._root.setProperty("fftVisible", False)  # bool(self._abscissa.unit == "s"))
        self._root.setProperty("stepToolVisible", False)  # bool(self._step_information.length > 1))
        self._root.setProperty("smithChartVisible", False)  # len(smith_chart_expressions) > 0)
        # connect signals from QML to Python handlers
        # self._root.zoomRegionSelected.connect(self._on_zoom_region_selected)
        # self._root.menuZoomToFit.connect(self._on_menu_zoom_to_fit)
        # self._root.menuAutorange.connect(self._on_menu_autorange)
        # self._root.menuZoomAbscissaExtent.connect(self._on_menu_zoom_abscissa_extent)
        # self._root.menuAddRemovePlots.connect(self._on_menu_add_remove_plots)
        # self._root.menuDeleteAllPlots.connect(self._on_menu_delete_all_plots)
        # self._root.menuAddChart.connect(self._on_menu_add_chart)
        # self._root.menuDeleteChart.connect(self._on_menu_delete_chart)
        # self._root.menuNewWindow.connect(self._on_menu_new_window)
        # self._root.menuFft.connect(self._on_menu_fft)
        # self._root.menuStepTool.connect(self._on_menu_step_tool)
        # self._root.menuSmithChart.connect(self._on_menu_smith_chart)
        # connect pointer hover signals to update the status bar
        # self._root.pointerMoved.connect(self._on_pointer_moved)
        # self._root.pointerExited.connect(self._on_pointer_exited)

        # wire custom log signals to qml panel
        self.logAppendRequested.connect(self._root.logAppendRequested)
        self.logClearRequested.connect(self._root.logClearRequested)

        # populate charts after the event loop starts so the window is visible first

        QTimer.singleShot(0, self._populate_charts)
        # log screen information for debugging purposes
        if logger.isEnabledFor(logging.DEBUG):
            QTimer.singleShot(0, lambda: log_screen_info(self.screen()))

    def _setup_netlist(self) -> str:
        # returns a placeholder netlist for simulation execution
        return "* Xyce Simulation\nV1 1 0 5V\nR1 1 0 1k\n.END"

    @Slot(str)
    def _on_simulation_started(self, netlist_path: str, output_path: str) -> None:
        # update status bar to indicate simulation started
        self.statusBar().showMessage("Simulation started...")
        # open the log panel and clear any previous session output
        self._root.setProperty("logVisible", True)
        self.logClearRequested.emit()

    @Slot(str)
    def _on_stdout_received(self, text: str) -> None:
        # append simulation output to logs or status bar
        logger.info("Xyce: %s", text)
        self.logAppendRequested.emit(text)

    @Slot(str)
    def _on_stderr_received(self, text: str) -> None:
        # log simulation errors
        logger.error("Xyce stderr: %s", text)
        self.logAppendRequested.emit(f"ERROR: {text}")
        self.statusBar().showMessage(f"Simulation error: {text}", 5000)

    @Slot(int, int, bool, str)
    def _on_simulation_finished(self, exit_code: int, exit_status: int, was_canceled: bool, output_path: str) -> None:
        # clean up and notify user
        if was_canceled:
            self.statusBar().showMessage("Simulation canceled")
        elif exit_code == 0:
            self.statusBar().showMessage("Simulation finished successfully")
        else:
            self.statusBar().showMessage(f"Simulation failed (exit code: {exit_code})", 5000)
        # release the runner reference now that simulation is complete
        self._runner = None

    def _on_menu_run_simulation(self):
        # prompt user for parameters if none are configured
        if self._simulation_parameters is None:
            self._on_menu_configure_simulation()
        # return early if user cancelled parameter configuration
        if self._simulation_parameters is None:
            return
        # construct the full netlist with the user-selected directive
        directive = self._simulation_parameters.to_xyce_directive()
        # insert directive before .END
        netlist = self._setup_netlist().replace(".END", f"{directive}\n.END")
        # log simulation netlist
        logger.info("Running simulation with netlist:\n%s", netlist)
        # launch simulation and store the runner reference
        try:
            self._runner = run_xyce_simulation(self._plugin_config, netlist)
            # wire signal handlers for UI progress updates
            self._runner.started.connect(self._on_simulation_started)
            self._runner.stdout_received.connect(self._on_stdout_received)
            self._runner.stderr_received.connect(self._on_stderr_received)
            self._runner.finished.connect(self._on_simulation_finished)
        except ValueError as e:
            # report configuration errors
            self.statusBar().showMessage(str(e), 5000)
            logger.error("Simulation startup failed: %s", e)

    def _on_menu_configure_simulation(self):
        # open the simulation dialog and wait for user input
        dialog = SimulationDialog(self, initial_parameters=self._simulation_parameters)
        # capture the result only when the dialog is accepted
        simulation_parameters = dialog.get_parameters()
        # keep the existing configuration when the dialog is canceled
        if simulation_parameters is None:
            return
        # store the latest parameters for future simulation execution
        self._simulation_parameters = simulation_parameters
        # log a netlist-ready directive so simulation wiring can reuse it later
        logger.info("Configured Xyce simulation directive: %s", simulation_parameters.to_xyce_directive())
        # show immediate confirmation in the status bar for the user
        self.statusBar().showMessage("Simulation parameters updated", 3000)

    def _on_menu_configuration(self):
        # open plugin configuration dialog with current values
        dialog = ConfigDialog(self, self._plugin_config)
        # collect updated configuration from dialog result
        config = dialog.get_config()
        # keep existing config when dialog is canceled
        if config is None:
            return
        # store latest settings in memory for subsequent actions
        self._plugin_config = config
        # log configured executable path for diagnostics
        logger.info("Configured Xyce executable path: %s", self._plugin_config.xyce_executable_path)
        # show immediate confirmation in status bar
        self.statusBar().showMessage("Plugin configuration updated", 3000)
