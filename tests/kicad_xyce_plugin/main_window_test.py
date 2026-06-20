import logging
import subprocess
import sys
from pathlib import Path
from unittest.mock import MagicMock, call, patch

import pytest

from PySide6.QtCore import QSize
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QApplication, QMainWindow

from kicad_xyce_plugin.kicad_icons import load_kicad_icons
from kicad_xyce_plugin.main_window import MainWindow
from kicad_xyce_plugin.config.plugin_config import PluginConfig

_app = QApplication.instance() or QApplication(sys.argv)

load_kicad_icons()


def _make_window() -> MainWindow:
    # create a MainWindow instance bypassing the full __init__ to avoid QML setup
    window = MainWindow.__new__(MainWindow)
    QMainWindow.__init__(window)
    window._root = MagicMock()
    window._status_timer = MagicMock()
    window._kicad_client = MagicMock()
    window._plugin_config = PluginConfig.default()
    window._schematic_last_modified = None
    window._runner = None
    window._netlist = None
    window._netlist_file_path = None
    window._topology = None
    window._simulation_parameters = None
    window._simulation_performed = False
    window._simulation_output_action = None
    window._simulation_config_action = None
    window._simulation_run_action = None
    window._show_netlist_action = None
    window._charts = []
    window._abscissa = None
    window._step_information = None
    window._expression_manager = None
    window._abscissa_scale = MagicMock()
    window._raw_file = None
    window._raw_file_path = None
    window._decimate_target = 9600
    window._initial_selected_steps = None
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

    def test_extract_schematic_netlist_exports_and_parses_topology_when_valid(self, tmp_path):
        # arrange
        window = _make_window()
        window._kicad_client.get_kicad_binary_path.return_value = "/usr/bin/kicad-cli"
        schematic_file = tmp_path / "test.kicad_sch"
        schematic_file.write_text("EESchema Schematic File Version 4\n", encoding="utf-8")
        output_file = tmp_path / "output.cir"
        output_file.write_text("* Test schematic\nV1 1 0 5V\nR1 1 0 1k\n.OP\n.END\n", encoding="utf-8")
        mock_tempfile = MagicMock()
        mock_tempfile.__enter__.return_value.name = str(output_file)
        topology = MagicMock(directives=[MagicMock()])
        with patch("kicad_xyce_plugin.main_window.get_active_schematic_path", return_value=(schematic_file.resolve(), schematic_file.stat().st_mtime)):
            with patch("kicad_xyce_plugin.main_window.tempfile.NamedTemporaryFile", return_value=mock_tempfile):
                with patch("kicad_xyce_plugin.main_window.subprocess.run") as mock_run:
                    with patch("kicad_xyce_plugin.main_window.parse_netlist", return_value=("netlist text", topology)):
                        # act
                        netlist, netlist_path, parsed_topology = window._extract_schematic_netlist()

        # assert
        assert netlist == "netlist text"
        assert netlist_path == schematic_file.resolve()
        assert parsed_topology is topology
        assert window._netlist == netlist
        assert window._topology is topology
        assert window._schematic_last_modified == schematic_file.stat().st_mtime
        mock_run.assert_called_once()
        assert str(output_file) in mock_run.call_args[0][0]
        assert str(schematic_file.resolve()) in mock_run.call_args[0][0]

    def test_extract_schematic_netlist_uses_cached_result_when_schematic_has_not_changed(self):
        # arrange
        window = _make_window()
        window._kicad_client.get_kicad_binary_path.return_value = "/usr/bin/kicad-cli"
        window._netlist = "cached netlist"
        cached_topology = MagicMock(directives=[MagicMock()])
        window._topology = cached_topology
        window._schematic_last_modified = 1.0
        schematic_path = Path("/tmp/test.kicad_sch")
        with patch("kicad_xyce_plugin.main_window.get_active_schematic_path", return_value=(schematic_path, 1.0)):
            with patch("kicad_xyce_plugin.main_window.subprocess.run") as mock_run:
                # act
                netlist, netlist_path, parsed_topology = window._extract_schematic_netlist()

        # assert
        assert netlist == "cached netlist"
        assert netlist_path == schematic_path
        assert parsed_topology is cached_topology
        mock_run.assert_not_called()

    def test_extract_schematic_netlist_raises_error_when_client_missing(self):
        # arrange
        window = _make_window()
        window._kicad_client = None
        schematic_path = Path("/tmp/test.kicad_sch")
        with patch("kicad_xyce_plugin.main_window.get_active_schematic_path", return_value=(schematic_path, 1.0)):
            # act/assert
            with pytest.raises(AttributeError, match="get_kicad_binary_path"):
                window._extract_schematic_netlist()

    def test_extract_schematic_netlist_raises_error_when_kicad_cli_missing(self):
        # arrange
        window = _make_window()
        window._kicad_client.get_kicad_binary_path.return_value = None
        schematic_path = Path("/tmp/test.kicad_sch")
        with patch("kicad_xyce_plugin.main_window.get_active_schematic_path", return_value=(schematic_path, 1.0)):
            # act/assert
            with pytest.raises(RuntimeError, match="KiCad CLI binary path could not be resolved"):
                window._extract_schematic_netlist()

    def test_extract_schematic_netlist_raises_error_when_cli_fails(self, tmp_path):
        # arrange
        window = _make_window()
        window._kicad_client.get_kicad_binary_path.return_value = "/usr/bin/kicad-cli"
        schematic_file = tmp_path / "test.kicad_sch"
        schematic_file.write_text("EESchema Schematic File Version 4\n", encoding="utf-8")
        output_file = tmp_path / "output.cir"
        mock_tempfile = MagicMock()
        mock_tempfile.__enter__.return_value.name = str(output_file)
        with patch("kicad_xyce_plugin.main_window.get_active_schematic_path", return_value=(schematic_file.resolve(), schematic_file.stat().st_mtime)):
            with patch("kicad_xyce_plugin.main_window.tempfile.NamedTemporaryFile", return_value=mock_tempfile):
                with patch("kicad_xyce_plugin.main_window.subprocess.run") as mock_run:
                    mock_run.side_effect = subprocess.CalledProcessError(returncode=1, cmd="cli", stderr="some error")

                    # act/assert
                    with pytest.raises(RuntimeError, match="Failed to export schematic netlist with kicad-cli"):
                        window._extract_schematic_netlist()


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
        window._on_simulation_started("/tmp/test.cir")
        # assert
        window._root.setProperty.assert_any_call("logVisible", True)


