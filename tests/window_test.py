import logging
import os
import sys

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from unittest import TestCase
from unittest.mock import MagicMock

from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QApplication, QMainWindow

from plugin.window import load_app_icon, log_screen_info, register_child_window, unregister_child_window

_app = QApplication.instance() or QApplication(sys.argv)


class TestRegisterUnregisterChildWindow(TestCase):

    def test_register_adds_window_to_registry(self):
        # arrange
        window = QMainWindow()
        # act
        register_child_window(window)
        # assert — re-registering does not raise and window is in set
        register_child_window(window)

    def test_unregister_removes_window_from_registry(self):
        # arrange
        window = QMainWindow()
        register_child_window(window)
        # act
        unregister_child_window(window)
        # assert — second unregister is a no-op (discard)
        unregister_child_window(window)

    def test_unregister_window_not_registered_is_no_op(self):
        # arrange — window that was never registered
        window = QMainWindow()
        # act / assert — no exception raised
        unregister_child_window(window)


class TestLoadAppIcon(TestCase):

    def test_returns_qicon_instance(self):
        # act
        icon = load_app_icon()
        # assert
        self.assertIsInstance(icon, QIcon)


class TestLogScreenInfo(TestCase):

    def test_logs_screen_name(self):
        # arrange
        mock_screen = MagicMock()
        mock_screen.name.return_value = "TestScreen"
        mock_screen.size.return_value.width.return_value = 1920
        mock_screen.size.return_value.height.return_value = 1080
        mock_screen.devicePixelRatio.return_value = 2.0
        mock_screen.refreshRate.return_value = 60.0
        # act — no exception raised
        with self.assertLogs("plugin.window", level=logging.DEBUG):
            log_screen_info(mock_screen)

    def test_log_screen_info_calls_screen_methods(self):
        # arrange
        mock_screen = MagicMock()
        mock_screen.name.return_value = "MyDisplay"
        mock_screen.size.return_value.width.return_value = 2560
        mock_screen.size.return_value.height.return_value = 1440
        mock_screen.devicePixelRatio.return_value = 1.0
        mock_screen.refreshRate.return_value = 144.0
        # act
        with self.assertLogs("plugin.window", level=logging.DEBUG):
            log_screen_info(mock_screen)
        # assert
        mock_screen.name.assert_called()
        mock_screen.size.assert_called()
        mock_screen.devicePixelRatio.assert_called()
        mock_screen.refreshRate.assert_called()
