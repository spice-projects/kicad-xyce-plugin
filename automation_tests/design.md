# Slint MCP Integration Testing Framework

## 1. Objective

Build a lightweight, independent integration-testing framework for a C++ desktop application built with Slint.

The framework must use the **MCP server embedded in the Slint application** as the UI automation interface.

The goal is to allow integration tests to launch the real application, interact with its real Slint UI, inspect UI state, and make assertions.

The framework should provide a simple, Playwright-inspired API without depending on Playwright or Slint's commercial `slint_testing` package.

Example target usage:

```python
def test_open_project(app):
    app.get_by_id("open-project").click()

    app.get_by_id("filename").fill("test-project.kicad_pro")

    app.get_by_id("open").click()

    expect(
        app.get_by_id("project-name")
    ).to_have_text("test-project")
```

The first implementation should prioritize correctness, simplicity, and a clean abstraction boundary over feature completeness.

---

# 2. Important constraints

## 2.1 Do not use Slint `slint_testing`

Do NOT depend on:

* `slint_testing`
* Slint's commercial GUI Test Framework
* any commercial Slint testing package

The framework must communicate with the application's embedded MCP server directly.

## 2.2 Do not use Selenium or Playwright

Do not make Playwright or Selenium dependencies of the framework.

The API may be inspired by Playwright's Locator/Expect model, but the implementation must be independent.

A future Playwright adapter may be considered, but it is explicitly out of scope for the initial implementation.

## 2.3 Do not implement an MCP server

The application already hosts the MCP server.

The framework is an **MCP client**.

Architecture:

```
Python test
    |
    v
Test Framework
    |
    v
MCP client
    |
    | Streamable HTTP / JSON-RPC
    v
Slint MCP server
    |
    v
Real C++/Slint application
```

## 2.4 Do not guess the Slint MCP API

Before implementing the MCP client:

1. Inspect the version of Slint used by the target application.
2. Inspect the actual Slint MCP server implementation/source.
3. Determine:

   * MCP endpoint behavior
   * initialization sequence
   * protocol version
   * available tools
   * tool names
   * tool arguments
   * tool return values
   * element identification
   * element handles
   * actions
   * property inspection
   * screenshots
   * error behavior
4. Implement against the actual API.

Do not invent MCP tool names or schemas based on examples from other versions.

The framework should isolate all Slint/MCP-specific assumptions in one layer.

---

# 3. Current Slint assumptions

The current Slint documentation states:

* the embedded MCP server is enabled using the `mcp` feature;
* when `SLINT_MCP_PORT` is set at runtime, the application starts an HTTP server implementing MCP Streamable HTTP;
* MCP clients can inspect and interact with the running UI;
* the MCP functionality is intended as a developer/debug feature and should not be enabled in production builds.

The implementation must verify these details against the exact Slint version used by the application rather than assuming the current documentation exactly matches the installed version.

The application is C++ and uses Slint.

The test framework should therefore treat the application as an external executable.

---

# 4. Repository structure

Create a dedicated testing framework package, preferably under a structure similar to:

```text
tests/
    framework/
        __init__.py
        mcp_client.py
        slint_app.py
        locator.py
        assertions.py
        errors.py
        process.py
        artifacts.py

    conftest.py

    integration/
        test_smoke.py
        ...
```

The exact location may be adapted to the existing repository structure.

Do not reorganize unrelated application code.

---

# 5. Architecture

The framework consists of five primary layers.

```text
                    pytest
                      |
                      v
                 SlintApp
                      |
              +-------+-------+
              |               |
              v               v
        Locator/Element   Artifacts
              |
              v
          MCPClient
              |
              v
        Slint MCP Server
              |
              v
        C++ Application
```

## Layer 1: MCPClient

Responsible exclusively for communication with MCP.

It must not know anything about Slint UI concepts.

Responsibilities:

* establish HTTP connection;
* perform MCP initialization;
* maintain JSON-RPC request IDs;
* send requests;
* invoke MCP tools;
* parse responses;
* detect MCP errors;
* expose raw tool results to higher layers.

