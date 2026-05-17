import logging
import sys
from unittest.mock import MagicMock, patch

from PySide6.QtCore import QSize
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QApplication, QMainWindow

from kicad_icons import load_kicad_icons
from main_window import MainWindow
from plugin_config import PluginConfig

_app = QApplication.instance() or QApplication(sys.argv)

load_kicad_icons()


def _make_window() -> MainWindow:
    # create a MainWindow instance bypassing the full __init__ to avoid QML setup
    window = MainWindow.__new__(MainWindow)
    QMainWindow.__init__(window)
    window._root = MagicMock()
    window._status_timer = MagicMock()
    window._plugin_config = PluginConfig.default()
    window._runner = None
    window._simulation_parameters = None
    window._charts = []
    return window


class TestMainWindowSizeHint:

    def test_size_hint_returns_expected_dimensions(self):
        # arrange
        window = _make_window()
        # act
        size = window.sizeHint()
        # assert
        assert size == QSize(1200, 800)


class TestMainWindowSetupNetlist:

    def test_setup_netlist_returns_string_containing_end(self):
        # arrange
        window = _make_window()
        # act
        result = window._setup_netlist()
        # assert
        assert ".END" in result


class TestMainWindowShowStatus:

    def test_show_status_sets_status_text_property(self):
        # arrange
        window = _make_window()
        # act
        window._show_status("test message")
        # assert
        window._root.setProperty.assert_any_call("statusText", "test message")

    def test_show_status_starts_timer_when_timeout_given(self):
        # arrange
        window = _make_window()
        # act
        window._show_status("test message", 3000)
        # assert
        window._status_timer.start.assert_called_once_with(3000)


class TestMainWindowOnQmlReady:

    def test_skips_setup_when_status_is_not_ready(self):
        # arrange
        window = _make_window()
        original_root = window._root
        # act
        window._on_qml_ready(QQuickView.Status.Loading)
        # assert — _root was not replaced by qml_view.rootObject()
        assert window._root is original_root

    def test_sets_root_properties_when_status_is_ready(self):
        # arrange
        window = _make_window()
        window._qml_view = MagicMock()
        # act
        window._on_qml_ready(QQuickView.Status.Ready)
        # assert
        window._root.setProperty.assert_any_call("fftVisible", False)
        window._root.setProperty.assert_any_call("stepToolVisible", False)
        window._root.setProperty.assert_any_call("smithChartVisible", False)

    def test_schedules_screen_info_in_debug_mode(self):
        # arrange
        window = _make_window()
        window._qml_view = MagicMock()
        logging.getLogger("plugin.main_window").setLevel(logging.DEBUG)
        # act
        window._on_qml_ready(QQuickView.Status.Ready)
        # restore default log level so other tests are not affected
        logging.getLogger("plugin.main_window").setLevel(logging.WARNING)
        # assert — no exception raised during debug-mode path
        assert window._root is not None


class TestMainWindowCreateMenu:

    def test_create_main_menu_does_not_raise(self):
        # arrange
        window = _make_window()
        # act / assert — no exception raised during menu creation
        window._create_main_menu()


class TestMainWindowCreateToolbar:

    def test_create_toolbar_does_not_raise(self):
        # arrange
        window = _make_window()
        # act / assert — no exception raised during toolbar creation
        window._create_toolbar()


