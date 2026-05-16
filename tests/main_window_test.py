import logging
import os
import sys
from unittest import TestCase
from unittest.mock import MagicMock, patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

# main_window.py uses bare intra-package imports; expose the plugin dir
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "plugin"))
# mock kipy before any plugin imports since main_window.py imports from kipy
sys.modules.setdefault("kipy", MagicMock())
# pre-import plugin modules in dependency order so bare-name aliases resolve correctly
import plugin.config_dialog
import plugin.expression
import plugin.kicad_icons
import plugin.plugin_config
import plugin.run_xyce_simulation
import plugin.simulation_dialog
import plugin.window  # noqa: F401
for _name in ["config_dialog", "expression", "kicad_icons", "plugin_config", "run_xyce_simulation", "simulation_dialog", "window"]:
    sys.modules.setdefault(_name, sys.modules[f"plugin.{_name}"])

from PySide6.QtCore import QSize
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QApplication, QMainWindow

from plugin.kicad_icons import load_kicad_icons
from plugin.main_window import MainWindow
from plugin.plugin_config import PluginConfig

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


class TestMainWindowSizeHint(TestCase):

    def test_size_hint_returns_expected_dimensions(self):
        # arrange
        window = _make_window()
        # act
        size = window.sizeHint()
        # assert
        self.assertEqual(size, QSize(1200, 800))


class TestMainWindowSetupNetlist(TestCase):

    def test_setup_netlist_returns_string_containing_end(self):
        # arrange
        window = _make_window()
        # act
        result = window._setup_netlist()
        # assert
        self.assertIn(".END", result)


class TestMainWindowShowStatus(TestCase):

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


class TestMainWindowOnQmlReady(TestCase):

    def test_skips_setup_when_status_is_not_ready(self):
        # arrange
        window = _make_window()
        original_root = window._root
        # act
        window._on_qml_ready(QQuickView.Status.Loading)
        # assert — _root was not replaced by qml_view.rootObject()
        self.assertIs(window._root, original_root)

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
        self.assertIsNotNone(window._root)


class TestMainWindowCreateMenu(TestCase):

    def test_create_main_menu_does_not_raise(self):
        # arrange
        window = _make_window()
        # act / assert — no exception raised during menu creation
        window._create_main_menu()


class TestMainWindowCreateToolbar(TestCase):

    def test_create_toolbar_does_not_raise(self):
        # arrange
        window = _make_window()
        # act / assert — no exception raised during toolbar creation
        window._create_toolbar()