It should NOT contain methods such as:

```text
click_button()
find_element()
get_text()
```

Those belong to higher layers.

Suggested API:

```python
class McpClient:
    def __init__(self, endpoint: str):
        ...

    def connect(self) -> None:
        ...

    def initialize(self) -> dict:
        ...

    def call(self, method: str, params: dict | None = None) -> dict:
        ...

    def call_tool(
        self,
        name: str,
        arguments: dict | None = None,
    ) -> dict:
        ...

    def close(self) -> None:
        ...
```

The exact implementation must match the actual MCP transport used by the Slint version under test.

---

# 6. SlintApp

`SlintApp` is the main public object used by tests.

It represents a running instance of the application.

Suggested API:

```python
class SlintApp:
    @classmethod
    def launch(
        cls,
        executable: str,
        *,
        args: list[str] | None = None,
        env: dict[str, str] | None = None,
        cwd: str | None = None,
        startup_timeout: float = 10.0,
    ) -> "SlintApp":
        ...

    def get_by_id(self, element_id: str) -> "Locator":
        ...

    def get_by_role(
        self,
        role: str,
        *,
        name: str | None = None,
    ) -> "Locator":
        ...

    def screenshot(self, path: str) -> None:
        ...

    def close(self) -> None:
        ...
```

The exact API can evolve.

The important principle is that tests interact with `SlintApp`, not `McpClient`.

Example:

```python
app.get_by_id("run-button").click()
```

not:

```python
app.mcp.call_tool(...)
```

---

# 7. Application process management

`SlintApp.launch()` must:

1. Start the application as a subprocess.
2. Allocate an available TCP port.
3. Set `SLINT_MCP_PORT` for the child process.
4. Preserve the caller's environment unless explicitly overridden.
5. Start the process.
6. Wait until the MCP server is available.
7. Initialize the MCP connection.
8. Return a ready `SlintApp`.

Do not use a fixed port such as `8080`.

Parallel tests must be possible eventually.

The framework should therefore allocate a free port dynamically.

Pseudo-flow:

```text
allocate_port()
       |
       v
build environment
       |
       +-- SLINT_MCP_PORT=<port>
       |
       v
spawn application
       |
       v
wait for MCP endpoint
       |
       v
MCP initialize
       |
       v
SlintApp ready
```

---

# 8. Process lifecycle

The application process must be owned by `SlintApp`.

Use a context manager:

```python
with SlintApp.launch("./MyApplication") as app:
    ...
```

On normal exit:

```text
terminate application
wait
```

On failure:

```text
capture artifacts
terminate application
wait
```

If graceful termination fails, use a stronger termination mechanism.

The implementation must not leave orphan application processes.

This is especially important on:

* macOS
* Windows
* Linux

because the application is a GUI executable rather than a normal command-line process.

---

# 9. Locator

Implement a Playwright-inspired but framework-specific `Locator`.

A locator represents a way to identify a UI element.

Example:

```python
button = app.get_by_id("run-button")
```

The locator should preferably resolve the element lazily.

Suggested API:

```python
class Locator:
    def click(self) -> None:
        ...

    def fill(self, text: str) -> None:
        ...

    def type(self, text: str) -> None:
        ...

    def press(self, key: str) -> None:
        ...

    def text(self) -> str:
        ...

    def property(self, name: str):
        ...

    def exists(self) -> bool:
        ...

    def screenshot(self, path: str) -> None:
        ...
```

Do not implement all methods if the Slint MCP server does not support the corresponding operation.

First determine the available MCP tools.

---

# 10. Element identification

The first and preferred locator should be:

```python
app.get_by_id("some-element-id")
```

The application should use stable Slint identifiers intended for testing.

For example:

```slint
Button {
    accessible-id: "run-button";
    text: "Run";
}
```

Use the appropriate Slint property/mechanism supported by the actual version of Slint.

Do not require tests to locate elements based on screen coordinates.

