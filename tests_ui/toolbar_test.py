import unittest

from slint_automation.assertions import expect
from slint_automation.session import TestSession
from slint_automation.slint_application import launch


class ToolbarInitialChecks(unittest.TestCase):

    def test_toolbar_contains_nine_tools_in_expected_state(self) -> None:
        # arrange: launch the application in a session scoped to this test
        with TestSession(launch(), self.id()) as app:
            # arrange: locate the toolbar tools by their slint type in declaration order
            tools = app.get_by_type("ToolbarButton")
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
