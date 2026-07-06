import logging
import subprocess
import sys
from pathlib import Path
from unittest.mock import MagicMock, call, patch

import pytest
from kicad_xyce_plugin.kicad_icons import load_kicad_icons
from kicad_xyce_plugin.main_window import MainWindow, _compute_decimate_target, _format_value, _format_values
from kicad_xyce_plugin.config.plugin_config import PluginConfig
from kicad_xyce_plugin.xyce_raw_file import AbscissaScale
from PySide6.QtCore import QSize
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QApplication, QMainWindow

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
    window._fft_files = None
    window._decimate_target = 9600
    window._initial_selected_steps = None
    window._plot_suggestion = None
    window._last_status_time = 0.0
    return window


def _configure_simulation_finish_context(window: MainWindow, raw_file_path: Path | None = None, fft_output_pattern: str | None = None) -> None:
    # configure runner and analysis object expected by _on_simulation_finished
    window._runner = MagicMock()
    window._runner.working_directory = Path("/tmp")
    window._runner.netlist_file_path = "/tmp/test.cir"
    # build simulation parameters with analysis helpers
    analysis = MagicMock()
    analysis.raw_output_file_path.return_value = raw_file_path
    analysis.fft_output_file_path_pattern.return_value = fft_output_pattern
    window._simulation_parameters = MagicMock()
    window._simulation_parameters.analysis = analysis


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
                        netlist, netlist_path, parsed_topology = window._extract_netlist_from_schematic()

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
                netlist, netlist_path, parsed_topology = window._extract_netlist_from_schematic()

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
                window._extract_netlist_from_schematic()

    def test_extract_schematic_netlist_raises_error_when_kicad_cli_missing(self):
        # arrange
        window = _make_window()
        window._kicad_client.get_kicad_binary_path.return_value = None
        schematic_path = Path("/tmp/test.kicad_sch")
        with patch("kicad_xyce_plugin.main_window.get_active_schematic_path", return_value=(schematic_path, 1.0)):
            # act/assert
            with pytest.raises(RuntimeError, match="KiCad CLI binary path could not be resolved"):
                window._extract_netlist_from_schematic()

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
                        window._extract_netlist_from_schematic()


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
        window._root.setProperty.assert_any_call("calculateFFTVisible", False)
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
        _configure_simulation_finish_context(window, raw_file_path=None, fft_output_pattern=None)
        # act
        window._on_simulation_finished(0, 0, False)
        # assert
        window._root.setProperty.assert_any_call("statusText", "Simulation finished successfully")
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
        with patch.object(window, "_extract_netlist_from_schematic", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))):
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
        with patch.object(window, "_extract_netlist_from_schematic", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))), \
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
        with patch.object(window, "_extract_netlist_from_schematic", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))):
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
        with patch.object(window, "_extract_netlist_from_schematic", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))):
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
        with patch.object(window, "_extract_netlist_from_schematic", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))), \
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
        with patch.object(window, "_extract_netlist_from_schematic", return_value=("", Path("/tmp/test.kicad_sch"), MagicMock(directives=[]))):
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
        with patch.object(window, "_extract_netlist_from_schematic", return_value=("", MagicMock(), MagicMock(directives=[]))):
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
        with patch.object(window, "_extract_netlist_from_schematic", return_value=("", MagicMock(), MagicMock(directives=[]))):
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
        raw_file.step_information.length = 1
        with patch("kicad_xyce_plugin.main_window.QFileDialog.getOpenFileName", return_value=("/tmp/test.raw", "")):
            with patch("kicad_xyce_plugin.main_window.xyce_raw_file_parser", return_value=raw_file):
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
        window._raw_file = MagicMock()
        window._raw_file_path = Path("/tmp/old.raw")
        window._fft_files = [MagicMock()]
        # act
        window._on_simulation_started("/tmp/test.cir")
        # assert
        assert window._raw_file is None
        assert window._raw_file_path is None
        assert window._fft_files is None
        window._simulation_output_action.setEnabled.assert_called_with(True)
        window._root.setProperty.assert_any_call("openFFTCalculationVisible", False)

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


