import shutil
import unittest

from slint_automation import expect, TestSession, launch


class PluginConfigDialogChecks(unittest.TestCase):

    def test_config_dialog_shows_configured_xyce_path(self) -> None:
        # arrange: resolve the xyce executable used to configure the application
        xyce = shutil.which("Xyce")
        # arrange: skip the scenario when the xyce executable is not available
        if xyce is None:
            self.skipTest("Xyce executable not found")
        # arrange: launch the application with the xyce executable
        with TestSession(launch(args=["--xyce", xyce]), self.id()) as app:
            # arrange: locate the toolbar tools by their slint type in declaration order
            tools = app.get_by_type("ToolbarButton")
            # step 1: open the plugin configuration dialog from the toolbar
            tools.nth(7).click()
            # arrange: locate the xyce path text box inside the dialog
            fields = app.get_by_type("LineEdit")
            # assert: exactly one text box is present in the dialog
            self.assertEqual(fields.count(), 1)
            # assert: the text box shows the configured xyce path
            expect(fields.nth(0)).to_have_text(xyce)
