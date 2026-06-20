import logging
import subprocess
import tempfile
from pathlib import Path

from kipy import KiCad
from PySide6.QtCore import QSize, QTimer, QUrl, Signal, Slot
from PySide6.QtGui import QAction, QColor, QGuiApplication, QKeySequence, QScreen
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QFileDialog, QMainWindow, QVBoxLayout, QWidget

from .add_plot_dialog import AddPlotDialog
from .chart import Chart
from .config import PluginConfig, PluginConfigDialog
from .expression import Expression, ExpressionManager
from .kicad_icons import KiCadIcon, get_kicad_icon, load_kicad_icons
from .kicad import get_active_schematic_path
from .netlist_parser import NetlistTopology, parse_netlist
from .netlist_viewer_dialog import NetlistViewerDialog
from .run_xyce_simulation import run_xyce_simulation, XyceSimulationRunner
from .smith_chart_window import SmithChartWindow
from .simulation_parameters import from_xyce_directives, OpSimulationParameters, SimulationConfig, SimulationParametersDialog
from .step_tool_dialog import StepToolDialog
from .window import load_app_icon, log_screen_info, register_child_window
from .xyce_raw_file import AbscissaScale, StepInformation, XyceRawFile

logger = logging.getLogger(__name__)

_QML_FILE = Path(__file__).parent / "main_window.qml"

_BG = "#efefe8"


_FALLBACK_DECIMATE_TARGET = 9600


def _compute_decimate_target(screen: QScreen) -> int:
    # return conservative fallback when no screen is available (headless / early startup)
    if screen is None:
        return _FALLBACK_DECIMATE_TARGET
    # physical pixels: width × clamped device-pixel ratio
    return screen.size().width() * max(5, int(screen.devicePixelRatio()))