class TestFormatValue:

    def test_formats_giga_range(self):
        assert _format_value(2.5e9, "Hz") == "2.50 GHz"

    def test_formats_mega_range(self):
        assert _format_value(1.5e6, "Hz") == "1.50 MHz"

    def test_formats_kilo_range(self):
        assert _format_value(3.3e3, "Ω") == "3.30 kΩ"

    def test_formats_base_range(self):
        assert _format_value(5.0, "V") == "5.00 V"

    def test_formats_zero(self):
        assert _format_value(0.0, "V") == "0 V"

    def test_formats_femto_range(self):
        assert _format_value(1e-14, "F") == "10.00 fF"

    def test_formats_pico_range(self):
        assert _format_value(1e-11, "F") == "10.00 pF"

    def test_formats_nano_range(self):
        assert _format_value(1e-8, "s") == "10.00 ns"

    def test_formats_micro_range(self):
        assert _format_value(1e-5, "s") == "10.00 µs"

    def test_formats_milli_range(self):
        assert _format_value(1e-2, "s") == "10.00 ms"

    def test_formats_negative_value(self):
        assert _format_value(-2.5e9, "Hz") == "-2.50 GHz"


class TestFormatValues:

    def test_single_value_returns_simple_string(self):
        result = _format_values("Time", [1e-3], "s")
        assert result == "Time = 1.00 ms"

    def test_multiple_values_returns_bracketed_string(self):
        result = _format_values("Freq", [1e3, 2e3], "Hz")
        assert result == "Freq = [1.00 kHz, 2.00 kHz]"


class TestComputeDecimateTarget:

    def test_returns_fallback_when_screen_is_none(self):
        result = _compute_decimate_target(None)
        assert result == 9600

    def test_returns_scaled_width_when_screen_provided(self):
        mock_screen = MagicMock()
        mock_screen.size.return_value.width.return_value = 2560
        mock_screen.devicePixelRatio.return_value = 2.0
        result = _compute_decimate_target(mock_screen)
        assert result == 2560 * max(5, 2)


class TestMainWindowCreateMenuStandalone:

    def test_create_main_menu_in_standalone_mode_adds_open_action(self):
        # arrange
        window = _make_window()
        window._kicad_client = None
        # act / assert — no exception raised and Open action is included
        window._create_main_menu()
        actions = [a.text() for a in window.menuBar().actions()]
        assert "&File" in actions


class TestMainWindowCreateToolbarStandalone:

    def test_create_toolbar_in_standalone_mode_does_not_raise(self):
        # arrange
        window = _make_window()
        window._kicad_client = None
        window._show_netlist_action = None
        window._simulation_config_action = None
        window._simulation_run_action = None
        # act / assert — no exception raised, Open/Save actions added
        window._create_toolbar()


class TestMainWindowOnQmlReadyWithPlotSuggestion:

    def test_schedules_populate_charts_when_plot_suggestion_set(self):
        # arrange
        window = _make_window()
        window._qml_view = MagicMock()
        window._plot_suggestion = [("V(net)", [])]
        with patch("kicad_xyce_plugin.main_window.QTimer.singleShot") as mock_shot:
            # act
            window._on_qml_ready(QQuickView.Status.Ready)
        # assert — singleShot scheduled for populate_charts
        calls = [str(c) for c in mock_shot.call_args_list]
        assert any("_populate_charts" in c for c in calls)


class TestMainWindowPopulateCharts:

    def test_populate_charts_creates_chart_per_suggestion_and_schedules_autorange(self):
        # arrange
        window = _make_window()
        window._plot_suggestion = [("A", [MagicMock()]), ("B", [MagicMock()])]
        mock_chart = MagicMock()
        with patch.object(window, "_add_chart", return_value=mock_chart) as mock_add, \
                patch("kicad_xyce_plugin.main_window.QTimer.singleShot") as mock_shot:
            # act
            window._populate_charts()
        # assert
        assert mock_add.call_count == 2
        mock_shot.assert_called_once()


class TestMainWindowLoadRawFileReturnsNone:

    def test_load_raw_file_returns_false_when_file_cannot_be_loaded(self):
        # arrange
        window = _make_window()
        with patch("kicad_xyce_plugin.main_window.xyce_raw_file_parser", return_value=None):
            # act
            result = window._load_raw_file(Path("/tmp/missing.raw"))
        # assert
        assert result is False


