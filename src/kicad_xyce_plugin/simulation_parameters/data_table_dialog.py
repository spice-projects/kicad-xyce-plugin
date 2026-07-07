import re
from pathlib import Path

from PySide6.QtCore import Qt, QUrl, Slot
from PySide6.QtGui import QColor
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QDialog, QVBoxLayout, QWidget

from .data_block import DataBlock

_QML_FILE = Path(__file__).parent / "data_table_dialog.qml"
_BG = "#1a1b1e"
_IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_RESERVED_NAMES = {"TIME", "FREQ", "HERTZ", "VT", "TEMP", "TEMPER", "GMIN"}


def _is_identifier(name: str) -> bool:
    # reject blank or whitespace-padded identifiers
    candidate = name.strip()
    if not candidate:
        return False
    # accept only simple identifier syntax for data-table names and columns
    return _IDENTIFIER_RE.fullmatch(candidate) is not None


def _is_reserved_name(name: str) -> bool:
    # compare against the Xyce reserved parameter names
    return name.strip().upper() in _RESERVED_NAMES


def _is_valid_name(name: str) -> bool:
    # enforce the same identifier rules for table and column names
    return _is_identifier(name) and not _is_reserved_name(name)


def _as_python_list(value) -> list:
    # unwrap qml list values when Qt exposes them as QVariant wrappers
    if hasattr(value, "toVariant"):
        value = value.toVariant()
    # normalize the result into a plain python list
    return list(value) if value is not None else []


class DataTableDialog(QDialog):

    def __init__(self, parent: QWidget, data_block: DataBlock | None = None, existing_table_names: tuple[str, ...] = (), original_table_name: str = ""):
        super().__init__(parent)
        # keep the original block so the caller can tell whether a rename occurred
        self._original_block = data_block
        # store the edited block result after accept
        self._result: DataBlock | None = None
        # keep the table list for uniqueness validation
        self._existing_table_names = existing_table_names
        # track the original name for rename validation
        self._original_table_name = original_table_name or (data_block.name if data_block is not None else "")
        # window setup
        self.setWindowTitle("Edit DATA Table")
        self.setWindowModality(Qt.WindowModality.WindowModal)
        self.resize(780, 560)
        # create the qml surface
        self._qml_view = QQuickView()
        self._qml_view.statusChanged.connect(self._on_qml_ready)
        self._qml_view.setResizeMode(QQuickView.ResizeMode.SizeRootObjectToView)
        self._qml_view.setColor(QColor(_BG))
        self._qml_view.setSource(QUrl.fromLocalFile(str(_QML_FILE)))
        # embed the qml view into the dialog
        container = QWidget.createWindowContainer(self._qml_view, self)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(container)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status):
        # ignore intermediate loading states
        if status != QQuickView.Status.Ready:
            return
        # load the qml root object
        root = self._qml_view.rootObject()
        # initialize the dialog with either the supplied block or an empty one
        if self._original_block is None:
            root.initialize("", [], [], list(self._existing_table_names), self._original_table_name)
        else:
            root.initialize(self._original_block.name, list(self._original_block.parameters), [list(record) for record in self._original_block.records], list(self._existing_table_names), self._original_table_name)
        # connect dialog actions back to Python
        root.dialogAccepted.connect(self._on_dialog_accepted)
        root.dialogRejected.connect(self.reject)

    @Slot()
    def _on_dialog_accepted(self):
        # read the edited values from qml
        root = self._qml_view.rootObject()
        # build a candidate block from the UI state
        block = DataBlock(name=str(root.property("tableName")).strip(), parameters=tuple(str(name).strip() for name in _as_python_list(root.property("columnNames"))), records=tuple(tuple(str(value).strip() for value in row) for row in _as_python_list(root.property("rowValues"))))
        # validate the candidate before closing the dialog
        error = self._validate_data_block(block)
        if error is not None:
            # keep the dialog open and surface the validation problem in qml
            root.setProperty("validationError", error)
            return
        # store the accepted result for the caller
        self._result = block
        # close the dialog with acceptance
        self.accept()

    def _validate_data_block(self, block: DataBlock) -> str | None:
        # require a table name
        if not block.name:
            return "Table name is required"
        # require a legal identifier for the table name
        if not _is_valid_name(block.name):
            return "Table name must be a legal identifier"
        # reject duplicate table names when editing an existing collection
        for existing_name in self._existing_table_names:
            if existing_name.strip().upper() == block.name.upper() and existing_name.strip().upper() != self._original_table_name.strip().upper():
                return "Table name must be unique"
        # require at least one column and row
        if len(block.parameters) < 1:
            return "At least one column is required"
        if len(block.records) < 1:
            return "At least one row is required"
        # validate each column name
        seen_columns: set[str] = set()
        for column_name in block.parameters:
            if not column_name:
                return "Column header names are required"
            if not _is_valid_name(column_name):
                return "Column header names must be legal identifiers"
            normalized = column_name.upper()
            if normalized in seen_columns:
                return "Column header names must be unique"
            seen_columns.add(normalized)
        # validate each row length and each cell value
        for row in block.records:
            if len(row) != len(block.parameters):
                return "Each row must contain one value per column"
            for value in row:
                if not value:
                    return "Data values are required"
                if not DataBlock._is_number(value):
                    return "Data values must be numeric"
        # the block is valid
        return None

    @property
    def data_block(self) -> DataBlock | None:
        return self._result