class MainWindow(QMainWindow):

    log_append_requested = Signal(str)
    log_clear_requested = Signal()

    def __init__(self, kicad_client: KiCad | None, plugin_config: PluginConfig, raw_file: XyceRawFile | None = None, raw_file_path: Path | None = None):
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
        # schematic last modification time (only in KiCad plugin mode, used for change detection and auto-reloading)
        self._schematic_last_modified: float | None = None
        # initialize state
        self._charts: list[Chart] = []
        self._runner: XyceSimulationRunner | None = None
        self._simulation_parameters: SimulationConfig | None = None
        self._simulation_performed: bool = False
        self._simulation_output_action: QAction | None = None
        self._simulation_config_action: QAction | None = None
        self._simulation_run_action: QAction | None = None
        self._show_netlist_action: QAction | None = None
        # netlist file opened from the filesystem in standalone mode
        self._netlist: str | None = None
        self._netlist_file_path: Path | None = None
        self._topology: NetlistTopology | None = None
        # simulation result state
        self._raw_file: XyceRawFile | None = raw_file
        self._expression_manager: ExpressionManager | None = raw_file.expression_manager if raw_file else None
        self._abscissa: Expression | None = raw_file.abscissa if raw_file else None
        self._abscissa_scale: AbscissaScale | None = raw_file.abscissa_scale if raw_file else None
        self._step_information: StepInformation | None = raw_file.step_information if raw_file else None
        # store the simulation file path for use by the Jupyter integration
        self._raw_file_path = raw_file_path if raw_file_path is not None else raw_file.filename if raw_file else None
        # optional initial step selection applied when charts are first created (used by FFT windows to pre-focus on the same steps the user was viewing in the source chart)
        self._initial_selected_steps: set[int] | None = None
        # set title
        self.setWindowTitle(f"Xyce Simulation - {self._raw_file_path.name if self._raw_file_path else 'KiCad Schematic' if self._kicad_client is not None else '<No netlist loaded>'}")
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
        # decimation target — physical pixels of the primary screen width
        self._decimate_target = _compute_decimate_target(QGuiApplication.primaryScreen())

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

    def _create_main_menu(self):
        # menu bar
        menu_bar = self.menuBar()
        # file menu
        file_menu = menu_bar.addMenu("&File")

        # no file operations on plugin mode, rely on KiCad's file management instead
        if self._kicad_client is None:
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
        add_chart_action = QAction(get_kicad_icon(KiCadIcon.ADD_CHART, dark=False), "Add Chart", self)
        add_chart_action.triggered.connect(lambda: self._on_menu_add_chart(len(self._charts) - 1))
        window_menu.addAction(add_chart_action)

        # new window action
        new_window_action = QAction(get_kicad_icon(KiCadIcon.NEW_WINDOW, dark=False), "New Window", self)
        new_window_action.triggered.connect(self._on_menu_new_window)
        window_menu.addAction(new_window_action)

        # help menu
        help_menu = menu_bar.addMenu("&Help")

        # about action
        about_action = QAction("About", self)
        about_action.triggered.connect(lambda: None)
        help_menu.addAction(about_action)

    def _create_toolbar(self):
        # toolbar
        toolbar = self.addToolBar("")
        toolbar.setMovable(False)
        toolbar.setIconSize(QSize(24, 24))

        # no file operations on plugin mode, rely on KiCad's file management instead
        if self._kicad_client is None:
            # open action
            open_action = QAction(get_kicad_icon(KiCadIcon.FILE_OPEN, dark=False), "Open", self)
            open_action.setShortcut(QKeySequence.StandardKey.Open)
            open_action.setToolTip("Open simulation file (Cmd+O)")
            open_action.triggered.connect(self._on_menu_open_file)
            toolbar.addAction(open_action)
            # save action
            save_action = QAction(get_kicad_icon(KiCadIcon.FILE_SAVE, dark=False), "Save", self)
            save_action.setShortcut(QKeySequence.StandardKey.Save)
            save_action.setToolTip("Save simulation file (Cmd+S)")
            toolbar.addAction(save_action)
            # separator
            toolbar.addSeparator()

        # show netlist
        self._show_netlist_action = QAction(get_kicad_icon(KiCadIcon.SHOW_NETLIST, dark=False), "Show Netlist", self)
        self._show_netlist_action.setEnabled(self._kicad_client is not None)
        self._show_netlist_action.setToolTip("Show processed netlist")
        self._show_netlist_action.triggered.connect(self._on_menu_show_netlist)
        toolbar.addAction(self._show_netlist_action)

        # simulation settings
        self._simulation_config_action = QAction(get_kicad_icon(KiCadIcon.SIM_CONFIG, dark=False), "Configure Simulation", self)
        self._simulation_config_action.setEnabled(self._kicad_client is not None)
        self._simulation_config_action.setToolTip("Configure simulation parameters")
        self._simulation_config_action.triggered.connect(self._on_menu_configure_simulation)
        toolbar.addAction(self._simulation_config_action)

        # simulation run
        self._simulation_run_action = QAction(get_kicad_icon(KiCadIcon.SIM_RUN, dark=False), "Run Simulation", self)
        self._simulation_run_action.setEnabled(self._kicad_client is not None)
        self._simulation_run_action.setToolTip("Run the simulation")
        self._simulation_run_action.triggered.connect(self._on_menu_run_simulation)
        toolbar.addAction(self._simulation_run_action)

        # separator
        toolbar.addSeparator()

        # config action
        config_action = QAction(get_kicad_icon(KiCadIcon.PREFERENCE, dark=False), "Configuration", self)
        config_action.setToolTip("Open plugin configuration")
        config_action.triggered.connect(self._on_menu_configuration)        # add
        toolbar.addAction(config_action)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status):
        # check ready
        if status != QQuickView.Status.Ready:
            # return
            return
        # root object
        self._root = self._qml_view.rootObject()
        # set window-level menu capability flags using built-in bool to avoid passing numpy.bool into QML properties
        self._root.setProperty("fftVisible", bool(self._abscissa and self._abscissa.unit == "s"))
        self._root.setProperty("stepToolVisible", bool(self._step_information and self._step_information.length > 1))
        self._root.setProperty("smithChartVisible", False)
        # connect signals from QML to Python handlers
        self._root.zoomRegionSelected.connect(self._on_zoom_region_selected)
        self._root.menuZoomToFit.connect(self._on_menu_zoom_to_fit)
        self._root.menuAutorange.connect(self._on_menu_autorange)
        self._root.menuZoomAbscissaExtent.connect(self._on_menu_zoom_abscissa_extent)
        self._root.menuAddRemovePlots.connect(self._on_menu_add_remove_plots)
        self._root.menuDeleteAllPlots.connect(self._on_menu_delete_all_plots)
        self._root.menuAddChart.connect(self._on_menu_add_chart)
        self._root.menuDeleteChart.connect(self._on_menu_delete_chart)
        self._root.menuNewWindow.connect(self._on_menu_new_window)
        # self._root.menuFft.connect(self._on_menu_fft)
        self._root.menuStepTool.connect(self._on_menu_step_tool)
        # self._root.menuSmithChart.connect(self._on_menu_smith_chart)
        self.log_append_requested.connect(self._root.logAppendRequested)
        self.log_clear_requested.connect(self._root.logClearRequested)
        # log screen information for debugging purposes
        if logger.isEnabledFor(logging.DEBUG):
            QTimer.singleShot(0, lambda: log_screen_info(self.screen()))

    @Slot(int, float, float, float, float)
    def _on_zoom_region_selected(self, chart_index: int, x_left_ratio: float, y_top_ratio: float, x_right_ratio: float, y_bottom_ratio: float):
        # log information
        logger.debug("User requested zoom region on chart at index: %d, rectangle: (%.3f, %.3f) to (%.3f, %.3f)", chart_index, x_left_ratio, y_top_ratio, x_right_ratio, y_bottom_ratio)
        # update charts
        for index, chart in enumerate(self._charts):
            # check if this is the chart that triggered the zoom to fit action
            if index == chart_index:
                # reset zoom window
                chart.update_zoom_window(min(x_left_ratio, x_right_ratio), max(x_left_ratio, x_right_ratio), min(y_top_ratio, y_bottom_ratio), max(y_top_ratio, y_bottom_ratio))
                # next
                continue
            # update horizontal zoom window only, keep vertical zoom as is
            chart.update_zoom_window(min(x_left_ratio, x_right_ratio), max(x_left_ratio, x_right_ratio), None, None)

    @Slot(int)
    def _on_menu_zoom_to_fit(self, chart_index: int):
        # log information
        logger.debug("User requested zoom to fit on chart at index: %d", chart_index)
        # update charts
        for index, chart in enumerate(self._charts):
            # check if this is the chart that triggered the zoom to fit action
            if index == chart_index:
                # reset zoom window
                chart.reset_zoom_window(True, True)
                # next
                continue
            # update horizontal zoom window only, keep vertical zoom as is
            chart.reset_zoom_window(True, False)

    @Slot(int)
    def _on_menu_autorange(self, chart_index: int):
        # log information
        logger.debug("User requested autorange on chart at index: %d", chart_index)
        # find chart at index
        chart = self._charts[chart_index]
        # reset zoom window
        chart.reset_zoom_window(False, True)

    @Slot(int)
    def _on_menu_zoom_abscissa_extent(self, chart_index: int):
        # log information
        logger.debug("User requested zoom abscissa extent on chart at index: %d", chart_index)
        # update charts
        for chart in self._charts:
            # update zoom window
            chart.reset_zoom_window(True, False)

    @Slot(int)
    def _on_menu_add_remove_plots(self, chart_index: int):
        # log information
        logger.debug("User requested adding/removing plots on chart at index: %d", chart_index)
        # find chart at index
        chart = self._charts[chart_index]
        # open the add plot dialog
        dialog = AddPlotDialog(self, self._expression_manager, chart.expressions)
        # exit if the user cancelled
        if dialog.exec() != AddPlotDialog.DialogCode.Accepted:
            return
        # plot selected expressions on the chart
        chart.plot_series(dialog.selected_expressions)
        # auto range axes to include the newly added series (wait for QT event loop)
        QTimer.singleShot(250, lambda: (chart.auto_range()))

    @Slot(int)
    def _on_menu_delete_all_plots(self, chart_index: int):
        # log information
        logger.debug("User requested deleting all plots on chart at index: %d", chart_index)
        # find chart
        chart = self._charts[chart_index]
        # clear chart
        chart.clear()

    @Slot(int)
    def _on_menu_add_chart(self, chart_index: int):
        # log information
        logger.debug("User requested adding a new chart at index: %d", chart_index)
        # add a new chart with no pre-populated expressions
        self._add_chart()

    @Slot(int)
    def _on_menu_delete_chart(self, chart_index: int):
        # log information
        logger.debug("User requested deleting chart at index: %d", chart_index)
        # delete chart at index (do ot swap these two statements, C++ objects get deleted immediately when their Python reference is deleted, so we need to remove the chart from the UI before deleting the Python object)
        self._root.removeChart(chart_index)
        # delete it from the list
        del self._charts[chart_index]

    @Slot()
    def _on_menu_new_window(self):
        # log information
        logger.debug("User requested opening a new window")
        # create a new independent main window sharing the same source data and path
        new_window = MainWindow(self._kicad_client, self._plugin_config, self._raw_file, self._raw_file_path)
        # keep reference alive independently of the source main window
        register_child_window(new_window)
        # show the new window
        new_window.show()

    @Slot()
    def _on_menu_open_file(self) -> None:
        # filters
        netlist_filter = "Netlist Files (*.cir *.sp *.spi *.net *.spice *.ckt)"
        raw_filter = "Raw Files (*.raw)"
        all_filter = "All Files (*)"
        # open existing file
        selected_file, _ = QFileDialog.getOpenFileName(self, "Open File", "", f"{netlist_filter};;{raw_filter};;{all_filter}")
        if not selected_file:
            return
        # path object
        selected_file_path = Path(selected_file)
        # check extension
        if selected_file_path.suffix.lower() == ".raw":
            # load raw file
            if self._load_raw_file(selected_file_path):
                # disable simulation actions
                self._simulation_config_action.setEnabled(False)
                self._simulation_run_action.setEnabled(False)
                self._show_netlist_action.setEnabled(False)
                # update title
                self.setWindowTitle(f"Xyce Simulation - {selected_file_path.name}")
                # delete all charts
                self._delete_all_charts()
                # create new chart
                return self._add_chart()
        # load netlist file
        return self._load_netlist_file(selected_file_path)

    def _load_raw_file(self, raw_file_path: Path) -> bool:
        # load and parse raw file
        raw_file = XyceRawFile.load(raw_file_path)
        if raw_file is None:
            # exit
            return False
        # update state
        self._raw_file = raw_file
        self._expression_manager = raw_file.expression_manager
        self._abscissa = raw_file.abscissa
        self._abscissa_scale = raw_file.abscissa_scale
        self._step_information = raw_file.step_information
        self._raw_file_path = raw_file_path
        # successfully loaded
        return True

    def _load_netlist_file(self, netlist_file_path: Path) -> None:
        try:
            # load and parse netlist
            content = netlist_file_path.read_text()
            # parse netlist file
            netlist, topology = parse_netlist(content)
            # new netlist and topology
            self._netlist = netlist
            self._netlist_file_path = netlist_file_path
            self._topology = topology
            # reset simulation parameters
            self._simulation_parameters = None
            # enable simulation actions now that a netlist is loaded
            self._simulation_config_action.setEnabled(True)
            self._simulation_run_action.setEnabled(True)
            self._show_netlist_action.setEnabled(True)
            # update title
            self.setWindowTitle(f"Xyce Simulation - {netlist_file_path.name}")
        except Exception as e:
            # error
            self._show_status(f"Failed to load netlist file [{netlist_file_path.name}]: {e}", 5000)

    @Slot(int)
    def _on_menu_step_tool(self, chart_index: int):
        # log information
        logger.debug("User requested step tool on chart at index: %d", chart_index)
        # check the chart index is valid
        if chart_index < 0 or chart_index >= len(self._charts):
            return
        # current chart
        chart = self._charts[chart_index]
        # get selected steps for this chart, make a copy
        selected_steps = set(chart.selected_steps)
        # open step tool dialog
        dialog = StepToolDialog(self, self._step_information, selected_steps)
        # exit if the user canceled
        if dialog.exec() != StepToolDialog.DialogCode.Accepted:
            return
        # store chart-local selected steps for later filtering phase
        chart.selected_steps = dialog.selected_steps
        # auto range axes
        chart.auto_range()

    @Slot(int)
    def _on_menu_smith_chart(self, chart_index: int):
        # log information
        logger.debug("User requested Smith chart on chart at index: %d", chart_index)
        # filter expressions to those suitable for Smith charting (network parameters with complex data)
        expressions = [expression for expression in self._expression_manager.expressions if expression.name.startswith(("S11", "S22")) and expression.variable_type == "parameter"]
        # create expression manager
        expression_manager = ExpressionManager([self._raw_file.abscissa] + expressions, self._expression_manager.function_definitions, self._expression_manager.step_slices)
        # create raw file
        qraw_file = XyceRawFile(filename=self._qraw_path, title=self._raw_file.title, date=self._raw_file.date, plotname=self._raw_file.plotname, complex=self._raw_file.complex, step_information=self._raw_file.step_information, abscissa=self._raw_file.abscissa, abscissa_scale=self._raw_file.abscissa_scale, command=self._raw_file.command, expression_manager=expression_manager, chart_type=self._raw_file.chart_type)
        # create a new SmithChart Window
        smith_window = SmithChartWindow(qraw_file)
        # keep reference alive independently of the source main window
        register_child_window(smith_window)
        # show the Smith chart window
        smith_window.show()

    def _add_chart(self):
        # chart index
        chart_index = len(self._charts)
        # create chart ui component in QML
        self._root.addChart()
        # get a reference to the chart's QML object so we can manipulate it
        chart_root = self._root.getChart(chart_index)
        # create chart instance
        chart = Chart(chart_root, self._expression_manager, self._step_information, self._abscissa, "", self._abscissa_scale.value, self._decimate_target)
        # apply initial step selection when provided (e.g. FFT window inheriting source chart visibility)
        if self._initial_selected_steps is not None:
            chart.selected_steps = self._initial_selected_steps
        # add it to the list of charts so we can keep track of it
        self._charts.append(chart)

    def _auto_range_all_charts(self):
        # loop existing charts
        for chart in self._charts:
            # auto range each chart
            chart.auto_range()

    def _update_all_charts(self):
        # loop existing charts
        for chart in self._charts:
            # redraw all expressions in chart
            chart.redraw_series(self._expression_manager, self._step_information, self._abscissa, "", self._abscissa_scale.value)
        # wait for QT event loop, adjust axes to fit the new data (do this after all charts are updated to avoid multiple redundant autorange calls while we're still updating charts)
        QTimer.singleShot(250, self._auto_range_all_charts)

    def _delete_all_charts(self):
        # remove all chart ui components from QML, in reverse order to avoid messing up indices when removing
        for index in reversed(range(len(self._charts))):
            self._root.removeChart(index)
        # clear list
        self._charts.clear()

    def _extract_schematic_netlist(self) -> tuple[str, Path, NetlistTopology]:
        # find schematic path and modification time
        schematic_path, schematic_last_modified = get_active_schematic_path()
        # check we need to export netlist from schematic (if the schematic has changed since the last export, or if we haven't exported yet)
        if self._schematic_last_modified is None or self._schematic_last_modified != schematic_last_modified:
            # resolve project directory path
            command_cwd = schematic_path.parent
            # resolve the installed kicad-cli binary path from the KiCad client
            kicad_cli_path = self._kicad_client.get_kicad_binary_path("kicad-cli")
            if not kicad_cli_path:
                # raise a runtime error if binary path could not be resolved
                raise RuntimeError("KiCad CLI binary path could not be resolved")
            # create a temporary output file for the exported netlist with explicit UTF-8 encoding
            with tempfile.NamedTemporaryFile(mode="w", suffix=".cir", prefix="kicad_xyce_", delete=True, encoding="utf-8") as output_file:
                try:
                    # resolve temporary output path
                    output_path = Path(output_file.name)
                    # build the command list for running the CLI export tool
                    command = [kicad_cli_path, "sch", "export", "netlist", "--format", "spice", "--output", str(output_path), str(schematic_path)]
                    # log the command execution details
                    logger.info("Executing kicad-cli: %s (CWD: %s)", " ".join(command), command_cwd)
                    # run the subprocess to perform netlist export
                    subprocess.run(command, check=True, capture_output=True, text=True, cwd=command_cwd)
                    # read the exported netlist file content
                    netlist_text = output_path.read_text(encoding="utf-8")
                    # parse the netlist content into text and topology structure
                    self._netlist, self._topology = parse_netlist(netlist_text)
                    # update the last modified time to avoid unnecessary re-exports until the schematic changes again
                    self._schematic_last_modified = schematic_last_modified
                # catch subprocess errors to report failures
                except subprocess.CalledProcessError as e:
                    # raise a runtime error with process output diagnostics
                    raise RuntimeError(f"Failed to export schematic netlist with kicad-cli: {e.stderr or e.stdout or e}") from e
        # return the extracted netlist data
        return self._netlist, schematic_path, self._topology

    def _on_menu_view_simulation_output(self) -> None:
        # check root
        if self._root:
            # toggle visibility
            visible = self._root.property("logVisible")
            # set property
            self._root.setProperty("logVisible", not visible)

    @Slot(str)
    def _on_simulation_started(self, netlist_path: str) -> None:
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
        self.log_clear_requested.emit()

    @Slot(str)
    def _on_stdout_received(self, text: str) -> None:
        # log
        logger.info("Xyce: %s", text)
        # log
        self.log_append_requested.emit(text)

    @Slot(str)
    def _on_stderr_received(self, text: str) -> None:
        # log
        logger.error("Xyce stderr: %s", text)
        # error
        msg = f"ERROR: {text}"
        self.log_append_requested.emit(msg)
        # status
        self._show_status(f"Simulation error: {text}", 5000)

    @Slot(int, int, bool)
    def _on_simulation_finished(self, exit_code: int, exit_status: int, was_canceled: bool) -> None:
        # check
        if was_canceled:
            # status
            self._show_status("Simulation canceled")
        elif exit_code == 0:
            # status
            self._show_status("Simulation finished successfully")
            # check raw file path is available
            if self._raw_file_path and self._raw_file_path.exists():
                # load raw file
                if self._load_raw_file(self._raw_file_path):
                    # check we need to create/update charts
                    if not self._charts:
                        # create new chart
                        self._add_chart()
                    else:
                        # update all charts with the new simulation data
                        self._update_all_charts()
            else:
                # error
                self._show_status("Simulation finished but output raw file could not be found", 5000)
        else:
            # status
            err_msg = f"Simulation failed (exit code: {exit_code})"
            self._show_status(err_msg, 5000)
        # runner
        self._runner = None

    def _on_menu_run_simulation(self):
        # netlist and topology to use
        netlist, netlist_file_path, topology = self._extract_schematic_netlist() if self._kicad_client else (self._netlist, self._netlist_file_path, self._topology)
        # initialize simulation parameters from netlist directives
        if self._simulation_parameters is None:
            self._simulation_parameters = from_xyce_directives(topology.directives)
        # initialize simulation parameters from dialog
        if self._simulation_parameters.analysis is None:
            # configure simulation
            self._on_menu_configure_simulation()
            # check simulation parameters
            if self._simulation_parameters.analysis is None:
                return
        # generate simulation directives
        directives = '\n'.join(self._simulation_parameters.to_xyce_directives(topology=topology))
        # final netlist
        netlist = netlist.replace(".END\n", f"\n{directives}\n\n.END\n")
        # log information
        logger.info("Running simulation with netlist:\n%s", netlist)
        # try
        try:
            # runner
            self._runner = run_xyce_simulation(self._plugin_config, netlist_file_path, netlist)
            # signals
            self._runner.started.connect(self._on_simulation_started)
            self._runner.stdout_received.connect(self._on_stdout_received)
            self._runner.stderr_received.connect(self._on_stderr_received)
            self._runner.finished.connect(self._on_simulation_finished)
            # determine the output raw file path based on the simulation configuration and netlist file path
            self._raw_file_path = self._simulation_parameters.analysis.raw_output_file_path(self._runner.working_directory, Path(self._runner.netlist_file_path))
            # start simulation
            self._runner.start()
        # except
        except ValueError as e:
            # status
            self._show_status(f"Simulation failed: {e}", 5000)
            self._show_status(str(e), 5000)
            # log
            logger.error("Simulation startup failed", exc_info=True)

    def _on_menu_show_netlist(self):
        # netlist and topology to use
        netlist, _, topology = self._extract_schematic_netlist() if self._kicad_client else (self._netlist, None, self._topology)
        # initialize simulation parameters from netlist directives
        if self._simulation_parameters is None:
            self._simulation_parameters = from_xyce_directives(topology.directives)
        # apply simulation parameters if present
        if self._simulation_parameters is not None:
            # directives
            directives = '\n'.join(self._simulation_parameters.to_xyce_directives(topology=topology))
            # final netlist
            netlist = netlist.replace(".END\n", f"\n{directives}\n\n.END\n")
        # dialog
        dialog = NetlistViewerDialog(parent=self, netlist=netlist)
        # exec
        dialog.exec()

    def _on_menu_configure_simulation(self) -> None:
        # simulation parameters
        initial_parameters = self._simulation_parameters
        # always resolve topology so the dialog can pre-populate the print variable lists
        _, _, topology = self._extract_schematic_netlist() if self._kicad_client else (None, None, self._topology)
        # extract topology to pre-populate parameters from schematic directives
        if initial_parameters is None:
            # load from directives if available
            initial_parameters = from_xyce_directives(topology.directives)
            # provide a default if no directives are available in the netlist
            if initial_parameters is None:
                initial_parameters = OpSimulationParameters()
        # dialog
        dialog = SimulationParametersDialog(self, initial_parameters=initial_parameters, topology=topology)
        # execute modal dialog and wait for user action
        if dialog.exec() != SimulationParametersDialog.DialogCode.Accepted:
            return None
        # update simulation parameters from dialog result
        self._simulation_parameters = dialog.get_parameters()

    def _on_menu_configuration(self) -> None:
        # dialog
        dialog = PluginConfigDialog(self, self._plugin_config)
        # execute modal dialog and wait for user action
        if dialog.exec() != PluginConfigDialog.DialogCode.Accepted:
            return None
        # store
        self._plugin_config = dialog.get_config()
        # log
        logger.info("Configured Xyce executable path: %s", self._plugin_config.xyce_executable_path)
        # status
        self.statusBar().showMessage("Plugin configuration updated", 3000)
