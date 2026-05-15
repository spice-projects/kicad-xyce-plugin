from enum import Enum
from pathlib import Path

from PySide6.QtGui import QIcon

_RESOURCE_DIR = Path(__file__).parent / "kicad-icons"


class KiCadIcon(Enum):
    FILE_SAVE = 0x00
    FILE_OPEN = 0x01
    SIM_COMMAND = 0x02
    SIM_RUN = 0x03
    PREFERENCE = 0x04
    CANCEL = 0x05
    CHECKED_OK = 0x06
    ADD_CHART = 0x07
    NEW_WINDOW = 0x08


_LIGHT_ICONS: dict[KiCadIcon, QIcon] = {}
_DARK_ICONS: dict[KiCadIcon, QIcon] = {}


def load_kicad_icons() -> None:
    # avoid loading icons multiple times
    if _LIGHT_ICONS and _DARK_ICONS:
        return
    # load icons
    _LIGHT_ICONS[KiCadIcon.FILE_SAVE] = QIcon(str(_RESOURCE_DIR / "save_24.png"))
    _DARK_ICONS[KiCadIcon.FILE_SAVE] = QIcon(str(_RESOURCE_DIR / "save_dark_24.png"))
    _LIGHT_ICONS[KiCadIcon.FILE_OPEN] = QIcon(str(_RESOURCE_DIR / "directory_open_24.png"))
    _DARK_ICONS[KiCadIcon.FILE_OPEN] = QIcon(str(_RESOURCE_DIR / "directory_open_dark_24.png"))
    _LIGHT_ICONS[KiCadIcon.SIM_COMMAND] = QIcon(str(_RESOURCE_DIR / "sim_command_24.png"))
    _DARK_ICONS[KiCadIcon.SIM_COMMAND] = QIcon(str(_RESOURCE_DIR / "sim_command_dark_24.png"))
    _LIGHT_ICONS[KiCadIcon.SIM_RUN] = QIcon(str(_RESOURCE_DIR / "sim_run_24.png"))
    _DARK_ICONS[KiCadIcon.SIM_RUN] = QIcon(str(_RESOURCE_DIR / "sim_run_dark_24.png"))
    _LIGHT_ICONS[KiCadIcon.PREFERENCE] = QIcon(str(_RESOURCE_DIR / "preference_24.png"))
    _DARK_ICONS[KiCadIcon.PREFERENCE] = QIcon(str(_RESOURCE_DIR / "preference_dark_24.png"))
    _LIGHT_ICONS[KiCadIcon.CANCEL] = QIcon(str(_RESOURCE_DIR / "cancel_24.png"))
    _DARK_ICONS[KiCadIcon.CANCEL] = QIcon(str(_RESOURCE_DIR / "cancel_dark_24.png"))
    _LIGHT_ICONS[KiCadIcon.CHECKED_OK] = QIcon(str(_RESOURCE_DIR / "checked_ok_24.png"))
    _DARK_ICONS[KiCadIcon.CHECKED_OK] = QIcon(str(_RESOURCE_DIR / "checked_ok_dark_24.png"))
    _LIGHT_ICONS[KiCadIcon.ADD_CHART] = QIcon(str(_RESOURCE_DIR / "sim_add_plot_24.png"))
    _DARK_ICONS[KiCadIcon.ADD_CHART] = QIcon(str(_RESOURCE_DIR / "sim_add_plot_dark_24.png"))
    _LIGHT_ICONS[KiCadIcon.NEW_WINDOW] = QIcon(str(_RESOURCE_DIR / "new_generic_24.png"))
    _DARK_ICONS[KiCadIcon.NEW_WINDOW] = QIcon(str(_RESOURCE_DIR / "new_generic_dark_24.png"))


def get_kicad_icon(icon: KiCadIcon, dark: bool = False) -> QIcon:
    # select the icon dictionary based on requested color variant
    icon_map = _DARK_ICONS if dark else _LIGHT_ICONS
    # return the requested icon or a default empty icon if not found
    return icon_map.get(icon, QIcon())
