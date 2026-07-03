import os
import sys
from unittest.mock import MagicMock

# mock wx to avoid errors in gh actions workflow
sys.modules["wx"] = MagicMock()

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "src"))

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
os.environ.setdefault("QT_LOGGING_RULES", "qt.qpa.fonts=false")


def pytest_configure(config):
    """Disable pytest-qt for the FFT dialog unit test when it is run standalone.

    The FFT dialog tests in this module use mocked Qt imports and do not require
    pytest-qt's event loop integration. Disabling the plugin avoids a Qt
    runtime segfault during pytest setup for this isolated module.
    """
    if any("fft_dialog_test.py" in str(arg) for arg in config.invocation_params.args):
        config.pluginmanager.set_blocked("qt")