Do not use screenshots or OCR for normal UI element identification.

Coordinates should only be considered later for cases where semantic element interaction is impossible.

---

# 11. Locator resolution

A locator should not necessarily resolve when it is created.

This:

```python
button = app.get_by_id("run-button")
```

should create a locator.

This:

```python
button.click()
```

should resolve the locator and perform the action.

This allows the application UI to change between creation and use.

It also allows future retry/wait behavior.

---

# 12. Element handles

The Slint MCP server may return internal element handles.

Those handles must be hidden from test code.

Bad:

```python
handle = app.find(...)
app.click(handle)
```

Good:

```python
app.get_by_id("run-button").click()
```

The framework owns the mapping:

```text
Locator
   |
   v
resolve element
   |
   v
Slint element handle
   |
   v
MCP operation
```

If handles expire or become invalid because the UI changes, the framework should re-resolve the locator.

---

# 13. Assertions

Create an `expect()` API.

Example:

```python
expect(
    app.get_by_id("status")
).to_have_text("Simulation complete")
```

Suggested initial API:

```python
def expect(locator: Locator) -> Expect:
    ...


class Expect:
    def to_have_text(self, expected: str) -> None:
        ...

    def to_have_property(self, name: str, expected) -> None:
        ...

    def to_be_visible(self) -> None:
        ...

    def to_be_enabled(self) -> None:
        ...

    def to_exist(self) -> None:
        ...
```

Also support negated assertions:

```python
expect(locator).not_to_be_visible()
```

or:

```python
expect(locator).not_to_have_text("Error")
```

Use whichever design results in the cleanest API.

---

# 14. Assertions must support waiting

Integration tests frequently fail because the test checks state before asynchronous UI/application work has completed.

Therefore assertions should eventually support polling.

Example:

```python
expect(
    app.get_by_id("status")
).to_have_text(
    "Simulation complete",
    timeout=10.0,
)
```

Initial implementation can use a framework-wide default timeout:

```python
DEFAULT_ASSERTION_TIMEOUT = 5.0
```

The assertion should:

1. Resolve locator.
2. Query current state.
3. Check condition.
4. If false, sleep briefly.
5. Retry.
6. Fail only after timeout.

Do not use excessively aggressive polling.

Make polling interval configurable.

---

# 15. Screenshots

Screenshots are important for debugging integration failures.

Provide:

```python
app.screenshot("artifacts/failure.png")
```

and, if supported:

```python
app.get_by_id("main-window").screenshot(
    "artifacts/window.png"
)
```

The Slint MCP server already provides screenshot functionality in supported configurations; determine the exact tool and response format before implementation.

Do not implement screenshot capture independently using OS-level screenshot tools unless the MCP interface cannot provide it.

---

# 16. Failure artifacts

When an integration test fails, collect as much useful diagnostic information as practical.

At minimum:

```text
artifacts/
    <test-name>/
        screenshot.png
        stdout.txt
        stderr.txt
```

If the MCP API makes UI-tree information available, also capture:

```text
ui-tree.json
```

If possible, capture:

```text
mcp-response.json
```

for the failing operation.

The framework should make artifact collection automatic through pytest integration where practical.

---

# 17. Pytest integration

Use pytest as the initial test runner.

Create a fixture:

```python
@pytest.fixture
def app():
    with SlintApp.launch(
        APPLICATION_PATH
    ) as app:
        yield app
```

Tests should then be concise:

```python
def test_application_starts(app):
    expect(
        app.get_by_id("main-window")
    ).to_exist()
```

Do not put application-specific behavior into the generic fixture.

Application-specific fixtures can be added later.

---

# 18. Test configuration

Do not hardcode the application executable.

Support configuration through:

1. pytest command-line options;
2. environment variables;
3. configuration file.

For example:

```text
SLINT_TEST_APPLICATION=/path/to/MyApplication
```

or a pytest option:

```text
pytest --application ./build/MyApplication
```

