import os

from pathlib import Path

from framework import launch
from framework.assertions import expect
from framework.test_case import SlintTestCase


class ApplicationSmokeChecks(SlintTestCase):

    def setUp(self) -> None:
        # arrange: initialize the base test case hooks
        super().setUp()
        # arrange: use the override executable when provided
        executable = os.environ.get("SLINT_TEST_APPLICATION")
        # arrange: fall back to the debug build at the repository root
        if not executable:
            executable = str(Path(__file__).resolve().parents[2] / ".build-debug" / "kicad-xyce-plugin")
        # arrange: launch the real application with a dynamic mcp port
        self.start_session(launch(executable))

    def test_application_starts(self) -> None:
        # assert
        self.assertTrue(self._app.is_running())

    def test_main_window_appears(self) -> None:
        # act
        window = self._app.client().get_window()
        # assert
        self.assertIsNotNone(window)

    def test_toolbar_element_is_located(self) -> None:
        # assert
        expect(self._app.get_by_id("MainWindow::toolbar")).to_exist(timeout=5.0)

    def test_initial_status_text_is_empty(self) -> None:
        # assert
        expect(self._app.get_by_role("Text")).to_have_text("", timeout=5.0)
