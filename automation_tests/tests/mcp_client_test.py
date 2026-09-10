import http.server
import json
import threading
import unittest

from framework.errors import McpError
from framework.mcp_client import McpClient


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
        elif method == "tools/call":
            # echo the tool arguments back as a nested json text block
            text = json.dumps({"echo": body.get("params", {})})
            result = {"content": [{"type": "text", "text": text}]}
            # flag the boom tool as a tool-level failure
            if body.get("params", {}).get("name") == "boom":
                result = {"content": [{"type": "text", "text": "tool exploded"}], "isError": True}
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


class McpClientTransportChecks(unittest.TestCase):

    def setUp(self) -> None:
        # arrange
        self._server = http.server.HTTPServer(("127.0.0.1", 0), MockMcpHandler)
        self._server.requests = []
        self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()
        self._client = McpClient(self._server.server_address[1])

    def tearDown(self) -> None:
        # cleanup
        self._server.shutdown()
        self._server.server_close()

    def test_initialize_returns_server_info(self) -> None:
        # act
        info = self._client.initialize()
        # assert
        self.assertEqual(info["serverInfo"]["name"], "mock-mcp")

    def test_call_builds_json_rpc_envelope(self) -> None:
        # act
        self._client.initialize()
        # assert
        request = self._server.requests[0]
        self.assertEqual(request["jsonrpc"], "2.0")
        self.assertEqual(request["id"], 1)
        self.assertEqual(request["method"], "initialize")
        self.assertEqual(request["params"]["protocolVersion"], "2025-06-18")

    def test_call_raises_on_rpc_error(self) -> None:
        # act
        with self.assertRaises(McpError) as context:
            self._client.call("resources/list")
        # assert
        self.assertIn("-32601", str(context.exception))

    def test_call_raises_on_malformed_response(self) -> None:
        # arrange
        self._server.malformed = True
        # act
        with self.assertRaises(McpError) as context:
            self._client.call("initialize")
        # assert
        self.assertIn("malformed json response", str(context.exception))

    def test_call_tool_decodes_nested_json(self) -> None:
        # act
        result = self._client.call_tool("echo", {"x": 1})
        # assert
        self.assertEqual(result["echo"]["name"], "echo")
        self.assertEqual(result["echo"]["arguments"], {"x": 1})

    def test_call_tool_raises_on_is_error(self) -> None:
        # act
        with self.assertRaises(McpError) as context:
            self._client.call_tool("boom")
        # assert
        self.assertIn("tool exploded", str(context.exception))

    def test_connection_failure_raises_mcp_error(self) -> None:
        # arrange
        self._server.shutdown()
        self._server.server_close()
        # act
        with self.assertRaises(McpError):
            self._client.call("initialize")
