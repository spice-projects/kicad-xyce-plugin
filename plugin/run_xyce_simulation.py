import logging
import os
import tempfile

from PySide6.QtCore import QObject, QProcess, QTimer, Signal

from plugin_config import PluginConfig

logger = logging.getLogger(__name__)


class XyceSimulationRunner(QObject):

    started = Signal(str, str)
    stdout_received = Signal(str)
    stderr_received = Signal(str)
    process_error = Signal(int, str)
    finished = Signal(int, int, bool, str)

    def __init__(self, program_path: str, netlist_file_path: str, output_file_path: str):
        # initialize QObject infrastructure used by Qt signals
        super().__init__()
        # keep the external executable path for process launch
        self._program_path = program_path
        # keep the persisted netlist file path for execution and cleanup
        self._netlist_file_path = netlist_file_path
        # keep the output file path passed to Xyce
        self._output_file_path = output_file_path
        # keep cancellation state for finished event reporting
        self._was_canceled = False
        # keep a guard flag so cleanup and final signals run once
        self._finished_emitted = False
        # keep partial stdout content across chunk callbacks
        self._stdout_buffer = ""
        # keep partial stderr content across chunk callbacks
        self._stderr_buffer = ""
        # create process object parented to this runner for lifecycle safety
        self._process = QProcess(self)
        # keep stdout and stderr separate so caller can render both streams
        self._process.setProcessChannelMode(QProcess.ProcessChannelMode.SeparateChannels)
        # route process started notification to the typed runner signal
        self._process.started.connect(self._on_started)
        # route stdout chunks to parser that emits line stream updates
        self._process.readyReadStandardOutput.connect(self._on_ready_read_standard_output)
        # route stderr chunks to parser that emits line stream updates
        self._process.readyReadStandardError.connect(self._on_ready_read_standard_error)
        # route process errors to caller-visible diagnostics
        self._process.errorOccurred.connect(self._on_error_occurred)
        # route process completion to final status and cleanup handling
        self._process.finished.connect(self._on_finished)

    @property
    def netlist_file_path(self) -> str:
        return self._netlist_file_path

    @property
    def output_file_path(self) -> str:
        return self._output_file_path

    def start(self) -> None:
        # pass output file path so Xyce writes binary results deterministically
        arguments = ["-r", self._output_file_path, self._netlist_file_path]
        # log process invocation details for diagnostics
        logger.info("Starting Xyce process: %s %s", self._program_path, " ".join(arguments))
        # configure the external executable selected by plugin configuration
        self._process.setProgram(self._program_path)
        # configure command arguments before starting the process
        self._process.setArguments(arguments)
        # start process asynchronously on the current Qt event loop thread
        self._process.start()

    def cancel(self) -> None:
        # no-op when process has already exited
        if self._process.state() == QProcess.ProcessState.NotRunning:
            return
        # mark cancellation state so final event reflects user intent
        self._was_canceled = True
        # request graceful process termination first
        self._process.terminate()
        # schedule hard kill fallback when process ignores termination
        QTimer.singleShot(2000, self._kill_if_still_running)

    def _kill_if_still_running(self) -> None:
        # skip kill when process already exited naturally
        if self._process.state() == QProcess.ProcessState.NotRunning:
            return
        # force-kill process after cancellation grace period
        self._process.kill()

    def _emit_buffered_lines(self, buffer_text: str, signal: Signal, flush_partial: bool) -> str:
        # keep a local copy to simplify parsing operations
        text = buffer_text
        # emit full lines whenever a newline delimiter is available
        while "\n" in text:
            # split once so remaining content stays in the buffer
            line, text = text.split("\n", 1)
            # strip optional carriage return from CRLF streams
            if line.endswith("\r"):
                line = line[:-1]
            # emit parsed line to caller stream listener
            signal.emit(line)
        # emit last partial chunk on process completion when requested
        if flush_partial and text:
            # strip optional carriage return from final partial line
            if text.endswith("\r"):
                text = text[:-1]
            # emit remaining text as a final line-like event
            signal.emit(text)
            # clear buffer once the final partial chunk is emitted
            return ""
        # return retained trailing partial text for future chunk assembly
        return text

    def _on_started(self) -> None:
        # notify caller that process start succeeded and temp files are in use
        self.started.emit(self._netlist_file_path, self._output_file_path)

    def _on_ready_read_standard_output(self) -> None:
        # decode available stdout bytes using replacement for robustness
        stdout_chunk = bytes(self._process.readAllStandardOutput()).decode("utf-8", errors="replace")
        # append chunk to retained partial-line buffer
        self._stdout_buffer = self._stdout_buffer + stdout_chunk
        # emit any complete stdout lines and keep trailing partial text
        self._stdout_buffer = self._emit_buffered_lines(self._stdout_buffer, self.stdout_received, False)

    def _on_ready_read_standard_error(self) -> None:
        # decode available stderr bytes using replacement for robustness
        stderr_chunk = bytes(self._process.readAllStandardError()).decode("utf-8", errors="replace")
        # append chunk to retained partial-line buffer
        self._stderr_buffer = self._stderr_buffer + stderr_chunk
        # emit any complete stderr lines and keep trailing partial text
        self._stderr_buffer = self._emit_buffered_lines(self._stderr_buffer, self.stderr_received, False)

    def _on_error_occurred(self, process_error: QProcess.ProcessError) -> None:
        # map process error enum into a readable diagnostic string
        error_message = self._process.errorString()
        # emit error details to typed error signal for caller state handling
        self.process_error.emit(int(process_error), error_message)
        # mirror process errors to stderr stream to keep a unified log view
        self.stderr_received.emit(error_message)
        # cleanup temp netlist when process fails before finished callback
        if process_error == QProcess.ProcessError.FailedToStart:
            self._finalize(0, QProcess.ExitStatus.NormalExit)

    def _on_finished(self, exit_code: int, exit_status: QProcess.ExitStatus) -> None:
        # forward completion state and perform one-time cleanup
        self._finalize(exit_code, exit_status)

    def _finalize(self, exit_code: int, exit_status: QProcess.ExitStatus) -> None:
        # prevent duplicate finalization from overlapping Qt callbacks
        if self._finished_emitted:
            return
        # mark finalization so repeated callbacks are ignored
        self._finished_emitted = True
        # flush remaining stdout partial line after process completion
        self._stdout_buffer = self._emit_buffered_lines(self._stdout_buffer, self.stdout_received, True)
        # flush remaining stderr partial line after process completion
        self._stderr_buffer = self._emit_buffered_lines(self._stderr_buffer, self.stderr_received, True)
        # remove temporary netlist file now that Xyce no longer needs it
        self._cleanup_netlist_file()
        # emit terminal process state including cancellation and output path
        self.finished.emit(exit_code, exit_status.value, self._was_canceled, self._output_file_path)

    def _cleanup_netlist_file(self) -> None:
        # skip filesystem operations when netlist path is unavailable
        if not self._netlist_file_path:
            return
        # skip deletion when file was already removed elsewhere
        if not os.path.exists(self._netlist_file_path):
            return
        # remove temporary input netlist file after execution completes
        os.unlink(self._netlist_file_path)


def run_xyce_simulation(plugin_config: PluginConfig, netlist: str) -> XyceSimulationRunner:
    # fail fast when executable path is missing or not runnable
    if not plugin_config.is_xyce_executable_valid():
        raise ValueError("Configured Xyce executable path is invalid")
    # fail fast when simulation netlist text is empty
    if not netlist.strip():
        raise ValueError("Netlist content cannot be empty")
    # create a persistent temporary netlist file used by the external process
    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", suffix=".cir", prefix="kicad_xyce_", delete=False) as netlist_file:
        # write netlist text exactly as provided by the caller
        netlist_file.write(netlist)
        # capture path for process invocation and later cleanup
        netlist_file_path = netlist_file.name
    # allocate a deterministic temporary output path for Xyce raw data
    output_fd, output_file_path = tempfile.mkstemp(prefix="kicad_xyce_", suffix=".raw")
    # close the file descriptor because Xyce will write this path itself
    os.close(output_fd)
    # create the asynchronous runner that owns process and stream wiring
    runner = XyceSimulationRunner(plugin_config.xyce_executable_path, netlist_file_path, output_file_path)
    # launch Xyce immediately so caller can subscribe to runtime signals
    runner.start()
    # return runner to caller for signal subscription and cancellation
    return runner
