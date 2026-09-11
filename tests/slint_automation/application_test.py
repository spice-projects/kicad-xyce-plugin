import http.server
import json
import os
import subprocess
import sys
import threading
import unittest
from unittest import mock

from slint_automation import launch
from slint_automation.errors import ApplicationStartupError
from slint_automation.mcp_client import McpClient
from slint_automation.slint_application import SlintApplication
from slint_automation.slint_client import SlintClient


class MockMcpHandler(http.server.BaseHTTPRequestHandler):

    def do_POST(self) -> None:
        # read the full request body and parse it as json
        length = int(self.headers.get("Content-Length", 0))
        body = json.loads(self.rfile.read(length))
        # record the request for later assertions
        self.server.requests.append(body)
        # respond with a non-json body when the server is in malformed mode
        if getattr(self.server, "malformed", False):
            data = b"not json"
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        # dispatch on the json-rpc method
        method = body.get("method")
        if method == "initialize":
            result = {"serverInfo": {"name": "mock-mcp"}}
            response = {"jsonrpc": "2.0", "id": body.get("id"), "result": result}
        elif method == "tools/call" and body.get("params", {}).get("name") == "list_windows":
            # serve the canned window list for window discovery
            text = json.dumps({"windowHandles": [{"index": "1", "generation": "1"}]})
            result = {"content": [{"type": "text", "text": text}]}
            response = {"jsonrpc": "2.0", "id": body.get("id"), "result": result}
        else:
            # every other method returns a json-rpc error
            response = {"jsonrpc": "2.0", "id": body.get("id"), "error": {"code": -32601, "message": f"method not found: {method}"}}
        # write the json response body
        data = json.dumps(response).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, format: str, *args: str) -> None:
        # silence the per-request console logging
        pass


class FakeProcess:

    def __init__(self, exit_code: int | None = None, hang_on_wait: bool = False) -> None:
        # exit_code is the code reported by poll once the process finished
        self._exit_code = exit_code
        # returncode mirrors the final exit code for startup error messages
        self.returncode = exit_code if exit_code is not None else 0
        # hang_on_wait makes wait raise a timeout until the process is killed
        self._hang_on_wait = hang_on_wait
        # _terminated tracks whether terminate was called
        self._terminated = False
        # _killed tracks whether kill was called
        self._killed = False
        # _terminate_count counts terminate invocations
        self._terminate_count = 0

    def poll(self) -> int | None:
        # report the exit code once the process finished
        return self._exit_code

    def terminate(self) -> None:
        # record the graceful termination request and finish the process
        self._terminated = True
        self._terminate_count += 1
        self._exit_code = 0
        self.returncode = 0

    def kill(self) -> None:
        # record the forced kill and release the hanging wait
        self._killed = True
        self._hang_on_wait = False
        self._exit_code = 0
        self.returncode = 0

    def wait(self, timeout: float | None = None) -> int:
        # simulate a hanging process until it is killed
        if self._hang_on_wait:
            raise subprocess.TimeoutExpired(cmd="fake-app", timeout=timeout)
        # report a zero exit code once the wait completes
        return 0

    def was_terminated(self) -> bool:
        # report whether terminate was requested
        return self._terminated

    def was_killed(self) -> bool:
        # report whether kill was requested
        return self._killed

    def terminate_count(self) -> int:
        # report how many times terminate was requested
        return self._terminate_count


