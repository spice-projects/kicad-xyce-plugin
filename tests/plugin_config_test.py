import json
import os
import tempfile
from unittest import TestCase
from unittest.mock import MagicMock, patch

from plugin.plugin_config import PluginConfig, discover_xyce_executable


class TestPluginConfigDefault(TestCase):

    def test_default_returns_instance(self):
        # act
        config = PluginConfig.default()
        # assert
        self.assertIsInstance(config, PluginConfig)

    def test_default_xyce_executable_path_is_empty_when_discovery_fails(self):
        # arrange
        with patch("plugin.plugin_config.discover_xyce_executable", return_value=""):
            # act
            config = PluginConfig.default()
        # assert
        self.assertEqual(config.xyce_executable_path, "")

    def test_default_xyce_executable_path_uses_discovered_path(self):
        # arrange
        with patch("plugin.plugin_config.discover_xyce_executable", return_value="/usr/local/XyceNF_7.6/bin/Xyce"):
            # act
            config = PluginConfig.default()
        # assert
        self.assertEqual(config.xyce_executable_path, "/usr/local/XyceNF_7.6/bin/Xyce")


class TestPluginConfigLoad(TestCase):

    def test_load_returns_default_when_file_missing(self):
        # arrange
        missing_path = "/nonexistent/path/config.json"
        # act
        config = PluginConfig.load(missing_path)
        # assert
        self.assertEqual(config, PluginConfig.default())

    def test_load_parses_json_file(self):
        # arrange
        data = {"xyceExecutablePath": "/usr/local/bin/xyce"}
        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            json.dump(data, f)
            settings_path = f.name
        try:
            # act
            config = PluginConfig.load(settings_path)
            # assert
            self.assertEqual(config.xyce_executable_path, "/usr/local/bin/xyce")
        finally:
            os.unlink(settings_path)

    def test_load_returns_plugin_config_instance(self):
        # arrange
        data = {"xyceExecutablePath": "/opt/xyce/bin/Xyce"}
        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            json.dump(data, f)
            settings_path = f.name
        try:
            # act
            config = PluginConfig.load(settings_path)
            # assert
            self.assertIsInstance(config, PluginConfig)
        finally:
            os.unlink(settings_path)


class TestPluginConfigIsXyceExecutableValid(TestCase):

    def test_returns_false_when_path_is_empty(self):
        # arrange
        config = PluginConfig(xyce_executable_path="")
        # act
        result = config.is_xyce_executable_valid()
        # assert
        self.assertFalse(result)

    def test_returns_false_when_path_does_not_exist(self):
        # arrange
        config = PluginConfig(xyce_executable_path="/nonexistent/path/xyce")
        # act
        result = config.is_xyce_executable_valid()
        # assert
        self.assertFalse(result)

    def test_returns_false_when_path_is_not_executable(self):
        # arrange — create a real file that is not executable
        with tempfile.NamedTemporaryFile(delete=False) as f:
            f.write(b"#!/bin/sh\necho hello\n")
            non_exec_path = f.name
        os.chmod(non_exec_path, 0o644)
        try:
            config = PluginConfig(xyce_executable_path=non_exec_path)
            # act
            result = config.is_xyce_executable_valid()
            # assert
            self.assertFalse(result)
        finally:
            os.unlink(non_exec_path)

    def test_returns_true_when_path_is_executable_file(self):
        # arrange — create a real executable file
        with tempfile.NamedTemporaryFile(delete=False) as f:
            f.write(b"#!/bin/sh\necho hello\n")
            exec_path = f.name
        os.chmod(exec_path, 0o755)
        try:
            config = PluginConfig(xyce_executable_path=exec_path)
            # act
            result = config.is_xyce_executable_valid()
            # assert
            self.assertTrue(result)
        finally:
            os.unlink(exec_path)


