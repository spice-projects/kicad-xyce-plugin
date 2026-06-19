import argparse
import logging
import os
import sys

from kipy import KiCad
from PySide6.QtWidgets import QApplication

from .main_window import MainWindow
from .config.plugin_config import PluginConfig
from .window import load_app_icon

PLUGIN_ID = "com.github.spice-projects.kicad-xyce-plugin"


def _detect_kicad_mode() -> bool:
    # both env vars must be non-empty for KiCad plugin mode
    socket = os.environ.get("KICAD_API_SOCKET", "")
    token = os.environ.get("KICAD_API_TOKEN", "")
    return bool(socket and token)


def _connect_kicad(logger: logging.Logger) -> KiCad | None:
    # log connection details
    logger.info("KICAD_API_SOCKET: %s", os.environ.get("KICAD_API_SOCKET"))
    logger.info("KICAD_API_TOKEN: %s", os.environ.get("KICAD_API_TOKEN"))
    logger.info("KIPRJMOD: %s", os.environ.get("KIPRJMOD"))
    # create KiCad instance and connect to the API
    kicad_client = KiCad(client_name="Xyce Simulator Plugin")
    # log some information about the KiCad instance
    logger.info("Connected to KiCad API version: %s", kicad_client.get_api_version())
    logger.info("KiCad version: %s", kicad_client.get_version())
    # exit
    return kicad_client


def main():
    # configure argument parser
    parser = argparse.ArgumentParser(description="Xyce Simulator Plugin for KiCad")
    # schematic file is optional; when omitted in GUI mode an open-file dialog is shown
    parser.add_argument("schematic", nargs="?", help="The KiCad Schematic file to simulate (required in headless mode; prompts via file dialog if omitted in GUI mode)")
    # headless mode parses and logs the file without opening the viewer UI; requires input
    parser.add_argument("-H", "--headless", action="store_true", help="Parse and log the file without opening the viewer UI (schematic file required)")
    # log level, defaults to INFO
    parser.add_argument("--log-level", default="INFO", choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"], help="Set the logging level")
    # parse command line arguments
    args = parser.parse_args()
    # configure logging with the requested level
    logging.basicConfig(level=getattr(logging, args.log_level), format="%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    # logger for this module (after configuring logging)
    logger = logging.getLogger(__name__)
    # detect startup mode
    is_kicad_mode = _detect_kicad_mode()
    if is_kicad_mode:
        # KiCad plugin mode: env vars are present, connect to the KiCad API
        logger.info("Starting in KiCad plugin mode")
        kicad_client = _connect_kicad(logger)
    else:
        # standalone mode: no KiCad API connection
        logger.info("Starting in standalone mode")
        kicad_client = None
    # log current working directory
    logger.info("Current working directory: %s", os.getcwd())
    # create application
    app = QApplication(sys.argv)
    # set application icon
    app_icon = load_app_icon()
    if not app_icon.isNull():
        app.setWindowIcon(app_icon)
    # create application main window
    window = MainWindow(kicad_client, PluginConfig.load())
    # show and focus the main window
    window.show()
    # enter the Qt application main loop
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