class TestMainWindowZoomSlots:

    def test_on_zoom_region_selected_updates_zoom_windows(self):
        # arrange
        window = _make_window()
        chart_a = MagicMock()
        chart_b = MagicMock()
        window._charts = [chart_a, chart_b]
        # act
        window._on_zoom_region_selected(0, 0.1, 0.2, 0.9, 0.8)
        # assert
        chart_a.update_zoom_window.assert_called_once_with(0.1, 0.9, 0.2, 0.8)
        chart_b.update_zoom_window.assert_called_once_with(0.1, 0.9, None, None)

    def test_on_menu_zoom_to_fit_resets_all_charts(self):
        # arrange
        window = _make_window()
        chart_a = MagicMock()
        chart_b = MagicMock()
        window._charts = [chart_a, chart_b]
        # act
        window._on_menu_zoom_to_fit(0)
        # assert
        chart_a.reset_zoom_window.assert_called_once_with(True, True)
        chart_b.reset_zoom_window.assert_called_once_with(True, False)

    def test_on_menu_autorange_resets_vertical_zoom(self):
        # arrange
        window = _make_window()
        chart = MagicMock()
        window._charts = [chart]
        # act
        window._on_menu_autorange(0)
        # assert
        chart.reset_zoom_window.assert_called_once_with(False, True)

    def test_on_menu_zoom_abscissa_extent_resets_horizontal_zoom(self):
        # arrange
        window = _make_window()
        chart_a = MagicMock()
        chart_b = MagicMock()
        window._charts = [chart_a, chart_b]
        # act
        window._on_menu_zoom_abscissa_extent(0)
        # assert
        chart_a.reset_zoom_window.assert_called_once_with(True, False)
        chart_b.reset_zoom_window.assert_called_once_with(True, False)


class TestMainWindowAddRemovePlots:

    def test_returns_early_when_dialog_canceled(self):
        # arrange
        window = _make_window()
        chart = MagicMock()
        window._charts = [chart]
        with patch("kicad_xyce_plugin.main_window.AddPlotDialog") as mock_cls:
            mock_cls.DialogCode.Accepted = "accepted"
            mock_cls.return_value.exec.return_value = "canceled"
            # act
            window._on_menu_add_remove_plots(0)
        # assert — plot_series not called
        chart.plot_series.assert_not_called()

    def test_plots_selected_expressions_when_dialog_accepted(self):
        # arrange
        window = _make_window()
        chart = MagicMock()
        window._charts = [chart]
        expressions = [MagicMock()]
        with patch("kicad_xyce_plugin.main_window.AddPlotDialog") as mock_cls, \
                patch("kicad_xyce_plugin.main_window.QTimer.singleShot"):
            mock_cls.DialogCode.Accepted = "accepted"
            mock_cls.return_value.exec.return_value = "accepted"
            mock_cls.return_value.selected_expressions = expressions
            # act
            window._on_menu_add_remove_plots(0)
        # assert
        chart.plot_series.assert_called_once_with(expressions)


class TestMainWindowDeleteAllPlots:

    def test_clears_chart_at_index(self):
        # arrange
        window = _make_window()
        chart = MagicMock()
        window._charts = [chart]
        # act
        window._on_menu_delete_all_plots(0)
        # assert
        chart.clear.assert_called_once()


class TestMainWindowOnMenuAddChart:

    def test_calls_add_chart(self):
        # arrange
        window = _make_window()
        with patch.object(window, "_add_chart") as mock_add:
            # act
            window._on_menu_add_chart(0)
        # assert
        mock_add.assert_called_once()


class TestMainWindowOnMenuDeleteChart:

    def test_removes_chart_from_qml_and_list(self):
        # arrange
        window = _make_window()
        chart = MagicMock()
        window._charts = [chart]
        # act
        window._on_menu_delete_chart(0)
        # assert
        window._root.removeChart.assert_called_once_with(0)
        assert window._charts == []


class TestMainWindowOnMenuNewWindow:

    def test_creates_registers_and_shows_new_window(self):
        # arrange
        window = _make_window()
        with patch("kicad_xyce_plugin.main_window.MainWindow") as mock_cls, \
                patch("kicad_xyce_plugin.main_window.register_child_window") as mock_reg:
            mock_new = mock_cls.return_value
            # act
            window._on_menu_new_window()
        # assert
        mock_cls.assert_called_once()
        mock_reg.assert_called_once_with(mock_new)
        mock_new.show.assert_called_once()


class TestMainWindowOnMenuStepToolCanceled:

    def test_does_not_update_steps_when_dialog_canceled(self):
        # arrange
        window = _make_window()
        chart = MagicMock(selected_steps={1, 2})
        window._charts = [chart]
        window._step_information = MagicMock()
        with patch("kicad_xyce_plugin.main_window.StepToolDialog") as mock_cls:
            mock_cls.DialogCode.Accepted = "accepted"
            mock_cls.return_value.exec.return_value = "canceled"
            # act
            window._on_menu_step_tool(0)
        # assert — selected_steps unchanged, auto_range not called
        assert chart.selected_steps == {1, 2}
        chart.auto_range.assert_not_called()


class TestMainWindowPointerExited:

    def test_clears_status_bar_on_pointer_exit(self):
        # arrange
        window = _make_window()
        # act
        window._on_pointer_exited(0)
        # assert
        window._root.setProperty.assert_any_call("statusText", " ")


