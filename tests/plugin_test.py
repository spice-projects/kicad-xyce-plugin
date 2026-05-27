import os
import subprocess
import sys
from pathlib import Path
from unittest.mock import MagicMock
from unittest.mock import patch

import pytest

from plugin import _candidate_python_executables
from plugin import _ensure_application_installed
from plugin import _ensure_python_venv
from plugin import _extract_python_version
from plugin import _get_clean_env
from plugin import _is_python_version_compatible
from plugin import find_python_executable_path
from plugin import main


class TestPlugin:

    def test_get_clean_env_removes_python_vars(self):
        # arrange
        with patch.dict(os.environ, {"PYTHONPATH": "/some/path", "PYTHONHOME": "/home/path", "OTHER_VAR": "value"}):
            # act
            env = _get_clean_env()
            # assert — verify pythonpath is removed
            assert "PYTHONPATH" not in env
            # assert — verify pythonhome is removed
            assert "PYTHONHOME" not in env
            # assert — verify other variables are preserved
            assert env["OTHER_VAR"] == "value"

    def test_extract_python_version_success(self):
        # arrange
        python_path = Path("/usr/bin/python3")
        # mock output from python version query
        mock_output = "3 10\n"
        # patch subprocess call to return the mock version string
        with patch("subprocess.check_output", return_value=mock_output) as mock_run:
            # act
            version = _extract_python_version(python_path)
            # assert — verify version was parsed correctly
            assert version == (3, 10)
            # assert — verify subprocess was called
            mock_run.assert_called_once()

    def test_extract_python_version_windows_launcher(self):
        # arrange
        python_path = Path("py.exe")
        # mock output for windows launcher
        mock_output = "3 11\n"
        # patch subprocess call for the windows scenario
        with patch("subprocess.check_output", return_value=mock_output) as mock_run:
            # act
            version = _extract_python_version(python_path)
            # assert — verify version extraction
            assert version == (3, 11)
            # extract the arguments passed to the mock call
            args = mock_run.call_args[0][0]
            # assert — verify that -3 flag was used for windows launcher
            assert "-3" in args

    def test_extract_python_version_failure(self):
        # arrange
        python_path = Path("/usr/bin/invalid")
        # simulate a subprocess error when querying the version
        with patch("subprocess.check_output", side_effect=subprocess.CalledProcessError(1, "cmd")):
            # act
            version = _extract_python_version(python_path)
            # assert — verify that failure returns none
            assert version is None

    def test_is_python_version_compatible_not_exists(self):
        # arrange
        python_path = Path("/nonexistent/python")
        # simulate filesystem check failing for the candidate
        with patch.object(Path, "exists", return_value=False):
            # act
            compatible, version = _is_python_version_compatible(python_path)
            # assert — verify compatibility is false
            assert compatible is False
            # assert — verify version is none
            assert version is None

    def test_is_python_version_compatible_success(self):
        # arrange
        python_path = Path("/usr/bin/python3.10")
        # mock existence and execution permission checks
        with patch.object(Path, "exists", return_value=True):
            # mock file access permission
            with patch("os.access", return_value=True):
                # mock internal version extraction helper
                with patch("plugin._extract_python_version", return_value=(3, 10)):
                    # act
                    compatible, version = _is_python_version_compatible(python_path)
                    # assert — verify version is compatible
                    assert compatible is True
                    # assert — verify version parts
                    assert version == (3, 10)

    def test_is_python_version_compatible_too_old(self):
        # arrange
        python_path = Path("/usr/bin/python3.9")
        # mock existence of the old version executable
        with patch.object(Path, "exists", return_value=True):
            # mock execution permission
            with patch("os.access", return_value=True):
                # mock version extraction returning an unsupported version
                with patch("plugin._extract_python_version", return_value=(3, 9)):
                    # act
                    compatible, version = _is_python_version_compatible(python_path)
                    # assert — verify version is rejected
                    assert compatible is False
                    # assert — verify version value
                    assert version == (3, 9)

    def test_candidate_python_executables(self):
        # arrange
        with patch.dict(os.environ, {"PYTHON_PATH": "/env/python"}):
            # mock current executable path
            with patch("sys.executable", "/sys/python"):
                # mock path resolution utility
                with patch("shutil.which", side_effect=lambda x: f"/path/{x}"):
                    # mock absolute path resolution on Path instances
                    with patch("pathlib.Path.resolve", side_effect=lambda self: self):
                        # act
                        candidates = _candidate_python_executables()
                        # map candidate objects to strings for easier assertion
                        paths = [str(p) for p in candidates]
                        # assert — verify environment candidate is present
                        assert "/env/python" in paths
                        # assert — verify runtime candidate is present
                        assert "/sys/python" in paths
                        # assert — verify search candidate is present
                        assert "/path/python3" in paths

    def test_find_python_executable_path_success(self):
        # arrange
        mock_candidates = [Path("/usr/bin/python3.10")]
        # mock candidate discovery helper
        with patch("plugin._candidate_python_executables", return_value=mock_candidates):
            # mock version compatibility helper
            with patch("plugin._is_python_version_compatible", return_value=(True, (3, 10))):
                # act
                path, version = find_python_executable_path()
                # assert — verify the correct path was found
                assert path == Path("/usr/bin/python3.10")
                # assert — verify the version
                assert version == (3, 10)

    def test_ensure_python_venv_exists(self):
        # arrange
        python_path = Path("/usr/bin/python3")
        # specify version for the test
        version = (3, 10)
        # mock app directory path
        with patch("plugin.APP_DIR") as mock_app_dir:
            # simulate an existing venv configuration file
            (mock_app_dir / "pyvenv.cfg").is_file.return_value = True
            # mock version check for the internal venv interpreter
            with patch("plugin._extract_python_version", return_value=(3, 10)):
                # mock subprocess call to track invocation
                with patch("subprocess.check_call") as mock_call:
                    # act
                    _ensure_python_venv(python_path, version)
                    # assert — verify that no venv creation was triggered
                    mock_call.assert_not_called()

    def test_ensure_python_venv_create(self):
        # arrange
        python_path = Path("/usr/bin/python3")
        # specify version for the creation scenario
        version = (3, 10)
        # mock app directory path
        with patch("plugin.APP_DIR") as mock_app_dir:
            # simulate a missing venv configuration
            (mock_app_dir / "pyvenv.cfg").is_file.return_value = False
            # mock subprocess call to verify execution
            with patch("subprocess.check_call") as mock_call:
                # act
                _ensure_python_venv(python_path, version)
                # assert — verify that the venv creation command was issued
                mock_call.assert_called_once()
                # extract command line arguments
                args = mock_call.call_args[0][0]
                # assert — verify module flag
                assert "-m" in args
                # assert — verify venv module name
                assert "venv" in args

    def test_ensure_application_installed_already_exists(self):
        # arrange
        with patch("plugin.APP_DIR") as mock_app_dir:
            # mock the current version string
            with patch("plugin.__version__", "1.0.0"):
                # simulate existing version marker file
                (mock_app_dir / "version.txt").is_file.return_value = True
                # create a mock file handle
                mock_file = MagicMock()
                # mock reading the version from file
                mock_file.read.return_value = "1.0.0"
                # configure the file context manager
                (mock_app_dir / "version.txt").open.return_value.__enter__.return_value = mock_file
                # act
                result = _ensure_application_installed()
                # assert — verify that a path was returned
                assert result is not None

    def test_ensure_application_installed_perform_install(self):
        # arrange
        with patch("plugin.APP_DIR") as mock_app_dir:
            # mock the current project version
            with patch("plugin.__version__", "1.0.0"):
                # mock successful python discovery
                with patch("plugin.find_python_executable_path", return_value=(Path("/usr/bin/python3"), (3, 10))):
                    # mock venv creation helper
                    with patch("plugin._ensure_python_venv"):
                        # mock search for wheel files
                        with patch("plugin.Path.glob", return_value=[Path("wheel.whl")]):
                            # mock subprocess for installation
                            with patch("subprocess.check_call") as mock_call:
                                # simulate missing version marker
                                (mock_app_dir / "version.txt").is_file.return_value = False
                                # act
                                result = _ensure_application_installed()
                                # assert — verify success path
                                assert result is not None
                                # assert — verify that pip was called
                                mock_call.assert_called_once()
                                # assert — verify that version was recorded
                                (mock_app_dir / "version.txt").write_text.assert_called_with("1.0.0")

    def test_main_missing_env_vars(self):
        # arrange
        with patch.dict(os.environ, {}, clear=True):
            # mock installation helper
            with patch("plugin._ensure_application_installed") as mock_install:
                # act
                main()
                # assert — verify that installation was not attempted
                mock_install.assert_not_called()

    def test_main_success(self):
        # arrange
        env_vars = {"KICAD_API_SOCKET": "sock", "KICAD_API_TOKEN": "tok"}
        # setup required environment variables
        with patch.dict(os.environ, env_vars):
            # mock successful installation
            with patch("plugin._ensure_application_installed", return_value=Path("/venv/python")):
                # mock the final application launch
                with patch("subprocess.check_call") as mock_call:
                    # act
                    main()
                    # assert — verify that the application was launched
                    mock_call.assert_called_once()
                    # extract command arguments
                    args = mock_call.call_args[0][0]
                    # assert — verify python path
                    assert "/venv/python" in args
                    # assert — verify module execution
                    assert "-m" in args
                    # assert — verify module name
                    assert "kicad_xyce_plugin" in args

    def test_show_error_dialog_macos(self):
        # arrange
        with patch("sys.platform", "darwin"):
            # mock subprocess for applescript alert
            with patch("subprocess.run") as mock_run:
                # import the helper within the patch scope
                from plugin import _show_error_dialog
                # act
                _show_error_dialog("error message")
                # assert — verify system call
                mock_run.assert_called_once()
                # extract command line
                args = mock_run.call_args[0][0]
                # convert args to string for flexible verification
                args_str = " ".join(args)
                # assert — verify script utility
                assert "osascript" in args_str
                # assert — verify dialog command
                assert "display alert" in args_str

    def test_show_error_dialog_windows(self):
        # arrange
        with patch("sys.platform", "win32"):
            # mock subprocess for powershell message box
            with patch("subprocess.run") as mock_run:
                # import helper for the windows scenario
                from plugin import _show_error_dialog
                # act
                _show_error_dialog("error message")
                # assert — verify that powershell was used
                mock_run.assert_called_once()
                # extract powershell command arguments
                args = mock_run.call_args[0][0]
                # convert args to string for flexible verification
                args_str = " ".join(args)
                # assert — verify powershell binary
                assert "powershell" in args_str
                # assert — verify winforms call
                assert "MessageBox" in args_str
