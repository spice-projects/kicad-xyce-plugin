import json
import urllib.error
import urllib.request
from typing import Any

from framework.errors import McpError
from framework.log import logger

DEFAULT_HTTP_TIMEOUT = 10.0
MCP_PROTOCOL_VERSION = "2025-06-18"
_LOGGER = logger()


class McpClient:

    def __init__(self, port: int) -> None:
        # build the endpoint url of the local mcp server
        self._endpoint = f"http://127.0.0.1:{port}/mcp"
        # track the sequential json-rpc request id
        self._next_id = 1
        # store the server info captured during initialize
        self._server_info: dict | None = None

    def connect(self) -> None:
        # run the initialize handshake
        self.initialize()
        # tell the server initialization is complete
        self._notify_initialized()

    def initialize(self) -> dict:
        # build the initialize handshake parameters
        params = {"protocolVersion": MCP_PROTOCOL_VERSION, "capabilities": {}, "clientInfo": {"name": "slint-test-framework", "version": "0.1.0"}}
        # send the initialize request and remember the server info
        result = self.call("initialize", params)
        # store the server info for later inspection
        self._server_info = result
        # expose the server info to callers
        return result

    def server_info(self) -> dict | None:
        # return the server info captured during initialize
        return self._server_info

    def list_tools(self) -> list[dict]:
        # return the tool definitions advertised by the server
        result = self.call("tools/list")
        return result["tools"]

    def call_tool(self, name: str, arguments: dict | None = None) -> Any:
        # invoke the tool by name through the tools/call method
        result = self.call("tools/call", {"name": name, "arguments": arguments or {}})
        # tool failures arrive as normal results flagged with isError
        if result.get("isError"):
            raise McpError(result["content"][0]["text"])
        # decode the text content blocks for the caller
        return _decode_content(result["content"])

    def call(self, method: str, params: dict | None = None) -> dict:
        # build the json-rpc request envelope with the next sequential id
        request = {"jsonrpc": "2.0", "id": self._next_id, "method": method}
        # expose the full protocol request at debug level
        _LOGGER.debug("mcp request: %s", json.dumps(request))
        # consume the request id
        self._next_id += 1
        # attach params when the request carries them
        if params is not None:
            request["params"] = params
        # post the request and keep the raw response body
        body = self._post(request)
        # expose the full protocol response at debug level
        _LOGGER.debug("mcp response: %s", body)
        try:
            # decode the json-rpc response envelope
            response = json.loads(body)
        except json.JSONDecodeError as error:
            # convert malformed responses into framework errors
            raise McpError(f"malformed json response: {body}") from error
        # surface json-rpc level errors as framework errors
        if "error" in response:
            raise McpError(f"json-rpc error {response['error'].get('code')}: {response['error'].get('message')}")
        # return the result payload on success
        return response["result"]

    def close(self) -> None:
        # http is stateless so there is nothing persistent to release
        pass

    def _notify_initialized(self) -> None:
        # notifications carry no request id and expect no response body
        self._post({"jsonrpc": "2.0", "method": "notifications/initialized"})

    def _post(self, payload: dict) -> str:
        # encode the payload as a utf-8 json http body
        data = json.dumps(payload).encode("utf-8")
        # build the http request for the local mcp endpoint
        request = urllib.request.Request(self._endpoint, data=data, headers={"Content-Type": "application/json"}, method="POST")
        try:
            # post the request and read the full response body
            with urllib.request.urlopen(request, timeout=DEFAULT_HTTP_TIMEOUT) as response:
                return response.read().decode("utf-8")
        except urllib.error.HTTPError as error:
            # convert http level failures into framework errors
            raise McpError(f"http error {error.code} from {self._endpoint}") from error
        except urllib.error.URLError as error:
            # connection failures mean the server is not reachable yet
            raise McpError(f"connection to {self._endpoint} failed: {error.reason}") from error


def _decode_content(content: list[dict]) -> Any:
    # json tool results carry pretty-printed json inside the first text block
    if content and content[0].get("type") == "text":
        try:
            # decode the nested json document
            return json.loads(content[0]["text"])
        except json.JSONDecodeError:
            # fall back to the raw text when the block is not json
            return content[0]["text"]
    # image and unknown block types are returned untouched
    return content