class TestMainWindowPointerMoved:

    def test_updates_status_bar_with_abscissa_value(self):
        # arrange
        window = _make_window()
        chart = MagicMock()
        chart.abscissa_value_at_cursor.return_value = 1.0
        chart.ordinate_values_at_abscissa_value.return_value = []
        window._charts = [chart]
        window._abscissa = MagicMock()
        window._abscissa.name = "Time"
        window._abscissa.unit = "s"
        window._abscissa_scale = AbscissaScale.LINEAR
        # act
        window._on_pointer_moved(0, 0.5)
        # assert
        window._root.setProperty.assert_any_call("statusText", "Time = 1.00 s")

    def test_skips_update_when_throttle_interval_not_elapsed(self):
        # arrange
        window = _make_window()
        window._last_status_time = 1e18  # far in the future
        chart = MagicMock()
        window._charts = [chart]
        # act
        window._on_pointer_moved(0, 0.5)
        # assert — status not updated, chart not accessed
        chart.abscissa_value_at_cursor.assert_not_called()

    def test_skips_update_for_invalid_chart_index(self):
        # arrange
        window = _make_window()
        window._charts = []
        window._abscissa_scale = AbscissaScale.LINEAR
        # act / assert — no exception raised
        window._on_pointer_moved(5, 0.5)

    def test_formats_decade_scale_abscissa(self):
        # arrange
        window = _make_window()
        chart = MagicMock()
        chart.abscissa_value_at_cursor.return_value = 3.0  # stored log10 value
        chart.ordinate_values_at_abscissa_value.return_value = []
        window._charts = [chart]
        window._abscissa = MagicMock()
        window._abscissa.name = "Frequency"
        window._abscissa.unit = "Hz"
        window._abscissa_scale = AbscissaScale.DECADE
        # act
        window._on_pointer_moved(0, 0.5)
        # assert — 10^3 = 1000 Hz = 1.00 kHz
        window._root.setProperty.assert_any_call("statusText", "Frequency = 1.00 kHz")

    def test_formats_octave_scale_abscissa(self):
        # arrange
        window = _make_window()
        chart = MagicMock()
        chart.abscissa_value_at_cursor.return_value = 10.0  # stored log2 value
        chart.ordinate_values_at_abscissa_value.return_value = []
        window._charts = [chart]
        window._abscissa = MagicMock()
        window._abscissa.name = "Frequency"
        window._abscissa.unit = "Hz"
        window._abscissa_scale = AbscissaScale.OCTAVE
        # act
        window._on_pointer_moved(0, 0.5)
        # assert — 2^10 = 1024 Hz ≈ 1.02 kHz
        window._root.setProperty.assert_any_call("statusText", "Frequency = 1.02 kHz")


class TestMainWindowAddChartWithInitialSteps:

    def test_applies_initial_selected_steps_to_new_chart(self):
        # arrange
        window = _make_window()
        window._initial_selected_steps = {0, 2}
        with patch("kicad_xyce_plugin.main_window.Chart") as mock_chart_cls:
            mock_chart = mock_chart_cls.return_value
            # act
            window._add_chart()
        # assert
        assert mock_chart.selected_steps == {0, 2}


