import unittest

from slint_automation.assertions import expect
from slint_automation.session import TestSession
from slint_automation.slint_application import launch


class ApplicationSmokeChecks(unittest.TestCase):

    def test_application_starts(self) -> None:
        # arrange: launch the application in a session scoped to this test
        with TestSession(launch(), self.id()) as app:
            # assert
            self.assertTrue(app.is_running())

    def test_main_window_appears(self) -> None:
        # arrange: launch the application in a session scoped to this test
        with TestSession(launch(), self.id()) as app:
            # act
            window = app.client().get_window()
            # assert
            self.assertIsNotNone(window)

    def test_toolbar_element_is_located(self) -> None:
        # arrange: launch the application in a session scoped to this test
        with TestSession(launch(), self.id()) as app:
            # assert
            expect(app.get_by_id("MainWindow::toolbar")).to_exist(timeout=5.0)

    def test_initial_status_text_is_empty(self) -> None:
        # arrange: launch the application in a session scoped to this test
        with TestSession(launch(), self.id()) as app:
            # assert
            expect(app.get_by_role("Text")).to_have_text("", timeout=5.0)