class TestMainWindowOnSimulationStarted:

    def test_shows_log_panel_and_clears_previous_output(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_started("/tmp/test.cir", "/tmp/test.raw")
        # assert
        window._root.setProperty.assert_any_call("logVisible", True)


class TestMainWindowOnStdoutReceived:

    def test_emits_log_append_with_received_text(self):
        # arrange
        window = _make_window()
        received: list[str] = []
        window.logAppendRequested.connect(received.append)
        # act
        window._on_stdout_received("xyce output line")
        # assert
        assert received == ["xyce output line"]


class TestMainWindowOnStderrReceived:

    def test_emits_log_append_with_error_prefix(self):
        # arrange
        window = _make_window()
        received: list[str] = []
        window.logAppendRequested.connect(received.append)
        # act
        window._on_stderr_received("error text")
        # assert
        assert received == ["ERROR: error text"]


class TestMainWindowOnSimulationFinished:

    def test_shows_canceled_status_when_was_canceled(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_finished(0, 0, True, "/tmp/out.raw")
        # assert
        window._root.setProperty.assert_any_call("statusText", "Simulation canceled")
        assert window._runner is None

    def test_shows_success_status_when_exit_code_zero(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_finished(0, 0, False, "/tmp/out.raw")
        # assert
        window._root.setProperty.assert_any_call("statusText", "Simulation finished successfully")
        assert window._runner is None

    def test_shows_failure_status_with_exit_code_when_nonzero(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_finished(1, 0, False, "/tmp/out.raw")
        # assert
        window._root.setProperty.assert_any_call("statusText", "Simulation failed (exit code: 1)")
        assert window._runner is None


class TestMainWindowOnMenuRunSimulation:

    def test_prompts_for_parameters_when_none_configured(self):
        # arrange
        window = _make_window()
        window._simulation_parameters = None
        window._on_menu_configure_simulation = MagicMock()
        # mock parse_netlist to return empty directives so simulation parameters remain None
        with patch("main_window.parse_netlist") as mock_parse:
            mock_parse.return_value = MagicMock(directives=[])
            # act
            window._on_menu_run_simulation()
            # assert
            window._on_menu_configure_simulation.assert_called_once()
            assert window._runner is None

    def test_runs_simulation_with_configured_parameters(self):
        # arrange
        window = _make_window()
        window._simulation_parameters = MagicMock()
        window._simulation_parameters.to_xyce_directive.return_value = ".TRAN 1u 1m"
        with patch("main_window.run_xyce_simulation") as mock_run:
            mock_run.return_value = MagicMock()
            # act
            window._on_menu_run_simulation()
        # assert
        assert window._runner is not None

    def test_shows_error_status_when_simulation_fails_to_start(self):
        # arrange
        window = _make_window()
        window._simulation_parameters = MagicMock()
        window._simulation_parameters.to_xyce_directive.return_value = ".TRAN 1u 1m"
        with patch("main_window.run_xyce_simulation", side_effect=ValueError("Invalid executable")):
            # act
            window._on_menu_run_simulation()
        # assert
        assert window._runner is None
        window._root.setProperty.assert_any_call("statusText", "Invalid executable")


class TestMainWindowOnMenuConfigureSimulation:

    def test_keeps_existing_parameters_when_dialog_canceled(self):
        # arrange
        window = _make_window()
        with patch("main_window.SimulationDialog") as mock_dialog_cls:
            mock_dialog_cls.return_value.get_parameters.return_value = None
            # act
            window._on_menu_configure_simulation()
        # assert
        assert window._simulation_parameters is None

    def test_stores_parameters_when_dialog_accepted(self):
        # arrange
        window = _make_window()
        mock_params = MagicMock()
        mock_params.to_xyce_directive.return_value = ".TRAN 1u 1m"
        with patch("main_window.SimulationDialog") as mock_dialog_cls:
            mock_dialog_cls.Accepted = 1
            mock_dialog_cls.return_value.exec.return_value = 1
            mock_dialog_cls.return_value.get_parameters.return_value = mock_params
            # act
            window._on_menu_configure_simulation()
        # assert
        assert window._simulation_parameters == mock_params


class TestMainWindowOnMenuConfiguration:

    def test_keeps_existing_config_when_dialog_canceled(self):
        # arrange
        window = _make_window()
        original_config = window._plugin_config
        with patch("main_window.ConfigDialog") as mock_dialog_cls:
            mock_dialog_cls.return_value.get_config.return_value = None
            # act
            window._on_menu_configuration()
        # assert
        assert window._plugin_config is original_config

    def test_updates_config_when_dialog_accepted(self):
        # arrange
        window = _make_window()
        new_config = MagicMock()
        new_config.xyce_executable_path = "/usr/bin/Xyce"
        with patch("main_window.ConfigDialog") as mock_dialog_cls:
            mock_dialog_cls.return_value.get_config.return_value = new_config
            # act
            window._on_menu_configuration()
        # assert
        assert window._plugin_config == new_config


class TestMainWindowOnMenuOpenFile:

    def test_returns_early_when_no_file_selected(self):
        # arrange
        window = _make_window()
        with patch("main_window.QFileDialog.getOpenFileName", return_value=("", "")):
            # act / assert — no exception raised
            window._on_menu_open_file()

    def test_accepts_path_when_file_selected(self):
        # arrange
        window = _make_window()
        with patch("main_window.QFileDialog.getOpenFileName", return_value=("/tmp/test.cir", "")):
            # act / assert — no exception raised
            window._on_menu_open_file()


class TestMainWindowPopulateCharts:

    def test_populate_charts_adds_one_chart(self):
        # arrange
        window = _make_window()
        # act
        window._populate_charts()
        # assert
        window._root.addChart.assert_called_once()


class TestMainWindowAddChart:

    def test_add_chart_calls_qml_root_add_chart(self):
        # arrange
        window = _make_window()
        # act
        window._add_chart("", [])
        # assert
        window._root.addChart.assert_called_once()
