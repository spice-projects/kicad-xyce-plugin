import sys
from unittest.mock import MagicMock

# Mock PySide6 for headless unit tests
mock_qt = MagicMock()

# Slot decorator should return the original function so it can be called in tests


def mock_slot(*args, **kwargs):
    return lambda func: func


mock_qt.QtCore.Slot = mock_slot

# Set up the sys.modules to point to our mock
sys.modules["PySide6"] = mock_qt
sys.modules["PySide6.QtCore"] = mock_qt.QtCore
sys.modules["PySide6.QtGui"] = mock_qt.QtGui
sys.modules["PySide6.QtQuick"] = mock_qt.QtQuick
sys.modules["PySide6.QtWidgets"] = mock_qt.QtWidgets

# Mock kipy (KiCad API) just in case
sys.modules["kipy"] = MagicMock()
