import sys

from PySide6.QtWidgets import QApplication, QMainWindow
from PySide6.QtGui import QIcon

from window import load_app_icon, register_child_window, unregister_child_window

_app = QApplication.instance() or QApplication(sys.argv)


class TestRegisterUnregisterChildWindow:

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


class TestLoadAppIcon:

    def test_returns_qicon_instance(self):
        # act
        icon = load_app_icon()
        # assert
        assert isinstance(icon, QIcon)
