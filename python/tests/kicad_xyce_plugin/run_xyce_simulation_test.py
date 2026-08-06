import os
import sys
import tempfile
from pathlib import Path
from unittest.mock import MagicMock

import pytest

from PySide6.QtCore import QProcess
from PySide6.QtWidgets import QApplication

from kicad_xyce_plugin.config.plugin_config import PluginConfig
from kicad_xyce_plugin.run_xyce_simulation import XyceSimulationRunner, run_xyce_simulation

_app = QApplication.instance() or QApplication(sys.argv)


def _make_valid_config() -> PluginConfig:
    """Return a PluginConfig with a real executable path pointing to /bin/sh."""
    return PluginConfig(xyce_executable_path="/bin/sh")


class TestRunXyceSimulationValidation:

    def test_raises_value_error_when_executable_invalid(self):
        # arrange — config with invalid (non-existent) executable path
        config = PluginConfig(xyce_executable_path="/nonexistent/xyce")
        # act / assert
        with pytest.raises(ValueError):
            run_xyce_simulation(config, Path("/tmp/test.cir"), "* Netlist\n.END")

    def test_raises_value_error_when_netlist_is_empty(self):
        # arrange — valid config but empty netlist
        config = _make_valid_config()
        # act / assert
        with pytest.raises(ValueError):
            run_xyce_simulation(config, Path("/tmp/test.cir"), "   ")

    def test_raises_value_error_when_netlist_is_whitespace_only(self):
        # arrange
        config = _make_valid_config()
        # act / assert
        with pytest.raises(ValueError):
            run_xyce_simulation(config, Path("/tmp/test.cir"), "\n\t  \n")

    def test_returns_runner_instance_when_inputs_are_valid(self):
        # arrange
        config = _make_valid_config()
        # act — process will start /bin/sh, which exits immediately
        runner = run_xyce_simulation(config, Path("/tmp/test.cir"), "* Netlist\n.END")
        # assert
        assert isinstance(runner, XyceSimulationRunner)
        # cleanup — cancel process to avoid leaving it running
        runner.cancel()

    def test_runner_has_netlist_file_path(self):
        # arrange
        config = _make_valid_config()
        # act
        runner = run_xyce_simulation(config, Path("/tmp/test.cir"), "* Test netlist\n.END")
        # assert — a temp file was created and path is non-empty
        assert runner.netlist_file_path
        runner.cancel()


