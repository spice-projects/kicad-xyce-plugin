import logging
from pathlib import Path

from kipy import KiCad
from PySide6.QtCore import QSize, QTimer, QUrl, Signal, Slot
from PySide6.QtGui import QAction, QColor, QKeySequence
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QFileDialog, QMainWindow, QVBoxLayout, QWidget

from config_dialog import ConfigDialog
from expression import Expression
from kicad_icons import KiCadIcon, get_kicad_icon, load_kicad_icons
from netlist_parser import parse_netlist
from netlist_viewer_dialog import NetlistViewerDialog
from plugin_config import PluginConfig
from run_xyce_simulation import run_xyce_simulation
from simulation_dialog import OpSimulationParameters, SimulationDialog
from window import load_app_icon, log_screen_info

logger = logging.getLogger(__name__)

_QML_FILE = Path(__file__).parent / "main_window.qml"

_BG = "#efefe8"


class MainWindow(QMainWindow):

    logAppendRequested = Signal(str)
    logClearRequested = Signal()

    def __init__(self, kicad_client: KiCad, plugin_config: PluginConfig):
        super().__init__()
        # load icons
        load_kicad_icons()
        # load icon
        icon = load_app_icon()
        if not icon.isNull():
            self.setWindowIcon(icon)
        # store client and config
        self._kicad_client = kicad_client
        self._plugin_config = plugin_config
        # initialize state
        self._charts = []
        self._runner = None
        self._simulation_parameters = None
        self._simulation_performed = False
        self._simulation_output_action = None
        # set title
        self.setWindowTitle("Xyce Simulation - No file loaded")
        # apply style
        self.setStyleSheet(f"QMainWindow {{ background: {_BG}; }}")
        # qml view
        self._qml_view = QQuickView()
        self._qml_view.statusChanged.connect(self._on_qml_ready)
        self._qml_view.setResizeMode(QQuickView.ResizeMode.SizeRootObjectToView)
        self._qml_view.setColor(QColor(_BG))
        # set source
        source_url = QUrl.fromLocalFile(str(_QML_FILE))
        self._qml_view.setSource(source_url)
        # container
        self._container = QWidget.createWindowContainer(self._qml_view, self)
        # central widget
        self._central_widget = QWidget()
        # layout
        self._layout = QVBoxLayout(self._central_widget)
        self._layout.setContentsMargins(0, 0, 0, 0)
        self._layout.setSpacing(0)
        self._layout.addWidget(self._container)
        # status bar
        self.statusBar().hide()
        # set central
        self.setCentralWidget(self._central_widget)
        # menu
        self._create_main_menu()
        # toolbar
        self._create_toolbar()
        # timer
        self._status_timer = QTimer(self)
        # set single shot
        self._status_timer.setSingleShot(True)
        # connect
        self._status_timer.timeout.connect(lambda: self._show_status(""))

    def _show_status(self, message: str, timeout_ms: int = 0) -> None:
        # stop timer
        self._status_timer.stop()
        # set text
        self._root.setProperty("statusText", message)
        # check timeout
        if timeout_ms > 0:
            # start timer
            self._status_timer.start(timeout_ms)

    def sizeHint(self):
        # return size
        return QSize(1200, 800)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status):
        # check ready
        if status != QQuickView.Status.Ready:
            # return
            return
        # root object
        self._root = self._qml_view.rootObject()
        # set property
        self._root.setProperty("fftVisible", False)
        # set property
        self._root.setProperty("stepToolVisible", False)
        # set property
        self._root.setProperty("smithChartVisible", False)
        # connect signal
        self.logAppendRequested.connect(self._root.logAppendRequested)
        # connect signal
        self.logClearRequested.connect(self._root.logClearRequested)
        # single shot
        QTimer.singleShot(0, self._populate_charts)
        # check log level
        is_debug = logger.isEnabledFor(logging.DEBUG)
        # check debug
        if is_debug:
            # single shot
            QTimer.singleShot(0, lambda: log_screen_info(self.screen()))

    def _create_main_menu(self):
        # menu bar
        menu_bar = self.menuBar()
        # file menu
        file_menu = menu_bar.addMenu("&File")

        # open action
        open_action = QAction("Open...", self)
        open_action.setShortcut(QKeySequence.Open)
        open_action.triggered.connect(self._on_menu_open_file)
        file_menu.addAction(open_action)

        # quit action
        quit_action = QAction("Quit", self)
        quit_action.setShortcut(QKeySequence.Quit)
        quit_action.triggered.connect(self.close)
        file_menu.addAction(quit_action)

        # tools menu
        tools_menu = menu_bar.addMenu("&Tools")

        # jupyter action
        jupyter_action = QAction("Open in JupyterLab...", self)
        tools_menu.addAction(jupyter_action)

        # view output action
        self._simulation_output_action = QAction("View Simulation Output", self)
        self._simulation_output_action.setEnabled(False)
        self._simulation_output_action.triggered.connect(self._on_menu_view_simulation_output)
        tools_menu.addAction(self._simulation_output_action)

        # config action
        config_action = QAction("Configuration...", self)
        config_action.triggered.connect(self._on_menu_configuration)
        tools_menu.addAction(config_action)

        # window menu
        window_menu = menu_bar.addMenu("&Window")

        # add chart action
        icon_add = get_kicad_icon(KiCadIcon.ADD_CHART, dark=False)
        add_chart_action = QAction(icon_add, "Add Chart", self)
        window_menu.addAction(add_chart_action)

        # new window action
        icon_new = get_kicad_icon(KiCadIcon.NEW_WINDOW, dark=False)
        new_window_action = QAction(icon_new, "New Window", self)
        window_menu.addAction(new_window_action)

        # help menu
        help_menu = menu_bar.addMenu("&Help")

        # about action
        about_action = QAction("About", self)
        about_action.triggered.connect(lambda: None)
        help_menu.addAction(about_action)

    def _create_toolbar(self):
        # toolbar
        toolbar = self.addToolBar("File")
        # set movable
        toolbar.setMovable(False)
        # icon size
        toolbar.setIconSize(QSize(24, 24))
        # open action
        open_action = QAction(get_kicad_icon(KiCadIcon.FILE_OPEN, dark=False), "Open", self)
        # shortcut
        open_action.setShortcut(QKeySequence.StandardKey.Open)
        # tooltip
        open_action.setToolTip("Open simulation file (Cmd+O)")
        # connect
        open_action.triggered.connect(self._on_menu_open_file)
        # add
        toolbar.addAction(open_action)
        # save action
        save_action = QAction(get_kicad_icon(KiCadIcon.FILE_SAVE, dark=False), "Save", self)
        # shortcut
        save_action.setShortcut(QKeySequence.StandardKey.Save)
        # tooltip
        save_action.setToolTip("Save simulation file (Cmd+S)")
        # add
        toolbar.addAction(save_action)
        # separator
        toolbar.addSeparator()
        # sim command
        simulation_cmd_action = QAction(get_kicad_icon(KiCadIcon.SIM_COMMAND, dark=False), "Configure Simulation", self)
        # tooltip
        simulation_cmd_action.setToolTip("Configure simulation parameters")
        # connect
        simulation_cmd_action.triggered.connect(self._on_menu_configure_simulation)
        # add
        toolbar.addAction(simulation_cmd_action)
        # sim run
        simulation_run_action = QAction(get_kicad_icon(KiCadIcon.SIM_RUN, dark=False), "Run Simulation", self)
        # tooltip
        simulation_run_action.setToolTip("Run the simulation")
        # connect
        simulation_run_action.triggered.connect(self._on_menu_run_simulation)
        # add
        toolbar.addAction(simulation_run_action)
        # separator
        toolbar.addSeparator()
        # show netlist
        icon_netlist = get_kicad_icon(KiCadIcon.SHOW_NETLIST, dark=False)
        show_netlist_action = QAction(icon_netlist, "Show Netlist", self)
        # tooltip
        show_netlist_action.setToolTip("Show processed netlist")
        # connect
        show_netlist_action.triggered.connect(self._on_menu_show_netlist)
        # add
        toolbar.addAction(show_netlist_action)
        # separator
        toolbar.addSeparator()
        # config action
        icon_pref = get_kicad_icon(KiCadIcon.PREFERENCE, dark=False)
        config_action = QAction(icon_pref, "Configuration", self)
        # tooltip
        config_action.setToolTip("Open plugin configuration")
        # connect
        config_action.triggered.connect(self._on_menu_configuration)
        # add
        toolbar.addAction(config_action)

    @Slot()
    def _on_menu_open_file(self) -> None:
        # get file
        res = QFileDialog.getOpenFileName(self, "Open Netlist File", "", "Netlist Files (*.cir);;All Files (*)")
        # path
        input_path_str = res[0]
        # check
        if not input_path_str:
            # return
            return

    def _populate_charts(self):
        # charts
        self._add_chart("", [])

    def _add_chart(self, chart_type: str, expressions: list[Expression]):
        # add
        self._root.addChart()

    def _setup_netlist(self) -> str:
        # line1
        l1 = "* Xyce Complex Test Circuit\n"
        # line2
        l2 = "V1 1 0 5V\n"
        # line3
        l3 = "R1 1 2 1k\n"
        # line4
        l4 = "R2 2 0 2k\n"
        # line5
        l5 = "R3 2 3 500\n"
        # line6
        l6 = "R4 3 0 1k\n"
        # line7
        l7 = ".OP\n"
        # line8
        l8 = ".PRINT DC V(2) I(R1)\n"
        # line9
        l9 = ".END"
        # netlist
        netlist = l1 + l2 + l3 + l4 + l5 + l6 + l7 + l8 + l9
        # return
        return netlist

    def _on_menu_view_simulation_output(self) -> None:
        # check root
        if self._root:
            # toggle visibility
            visible = self._root.property("logVisible")
            # set property
            self._root.setProperty("logVisible", not visible)

    @Slot(str, str)
    def _on_simulation_started(self, netlist_path: str, output_path: str) -> None:
        # status
        self._show_status("Simulation started...")
        # set flag
        self._simulation_performed = True
        # enable action
        if self._simulation_output_action:
            # enable
            self._simulation_output_action.setEnabled(True)
        # log
        self._root.setProperty("logVisible", True)
        # clear
        self.logClearRequested.emit()

    @Slot(str)
    def _on_stdout_received(self, text: str) -> None:
        # log
        logger.info("Xyce: %s", text)
        # log
        self.logAppendRequested.emit(text)

    @Slot(str)
    def _on_stderr_received(self, text: str) -> None:
        # log
        logger.error("Xyce stderr: %s", text)
        # error
        msg = f"ERROR: {text}"
        self.logAppendRequested.emit(msg)
        # status
        self._show_status(f"Simulation error: {text}", 5000)

    @Slot(int, int, bool, str)
    def _on_simulation_finished(self, exit_code: int, exit_status: int, was_canceled: bool, output_path: str) -> None:
        # check
        if was_canceled:
            # status
            self._show_status("Simulation canceled")
        elif exit_code == 0:
            # status
            self._show_status("Simulation finished successfully")
        else:
            # status
            err_msg = f"Simulation failed (exit code: {exit_code})"
            self._show_status(err_msg, 5000)
        # runner
        self._runner = None

    def _on_menu_run_simulation(self):
        # netlist
        netlist = self._setup_netlist()
        # topology
        topology = parse_netlist(netlist)
        # check
        if self._simulation_parameters is None:
            # params
            params = OpSimulationParameters.from_directives(topology.directives)
            # check
            if params.print_dc_enabled or params.save_enabled or params.nodeset_entries:
                # set
                self._simulation_parameters = params
        # check
        if self._simulation_parameters is None:
            # config
            self._on_menu_configure_simulation()
        # check
        if self._simulation_parameters is None:
            # return
            return
        # directives
        for directive in topology.directives:
            # replace
            netlist = netlist.replace(directive, "")
        # generate
        directive = self._simulation_parameters.to_xyce_directive(topology=topology)
        # insert
        netlist = netlist.replace(".END", f"{directive.strip()}\n.END")
        # log
        logger.info("Running simulation with netlist:\n%s", netlist)
        # try
        try:
            # runner
            self._runner = run_xyce_simulation(self._plugin_config, netlist)
            # signal
            self._runner.started.connect(self._on_simulation_started)
            # signal
            self._runner.stdout_received.connect(self._on_stdout_received)
            # signal
            self._runner.stderr_received.connect(self._on_stderr_received)
            # signal
            self._runner.finished.connect(self._on_simulation_finished)
            # start
            self._runner.start()
        # except
        except ValueError as e:
            # log
            logger.error("Simulation failed to start: %s", e)
            # status
            self._show_status(f"Simulation failed: {e}", 5000)
            self._show_status(str(e), 5000)
            # log
            logger.error("Simulation startup failed", exc_info=True)

    def _on_menu_show_netlist(self):
        # netlist
        netlist = self._setup_netlist()
        # topology
        topology = parse_netlist(netlist)
        # check
        if self._simulation_parameters is None:
            # params
            self._simulation_parameters = OpSimulationParameters.from_directives(topology.directives)
        # directives
        for directive in topology.directives:
            # replace
            netlist = netlist.replace(directive, "")
        # generate
        directive = self._simulation_parameters.to_xyce_directive(topology=topology)
        # insert
        netlist = netlist.replace(".END", f"{directive.strip()}\n.END")
        # dialog
        dialog = NetlistViewerDialog(parent=self, netlist=netlist)
        # exec
        dialog.exec()

    def _on_menu_configure_simulation(self):
        # dialog
        dialog = SimulationDialog(self, initial_parameters=self._simulation_parameters)
        # result
        params = dialog.get_parameters()
        # check
        if params is not None:
            # params
            self._simulation_parameters = params

    def _on_menu_configuration(self):
        # dialog
        dialog = ConfigDialog(self, self._plugin_config)
        # result
        config = dialog.get_config()
        # check
        if config is None:
            # return
            return
        # store
        self._plugin_config = config
        # log
        logger.info("Configured Xyce executable path: %s", self._plugin_config.xyce_executable_path)
        # status
        self.statusBar().showMessage("Plugin configuration updated", 3000)
