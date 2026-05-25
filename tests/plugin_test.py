import os
from pathlib import Path
from unittest.mock import patch

import plugin


def test_find_python_executable_path_uses_python_path_env(monkeypatch, tmp_path):
    # arrange
    python_path = tmp_path / "python"
    python_path.write_text("#!/usr/bin/env python\n")
    python_path.chmod(0o755)
    monkeypatch.setenv("PYTHON_PATH", str(python_path))
    # act
    with patch.object(plugin, "_is_python_version_compatible", return_value=True) as mock_check:
        result = plugin.find_python_executable_path()
    # assert
    assert result == python_path
    mock_check.assert_called_once_with(python_path)


def test_find_python_executable_path_falls_back_to_sys_executable(monkeypatch):
    # arrange
    monkeypatch.delenv("PYTHON_PATH", raising=False)
    monkeypatch.setattr(plugin.sys, "executable", "/usr/bin/python3", raising=False)
    monkeypatch.setattr(plugin.shutil, "which", lambda name: None)
    patch_target = patch.object(plugin, "_is_python_version_compatible", side_effect=lambda path: path == Path("/usr/bin/python3"))
    # act
    with patch_target:
        result = plugin.find_python_executable_path()
    # assert
    assert result == Path("/usr/bin/python3")


def test_find_python_executable_path_uses_py_launcher_on_windows(monkeypatch):
    # arrange
    monkeypatch.delenv("PYTHON_PATH", raising=False)
    monkeypatch.setattr(plugin.sys, "executable", "", raising=False)
    monkeypatch.setattr(plugin.sys, "platform", "win32", raising=False)
    monkeypatch.setattr(plugin.shutil, "which", lambda name: r"C:\Python\py.exe" if name == "py" else None)
    patch_target = patch.object(plugin, "_is_python_version_compatible", side_effect=lambda path: path == Path(r"C:\Python\py.exe"))
    # act
    with patch_target:
        result = plugin.find_python_executable_path()
    # assert
    assert result == Path(r"C:\Python\py.exe")


def test_find_python_executable_path_returns_none_when_no_compatible_python_found(monkeypatch):
    # arrange
    monkeypatch.delenv("PYTHON_PATH", raising=False)
    monkeypatch.setattr(plugin.sys, "executable", "/usr/bin/python3", raising=False)
    monkeypatch.setattr(plugin.shutil, "which", lambda name: None)
    # act
    with patch.object(plugin, "_is_python_version_compatible", return_value=False):
        result = plugin.find_python_executable_path()
    # assert
    assert result is None


def test_is_python_version_compatible_uses_clean_environment(monkeypatch, tmp_path):
    # arrange
    python_path = tmp_path / "python"
    python_path.write_text("#!/usr/bin/env python\n")
    python_path.chmod(0o755)

    expected_command = [str(python_path), "-c", "import sys; print(sys.version_info.major, sys.version_info.minor)"]

    def fake_check_output(command, env, text, stderr, timeout):
        assert command == expected_command
        assert env == {"PATH": os.defpath}
        assert text is True
        assert stderr == plugin.subprocess.STDOUT
        assert timeout == 5
        return "3 10\n"

    monkeypatch.setattr(plugin.subprocess, "check_output", fake_check_output)

    # act
    result = plugin._is_python_version_compatible(python_path)

    # assert
    assert result is True