class TestXyceSimulationRunnerEmitBufferedLines:

    def test_emits_single_complete_line(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        signal_mock = MagicMock()
        buffer = "hello world\n"
        # act
        remaining = runner._emit_buffered_lines(buffer, signal_mock, False)
        # assert
        signal_mock.emit.assert_called_once_with("hello world")
        assert remaining == ""

    def test_emits_multiple_complete_lines(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        signal_mock = MagicMock()
        buffer = "line one\nline two\nline three\n"
        # act
        remaining = runner._emit_buffered_lines(buffer, signal_mock, False)
        # assert
        assert signal_mock.emit.call_count == 3
        assert remaining == ""

    def test_retains_partial_line_without_flush(self):
        # arrange — no trailing newline
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        signal_mock = MagicMock()
        buffer = "partial line"
        # act
        remaining = runner._emit_buffered_lines(buffer, signal_mock, False)
        # assert
        signal_mock.emit.assert_not_called()
        assert remaining == "partial line"

    def test_flushes_partial_line_when_flush_partial_true(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        signal_mock = MagicMock()
        buffer = "final partial"
        # act
        remaining = runner._emit_buffered_lines(buffer, signal_mock, True)
        # assert
        signal_mock.emit.assert_called_once_with("final partial")
        assert remaining == ""

    def test_strips_carriage_return_from_crlf_lines(self):
        # arrange — Windows-style CRLF line endings
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        signal_mock = MagicMock()
        buffer = "windows line\r\n"
        # act
        runner._emit_buffered_lines(buffer, signal_mock, False)
        # assert
        signal_mock.emit.assert_called_once_with("windows line")

    def test_strips_carriage_return_from_partial_flush(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        signal_mock = MagicMock()
        buffer = "partial crlf\r"
        # act
        runner._emit_buffered_lines(buffer, signal_mock, True)
        # assert
        signal_mock.emit.assert_called_once_with("partial crlf")

    def test_empty_buffer_emits_nothing(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        signal_mock = MagicMock()
        buffer = ""
        # act
        remaining = runner._emit_buffered_lines(buffer, signal_mock, False)
        # assert
        signal_mock.emit.assert_not_called()
        assert remaining == ""

    def test_empty_buffer_with_flush_emits_nothing(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        signal_mock = MagicMock()
        buffer = ""
        # act
        remaining = runner._emit_buffered_lines(buffer, signal_mock, True)
        # assert
        signal_mock.emit.assert_not_called()
        assert remaining == ""

    def test_partial_line_followed_by_more_data(self):
        # arrange — simulate two chunks arriving sequentially
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        signal_mock = MagicMock()
        buffer = "first line\nsecon"
        # act
        remaining = runner._emit_buffered_lines(buffer, signal_mock, False)
        # assert
        signal_mock.emit.assert_called_once_with("first line")
        assert remaining == "secon"


class TestXyceSimulationRunnerConstruction:

    def test_netlist_file_path_property(self):
        # arrange / act
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/my.cir")
        # assert
        assert runner.netlist_file_path == "/tmp/my.cir"


class TestXyceSimulationRunnerCancel:

    def test_cancel_when_not_running_is_no_op(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        # act / assert — process is NotRunning, cancel should not raise
        runner.cancel()

    def test_cancel_sets_was_canceled_when_running(self):
        # arrange — patch the process to appear as running
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        runner._process = MagicMock()
        runner._process.state.return_value = QProcess.ProcessState.Running
        # act
        runner.cancel()
        # assert
        assert runner._was_canceled
        runner._process.terminate.assert_called_once()


class TestXyceSimulationRunnerSignals:

    def test_has_started_signal(self):
        # arrange / act
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        # assert — signal attribute exists
        assert hasattr(runner, "started")

    def test_has_finished_signal(self):
        # assert
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        assert hasattr(runner, "finished")

    def test_has_stdout_received_signal(self):
        # assert
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        assert hasattr(runner, "stdout_received")

    def test_has_stderr_received_signal(self):
        # assert
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        assert hasattr(runner, "stderr_received")

    def test_has_process_error_signal(self):
        # assert
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        assert hasattr(runner, "process_error")


class TestXyceSimulationRunnerFinalize:

    def test_finalize_emits_finished_signal(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test_finalize.cir")
        finished_args: list = []
        runner.finished.connect(lambda *args: finished_args.extend(args))
        # act
        runner._finalize(0, QProcess.ExitStatus.NormalExit)
        # assert
        assert len(finished_args) == 3
        assert finished_args[0] == 0

    def test_finalize_is_idempotent(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test_finalize.cir")
        finished_args: list = []
        runner.finished.connect(lambda *args: finished_args.extend(args))
        # act — call twice; second call must be a no-op
        runner._finalize(0, QProcess.ExitStatus.NormalExit)
        runner._finalize(1, QProcess.ExitStatus.NormalExit)
        # assert — finished was emitted exactly once so list has exactly 3 items
        assert len(finished_args) == 3

    def test_finalize_includes_cancellation_state(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test_finalize.cir")
        finished_args: list = []
        runner.finished.connect(lambda *args: finished_args.extend(args))
        runner._was_canceled = True
        # act
        runner._finalize(0, QProcess.ExitStatus.NormalExit)
        # assert — third element is was_canceled flag
        assert finished_args[2]

    def test_finalize_includes_output_file_path(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test_finalize.cir")
        finished_args: list = []
        runner.finished.connect(lambda *args: finished_args.extend(args))
        # act
        runner._finalize(0, QProcess.ExitStatus.NormalExit)


class TestXyceSimulationRunnerOnStarted:

    def test_on_started_emits_started_signal(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/start.cir")
        started_args: list = []
        runner.started.connect(lambda *args: started_args.extend(args))
        # act
        runner._on_started()
        # assert
        assert started_args[0] == "/tmp/start.cir"


class TestXyceSimulationRunnerOnErrorOccurred:

    def test_on_error_occurred_emits_process_error_signal(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/err.cir")
        runner._process = MagicMock()
        runner._process.errorString.return_value = "Failed to start"
        error_signals: list = []
        runner.process_error.connect(lambda *args: error_signals.extend(args))
        # act
        runner._on_error_occurred(QProcess.ProcessError.FailedToStart)
        # assert
        assert error_signals[1] == "Failed to start"

    def test_on_error_occurred_failed_to_start_triggers_finalize(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/err.cir")
        runner._process = MagicMock()
        runner._process.errorString.return_value = "Failed to start"
        finished_calls: list = []
        runner.finished.connect(lambda *args: finished_calls.append(args))
        # act
        runner._on_error_occurred(QProcess.ProcessError.FailedToStart)
        # assert — finalize was triggered
        assert runner._finished_emitted


class TestXyceSimulationRunnerKillIfStillRunning:

    def test_no_op_when_process_not_running(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        runner._process = MagicMock()
        runner._process.state.return_value = QProcess.ProcessState.NotRunning
        # act
        runner._kill_if_still_running()
        # assert
        runner._process.kill.assert_not_called()

    def test_force_kills_when_process_still_running(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        runner._process = MagicMock()
        runner._process.state.return_value = QProcess.ProcessState.Running
        # act
        runner._kill_if_still_running()
        # assert
        runner._process.kill.assert_called_once()


class TestXyceSimulationRunnerReadyRead:

    def test_on_ready_read_standard_output_emits_lines(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        runner._process = MagicMock()
        runner._process.readAllStandardOutput.return_value = b"stdout line\n"
        received: list[str] = []
        runner.stdout_received.connect(received.append)
        # act
        runner._on_ready_read_standard_output()
        # assert
        assert received == ["stdout line"]

    def test_on_ready_read_standard_error_emits_lines(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        runner._process = MagicMock()
        runner._process.readAllStandardError.return_value = b"stderr line\n"
        received: list[str] = []
        runner.stderr_received.connect(received.append)
        # act
        runner._on_ready_read_standard_error()
        # assert
        assert received == ["stderr line"]


class TestXyceSimulationRunnerOnFinished:

    def test_on_finished_triggers_finalize(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "/tmp/test.cir")
        # act
        runner._on_finished(0, QProcess.ExitStatus.NormalExit)
        # assert
        assert runner._finished_emitted


class TestXyceSimulationRunnerCleanupNetlistFile:

    def test_no_op_when_netlist_path_is_empty(self):
        # arrange
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), "")
        # act / assert — no exception raised when path is empty
        runner._cleanup_netlist_file()

    def test_deletes_existing_netlist_file(self):
        # arrange
        with tempfile.NamedTemporaryFile(delete=False) as f:
            tmp_path = f.name
        runner = XyceSimulationRunner("/bin/sh", Path("/tmp"), tmp_path)
        # act
        runner._cleanup_netlist_file()
        # assert
        assert not os.path.exists(tmp_path)
