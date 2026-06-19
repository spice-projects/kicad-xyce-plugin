import os
import subprocess
from pathlib import Path
from unittest.mock import MagicMock
from unittest.mock import patch

# import the module to be tested
from plugin import _candidate_python_executables
from plugin import _ensure_application_installed
from plugin import _ensure_python_venv
from plugin import _extract_python_version
from plugin import _get_clean_env
from plugin import _is_python_version_compatible
from plugin import main


class TestPlugin:

    def test_get_clean_env_removes_python_vars(self):
        with patch.dict(os.environ, {"PYTHONPATH": "/some", "PYTHONHOME": "/home"}):
            env = _get_clean_env()
            assert "PYTHONPATH" not in env
            assert "PYTHONHOME" not in env

    def test_extract_python_version_success(self):
        with patch("subprocess.check_output", return_value="3 10\n"):
            assert _extract_python_version(Path("python")) == (3, 10)

    def test_extract_python_version_failure(self):
        with patch("subprocess.check_output", side_effect=subprocess.CalledProcessError(1, "cmd")):
            assert _extract_python_version(Path("invalid")) is None

    def test_is_python_version_compatible_success(self):
        with patch.object(Path, "exists", return_value=True):
            with patch("os.access", return_value=True):
                with patch("plugin._extract_python_version", return_value=(3, 10)):
                    compatible, version = _is_python_version_compatible(Path("python"))
                    assert compatible is True
                    assert version == (3, 10)

    def test_candidate_python_executables(self):
        with patch.dict(os.environ, {"PYTHON_PATH": "/env/python"}):
            with patch("sys.executable", "/sys/python"):
                with patch("shutil.which", side_effect=lambda x: f"/path/{x}"):
                    def mock_path_factory(p):
                        m = MagicMock()
                        m.resolve.return_value = MagicMock(__str__=lambda s: str(p))
                        m.__str__.return_value = str(p)
                        return m
                    with patch("plugin.Path", side_effect=mock_path_factory):
                        candidates = _candidate_python_executables()
                        paths = [str(p) for p in candidates]
                        assert "/env/python" in paths
                        assert "/sys/python" in paths

    def test_ensure_python_venv_reuses_existing(self):
        with patch("plugin.APP_DIR") as mock_app_dir:
            (mock_app_dir / "pyvenv.cfg").is_file.return_value = True
            with patch("plugin._extract_python_version", return_value=(3, 10)):
                with patch("subprocess.check_call") as mock_call:
                    _ensure_python_venv(Path("python"), (3, 10))
                    mock_call.assert_not_called()

    def test_ensure_application_installed_already_exists(self):
        with patch("plugin.APP_DIR") as mock_app_dir:
            with patch("plugin.__version__", "1.0.0"):
                (mock_app_dir / "version.txt").is_file.return_value = True
                (mock_app_dir / "version.txt").read_text.return_value = "1.0.0"
                result = _ensure_application_installed()
                assert result is not None

    def test_ensure_application_installed_perform_install(self):
        with patch("plugin.APP_DIR") as mock_app_dir:
            with patch("plugin.__version__", "1.0.0"):
                with patch("plugin.find_python_executable_path", return_value=(Path("python"), (3, 10))):
                    with patch("plugin._ensure_python_venv"):
                        with patch("plugin.Path.glob", return_value=[Path("wheel.whl")]):
                            with patch("subprocess.check_call"):
                                with patch("plugin.wx") as mock_wx:
                                    (mock_app_dir / "version.txt").is_file.return_value = False
                                    result = _ensure_application_installed()
                                    assert result is not None
                                    mock_wx.ProgressDialog.assert_called_once()

    def test_main_success(self):
        env_vars = {"KICAD_API_SOCKET": "sock", "KICAD_API_TOKEN": "tok"}
        with patch.dict(os.environ, env_vars):
            with patch("plugin._ensure_application_installed", return_value=Path("python")):
                with patch("subprocess.check_call") as mock_call:
                    main()
                    mock_call.assert_called_once()
                    assert "python" in str(mock_call.call_args[0][0])

    def test_show_error_dialog_macos(self):
        with patch("sys.platform", "darwin"):
            with patch("subprocess.run") as mock_run:
                with patch("plugin.wx", None):
                    from plugin import _show_error_dialog
                    _show_error_dialog("error")
                    mock_run.assert_called_once()
                    assert "osascript" in mock_run.call_args[0][0][0]

    def test_show_error_dialog_windows(self):
        with patch("sys.platform", "win32"):
            with patch("subprocess.run") as mock_run:
                with patch("plugin.wx", None):
                    from plugin import _show_error_dialog
                    _show_error_dialog("error")
                    mock_run.assert_called_once()
                    assert "powershell" in mock_run.call_args[0][0][0]

    def test_show_error_dialog_wx(self):
        with patch("plugin.wx") as mock_wx:
            mock_wx.GetApp.return_value = MagicMock()
            from plugin import _show_error_dialog
            _show_error_dialog("wx error")
            mock_wx.MessageBox.assert_called_once()
