import os
import sys
import tempfile

from unittest.mock import MagicMock, patch

from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QApplication, QDialog

from config_dialog import ConfigDialog
from plugin_config import PluginConfig


_app = QApplication.instance() or QApplication(sys.argv)


def _make_dialog(initial_config: PluginConfig | None = None) -> ConfigDialog:

    """Create a ConfigDialog bypassing QML setup, with a mock QML root."""
    dialog = ConfigDialog.__new__(ConfigDialog)
    QDialog.__init__(dialog)
    dialog._initial_config = initial_config if initial_config is not None else PluginConfig.default()
    dialog._result = None
    dialog._root = MagicMock()
    return dialog


def _make_dialog_with_accept(initial_config: PluginConfig | None = None) -> ConfigDialog:
    dialog = _make_dialog(initial_config)
    dialog.accept = MagicMock()
    return dialog


class TestConfigDialogConstruction:

    def test_dialog_can_be_instantiated(self):
        # act — full construction path
        dialog = ConfigDialog(None)
        # assert
        assert isinstance(dialog, ConfigDialog)
        dialog.reject()

    def test_dialog_result_is_none_initially(self):
        # act
        dialog = ConfigDialog(None)
        # assert
        assert dialog._result is None
        dialog.reject()

    def test_on_qml_ready_skips_when_not_ready(self):
        # arrange
        dialog = _make_dialog()
        dialog._qml_view = MagicMock()
        # act — non-Ready status is ignored
        dialog._on_qml_ready(QQuickView.Status.Loading)
        # assert — root setProperty was not called
        dialog._root.setProperty.assert_not_called()


class TestConfigDialogOnQmlReady:

    def test_on_qml_ready_populates_path_field(self):
        # arrange
        config = PluginConfig(xyce_executable_path="/usr/bin/Xyce")
        dialog = _make_dialog(initial_config=config)
        dialog._qml_view = MagicMock()
        dialog._qml_view.rootObject.return_value = MagicMock()
        # act
        dialog._on_qml_ready(QQuickView.Status.Ready)
        # assert
        dialog._root.setProperty.assert_any_call("xyceExecutablePath", "/usr/bin/Xyce")

    def test_on_qml_ready_clears_error_text(self):
        # arrange
        dialog = _make_dialog()
        dialog._qml_view = MagicMock()
        dialog._qml_view.rootObject.return_value = MagicMock()
        # act
        dialog._on_qml_ready(QQuickView.Status.Ready)
        # assert
        dialog._root.setProperty.assert_any_call("errorText", "")

    def test_on_qml_ready_connects_signals(self):
        # arrange
        dialog = _make_dialog()
        mock_root = MagicMock()
        dialog._qml_view = MagicMock()
        dialog._qml_view.rootObject.return_value = mock_root
        # act
        dialog._on_qml_ready(QQuickView.Status.Ready)
        # assert — signals were connected
        mock_root.browseRequested.connect.assert_called_once()
        mock_root.submit.connect.assert_called_once()
        mock_root.cancelRequested.connect.assert_called_once()


class TestConfigDialogOnBrowseRequested:

    def test_on_browse_updates_path_field(self):
        # arrange
        dialog = _make_dialog()
        with patch("config_dialog.QFileDialog.getOpenFileName", return_value=("/usr/bin/Xyce", "")):
            # act
            dialog._on_browse_requested()
        # assert
        dialog._root.setProperty.assert_any_call("xyceExecutablePath", "/usr/bin/Xyce")

    def test_on_browse_clears_error_text_after_selection(self):
        # arrange
        dialog = _make_dialog()
        with patch("config_dialog.QFileDialog.getOpenFileName", return_value=("/usr/bin/Xyce", "")):
            # act
            dialog._on_browse_requested()
        # assert
        dialog._root.setProperty.assert_any_call("errorText", "")

    def test_on_browse_no_op_when_user_cancels(self):
        # arrange
        dialog = _make_dialog()
        with patch("config_dialog.QFileDialog.getOpenFileName", return_value=("", "")):
            # act
            dialog._on_browse_requested()
        # assert — no setProperty calls (user canceled)
        dialog._root.setProperty.assert_not_called()


class TestConfigDialogOnSubmit:

    def test_on_submit_rejects_empty_path(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit("")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Xyce executable path is required")

    def test_on_submit_rejects_whitespace_only_path(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit("   ")
        # assert
        dialog.accept.assert_not_called()

    def test_on_submit_rejects_non_executable_path(self):
        # arrange
        dialog = _make_dialog_with_accept()
        with tempfile.NamedTemporaryFile(delete=False) as f:
            non_exec_path = f.name
        os.chmod(non_exec_path, 0o644)
        try:
            # act
            dialog._on_submit(non_exec_path)
            # assert
            dialog.accept.assert_not_called()
            dialog._root.setProperty.assert_any_call("errorText", "Selected path is not an executable file")
        finally:
            os.unlink(non_exec_path)

    def test_on_submit_accepts_valid_executable(self):
        # arrange
        dialog = _make_dialog_with_accept()
        with tempfile.NamedTemporaryFile(delete=False) as f:
            exec_path = f.name
        os.chmod(exec_path, 0o755)
        try:
            with patch("config_dialog.PluginConfig.save"):
                # act
                dialog._on_submit(exec_path)
            # assert
            dialog.accept.assert_called_once()
            assert isinstance(dialog._result, PluginConfig)
            assert dialog._result.xyce_executable_path == exec_path
        finally:
            os.unlink(exec_path)

    def test_on_submit_strips_whitespace_from_path(self):
        # arrange
        dialog = _make_dialog_with_accept()
        with tempfile.NamedTemporaryFile(delete=False) as f:
            exec_path = f.name
        os.chmod(exec_path, 0o755)
        try:
            with patch("config_dialog.PluginConfig.save"):
                # act
                dialog._on_submit(f"  {exec_path}  ")
            # assert — whitespace stripped
            assert dialog._result.xyce_executable_path == exec_path
        finally:
            os.unlink(exec_path)

    def test_on_submit_calls_config_save_on_success(self):
        # arrange
        dialog = _make_dialog_with_accept()
        with tempfile.NamedTemporaryFile(delete=False) as f:
            exec_path = f.name
        os.chmod(exec_path, 0o755)
        try:
            with patch("config_dialog.PluginConfig.save") as mock_save:
                # act
                dialog._on_submit(exec_path)
            # assert
            mock_save.assert_called_once()
        finally:
            os.unlink(exec_path)


class TestConfigDialogGetConfig:

    def test_get_config_returns_none_when_rejected(self):
        # arrange
        dialog = _make_dialog()
        with patch.object(dialog, "exec", return_value=QDialog.DialogCode.Rejected):
            # act
            result = dialog.get_config()
        # assert
        assert result is None

    def test_get_config_returns_result_when_accepted(self):
        # arrange
        dialog = _make_dialog()
        dialog._result = PluginConfig(xyce_executable_path="/usr/bin/Xyce")
        with patch.object(dialog, "exec", return_value=QDialog.DialogCode.Accepted):
            # act
            result = dialog.get_config()
        # assert
        assert isinstance(result, PluginConfig)
        assert result.xyce_executable_path == "/usr/bin/Xyce"