class ApplicationLaunchChecks(unittest.TestCase):

    def setUp(self) -> None:
        # arrange: start the mock mcp server for the readiness handshake
        self._server = http.server.HTTPServer(("127.0.0.1", 0), MockMcpHandler)
        self._server.requests = []
        self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()
        # arrange: fake the application process
        self._process = FakeProcess()
        # arrange: point the port allocation at the mock server port
        port_patcher = mock.patch("slint_automation.slint_application._allocate_port", return_value=self._server.server_address[1])
        port_patcher.start()
        self.addCleanup(port_patcher.stop)
        # arrange: launch the application through the patched popen
        patcher = mock.patch("slint_automation.slint_application.subprocess.Popen", return_value=self._process)
        self._popen = patcher.start()
        self.addCleanup(patcher.stop)
        # arrange: launch the application for testing
        self._app = launch("/fake/xyce-studio", startup_timeout=5.0)
        # cleanup: shut the mock server down after the test
        self.addCleanup(self._server.shutdown)
        self.addCleanup(self._server.server_close)

    def tearDown(self) -> None:
        # cleanup: terminate the application after each test
        self._app.close()

    def test_application_launch(self) -> None:
        # assert: verify the application launched successfully
        self.assertIsNotNone(self._app)

    def test_application_has_client(self) -> None:
        # act: get the client from the application
        client = self._app.client()
        # assert: verify the client is not None
        self.assertIsNotNone(client)

    def test_application_status(self) -> None:
        # act: check the application status
        client = self._app.client()
        status = client.get_status()
        # assert: verify the application is running
        self.assertEqual(status, "running")

    def test_application_window(self) -> None:
        # act: get the application window
        client = self._app.client()
        window = client.get_window()
        # assert: verify the mock window handle is retrieved
        self.assertEqual(window, {"index": "1", "generation": "1"})

    def test_application_uses_dynamic_mcp_port(self) -> None:
        # assert: verify the app talks to the mock server port
        self.assertEqual(self._app.port(), self._server.server_address[1])

    def test_application_passes_mcp_port_env(self) -> None:
        # act: inspect the environment passed to the spawned process
        _, kwargs = self._popen.call_args
        # assert: verify the child received the mock server port
        self.assertEqual(kwargs["env"]["SLINT_MCP_PORT"], str(self._server.server_address[1]))

    def test_application_passes_command_line_arguments(self) -> None:
        # arrange: relaunch with command line arguments for the application
        self._popen.reset_mock()
        # act: launch with the file loading arguments
        launch("/fake/kicad-xyce-plugin", startup_timeout=5.0, args=["--netlist", "/tmp/amp.cir", "--raw=/tmp/out.raw"])
        # assert: verify the arguments follow the executable path
        args, _ = self._popen.call_args
        self.assertEqual(args[0][0], "/fake/kicad-xyce-plugin")
        self.assertEqual(args[0][1:], ["--netlist", "/tmp/amp.cir", "--raw=/tmp/out.raw"])

    def test_application_receives_clean_environment(self) -> None:
        # arrange: inject an external variable like an editor .env file would
        os.environ["KICAD_API_TOKEN"] = "external-value"
        self.addCleanup(os.environ.pop, "KICAD_API_TOKEN", None)
        # act: inspect the environment passed to the spawned process
        _, kwargs = self._popen.call_args
        # assert: verify the external variable never reaches the application
        self.assertNotIn("KICAD_API_TOKEN", kwargs["env"])
        # assert: verify the essential system variables are preserved
        self.assertIn("PATH", kwargs["env"])
        self.assertIn("HOME", kwargs["env"])

    def test_application_injects_explicit_environment(self) -> None:
        # arrange: relaunch with the plugin simulation variables injected
        self._popen.reset_mock()
        # arrange: snapshot the test process environment to detect leaks
        before = dict(os.environ)
        # act: launch with explicit application variables
        launch("/fake/kicad-xyce-plugin", startup_timeout=5.0, env={"KICAD_API_SOCKET": "ipc://test", "KICAD_API_TOKEN": "explicit"})
        # assert: verify the injected variables reach the application
        _, kwargs = self._popen.call_args
        self.assertEqual(kwargs["env"]["KICAD_API_SOCKET"], "ipc://test")
        self.assertEqual(kwargs["env"]["KICAD_API_TOKEN"], "explicit")
        # assert: verify the launch left the test process environment untouched
        self.assertEqual(os.environ, before)


class ApplicationLifecycleChecks(unittest.TestCase):

    def setUp(self) -> None:
        # arrange: start the mock mcp server for the readiness handshake
        self._server = http.server.HTTPServer(("127.0.0.1", 0), MockMcpHandler)
        self._server.requests = []
        self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()
        self.addCleanup(self._server.shutdown)
        self.addCleanup(self._server.server_close)

    def test_close_terminates_process(self) -> None:
        # arrange
        process = FakeProcess()
        app = SlintApplication(process, SlintClient(McpClient(1)), 1)
        # act
        app.close()
        # assert
        self.assertTrue(process.was_terminated())

    def test_close_is_idempotent(self) -> None:
        # arrange
        process = FakeProcess()
        app = SlintApplication(process, SlintClient(McpClient(1)), 1)
        # act
        app.close()
        app.close()
        # assert
        self.assertEqual(process.terminate_count(), 1)

    def test_close_kills_when_wait_times_out(self) -> None:
        # arrange
        process = FakeProcess(hang_on_wait=True)
        app = SlintApplication(process, SlintClient(McpClient(1)), 1)
        # act
        app.close()
        # assert
        self.assertTrue(process.was_killed())

    def test_launch_raises_after_retries_when_process_exits(self) -> None:
        # arrange
        process = FakeProcess(exit_code=3)
        port_patcher = mock.patch("slint_automation.slint_application._allocate_port", return_value=self._server.server_address[1])
        port_patcher.start()
        self.addCleanup(port_patcher.stop)
        patcher = mock.patch("slint_automation.slint_application.subprocess.Popen", return_value=process)
        popen = patcher.start()
        self.addCleanup(patcher.stop)
        # act
        with self.assertRaises(ApplicationStartupError) as context:
            launch("/fake/xyce-studio", startup_timeout=0.3)
        # assert
        self.assertIn("failed to launch application", str(context.exception))
        self.assertEqual(popen.call_count, 3)

    def test_launch_closes_process_when_server_never_ready(self) -> None:
        # arrange: make the server return malformed responses
        self._server.malformed = True
        process = FakeProcess()
        port_patcher = mock.patch("slint_automation.slint_application._allocate_port", return_value=self._server.server_address[1])
        port_patcher.start()
        self.addCleanup(port_patcher.stop)
        patcher = mock.patch("slint_automation.slint_application.subprocess.Popen", return_value=process)
        popen = patcher.start()
        self.addCleanup(patcher.stop)
        # act
        with self.assertRaises(ApplicationStartupError):
            launch("/fake/xyce-studio", startup_timeout=0.3)
        # assert
        self.assertEqual(popen.call_count, 3)
        self.assertTrue(process.was_terminated())