class TestMainWindowOnSimulationStarted(TestCase):

    def test_shows_log_panel_and_clears_previous_output(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_started("/tmp/test.cir", "/tmp/test.raw")
        # assert
        window._root.setProperty.assert_any_call("logVisible", True)


class TestMainWindowOnStdoutReceived(TestCase):

    def test_emits_log_append_with_received_text(self):
        # arrange
        window = _make_window()
        received: list[str] = []
        window.logAppendRequested.connect(received.append)
        # act
        window._on_stdout_received("xyce output line")
        # assert
        self.assertEqual(received, ["xyce output line"])


class TestMainWindowOnStderrReceived(TestCase):

    def test_emits_log_append_with_error_prefix(self):
        # arrange
        window = _make_window()
        received: list[str] = []
        window.logAppendRequested.connect(received.append)
        # act
        window._on_stderr_received("error text")
        # assert
        self.assertEqual(received, ["ERROR: error text"])


class TestMainWindowOnSimulationFinished(TestCase):

    def test_shows_canceled_status_when_was_canceled(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_finished(0, 0, True, "/tmp/out.raw")
        # assert
        window._root.setProperty.assert_any_call("statusText", "Simulation canceled")
        self.assertIsNone(window._runner)

    def test_shows_success_status_when_exit_code_zero(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_finished(0, 0, False, "/tmp/out.raw")
        # assert
        window._root.setProperty.assert_any_call("statusText", "Simulation finished successfully")
        self.assertIsNone(window._runner)

    def test_shows_failure_status_with_exit_code_when_nonzero(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_finished(1, 0, False, "/tmp/out.raw")
        # assert
        window._root.setProperty.assert_any_call("statusText", "Simulation failed (exit code: 1)")
        self.assertIsNone(window._runner)


class TestMainWindowOnMenuRunSimulation(TestCase):

    def test_prompts_for_parameters_when_none_configured(self):
        # arrange
        window = _make_window()
        window._on_menu_configure_simulation = MagicMock()
        # act
        window._on_menu_run_simulation()
        # assert
        window._on_menu_configure_simulation.assert_called_once()
        self.assertIsNone(window._runner)

    def test_runs_simulation_with_configured_parameters(self):
        # arrange
        window = _make_window()
        window._simulation_parameters = MagicMock()
        window._simulation_parameters.to_xyce_directive.return_value = ".TRAN 1u 1m"
        with patch("plugin.main_window.run_xyce_simulation") as mock_run:
            mock_run.return_value = MagicMock()
            # act
            window._on_menu_run_simulation()
        # assert
        self.assertIsNotNone(window._runner)

    def test_shows_error_status_when_simulation_fails_to_start(self):
        # arrange
        window = _make_window()
        window._simulation_parameters = MagicMock()
        window._simulation_parameters.to_xyce_directive.return_value = ".TRAN 1u 1m"
        with patch("plugin.main_window.run_xyce_simulation", side_effect=ValueError("Invalid executable")):
            # act
            window._on_menu_run_simulation()
        # assert
        self.assertIsNone(window._runner)
        window._root.setProperty.assert_any_call("statusText", "Invalid executable")


class TestMainWindowOnMenuConfigureSimulation(TestCase):

    def test_keeps_existing_parameters_when_dialog_canceled(self):
        # arrange
        window = _make_window()
        with patch("plugin.main_window.SimulationDialog") as mock_dialog_cls:
            mock_dialog_cls.return_value.get_parameters.return_value = None
            # act
            window._on_menu_configure_simulation()
        # assert
        self.assertIsNone(window._simulation_parameters)

    def test_stores_parameters_when_dialog_accepted(self):
        # arrange
        window = _make_window()
        mock_params = MagicMock()
        mock_params.to_xyce_directive.return_value = ".TRAN 1u 1m"
        with patch("plugin.main_window.SimulationDialog") as mock_dialog_cls:
            mock_dialog_cls.return_value.get_parameters.return_value = mock_params
            # act
            window._on_menu_configure_simulation()
        # assert
        self.assertEqual(window._simulation_parameters, mock_params)


class TestMainWindowOnMenuConfiguration(TestCase):

    def test_keeps_existing_config_when_dialog_canceled(self):
        # arrange
        window = _make_window()
        original_config = window._plugin_config
        with patch("plugin.main_window.ConfigDialog") as mock_dialog_cls:
            mock_dialog_cls.return_value.get_config.return_value = None
            # act
            window._on_menu_configuration()
        # assert
        self.assertIs(window._plugin_config, original_config)

    def test_updates_config_when_dialog_accepted(self):
        # arrange
        window = _make_window()
        new_config = MagicMock()
        new_config.xyce_executable_path = "/usr/bin/Xyce"
        with patch("plugin.main_window.ConfigDialog") as mock_dialog_cls:
            mock_dialog_cls.return_value.get_config.return_value = new_config
            # act
            window._on_menu_configuration()
        # assert
        self.assertEqual(window._plugin_config, new_config)


class TestMainWindowOnMenuOpenFile(TestCase):

    def test_returns_early_when_no_file_selected(self):
        # arrange
        window = _make_window()
        with patch("plugin.main_window.QFileDialog.getOpenFileName", return_value=("", "")):
            # act / assert — no exception raised
            window._on_menu_open_file()

    def test_accepts_path_when_file_selected(self):
        # arrange
        window = _make_window()
        with patch("plugin.main_window.QFileDialog.getOpenFileName", return_value=("/tmp/test.cir", "")):
            # act / assert — no exception raised
            window._on_menu_open_file()


class TestMainWindowPopulateCharts(TestCase):

    def test_populate_charts_adds_one_chart(self):
        # arrange
        window = _make_window()
        # act
        window._populate_charts()
        # assert
        window._root.addChart.assert_called_once()


class TestMainWindowAddChart(TestCase):

    def test_add_chart_calls_qml_root_add_chart(self):
        # arrange
        window = _make_window()
        # act
        window._add_chart("", [])
        # assert
        window._root.addChart.assert_called_once()
