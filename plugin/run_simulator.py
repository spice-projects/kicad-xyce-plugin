import argparse
import logging
import os
import sys

from kipy import KiCad
from PySide6.QtWidgets import QApplication

from main_window import MainWindow
from plugin_config import PluginConfig
from window import load_app_icon

KICAD_API_SOCKET = os.environ.get("KICAD_API_SOCKET")
KICAD_API_TOKEN = os.environ.get("KICAD_API_TOKEN")

PLUGIN_ID = "com.github.spice-projects.kicad-xyce-plugin"


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
    # log KiCad API connection details (for debugging purposes)
    logger.info("KICAD_API_SOCKET: %s", KICAD_API_SOCKET)
    logger.info("KICAD_API_TOKEN: %s", KICAD_API_TOKEN)
    # create KiCad instance and connect to the API
    kicad_client = KiCad(client_name="Xyce Simulator Plugin")
    # log some information about the KiCad instance
    logger.info("Connected to KiCad API version: %s", kicad_client.get_api_version())
    logger.info("KiCad version: %s", kicad_client.get_version())
    logger.info("Current working directory: %s", os.getcwd())
    # create application
    app = QApplication(sys.argv)
    # set application icon
    app_icon = load_app_icon()
    if not app_icon.isNull():
        app.setWindowIcon(app_icon)
    # create application main window
    window = MainWindow(kicad_client, PluginConfig.default())
    # show and focus the main window
    window.show()
    # enter the Qt application main loop only if we created a new application
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