class TestMainWindowOnSimulationFinishedSuccessPaths:

    def test_adds_new_chart_when_raw_file_loaded_and_no_charts_exist(self):
        # arrange
        window = _make_window()
        raw_file_path = Path("/tmp/test.raw")
        _configure_simulation_finish_context(window, raw_file_path=raw_file_path, fft_output_pattern=None)
        with patch.object(window, "_load_raw_file", return_value=True), \
                patch.object(window, "_add_chart") as mock_add:
            with patch.object(Path, "exists", return_value=True):
            # act
                window._on_simulation_finished(0, 0, False)
        # assert
        mock_add.assert_called_once()

    def test_updates_existing_charts_when_raw_file_loaded_and_charts_exist(self):
        # arrange
        window = _make_window()
        raw_file_path = Path("/tmp/test.raw")
        _configure_simulation_finish_context(window, raw_file_path=raw_file_path, fft_output_pattern=None)
        window._charts = [MagicMock()]
        with patch.object(window, "_load_raw_file", return_value=True), \
                patch.object(window, "_update_all_charts") as mock_update:
            with patch.object(Path, "exists", return_value=True):
            # act
                window._on_simulation_finished(0, 0, False)
        # assert
        mock_update.assert_called_once()

    def test_shows_error_when_load_raw_file_returns_false(self):
        # arrange
        window = _make_window()
        raw_file_path = Path("/tmp/test.raw")
        _configure_simulation_finish_context(window, raw_file_path=raw_file_path, fft_output_pattern=None)
        with patch.object(window, "_load_raw_file", return_value=False):
            with patch.object(Path, "exists", return_value=True):
                # act — should not raise
                window._on_simulation_finished(0, 0, False)
        # assert runner cleared
        assert window._runner is None

    def test_parses_fft_outputs_and_toggles_open_fft_visibility(self):
        # arrange
        window = _make_window()
        raw_file_path = Path("/tmp/test.raw")
        fft_pattern = "/tmp/test.cir.fft*"
        _configure_simulation_finish_context(window, raw_file_path=raw_file_path, fft_output_pattern=fft_pattern)
        window._raw_file = MagicMock()
        window._raw_file.expression_manager = MagicMock()
        window._step_information = MagicMock()
        parsed_fft_files = [MagicMock()]
        with patch.object(window, "_load_raw_file", return_value=True), \
            patch.object(window, "_add_chart"), \
                patch.object(Path, "exists", return_value=True), \
                patch("kicad_xyce_plugin.main_window.xyce_fft_file_parser", return_value=parsed_fft_files) as mock_fft_parser:
            # act
            window._on_simulation_finished(0, 0, False)
        # assert
        mock_fft_parser.assert_called_once_with(fft_pattern, window._step_information, window._raw_file.expression_manager)
        assert window._fft_files == parsed_fft_files
        window._root.setProperty.assert_any_call("openFFTCalculationVisible", True)


class TestMainWindowOpenFftCalculation:

    def test_does_nothing_when_no_fft_files_available(self):
        # arrange
        window = _make_window()
        window._charts = [MagicMock()]
        window._fft_files = None
        # act
        window._on_menu_open_fft_calculation(0)
        # assert
        assert window._fft_files is None

    def test_opens_window_for_each_fft_file(self):
        # arrange
        window = _make_window()
        window._charts = [MagicMock()]
        window._plugin_config = MagicMock()
        window._raw_file_path = Path("/tmp/test.raw")

        abscissa_expression = MagicMock()
        abscissa_expression.name = "frequency"
        magnitude_expression = MagicMock()
        magnitude_expression.name = "FFT(V(OUT))"

        fft_file = MagicMock()
        fft_file.abscissa = abscissa_expression
        fft_file.expression_manager.expressions = [abscissa_expression, magnitude_expression]
        window._fft_files = [fft_file]

        with patch("kicad_xyce_plugin.main_window.MainWindow") as mock_window_cls, \
                patch("kicad_xyce_plugin.main_window.register_child_window") as mock_register:
            created_window = mock_window_cls.return_value
            # act
            window._on_menu_open_fft_calculation(0)

        # assert
        mock_window_cls.assert_called_once_with(None, window._plugin_config, fft_file, window._raw_file_path, [("FFT(V(OUT))", [magnitude_expression])])
        mock_register.assert_called_once_with(created_window)
        created_window.show.assert_called_once()


class TestMainWindowOnMenuShowNetlistNoParameters:

    def test_initializes_parameters_from_directives_when_none_set(self):
        # arrange
        window = _make_window()
        window._kicad_client = None
        window._netlist = "* test\n.END\n"
        window._topology = MagicMock(directives=[])
        mock_params = MagicMock()
        with patch("kicad_xyce_plugin.main_window.from_xyce_directives", return_value=mock_params), \
                patch("kicad_xyce_plugin.main_window.NetlistViewerDialog") as mock_dlg:
            mock_dlg.return_value.exec.return_value = None
            # act
            window._on_menu_show_netlist()
        # assert simulation parameters initialized
        assert window._simulation_parameters is mock_params


class TestMainWindowOnMenuConfigureSimulationFallback:

    def test_uses_op_parameters_as_fallback_when_directives_empty(self):
        # arrange
        window = _make_window()
        window._kicad_client = None
        window._topology = MagicMock(directives=[])
        with patch("kicad_xyce_plugin.main_window.from_xyce_directives", return_value=None), \
                patch("kicad_xyce_plugin.main_window.SimulationParametersDialog") as mock_dlg, \
                patch("kicad_xyce_plugin.main_window.OpSimulationParameters") as mock_op:
            mock_dlg.DialogCode.Accepted = "not_accepted"
            mock_dlg.return_value.exec.return_value = "not_accepted"
            # act
            window._on_menu_configure_simulation()
        # assert OpSimulationParameters was instantiated as fallback
        mock_op.assert_called_once()
