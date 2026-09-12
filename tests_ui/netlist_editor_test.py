import shutil
import tempfile
import unittest
from pathlib import Path

from slint_automation import TestSession, expect, launch


class NetlistEditorChecks(unittest.TestCase):

    def test_edit_and_save_netlist_round_trip(self) -> None:
        # arrange: resolve the sample netlist shipped with the repository
        netlist = Path(__file__).resolve().parents[1] / "netlists" / "tran-simple-01.cir"
        # arrange: work on a scratch copy so the scenario never mutates the fixture
        with tempfile.TemporaryDirectory() as scratch:
            working_copy = Path(scratch) / "tran-simple-01.cir"
            shutil.copy(netlist, working_copy)
            # arrange: launch the application with the working copy
            with TestSession(launch(args=["--netlist", str(working_copy)]), self.id()) as app:
                # arrange: locate the netlist editor and the toolbar tools
                editor = app.get_by_id("NetlistEditor::input")
                tools = app.get_by_type("ToolbarButton")
                # assert: the editor shows the netlist file content
                expect(editor).to_have_text(netlist.read_text())
                # step 1: focus the editor and type an edit
                editor.click()
                editor.press("x")
                # assert: the edit marks the netlist dirty and enables save
                expect(tools.nth(1).child("Image")).to_have_opacity(1.0)
                # step 2: save the netlist from the toolbar
                tools.nth(1).click()
                # assert: the file on disk received the typed character
                saved = working_copy.read_text()
                self.assertNotEqual(saved, netlist.read_text())
                self.assertIn("x", saved)
                # assert: the save tool is disabled again after the save
                expect(tools.nth(1).child("Image")).to_have_opacity(0.4)
