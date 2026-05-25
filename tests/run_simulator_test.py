# import sys
# from unittest.mock import patch

# from PySide6.QtWidgets import QApplication

# from kicad_xyce_plugin.run_simulator import PLUGIN_ID, _detect_kicad_mode

# _app = QApplication.instance() or QApplication(sys.argv)


# class TestRunSimulatorConstants:

#     def test_plugin_id_matches_expected_value(self):
#         # act / assert
#         assert PLUGIN_ID == "com.github.spice-projects.kicad-xyce-plugin"


# class TestDetectKicadMode:

#     def test_returns_true_when_both_env_vars_are_set(self):
#         # arrange
#         env = {"KICAD_API_SOCKET": "/tmp/kicad.sock", "KICAD_API_TOKEN": "token123"}
#         # act / assert
#         with patch.dict("os.environ", env):
#             assert _detect_kicad_mode() is True

#     def test_returns_false_when_socket_is_empty(self):
#         # arrange — empty socket, token present
#         env = {"KICAD_API_SOCKET": "", "KICAD_API_TOKEN": "token123"}
#         # act / assert
#         with patch.dict("os.environ", env):
#             assert _detect_kicad_mode() is False

#     def test_returns_false_when_token_is_empty(self):
#         # arrange — socket present, empty token
#         env = {"KICAD_API_SOCKET": "/tmp/kicad.sock", "KICAD_API_TOKEN": ""}
#         # act / assert
#         with patch.dict("os.environ", env):
#             assert _detect_kicad_mode() is False

#     def test_returns_false_when_both_env_vars_are_empty(self):
#         # arrange
#         env = {"KICAD_API_SOCKET": "", "KICAD_API_TOKEN": ""}
#         # act / assert
#         with patch.dict("os.environ", env):
#             assert _detect_kicad_mode() is False
