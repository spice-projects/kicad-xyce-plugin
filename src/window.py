import logging
from pathlib import Path

from PySide6.QtGui import QIcon
from PySide6.QtGui import QScreen
from PySide6.QtWidgets import QMainWindow


logger = logging.getLogger(__name__)

_CHILD_WINDOWS: set[QMainWindow] = set()
_RESOURCE_DIR = Path(__file__).parent / "resources"
_ICON_PATH = _RESOURCE_DIR / "xyce-window-icon-2.ico"


def register_child_window(window: QMainWindow) -> None:
    # add window
    _CHILD_WINDOWS.add(window)


def unregister_child_window(window: QMainWindow) -> None:
    # discard window
    _CHILD_WINDOWS.discard(window)


def load_app_icon() -> QIcon:
    # get icon path as string
    path_str = str(_ICON_PATH)
    # create icon
    icon = QIcon(path_str)
    # return icon
    return icon


def log_screen_info(screen: QScreen) -> None:
    # log information
    logger.debug("Screen information:")
    # log screen name
    logger.debug("Screen name: %s", screen.name())
    # get size
    size = screen.size()
    # get width
    width = size.width()
    # get height
    height = size.height()
    # log screen size
    logger.debug("Screen size: %d x %d", width, height)
    # get pixel ratio
    ratio = screen.devicePixelRatio()
    # log pixel ratio
    logger.debug("Device pixel ratio: %f", ratio)
    # get refresh rate
    rate = screen.refreshRate()
    # log refresh rate
    logger.debug("Refresh rate: %f", rate)