class TestMainWindowOnStdoutReceived:

    def test_emits_log_append_with_received_text(self):
        # arrange
        window = _make_window()
        received: list[str] = []
        window.log_append_requested.connect(received.append)
        # act
        window._on_stdout_received("xyce output line")
        # assert
        assert received == ["xyce output line"]


class TestMainWindowOnStderrReceived:

    def test_emits_log_append_with_error_prefix(self):
        # arrange
        window = _make_window()
        received: list[str] = []
        window.log_append_requested.connect(received.append)
        # act
        window._on_stderr_received("error text")
        # assert
        assert received == ["ERROR: error text"]


class TestMainWindowOnSimulationFinished:

    def test_shows_canceled_status_when_was_canceled(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_finished(0, 0, True)
        # assert
        window._root.setProperty.assert_any_call("statusText", "Simulation canceled")
        assert window._runner is None

    def test_shows_success_status_when_exit_code_zero(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_finished(0, 0, False)
        # assert
        window._root.setProperty.assert_any_call("statusText", "Simulation finished but output raw file could not be found")
        assert window._runner is None

    def test_shows_failure_status_with_exit_code_when_nonzero(self):
        # arrange
        window = _make_window()
        # act
        window._on_simulation_finished(1, 0, False)
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
        with patch.object(window, "_extract_schematic_netlist", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))):
            with patch("kicad_xyce_plugin.main_window.parse_netlist") as mock_parse:
                mock_parse.return_value = ("", MagicMock(directives=[]))
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
        with patch.object(window, "_extract_schematic_netlist", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))), \
             patch("kicad_xyce_plugin.main_window.run_xyce_simulation") as mock_run:
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
        with patch.object(window, "_extract_schematic_netlist", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))):
            with patch("kicad_xyce_plugin.main_window.run_xyce_simulation", side_effect=ValueError("Invalid executable")):
                # act
                window._on_menu_run_simulation()
        # assert
        assert window._runner is None
        window._root.setProperty.assert_any_call("statusText", "Invalid executable")

    def test_uses_stored_topology_in_standalone_mode(self):
        # arrange
        window = _make_window()
        window._kicad_client = None
        window._netlist = "* test\nR1 1 0 1k\n.OP\n.END"
        window._topology = MagicMock(directives=[])
        window._simulation_parameters = MagicMock()
        window._simulation_parameters.to_xyce_directive.return_value = ".OP"
        with patch.object(window, "_extract_schematic_netlist", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))):
            with patch("kicad_xyce_plugin.main_window.run_xyce_simulation") as mock_run:
                mock_run.return_value = MagicMock()
                # act
                window._on_menu_run_simulation()
        # assert
        assert window._runner is not None


