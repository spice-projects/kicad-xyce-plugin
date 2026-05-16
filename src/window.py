import logging
from pathlib import Path

from PySide6.QtGui import QIcon, QScreen
from PySide6.QtWidgets import QMainWindow

logger = logging.getLogger(__name__)

# application-level registry keeps child windows alive independently
# of the source main window that created them
_CHILD_WINDOWS: set[QMainWindow] = set()

_RESOURCE_DIR = Path(__file__).parent / "resources"
_ICON_PATH = _RESOURCE_DIR / "xyce-window-icon-2.ico"


def register_child_window(window: QMainWindow) -> None:
    _CHILD_WINDOWS.add(window)


def unregister_child_window(window: QMainWindow) -> None:
    _CHILD_WINDOWS.discard(window)


def load_app_icon() -> QIcon:
    return QIcon(str(_ICON_PATH))


def log_screen_info(screen: QScreen) -> None:
    # log information
    logger.debug("Screen information:")
    logger.debug("Screen name: %s", screen.name())
    logger.debug("Screen size: %d x %d", screen.size().width(), screen.size().height())
    logger.debug("Device pixel ratio: %f", screen.devicePixelRatio())
    logger.debug("Refresh rate: %f", screen.refreshRate())
