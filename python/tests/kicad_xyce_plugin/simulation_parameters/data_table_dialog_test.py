import sys
from unittest.mock import MagicMock

from PySide6.QtWidgets import QApplication, QDialog

from kicad_xyce_plugin.simulation_parameters import DataBlock
from kicad_xyce_plugin.simulation_parameters.data_table_dialog import DataTableDialog

_app = QApplication.instance() or QApplication(sys.argv)


def _make_dialog(existing_table_names=(), original_table_name="", data_block=None, table_name="", column_names=None, row_values=None):
    # build a DataTableDialog bypassing __init__ so no QML objects are created
    dialog = DataTableDialog.__new__(DataTableDialog)
    QDialog.__init__(dialog)
    dialog._original_block = data_block
    dialog._existing_table_names = existing_table_names
    dialog._original_table_name = original_table_name
    dialog._result = None
    dialog._qml_view = MagicMock()
    root = MagicMock()
    dialog._qml_view.rootObject.return_value = root
    properties = {
        "tableName": table_name or (data_block.name if data_block else ""),
        "columnNames": list(column_names if column_names is not None else (data_block.parameters if data_block else ())),
        "rowValues": [list(row) for row in (row_values if row_values is not None else (data_block.records if data_block else ()))],
    }
    root.property.side_effect = lambda name: properties[name]
    return dialog, root


class TestDataTableDialogValidation:

    def test_accepts_valid_block(self):
        # arrange
        dialog, _root = _make_dialog()
        block = DataBlock(name="myTable", parameters=("r1", "r2"), records=(("1.0", "2.0"), ("3.0", "4.0")))
        # act
        error = dialog._validate_data_block(block)
        # assert
        assert error is None

    def test_rejects_duplicate_table_name(self):
        # arrange
        dialog, _root = _make_dialog(existing_table_names=("existingTable",))
        block = DataBlock(name="existingTable", parameters=("r1",), records=(("1.0",),))
        # act
        error = dialog._validate_data_block(block)
        # assert
        assert error == "Table name must be unique"

    def test_rejects_duplicate_column_name(self):
        # arrange
        dialog, _root = _make_dialog()
        block = DataBlock(name="myTable", parameters=("r1", "r1"), records=(("1.0", "2.0"),))
        # act
        error = dialog._validate_data_block(block)
        # assert
        assert error == "Column header names must be unique"

    def test_rejects_non_numeric_value(self):
        # arrange
        dialog, _root = _make_dialog()
        block = DataBlock(name="myTable", parameters=("r1",), records=(("abc",),))
        # act
        error = dialog._validate_data_block(block)
        # assert
        assert error == "Data values must be numeric"


class TestDataTableDialogAcceptance:

    def test_dialog_accepted_stores_result(self):
        # arrange
        block = DataBlock(name="myTable", parameters=("r1", "r2"), records=(("1.0", "2.0"),))
        dialog, root = _make_dialog(data_block=block)
        dialog.accept = MagicMock()
        # act
        dialog._on_dialog_accepted()
        # assert
        assert dialog.data_block == block
        dialog.accept.assert_called_once()

    def test_dialog_accepted_rejects_invalid_data(self):
        # arrange
        dialog, root = _make_dialog(table_name="myTable", column_names=("r1",), row_values=(("abc",),))
        dialog.accept = MagicMock()
        # act
        dialog._on_dialog_accepted()
        # assert
        assert dialog.data_block is None
        root.setProperty.assert_called_once()
        dialog.accept.assert_not_called()