class TestMainWindowOnMenuShowNetlist:

    def test_shows_netlist_dialog_in_plugin_mode(self):
        # arrange
        window = _make_window()
        window._simulation_parameters = MagicMock()
        window._simulation_parameters.to_xyce_directive.return_value = ".OP"
        with patch.object(window, "_extract_schematic_netlist", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))), \
             patch("kicad_xyce_plugin.main_window.NetlistViewerDialog") as mock_dialog_cls:
            mock_dialog_cls.return_value.exec.return_value = None
            # act
            window._on_menu_show_netlist()
        # assert
        mock_dialog_cls.assert_called_once()

    def test_shows_netlist_dialog_in_standalone_mode(self):
        # arrange
        window = _make_window()
        window._kicad_client = None
        window._netlist = "* test\nR1 1 0 1k\n.OP\n.END"
        window._topology = MagicMock(directives=[])
        window._simulation_parameters = MagicMock()
        window._simulation_parameters.to_xyce_directive.return_value = ".OP"
        with patch.object(window, "_extract_schematic_netlist", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))):
            with patch("kicad_xyce_plugin.main_window.NetlistViewerDialog") as mock_dialog_cls:
                mock_dialog_cls.return_value.exec.return_value = None
                # act
                window._on_menu_show_netlist()
        # assert
        mock_dialog_cls.assert_called_once()


class TestMainWindowOnMenuConfigureSimulation:

    def test_keeps_existing_parameters_when_dialog_canceled(self):
        # arrange
        window = _make_window()
        with patch.object(window, "_extract_schematic_netlist", return_value=("", MagicMock(), MagicMock(directives=[]))):
            with patch("kicad_xyce_plugin.main_window.SimulationParametersDialog") as mock_dialog_cls:
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
        with patch.object(window, "_extract_schematic_netlist", return_value=("", MagicMock(), MagicMock(directives=[]))):
            with patch("kicad_xyce_plugin.main_window.SimulationParametersDialog") as mock_dialog_cls:
                mock_dialog_cls.DialogCode.Accepted = "accepted"
                mock_dialog_cls.return_value.exec.return_value = "accepted"
                mock_dialog_cls.return_value.get_parameters.return_value = mock_params
                # act
                window._on_menu_configure_simulation()
        # assert
        assert window._simulation_parameters == mock_params

    def test_uses_stored_topology_in_standalone_mode(self):
        # arrange
        window = _make_window()
        window._kicad_client = None
        window._topology = MagicMock(directives=[])
        mock_params = MagicMock()
        with patch("kicad_xyce_plugin.main_window.SimulationParametersDialog") as mock_dialog_cls:
            mock_dialog_cls.DialogCode.Accepted = "accepted"
            mock_dialog_cls.return_value.exec.return_value = "accepted"
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
        with patch("kicad_xyce_plugin.main_window.PluginConfigDialog") as mock_dialog_cls:
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
        with patch("kicad_xyce_plugin.main_window.PluginConfigDialog") as mock_dialog_cls:
            mock_dialog_cls.DialogCode.Accepted = "accepted"
            mock_dialog_cls.return_value.exec.return_value = "accepted"
            mock_dialog_cls.return_value.get_config.return_value = new_config
            # act
            window._on_menu_configuration()
        # assert
        assert window._plugin_config == new_config