class ConfigurationIsolationChecks(unittest.TestCase):

    def setUp(self) -> None:
        # arrange: start the mock mcp server for the readiness handshake
        self._server = http.server.HTTPServer(("127.0.0.1", 0), MockMcpHandler)
        self._server.requests = []
        self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()
        # arrange: fake the application process
        self._process = FakeProcess()
        # arrange: point the port allocation at the mock server port
        port_patcher = mock.patch("slint_automation.slint_application._allocate_port", return_value=self._server.server_address[1])
        port_patcher.start()
        self.addCleanup(port_patcher.stop)
        # arrange: launch the application through the patched popen
        patcher = mock.patch("slint_automation.slint_application.subprocess.Popen", return_value=self._process)
        self._popen = patcher.start()
        self.addCleanup(patcher.stop)
        # cleanup: shut the mock server down after the test
        self.addCleanup(self._server.shutdown)
        self.addCleanup(self._server.server_close)

    def _configuration_variable(self) -> str:
        # report the environment variable carrying the application configuration root
        return "APPDATA" if sys.platform == "win32" else "XDG_CONFIG_HOME"

    def test_application_isolates_persistent_configuration(self) -> None:
        # arrange: poison the test process configuration root like an external tool would
        os.environ[self._configuration_variable()] = "/external/config"
        self.addCleanup(os.environ.pop, self._configuration_variable(), None)
        # act: launch the application
        launch("/fake/xyce-studio", startup_timeout=5.0)
        # act: inspect the environment passed to the spawned process
        _, kwargs = self._popen.call_args
        # assert: the external configuration root never reaches the application
        self.assertNotEqual(kwargs["env"][self._configuration_variable()], "/external/config")
        # assert: the child configuration root is an isolated temporary directory
        self.assertTrue(os.path.basename(kwargs["env"][self._configuration_variable()]).startswith("xyce-studio-config-"))

    def test_application_removes_isolated_configuration_on_close(self) -> None:
        # arrange: launch the application
        app = launch("/fake/xyce-studio", startup_timeout=5.0)
        # arrange: read the isolated configuration root passed to the application
        _, kwargs = self._popen.call_args
        config_dir = kwargs["env"][self._configuration_variable()]
        # assert: the isolated directory exists while the application runs
        self.assertTrue(os.path.isdir(config_dir))
        # act: close the application
        app.close()
        # assert: the isolated directory is removed after the application closed
        self.assertFalse(os.path.exists(config_dir))


class CallerConfigurationChecks(unittest.TestCase):

    def setUp(self) -> None:
        # arrange: start the mock mcp server for the readiness handshake
        self._server = http.server.HTTPServer(("127.0.0.1", 0), MockMcpHandler)
        self._server.requests = []
        self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()
        # arrange: fake the application process
        self._process = FakeProcess()
        # arrange: point the port allocation at the mock server port
        port_patcher = mock.patch("slint_automation.slint_application._allocate_port", return_value=self._server.server_address[1])
        port_patcher.start()
        self.addCleanup(port_patcher.stop)
        # arrange: launch the application through the patched popen
        patcher = mock.patch("slint_automation.slint_application.subprocess.Popen", return_value=self._process)
        self._popen = patcher.start()
        self.addCleanup(patcher.stop)
        # cleanup: shut the mock server down after the test
        self.addCleanup(self._server.shutdown)
        self.addCleanup(self._server.server_close)

    def _configuration_variable(self) -> str:
        # report the environment variable carrying the application configuration root
        return "APPDATA" if sys.platform == "win32" else "XDG_CONFIG_HOME"

    def test_caller_provided_configuration_root_wins(self) -> None:
        # arrange: launch the application with an explicit configuration root
        launch("/fake/xyce-studio", startup_timeout=5.0, env={self._configuration_variable(): "/shared/config"})
        # act: inspect the environment passed to the spawned process
        _, kwargs = self._popen.call_args
        # assert: the caller configuration root reaches the application untouched
        self.assertEqual(kwargs["env"][self._configuration_variable()], "/shared/config")