class TestPluginConfigSave(TestCase):

    def test_save_calls_qsettings_set_value(self):
        # arrange
        config = PluginConfig(xyce_executable_path="/usr/bin/Xyce")
        mock_settings = MagicMock()
        with patch("plugin.plugin_config.QSettings", return_value=mock_settings):
            # act
            config.save()
        # assert
        mock_settings.setValue.assert_called_once_with("xyceExecutablePath", "/usr/bin/Xyce")

    def test_save_uses_correct_organization_and_application(self):
        # arrange
        config = PluginConfig(xyce_executable_path="/some/path")
        mock_settings = MagicMock()
        with patch("plugin.plugin_config.QSettings", return_value=mock_settings) as mock_qsettings_class:
            # act
            config.save()
        # assert
        mock_qsettings_class.assert_called_once_with("KiCad", "XyceSimulatorPlugin")


class TestDiscoverXyceExecutable(TestCase):

    def test_returns_path_when_found_on_path(self):
        # arrange
        with patch("plugin.plugin_config.shutil.which", return_value="/usr/bin/Xyce"):
            # act
            result = discover_xyce_executable()
        # assert
        self.assertEqual(result, "/usr/bin/Xyce")

    def test_returns_empty_string_when_nothing_found_on_linux(self):
        # arrange
        with patch("plugin.plugin_config.shutil.which", return_value=None):
            with patch("plugin.plugin_config.platform.system", return_value="Linux"):
                with patch("plugin.plugin_config.glob.glob", return_value=[]):
                    # act
                    result = discover_xyce_executable()
        # assert
        self.assertEqual(result, "")

    def test_returns_empty_string_when_nothing_found_on_windows(self):
        # arrange
        with patch("plugin.plugin_config.shutil.which", return_value=None):
            with patch("plugin.plugin_config.platform.system", return_value="Windows"):
                with patch("plugin.plugin_config.glob.glob", return_value=[]):
                    # act
                    result = discover_xyce_executable()
        # assert
        self.assertEqual(result, "")

    def test_returns_glob_match_on_linux(self):
        # arrange
        with patch("plugin.plugin_config.shutil.which", return_value=None):
            with patch("plugin.plugin_config.platform.system", return_value="Linux"):
                with patch("plugin.plugin_config.glob.glob", return_value=["/usr/local/XyceNF_7.6/bin/Xyce"]):
                    # act
                    result = discover_xyce_executable()
        # assert
        self.assertEqual(result, "/usr/local/XyceNF_7.6/bin/Xyce")

    def test_returns_glob_match_on_macos(self):
        # arrange
        with patch("plugin.plugin_config.shutil.which", return_value=None):
            with patch("plugin.plugin_config.platform.system", return_value="Darwin"):
                with patch("plugin.plugin_config.glob.glob", return_value=["/usr/local/XyceNF_7.6/bin/Xyce"]):
                    # act
                    result = discover_xyce_executable()
        # assert
        self.assertEqual(result, "/usr/local/XyceNF_7.6/bin/Xyce")

    def test_returns_glob_match_on_windows(self):
        # arrange
        with patch("plugin.plugin_config.shutil.which", return_value=None):
            with patch("plugin.plugin_config.platform.system", return_value="Windows"):
                with patch("plugin.plugin_config.glob.glob", return_value=[r"C:\Program Files\XyceNF_7.6\bin\Xyce.exe"]):
                    # act
                    result = discover_xyce_executable()
        # assert
        self.assertEqual(result, r"C:\Program Files\XyceNF_7.6\bin\Xyce.exe")

    def test_returns_highest_version_when_multiple_candidates(self):
        # arrange
        candidates = [
            "/usr/local/XyceNF_7.5/bin/Xyce",
            "/usr/local/XyceNF_7.4/bin/Xyce",
            "/usr/local/XyceNF_7.6/bin/Xyce",
        ]
        with patch("plugin.plugin_config.shutil.which", return_value=None):
            with patch("plugin.plugin_config.platform.system", return_value="Linux"):
                with patch("plugin.plugin_config.glob.glob", return_value=candidates):
                    # act
                    result = discover_xyce_executable()
        # assert
        self.assertEqual(result, "/usr/local/XyceNF_7.6/bin/Xyce")
