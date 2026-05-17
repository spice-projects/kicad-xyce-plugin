import sys

from PySide6.QtWidgets import QApplication

from run_simulator import KICAD_API_SOCKET, KICAD_API_TOKEN, PLUGIN_ID

_app = QApplication.instance() or QApplication(sys.argv)


class TestRunSimulatorConstants:

    def test_plugin_id_matches_expected_value(self):
        # act / assert
        assert PLUGIN_ID == "com.github.spice-projects.kicad-xyce-plugin"

    def test_kicad_api_socket_is_string_or_none(self):
        # act / assert — value comes from os.environ.get
        assert KICAD_API_SOCKET is None or isinstance(KICAD_API_SOCKET, str)

    def test_kicad_api_token_is_string_or_none(self):
        # act / assert — value comes from os.environ.get
        assert KICAD_API_TOKEN is None or isinstance(KICAD_API_TOKEN, str)
