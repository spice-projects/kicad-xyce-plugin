from enum import Enum
from pathlib import Path


from PySide6.QtGui import QIcon


# resource directory path
_RESOURCE_DIR = Path(__file__).parent / "kicad-icons"


# enum representing supported kicad icons
class KiCadIcon(Enum):
    # save file icon
    FILE_SAVE = 0x00
    # open file icon
    FILE_OPEN = 0x01
    # simulation config icon
    SIM_CONFIG = 0x02
    # run simulation icon
    SIM_RUN = 0x03
    # preference icon
    PREFERENCE = 0x04
    # cancel icon
    CANCEL = 0x05
    # checked ok icon
    CHECKED_OK = 0x06
    # add chart icon
    ADD_CHART = 0x07
    # new window icon
    NEW_WINDOW = 0x08
    # show netlist icon
    SHOW_NETLIST = 0x09


# mapping for light icons
_LIGHT_ICONS: dict[KiCadIcon, QIcon] = {}
# mapping for dark icons
_DARK_ICONS: dict[KiCadIcon, QIcon] = {}


# function to load all icons
def load_kicad_icons() -> None:
    # check if icons are already loaded
    if _LIGHT_ICONS and _DARK_ICONS:
        # return early
        return
    # load save icons
    _LIGHT_ICONS[KiCadIcon.FILE_SAVE] = QIcon(str(_RESOURCE_DIR / "save_24.png"))
    # load save dark icons
    _DARK_ICONS[KiCadIcon.FILE_SAVE] = QIcon(str(_RESOURCE_DIR / "save_dark_24.png"))
    # load open icons
    _LIGHT_ICONS[KiCadIcon.FILE_OPEN] = QIcon(str(_RESOURCE_DIR / "directory_open_24.png"))
    # load open dark icons
    _DARK_ICONS[KiCadIcon.FILE_OPEN] = QIcon(str(_RESOURCE_DIR / "directory_open_dark_24.png"))
    # load config icons
    _LIGHT_ICONS[KiCadIcon.SIM_CONFIG] = QIcon(str(_RESOURCE_DIR / "sim_command_24.png"))
    # load config dark icons
    _DARK_ICONS[KiCadIcon.SIM_CONFIG] = QIcon(str(_RESOURCE_DIR / "sim_command_dark_24.png"))
    # load run icons
    _LIGHT_ICONS[KiCadIcon.SIM_RUN] = QIcon(str(_RESOURCE_DIR / "sim_run_24.png"))
    # load run dark icons
    _DARK_ICONS[KiCadIcon.SIM_RUN] = QIcon(str(_RESOURCE_DIR / "sim_run_dark_24.png"))
    # load preference icons
    _LIGHT_ICONS[KiCadIcon.PREFERENCE] = QIcon(str(_RESOURCE_DIR / "preference_24.png"))
    # load preference dark icons
    _DARK_ICONS[KiCadIcon.PREFERENCE] = QIcon(str(_RESOURCE_DIR / "preference_dark_24.png"))
    # load cancel icons
    _LIGHT_ICONS[KiCadIcon.CANCEL] = QIcon(str(_RESOURCE_DIR / "cancel_24.png"))
    # load cancel dark icons
    _DARK_ICONS[KiCadIcon.CANCEL] = QIcon(str(_RESOURCE_DIR / "cancel_dark_24.png"))
    # load checked ok icons
    _LIGHT_ICONS[KiCadIcon.CHECKED_OK] = QIcon(str(_RESOURCE_DIR / "checked_ok_24.png"))
    # load checked ok dark icons
    _DARK_ICONS[KiCadIcon.CHECKED_OK] = QIcon(str(_RESOURCE_DIR / "checked_ok_dark_24.png"))
    # load add chart icons
    _LIGHT_ICONS[KiCadIcon.ADD_CHART] = QIcon(str(_RESOURCE_DIR / "sim_add_plot_24.png"))
    # load add chart dark icons
    _DARK_ICONS[KiCadIcon.ADD_CHART] = QIcon(str(_RESOURCE_DIR / "sim_add_plot_dark_24.png"))
    # load new window icons
    _LIGHT_ICONS[KiCadIcon.NEW_WINDOW] = QIcon(str(_RESOURCE_DIR / "new_generic_24.png"))
    # load new window dark icons
    _DARK_ICONS[KiCadIcon.NEW_WINDOW] = QIcon(str(_RESOURCE_DIR / "new_generic_dark_24.png"))
    # load show netlist icons
    _LIGHT_ICONS[KiCadIcon.SHOW_NETLIST] = QIcon(str(_RESOURCE_DIR / "netlist_24.png"))
    # load show netlist dark icons
    _DARK_ICONS[KiCadIcon.SHOW_NETLIST] = QIcon(str(_RESOURCE_DIR / "netlist_dark_24.png"))


# function to get a specific icon
def get_kicad_icon(icon: KiCadIcon, dark: bool = False) -> QIcon:
    # select icon map
    icon_map = _DARK_ICONS if dark else _LIGHT_ICONS
    # return the icon
    return icon_map.get(icon, QIcon())
