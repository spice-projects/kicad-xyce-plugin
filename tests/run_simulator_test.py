import os
import sys
from unittest import TestCase
from unittest.mock import MagicMock, patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

# run_simulator.py uses bare intra-package imports; expose the plugin dir
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "plugin"))
# mock kipy before any plugin imports since run_simulator.py imports from kipy
sys.modules.setdefault("kipy", MagicMock())
# pre-import plugin modules in dependency order so bare-name aliases resolve correctly
import plugin.config_dialog
import plugin.expression
import plugin.kicad_icons
import plugin.main_window
import plugin.plugin_config
import plugin.run_xyce_simulation
import plugin.simulation_dialog
import plugin.window  # noqa: F401
for _name in ["config_dialog", "expression", "kicad_icons", "main_window", "plugin_config", "run_xyce_simulation", "simulation_dialog", "window"]:
    sys.modules.setdefault(_name, sys.modules[f"plugin.{_name}"])

from PySide6.QtWidgets import QApplication

from plugin.run_simulator import KICAD_API_SOCKET, KICAD_API_TOKEN, PLUGIN_ID, main

_app = QApplication.instance() or QApplication(sys.argv)


class TestRunSimulatorConstants(TestCase):

    def test_plugin_id_matches_expected_value(self):
        # act / assert
        self.assertEqual(PLUGIN_ID, "com.github.spice-projects.kicad-xyce-plugin")

    def test_kicad_api_socket_is_string_or_none(self):
        # act / assert — value comes from os.environ.get
        self.assertTrue(KICAD_API_SOCKET is None or isinstance(KICAD_API_SOCKET, str))

    def test_kicad_api_token_is_string_or_none(self):
        # act / assert — value comes from os.environ.get
        self.assertTrue(KICAD_API_TOKEN is None or isinstance(KICAD_API_TOKEN, str))


class TestRunSimulatorMain(TestCase):

    def test_main_exits_with_app_exec_return_value(self):
        # arrange
        with patch.object(sys, "argv", ["run_simulator"]):
            with patch("plugin.run_simulator.load_app_icon") as mock_icon:
                with patch("plugin.run_simulator.MainWindow"):
                    with patch("plugin.run_simulator.QApplication") as mock_app_cls:
                        with patch("sys.exit") as mock_exit:
                            mock_app_cls.return_value.exec.return_value = 0
                            mock_icon.return_value.isNull.return_value = True
                            # act
                            main()
        # assert
        mock_exit.assert_called_once_with(0)

    def test_main_sets_window_icon_when_icon_is_valid(self):
        # arrange
        with patch.object(sys, "argv", ["run_simulator"]):
            with patch("plugin.run_simulator.load_app_icon") as mock_icon:
                with patch("plugin.run_simulator.MainWindow"):
                    with patch("plugin.run_simulator.QApplication") as mock_app_cls:
                        with patch("sys.exit"):
                            mock_app_cls.return_value.exec.return_value = 0
                            mock_icon.return_value.isNull.return_value = False
                            # act
                            main()
        # assert
        mock_app_cls.return_value.setWindowIcon.assert_called_once()

    def test_main_creates_and_shows_window(self):
        # arrange
        with patch.object(sys, "argv", ["run_simulator"]):
            with patch("plugin.run_simulator.load_app_icon") as mock_icon:
                with patch("plugin.run_simulator.MainWindow") as mock_window_cls:
                    with patch("plugin.run_simulator.QApplication") as mock_app_cls:
                        with patch("sys.exit"):
                            mock_app_cls.return_value.exec.return_value = 0
                            mock_icon.return_value.isNull.return_value = True
                            # act
                            main()
        # assert
        mock_window_cls.assert_called_once()
        mock_window_cls.return_value.show.assert_called_once()
