import unittest
from pathlib import Path

from slint_automation import TestSession, expect, launch


class ConfigureSimulationChecks(unittest.TestCase):

    def test_transient_parameters_round_trip(self) -> None:
        # arrange: resolve the sample netlist shipped with the repository
        netlist = Path(__file__).resolve().parents[1] / "netlists" / "tran-simple-01.cir"
        # arrange: launch the application with the netlist through the command line
        with TestSession(launch(args=["--netlist", str(netlist)]), self.id()) as app:
            # step 1: open the configure simulation dialog from the toolbar
            app.get_by_type("ToolbarButton").nth(6).click()
            # arrange: locate the transient analysis form fields
            fields = app.get_by_type("LineEdit")
            fields.nth(0).wait_for_exists()
            # assert: the form fields show the .TRAN directive values
            expect(fields.nth(0)).to_have_text("1u")
            expect(fields.nth(1)).to_have_text("20m")
            expect(fields.nth(2)).to_have_text("0")
            # step 2: edit the simulation end time and accept the dialog
            fields.nth(1).fill("25m")
            root = app.client().get_window_properties()["rootElementHandle"]
            ok = [handle for handle in app.client().find_by_type_in(root, "Button") if app.client().get_element_properties(handle).get("accessibleLabel") == "OK"][0]
            app.client().click_element(ok)
            # assert: the dialog closed and the netlist directive was rewritten
            fields.nth(0).wait_for_gone()
            tran_lines = [line for line in app.get_by_id("NetlistEditor::input").text().splitlines() if line.strip().startswith(".TRAN")]
            self.assertEqual(tran_lines, [".TRAN 1u 25m 0"])
            # step 3: reopen the dialog to verify the accepted values persisted
            app.get_by_type("ToolbarButton").nth(6).click()
            fields.nth(0).wait_for_exists()
            # assert: the end time field shows the accepted value
            expect(fields.nth(1)).to_have_text("25m")

    def test_dc_sweep_validation_rejects_empty_sweep(self) -> None:
        # arrange: resolve the sample netlist (transient only, no .DC directive)
        netlist = Path(__file__).resolve().parents[1] / "netlists" / "tran-simple-01.cir"
        # arrange: launch the application with the netlist through the command line
        with TestSession(launch(args=["--netlist", str(netlist)]), self.id()) as app:
            # step 1: open the configure simulation dialog from the toolbar
            app.get_by_type("ToolbarButton").nth(6).click()
            root = app.client().get_window_properties()["rootElementHandle"]
            ok = [handle for handle in app.client().find_by_type_in(root, "Button") if app.client().get_element_properties(handle).get("accessibleLabel") == "OK"][0]
            # step 2: switch to the dc analysis tab (declaration order: .op, .dc, .tran, ...)
            tabs = app.client().find_by_type_in(root, "TabButton")
            app.client().click_element(tabs[1])
            # step 3: accept the dialog with an empty dc sweep (no sweep rows configured)
            app.client().click_element(ok)
            # assert: the dialog stays open for corrections (the footer ok button remains)
            app.wait_for_condition(lambda: any(app.client().get_element_properties(handle).get("accessibleLabel") == "OK" for handle in app.client().find_by_type_in(root, "Button")), timeout=5.0, message="expected the dialog to stay open after the rejected accept")
            # assert: the validation error names the missing sweep variable
            app.wait_for_condition(lambda: any("requires at least one sweep variable" in (app.client().get_element_properties(handle).get("accessibleLabel") or "") for handle in app.client().find_by_type_in(root, "Text")), timeout=5.0, message="expected the dc sweep validation error to appear")

    def test_cancel_and_escape_keep_original_values(self) -> None:
        # arrange: resolve the sample netlist shipped with the repository
        netlist = Path(__file__).resolve().parents[1] / "netlists" / "tran-simple-01.cir"
        # arrange: launch the application with the netlist through the command line
        with TestSession(launch(args=["--netlist", str(netlist)]), self.id()) as app:
            # step 1: open the configure simulation dialog and edit the end time
            app.get_by_type("ToolbarButton").nth(6).click()
            fields = app.get_by_type("LineEdit")
            fields.nth(0).wait_for_exists()
            fields.nth(1).fill("25m")
            # step 2: cancel the dialog from the footer
            root = app.client().get_window_properties()["rootElementHandle"]
            cancel = [handle for handle in app.client().find_by_type_in(root, "Button") if app.client().get_element_properties(handle).get("accessibleLabel") == "Cancel"][0]
            app.client().click_element(cancel)
            fields.nth(0).wait_for_gone()
            # step 3: reopen the dialog
            app.get_by_type("ToolbarButton").nth(6).click()
            fields.nth(0).wait_for_exists()
            # assert: the cancel discarded the edited value
            expect(fields.nth(1)).to_have_text("20m")
            # step 4: edit again and dismiss through the escape key
            fields.nth(1).fill("25m")
            fields.nth(1).press("\x1b")
            fields.nth(0).wait_for_gone()
            # step 5: reopen the dialog
            app.get_by_type("ToolbarButton").nth(6).click()
            fields.nth(0).wait_for_exists()
            # assert: the escape discarded the edited value too
            expect(fields.nth(1)).to_have_text("20m")