The exact mechanism should follow the repository's existing testing conventions.

---

# 19. Configuration object

Avoid scattering configuration throughout the code.

Create a configuration object such as:

```python
@dataclass
class TestConfig:
    executable: str
    startup_timeout: float = 10.0
    assertion_timeout: float = 5.0
    poll_interval: float = 0.1
    artifact_directory: Path = ...
```

Additional options can be added later.

---

# 20. Error handling

Define framework-specific exceptions.

At minimum:

```python
class SlintTestError(Exception):
    pass


class ApplicationStartupError(SlintTestError):
    pass


class McpError(SlintTestError):
    pass


class LocatorError(SlintTestError):
    pass


class AssertionError(SlintTestError):
    pass
```

Do not shadow Python's built-in `AssertionError` unless there is a strong reason.

Prefer names such as:

```python
SlintAssertionError
```

if a custom assertion exception is required.

Error messages must contain useful context.

Example:

```text
Timed out waiting for element 'simulation-status'
to have text:

Expected:
    "Simulation complete"

Last observed:
    "Running simulation..."

Timeout:
    5.0 seconds
```

---

# 21. Logging

The framework should support debug logging.

Example:

```text
[slint-test] launching application
[slint-test] MCP port: 53142
[slint-test] waiting for MCP server
[slint-test] MCP initialized
[slint-test] resolving element: run-button
[slint-test] invoking click
```

Do not print excessive protocol-level details by default.

Provide a debug mode that can expose MCP requests/responses.

---

# 22. Security

The embedded MCP server is intended for local development/testing.

The test framework must therefore:

* connect only to localhost;
* not expose the MCP endpoint externally;
* not require authentication;
* not add network exposure;
* never enable the MCP server in release builds.

Do not modify the application to bind the MCP server to a public interface.

---

# 23. Production build separation

The application must have separate testing/development and production configurations.

Conceptually:

```text
Production:
    MCP disabled

Integration-test build:
    MCP enabled
```

Do not make the application depend on the testing framework at runtime in production.

Do not add testing-specific code paths to normal application behavior unless required by Slint itself.

---

# 24. First milestone: MCP connectivity

Do NOT implement the complete framework immediately.

First implement a minimal MCP client capable of:

1. Launching the application.
2. Waiting for the MCP endpoint.
3. Initializing MCP.
4. Listing available tools.
5. Calling one read-only tool.
6. Printing the result.

Create a smoke test:

```python
def test_mcp_connection(app):
    ...
```

Acceptance criteria:

* application starts;
* MCP endpoint becomes available;
* MCP initialization succeeds;
* at least one Slint UI inspection operation succeeds;
* application shuts down cleanly.

Do not proceed to locator abstractions until this works.

---

# 25. Second milestone: UI inspection

Implement:

```python
app.get_by_id(...)
```

and whatever internal operations are necessary to resolve an element.

Create a test that:

1. launches the real application;
2. finds a known UI element;
3. reads a property/text value;
4. asserts the expected value.

Example:

```python
def test_main_window(app):
    expect(
        app.get_by_id("status")
    ).to_have_text("Ready")
```

---

# 26. Third milestone: interaction

Implement:

```python
click()
fill()
type()
press()
```

only for operations actually supported by the Slint MCP interface.

Create a test such as:

```python
def test_button(app):
    app.get_by_id("run-button").click()

    expect(
        app.get_by_id("status")
    ).to_have_text("Running")
```

---

# 27. Fourth milestone: assertions and waiting

Implement:

```python
expect(locator).to_have_text(...)
expect(locator).to_have_property(...)
expect(locator).to_exist()
expect(locator).to_be_visible()
```

Add polling and timeout support.

Create tests demonstrating asynchronous state transitions.

---

# 28. Fifth milestone: diagnostics

Implement:

* screenshots;
* stdout capture;
* stderr capture;
* automatic failure artifacts;
* useful exception messages;
* debug logging.

At this point the framework should be usable for real integration testing.

---

