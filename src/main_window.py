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
from netlist_parser import parse_netlist
from netlist_viewer_dialog import NetlistViewerDialog
from plugin_config import PluginConfig
from run_xyce_simulation import run_xyce_simulation, XyceSimulationRunner
from simulation_dialog import OpSimulationParameters, SimulationDialog
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
        self._qml_view.statusChanged.connect(self._on_qml_ready)
        self._qml_view.setResizeMode(QQuickView.ResizeMode.SizeRootObjectToView)
        self._qml_view.setColor(QColor(_BG))
        self._qml_view.setSource(QUrl.fromLocalFile(str(_QML_FILE)))
        # embed the single QWindow into the main window's central area
        self._container = QWidget.createWindowContainer(self._qml_view, self)
        # Wrap the container in a widget to handle resizing better
        self._central_widget = QWidget()
        self._layout = QVBoxLayout(self._central_widget)
        self._layout.setContentsMargins(0, 0, 0, 0)
        self._layout.setSpacing(0)
        self._layout.addWidget(self._container)
        # hide the built-in QMainWindow status bar so it never takes any space
        self.statusBar().hide()
        self.setCentralWidget(self._central_widget)
        # create the native main menu structure
        self._create_main_menu()
        # create the native toolbar
        self._create_toolbar()
        # timer used to auto-clear timed status messages
        self._status_timer = QTimer(self)
        self._status_timer.setSingleShot(True)
        self._status_timer.timeout.connect(lambda: self._show_status(""))

    def _show_status(self, message: str, timeout_ms: int = 0) -> None:
        # display a status message as an overlay inside the QML view
        self._status_timer.stop()
        self._root.setProperty("statusText", message)
        if timeout_ms > 0:
            self._status_timer.start(timeout_ms)

    def sizeHint(self):
        return QSize(1200, 800)

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

    def _create_main_menu(self):
        # menu bar
        menu_bar = self.menuBar()

        # File menu
        file_menu = menu_bar.addMenu("&File")

        # File | Open
        open_action = QAction("Open...", self)
        open_action.setShortcut(QKeySequence.Open)
        open_action.triggered.connect(self._on_menu_open_file)
        file_menu.addAction(open_action)

        # File | Quit
        quit_action = QAction("Quit", self)
        quit_action.setShortcut(QKeySequence.Quit)
        quit_action.triggered.connect(self.close)
        file_menu.addAction(quit_action)

        # Tools menu
        tools_menu = menu_bar.addMenu("&Tools")

        # Tools | Open in JupyterLab
        jupyter_action = QAction("Open in JupyterLab...", self)
        tools_menu.addAction(jupyter_action)

        # Tools | Configuration
        config_action = QAction("Configuration...", self)
        config_action.triggered.connect(self._on_menu_configuration)
        tools_menu.addAction(config_action)

        # Window menu
        window_menu = menu_bar.addMenu("&Window")

        # Window | Add Chart
        add_chart_action = QAction(get_kicad_icon(KiCadIcon.ADD_CHART, dark=False), "Add Chart", self)
        # add_chart_action.triggered.connect(lambda: self._on_menu_add_chart(len(self._charts) - 1))
        window_menu.addAction(add_chart_action)

        # Window | New Window
        new_window_action = QAction(get_kicad_icon(KiCadIcon.NEW_WINDOW, dark=False), "New Window", self)
        # new_window_action.triggered.connect(self._on_menu_new_window)
        window_menu.addAction(new_window_action)

        # Help menu
        help_menu = menu_bar.addMenu("&Help")

        # Help | About
        about_action = QAction("About", self)
        about_action.triggered.connect(lambda: None)
        help_menu.addAction(about_action)

    def _create_toolbar(self):
        # create a native toolbar with common file operations
        toolbar = self.addToolBar("File")
        # keep toolbar position stable in the main window
        toolbar.setMovable(False)
        # set toolbar icon size for better visibility
        toolbar.setIconSize(QSize(24, 24))

        # add an open action with standard icon and platform shortcut
        open_action = QAction(get_kicad_icon(KiCadIcon.FILE_OPEN, dark=False), "Open", self)
        open_action.setShortcut(QKeySequence.StandardKey.Open)
        open_action.setToolTip("Open simulation file (Cmd+O)")
        open_action.triggered.connect(self._on_menu_open_file)
        toolbar.addAction(open_action)

        # add a save action with standard icon and platform shortcut
        save_action = QAction(get_kicad_icon(KiCadIcon.FILE_SAVE, dark=False), "Save", self)
        save_action.setShortcut(QKeySequence.StandardKey.Save)
        save_action.setToolTip("Save simulation file (Cmd+S)")
        # save_action.triggered.connect(self._on_menu_save_file)
        toolbar.addAction(save_action)

        # add a separator for visual grouping
        toolbar.addSeparator()

        # simulation command
        simulation_cmd_action = QAction(get_kicad_icon(KiCadIcon.SIM_COMMAND, dark=False), "Configure Simulation", self)
        simulation_cmd_action.setToolTip("Configure simulation parameters")
        simulation_cmd_action.triggered.connect(self._on_menu_configure_simulation)
        toolbar.addAction(simulation_cmd_action)

        # simulation run
        simulation_run_action = QAction(get_kicad_icon(KiCadIcon.SIM_RUN, dark=False), "Run Simulation", self)
        simulation_run_action.setToolTip("Run the simulation")
        simulation_run_action.triggered.connect(self._on_menu_run_simulation)
        toolbar.addAction(simulation_run_action)

        # add a separator for visual grouping
        toolbar.addSeparator()

        # show netlist
        show_netlist_action = QAction(get_kicad_icon(KiCadIcon.SHOW_NETLIST, dark=False), "Show Netlist", self)
        show_netlist_action.setToolTip("Show processed netlist")
        show_netlist_action.triggered.connect(self._on_menu_show_netlist)
        toolbar.addAction(show_netlist_action)

        # add a separator for visual grouping
        toolbar.addSeparator()

        # preference/configuration action
        config_action = QAction(get_kicad_icon(KiCadIcon.PREFERENCE, dark=False), "Configuration", self)
        config_action.setToolTip("Open plugin configuration")
        config_action.triggered.connect(self._on_menu_configuration)
        toolbar.addAction(config_action)

    @Slot()
    def _on_menu_open_file(self) -> None:
        # open netlist file
        input_path_str, _ = QFileDialog.getOpenFileName(self, "Open Netlist File", "", "Netlist Files (*.cir);;All Files (*)")
        # exit when the user cancels the dialog
        if not input_path_str:
            return
        # parse selected path
        # input_path = Path(input_path_str)

    def _populate_charts(self):
        # fall back to one empty chart when there are none
        # if not self._plot_suggestions:
        #     # add a single chart with the default type for this file, but no series (empty)
        #     self._add_chart(self._default_chart_type, [])
        #     # exit
        #     return
        # loop suggestions — each suggestion carries its own chart type
        # for suggestion in self._plot_suggestions:
        #     # append chart using the type encoded in the suggestion
        #     self._add_chart(suggestion.chart_type, suggestion.expressions)
        self._add_chart("", [])

    def _add_chart(self, chart_type: str, expressions: list[Expression]):
        # chart index
        # chart_index = len(self._charts)
        # create chart ui component in QML
        self._root.addChart()
        # get a reference to the chart's QML object so we can manipulate it
        # chart_root = self._root.getChart(chart_index)
        # # create chart instance
        # chart = Chart(chart_root, chart_type, self._expression_manager, self._abscissa, self._step_information, self._decimate_target)
        # # apply initial step selection when provided (e.g. FFT window inheriting source chart visibility)
        # if self._initial_selected_steps is not None:
        #     chart.selected_steps = self._initial_selected_steps
        # # add it to the list of charts so we can keep track of it
        # self._charts.append(chart)
        # # render chart
        # chart.render("", self._abscissa_scale.value, set(expressions))

    def _setup_netlist(self) -> str:
        # returns a complex test netlist for simulation execution
        return (
            "* Xyce Complex Test Circuit\n"
            "V1 1 0 5V\n"
            "R1 1 2 1k\n"
            "R2 2 0 2k\n"
            "R3 2 3 500\n"
            "R4 3 0 1k\n"
            ".OP\n"
            ".PRINT DC V(2) I(R1)\n"
            ".END"
        )

    @Slot(str)
    def _on_simulation_started(self, netlist_path: str, output_path: str) -> None:
        # update status to indicate simulation started
        self._show_status("Simulation started...")
        # open the log panel and clear any previous session output
        self._root.setProperty("logVisible", True)
        # clear previous logs to prepare for new simulation output
        self.logClearRequested.emit()

    @Slot(str)
    def _on_stdout_received(self, text: str) -> None:
        # append simulation output to logs or status bar
        logger.info("Xyce: %s", text)
        # append line in log panel with info styling
        self.logAppendRequested.emit(text)

    @Slot(str)
    def _on_stderr_received(self, text: str) -> None:
        # log simulation errors
        logger.error("Xyce stderr: %s", text)
        # show log in simulation log panel with error styling
        self.logAppendRequested.emit(f"ERROR: {text}")
        # update status bar
        self._show_status(f"Simulation error: {text}", 5000)

    @Slot(int, int, bool, str)
    def _on_simulation_finished(self, exit_code: int, exit_status: int, was_canceled: bool, output_path: str) -> None:
        # clean up and notify user
        if was_canceled:
            self._show_status("Simulation canceled")
        elif exit_code == 0:
            self._show_status("Simulation finished successfully")
        else:
            self._show_status(f"Simulation failed (exit code: {exit_code})", 5000)
        # release the runner reference now that simulation is complete
        self._runner = None

    def _on_menu_run_simulation(self):
        # construct the initial netlist
        netlist = self._setup_netlist()
        # parse the netlist topology
        topology = parse_netlist(netlist)
        # initialize parameters if not already set
        if self._simulation_parameters is None:
            # create from existing directives
            params = OpSimulationParameters.from_directives(topology.directives)
            # only set if directives were actually found
            if params.print_dc_enabled or params.save_enabled or params.nodeset_entries:
                self._simulation_parameters = params
        # prompt user for parameters if none are configured
        if self._simulation_parameters is None:
            # open the configuration dialog
            self._on_menu_configure_simulation()
        # return early if user cancelled parameter configuration
        if self._simulation_parameters is None:
            # stop execution
            return
        # remove existing simulation directives from the netlist
        for directive in topology.directives:
            # replace directive line with empty string
            netlist = netlist.replace(directive, "")
        # generate the simulation directive
        directive = self._simulation_parameters.to_xyce_directive(topology=topology)
        # insert directive before .END
        netlist = netlist.replace(".END", f"{directive.strip()}\n.END")
        # log simulation netlist
        logger.info("Running simulation with netlist:\n%s", netlist)
        # launch simulation and store the runner reference
        try:
            # create simulation runner
            self._runner = run_xyce_simulation(self._plugin_config, netlist)
            # wire signal handlers for UI progress updates
            self._runner.started.connect(self._on_simulation_started)
            # wire stdout signal
            self._runner.stdout_received.connect(self._on_stdout_received)
            # wire stderr signal
            self._runner.stderr_received.connect(self._on_stderr_received)
            # wire finished signal
            self._runner.finished.connect(self._on_simulation_finished)
            # start simulation process
            self._runner.start()
        # show error details
        except ValueError as e:
            # log failure
            logger.error("Simulation failed to start: %s", e)
            # update status
            self._show_status(f"Simulation failed: {e}", 5000)
            self._show_status(str(e), 5000)
            # log information for diagnostics
            logger.error("Simulation startup failed", exc_info=True)

    def _on_menu_show_netlist(self):
        # construct the initial netlist
        netlist = self._setup_netlist()
        # parse the netlist topology
        topology = parse_netlist(netlist)
        # initialize parameters if not already set
        if self._simulation_parameters is None:
            # create from existing directives
            self._simulation_parameters = OpSimulationParameters.from_directives(topology.directives)
        # remove existing simulation directives from the netlist
        for directive in topology.directives:
            # replace directive line with empty string
            netlist = netlist.replace(directive, "")
        # generate the simulation directive
        directive = self._simulation_parameters.to_xyce_directive(topology=topology)
        # insert directive before .END
        netlist = netlist.replace(".END", f"{directive.strip()}\n.END")
        # create the viewer dialog
        dialog = NetlistViewerDialog(parent=self, netlist=netlist)
        # show the dialog
        dialog.exec()

    def _on_menu_configure_simulation(self):
        # open the simulation dialog and wait for user input
        dialog = SimulationDialog(self, initial_parameters=self._simulation_parameters)
        # capture the result only when the dialog is accepted
        if dialog.exec() == SimulationDialog.Accepted:
            # update parameters
            self._simulation_parameters = dialog.get_parameters()

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