class TestMainWindowOnMenuOpenFile:

    def test_returns_early_when_no_file_selected(self):
        # arrange
        window = _make_window()
        with patch("kicad_xyce_plugin.main_window.QFileDialog.getOpenFileName", return_value=("", "")):
            # act / assert — no exception raised
            window._on_menu_open_file()

    def test_accepts_path_when_file_selected(self):
        # arrange
        window = _make_window()
        with patch("kicad_xyce_plugin.main_window.QFileDialog.getOpenFileName", return_value=("/tmp/test.cir", "")):
            with patch("kicad_xyce_plugin.main_window.Path.read_text", return_value="* test\n.END"):
                # act / assert — no exception raised
                window._on_menu_open_file()

    def test_stores_netlist_content_when_netlist_file_selected(self):
        # arrange
        window = _make_window()
        with patch("kicad_xyce_plugin.main_window.QFileDialog.getOpenFileName", return_value=("/tmp/test.cir", "")):
            with patch("kicad_xyce_plugin.main_window.Path.read_text", return_value="* test\n.END\n"):
                # act
                window._on_menu_open_file()
        # assert
        assert window._netlist == "* test\n.END\n"

    def test_does_not_store_netlist_for_raw_file(self):
        # arrange
        window = _make_window()
        with patch("kicad_xyce_plugin.main_window.QFileDialog.getOpenFileName", return_value=("/tmp/test.raw", "")):
            # act
            window._on_menu_open_file()
        # assert
        assert window._netlist is None

    def test_enables_simulation_actions_when_netlist_file_selected(self):
        # arrange
        window = _make_window()
        window._simulation_config_action = MagicMock()
        window._simulation_run_action = MagicMock()
        window._show_netlist_action = MagicMock()
        with patch("kicad_xyce_plugin.main_window.QFileDialog.getOpenFileName", return_value=("/tmp/test.cir", "")):
            with patch("kicad_xyce_plugin.main_window.Path.read_text", return_value="* test\n.END"):
                # act
                window._on_menu_open_file()
        # assert
        window._simulation_config_action.setEnabled.assert_called_once_with(True)
        window._simulation_run_action.setEnabled.assert_called_once_with(True)
        window._show_netlist_action.setEnabled.assert_called_once_with(True)

    def test_does_not_enable_simulation_actions_when_raw_file_selected(self):
        # arrange
        window = _make_window()
        window._simulation_config_action = MagicMock()
        window._simulation_run_action = MagicMock()
        window._show_netlist_action = MagicMock()
        with patch("kicad_xyce_plugin.main_window.QFileDialog.getOpenFileName", return_value=("/tmp/test.raw", "")):
            # act
            window._on_menu_open_file()
        # assert
        window._simulation_config_action.setEnabled.assert_not_called()
        window._simulation_run_action.setEnabled.assert_not_called()
        window._show_netlist_action.setEnabled.assert_not_called()

    def test_does_not_enable_simulation_actions_when_no_file_selected(self):
        # arrange
        window = _make_window()
        window._simulation_config_action = MagicMock()
        window._simulation_run_action = MagicMock()
        window._show_netlist_action = MagicMock()
        with patch("kicad_xyce_plugin.main_window.QFileDialog.getOpenFileName", return_value=("", "")):
            # act
            window._on_menu_open_file()
        # assert
        window._simulation_config_action.setEnabled.assert_not_called()
        window._simulation_run_action.setEnabled.assert_not_called()
        window._show_netlist_action.setEnabled.assert_not_called()

    def test_loads_raw_file_and_disables_simulation_actions(self):
        # arrange
        window = _make_window()
        window._simulation_config_action = MagicMock()
        window._simulation_run_action = MagicMock()
        window._show_netlist_action = MagicMock()
        raw_file = MagicMock()
        with patch("kicad_xyce_plugin.main_window.QFileDialog.getOpenFileName", return_value=("/tmp/test.raw", "")):
            with patch("kicad_xyce_plugin.main_window.XyceRawFile.load", return_value=raw_file):
                with patch.object(window, "_delete_all_charts") as mock_delete:
                    with patch.object(window, "_add_chart") as mock_add_chart:
                        # act
                        window._on_menu_open_file()
        # assert
        window._simulation_config_action.setEnabled.assert_called_once_with(False)
        window._simulation_run_action.setEnabled.assert_called_once_with(False)
        window._show_netlist_action.setEnabled.assert_called_once_with(False)
        mock_delete.assert_called_once()
        mock_add_chart.assert_called_once()

    def test_loads_netlist_file_and_stores_content(self):
        # arrange
        window = _make_window()
        window._simulation_config_action = MagicMock()
        window._simulation_run_action = MagicMock()
        window._show_netlist_action = MagicMock()
        netlist_text = "* test\n.END\n"
        topology = MagicMock(directives=[])
        with patch("kicad_xyce_plugin.main_window.QFileDialog.getOpenFileName", return_value=("/tmp/test.cir", "")):
            with patch("kicad_xyce_plugin.main_window.Path.read_text", return_value=netlist_text):
                with patch("kicad_xyce_plugin.main_window.parse_netlist", return_value=(netlist_text, topology)):
                    # act
                    window._on_menu_open_file()
        # assert
        assert window._netlist == netlist_text
        assert window._topology is topology
        window._simulation_config_action.setEnabled.assert_called_once_with(True)
        window._simulation_run_action.setEnabled.assert_called_once_with(True)
        window._show_netlist_action.setEnabled.assert_called_once_with(True)

    def test_load_netlist_file_shows_status_when_parse_fails(self):
        # arrange
        window = _make_window()
        window._simulation_config_action = MagicMock()
        window._simulation_run_action = MagicMock()
        window._show_netlist_action = MagicMock()
        window._show_status = MagicMock()
        netlist_text = "* test\n.END\n"
        with patch("kicad_xyce_plugin.main_window.Path.read_text", return_value=netlist_text):
            with patch("kicad_xyce_plugin.main_window.parse_netlist", side_effect=ValueError("parse error")):
                # act
                window._load_netlist_file(Path("/tmp/test.cir"))
        # assert
        window._show_status.assert_called_once()
        assert "Failed to load netlist file" in window._show_status.call_args[0][0]
        window._simulation_config_action.setEnabled.assert_not_called()
        window._simulation_run_action.setEnabled.assert_not_called()
        window._show_netlist_action.setEnabled.assert_not_called()