# 29. Sixth milestone: first real application tests

Only after the framework itself is stable should application-specific tests be added.

Do not start by testing complicated application workflows.

Begin with smoke tests:

```text
application starts
main window appears
expected initial state
basic navigation
basic dialog
basic input
basic save/open workflow
```

Then move to complex workflows.

---

# 30. Test design principles

Tests should interact with the application through the same mechanisms a user would use.

Prefer:

```python
app.get_by_id("save-button").click()
```

over:

```python
application_internal_state.save()
```

Do not directly manipulate C++ business logic from integration tests.

The purpose of these tests is to verify integration between:

```text
UI
+
C++ application logic
+
application state
+
external processes/services
```

Unit tests remain responsible for testing individual C++ components.

---

# 31. Stable UI identifiers

Application UI code should use stable identifiers for important testable elements.

For example:

```slint
Button {
    accessible-id: "save-button";
    text: "Save";
}
```

Use semantic identifiers:

Good:

```text
save-button
open-project
simulation-run
simulation-status
project-name
```

Bad:

```text
button1
rectangle7
foo
element123
```

IDs should describe the semantic purpose of the element rather than its implementation.

---

# 32. Do not over-automate selectors initially

Initially support only the most reliable selector:

```python
get_by_id()
```

Later consider:

```python
get_by_role()
get_by_text()
get_by_label()
```

Only implement them if the underlying Slint accessibility/introspection API provides reliable information.

Do not emulate browser DOM semantics unnecessarily.

---

# 33. Do not use pixel-based testing initially

Do not make screenshot comparison the primary test mechanism.

The first framework should validate:

```text
element exists
element state
element properties
element actions
application state
```

Pixel comparison can be added as a separate feature later.

---

# 34. Future extension: Playwright-style API

The API should be designed so that the following is possible later:

```python
app.get_by_id("foo")
app.get_by_role("button", name="Run")
expect(locator).to_have_text("Done")
```

But do not attempt to become Playwright.

The framework should remain a native Slint testing abstraction.

---

# 35. Future extension: TypeScript client

If the Python implementation proves useful, a TypeScript implementation may eventually be added.

It should communicate with the same MCP endpoint:

```text
Python framework ─┐
                  |
TypeScript client ├──> Slint MCP
                  |
future clients ───┘
```

Therefore the MCP protocol layer must remain independent from the test-language API.

---

# 36. Future extension: Playwright adapter

A Playwright adapter is explicitly out of scope for the initial implementation.

If later desired, it could expose a Playwright-like API or integrate with Playwright's test runner.

Do not compromise the initial framework architecture to support this prematurely.

---

# 37. Dependencies

Keep dependencies minimal.

Preferred initial dependencies:

```text
pytest
HTTP/MCP client implementation
```

Use an existing MCP client library only if:

1. it supports the exact Streamable HTTP MCP protocol required by the Slint version;
2. it is stable;
3. it does not introduce unnecessary complexity.

Otherwise implement the minimal MCP transport required by Slint.

Do not add Selenium.

Do not add Playwright.

Do not add another GUI automation library.

Do not add OCR.

---

# 38. Coding standards

Follow the existing repository's Python/C++ conventions.

For Python:

* type annotations;
* dataclasses where appropriate;
* clear exception hierarchy;
* no global mutable state;
* context managers for process ownership;
* small classes with clear responsibilities;
* unit tests for framework internals.

Do not introduce unnecessary abstraction layers.

---

# 39. Framework unit tests

The framework itself must have unit tests.

At minimum test:

```text
MCP request construction
MCP response parsing
MCP errors
request IDs
timeouts
port allocation
process startup
process shutdown
locator behavior
assertion polling
timeout handling
```

Do not require a real Slint application for tests of the MCP transport layer.

Use mocked HTTP/MCP responses where appropriate.

Then use a small real Slint application for end-to-end framework tests.

---

# 40. Reference test application

If necessary, create a minimal test-only Slint application:

```text
test-app/
    main.cpp
    test-window.slint
```

