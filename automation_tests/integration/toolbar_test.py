import os

from pathlib import Path

from framework import launch
from framework.assertions import expect
from framework.test_case import SlintTestCase


class TestToolbarInitialChecks(SlintTestCase):

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

    def test_toolbar_contains_nine_tools_in_expected_state(self) -> None:
        # arrange: locate the toolbar tools by their slint type in declaration order
        tools = self._app.get_by_type("ToolbarButton")
        # assert: exactly nine action tools are present
        self.assertEqual(tools.count(), 9)
        # assert: every tool reports its initial state from left to right:
        # open enabled, save disabled, netlist disabled, simulation charts
        # disabled, simulation output disabled, run simulation disabled,
        # configure simulation disabled, plugin config enabled, exit enabled
        expected_states = [True, False, False, False, False, False, False, True, True]
        for index, expected_enabled in enumerate(expected_states):
            # the disabled state is rendered by the dimmed icon inside the tool
            icon = tools.nth(index).child("Image")
            # enabled tools render their icon at full opacity
            if expected_enabled:
                expect(icon).to_have_opacity(1.0)
            # disabled tools render their icon dimmed
            else:
                expect(icon).to_have_opacity(0.4)