class TestMainWindowChartActions:

    def test_auto_range_all_charts_calls_each_chart(self):
        # arrange
        window = _make_window()
        chart_one = MagicMock()
        chart_two = MagicMock()
        window._charts = [chart_one, chart_two]
        # act
        window._auto_range_all_charts()
        # assert
        chart_one.auto_range.assert_called_once()
        chart_two.auto_range.assert_called_once()

    def test_update_all_charts_redraws_series_and_schedules_auto_range(self):
        # arrange
        window = _make_window()
        chart = MagicMock()
        window._charts = [chart]
        window._expression_manager = MagicMock()
        window._step_information = MagicMock()
        window._abscissa = MagicMock()
        window._abscissa_scale = MagicMock(value=42)
        with patch("kicad_xyce_plugin.main_window.QTimer.singleShot") as mock_single_shot:
            # act
            window._update_all_charts()
        # assert
        chart.redraw_series.assert_called_once_with(window._expression_manager, window._step_information, window._abscissa, "", 42)
        mock_single_shot.assert_called_once()

    def test_delete_all_charts_removes_charts_in_reverse_order(self):
        # arrange
        window = _make_window()
        window._root.removeChart = MagicMock()
        window._charts = [MagicMock(), MagicMock(), MagicMock()]
        # act
        window._delete_all_charts()
        # assert
        assert window._charts == []
        window._root.removeChart.assert_has_calls([call(2), call(1), call(0)])

    def test_on_menu_step_tool_does_nothing_for_invalid_index(self):
        # arrange
        window = _make_window()
        window._charts = []
        # act
        window._on_menu_step_tool(0)
        # assert
        assert window._charts == []

    def test_on_menu_step_tool_updates_selected_steps_when_accepted(self):
        # arrange
        window = _make_window()
        chart = MagicMock(selected_steps={1, 2})
        window._charts = [chart]
        window._step_information = MagicMock()
        with patch("kicad_xyce_plugin.main_window.StepToolDialog") as mock_dialog_cls:
            mock_dialog = mock_dialog_cls.return_value
            mock_dialog_cls.DialogCode.Accepted = "accepted"
            mock_dialog.exec.return_value = "accepted"
            mock_dialog.selected_steps = {3, 4}
            # act
            window._on_menu_step_tool(0)
        # assert
        assert chart.selected_steps == {3, 4}
        chart.auto_range.assert_called_once()


class TestMainWindowViewSimulationOutput:

    def test_view_simulation_output_enabled_after_simulation_starts(self):
        # arrange
        window = _make_window()
        window._simulation_output_action = MagicMock()
        # act
        window._on_simulation_started("/tmp/test.cir")
        # assert
        assert window._simulation_performed is True
        window._simulation_output_action.setEnabled.assert_called_with(True)

    def test_view_simulation_output_toggles_log_visibility(self):
        # arrange
        window = _make_window()
        window._root = MagicMock()
        window._root.property.return_value = False
        # act
        window._on_menu_view_simulation_output()
        # assert
        window._root.setProperty.assert_called_with("logVisible", True)


class TestMainWindowAddChart:

    def test_add_chart_calls_qml_root_add_chart(self):
        # arrange
        window = _make_window()
        # act
        with patch("kicad_xyce_plugin.main_window.Chart"):
            window._add_chart()
        # assert
        window._root.addChart.assert_called_once()