It should contain:

```text
Button
Text
LineEdit/TextInput
CheckBox
ComboBox
```

and stable accessibility IDs.

Use this application to validate the framework independently of the real production application.

This avoids confusing framework failures with application failures.

---

# 41. Acceptance criteria for version 0.1

Version 0.1 is complete when all of the following are true:

### Process

* [ ] Test framework can launch the real C++ application.
* [ ] MCP port is dynamically allocated.
* [ ] Application receives `SLINT_MCP_PORT`.
* [ ] Framework waits for MCP readiness.
* [ ] Application is terminated correctly.
* [ ] No orphan application processes remain.

### MCP

* [ ] MCP initialization works.
* [ ] MCP tool invocation works.
* [ ] MCP errors are converted into useful Python exceptions.
* [ ] MCP implementation is isolated in `McpClient`.

### UI

* [ ] UI element can be located by stable ID.
* [ ] UI text/property can be read.
* [ ] At least one UI action can be performed.
* [ ] Element handles are hidden from tests.

### Assertions

* [ ] `expect(locator)` exists.
* [ ] text assertion works.
* [ ] existence assertion works.
* [ ] at least one property assertion works.
* [ ] assertions support timeout/polling.

### Diagnostics

* [ ] screenshots can be captured.
* [ ] stdout/stderr are captured.
* [ ] failed tests generate useful artifacts.

### Testing

* [ ] framework unit tests exist;
* [ ] framework end-to-end tests exist;
* [ ] at least one real application integration test exists.

---

# 42. Agent instructions

The coding agent must work incrementally.

Do not implement the entire framework in one change.

Use this sequence:

```text
Phase 1
  Inspect repository
  Inspect Slint version
  Inspect actual MCP implementation
  Determine exact protocol/tools

Phase 2
  Implement MCP client
  Add MCP connectivity smoke test

Phase 3
  Implement SlintApp
  Implement process management

Phase 4
  Implement Locator
  Implement get_by_id()

Phase 5
  Implement basic UI actions

Phase 6
  Implement expect/assertions

Phase 7
  Implement diagnostics/artifacts

Phase 8
  Add real application integration tests
```

After each phase:

1. Build.
2. Run unit tests.
3. Run integration tests where applicable.
4. Fix failures.
5. Only then continue.

Do not make assumptions about Slint MCP APIs without inspecting the installed/current source.

---

# 43. Important architectural rule

The following dependency direction must be maintained:

```text
Tests
  |
  v
SlintApp / Locator / Expect
  |
  v
McpClient
  |
  v
MCP protocol
```

Never reverse this relationship.

`McpClient` must not depend on:

```text
Locator
SlintApp
pytest
application-specific concepts
```

`Locator` must not know how HTTP or JSON-RPC works.

`SlintApp` must not construct raw MCP requests directly.

This separation is the primary architectural requirement.

---

# 44. Desired final test experience

The final framework should make tests look approximately like:

```python
def test_run_simulation(app):
    expect(
        app.get_by_id("simulation-status")
    ).to_have_text("Ready")

    app.get_by_id("run-simulation").click()

    expect(
        app.get_by_id("simulation-status")
    ).to_have_text(
        "Simulation complete",
        timeout=30,
    )
```

The test author should not need to know:

* MCP;
* JSON-RPC;
* HTTP;
* TCP ports;
* element handles;
* Slint internal implementation;
* process management.

Those are framework implementation details.

The framework's purpose is to make **real Slint application integration tests simple and maintainable**.

---

# 45. Explicit non-goals

Do NOT implement initially:

* Playwright compatibility;
* Selenium compatibility;
* browser automation;
* OCR;
* computer-vision element detection;
* pixel-perfect screenshot testing;
* distributed test execution;
* remote application testing;
* authentication;
* production MCP support;
* a replacement for pytest;
* a replacement for Slint's commercial GUI Test Framework.

The objective is a small, reliable test framework built on the MCP server that already exists in the Slint application.
