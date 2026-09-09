# tests/application_test.py
# Test cases for the SlintApplication testing framework
# Uses unittest with setUp/tearDown to manage application lifecycle

import os
import unittest
from pathlib import Path

from framework import launch


class ApplicationLaunchChecks(unittest.TestCase):
    def setUp(self):
        # arrange: use the override executable when provided
        executable = os.environ.get("SLINT_TEST_APPLICATION")
        # arrange: fall back to the debug build at the repository root
        if not executable:
            executable = str(Path(__file__).resolve().parents[2] / ".build-debug" / "kicad-xyce-plugin")
        # arrange: launch the application for testing
        self._app = launch(executable)

    def tearDown(self):
        # cleanup: terminate the application after each test
        self._app.close()

    def test_application_launch(self):
        # assert: verify the application launched successfully
        self.assertIsNotNone(self._app)

    def test_application_has_client(self):
        # act: get the client from the application
        client = self._app.client()
        # assert: verify the client is not None
        self.assertIsNotNone(client)

    def test_application_status(self):
        # act: check the application status
        client = self._app.client()
        status = client.get_status()
        # assert: verify the application is running
        self.assertEqual(status, "running")

    def test_application_window(self):
        # act: get the application window
        client = self._app.client()
        window = client.get_window()
        # assert: verify the window is retrieved
        self.assertIsNotNone(window)
