import os
from pathlib import Path

import pytest
from PySide6.QtCore import QUrl, QtMsgType, qInstallMessageHandler
from PySide6.QtQuick import QQuickItem, QQuickView
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")


QML_PATH = Path(__file__).parent.parent.parent.parent / "src" / "kicad_xyce_plugin" / "simulation_parameters" / "data_table_dialog.qml"


@pytest.fixture
def view(qapp: QApplication):
    # collect warnings loading the qml file
    warnings = []

    def message_handler(msg_type, context, message):
        # check for warning messages
        if msg_type == QtMsgType.QtWarningMsg and not str(message).startswith("Populating font family aliases"):
            warnings.append(message)

    # install custom message handler
    previous_handler = qInstallMessageHandler(message_handler)
    # create view
    v = QQuickView()
    # set qml source
    v.setSource(QUrl.fromLocalFile(str(QML_PATH)))
    # wait for qml loading
    QTest.qWait(100)
    # use view in tests
    yield v
    # restore previous message handler
    qInstallMessageHandler(previous_handler)
    # close it
    v.close()
    # assert no warnings were emitted
    assert warnings == [], warnings


@pytest.fixture
def root(view: QQuickView) -> QQuickItem:
    # extract root component
    r = view.rootObject()
    # ensure it is valid
    assert r is not None, "QML failed to load"
    # use it
    return r


def test_loads_without_errors(view: QQuickView):
    # act
    errors = view.errors()
    # assert
    assert errors == [], [e.description() for e in errors]


def test_property_default_values(root: QQuickItem):
    # assert default values for top-level properties
    assert root.property("tableName") == ""
    assert root.property("existingTableNames").toVariant() == []
    assert root.property("originalTableName") == ""
    assert root.property("columnNames").toVariant() == []
    assert root.property("rowValues").toVariant() == []
    assert root.property("validationError") == ""
    assert root.property("canAccept") is True


def test_initialize_sets_valid_state(root: QQuickItem):
    # act
    root.initialize("myTable", ["r1", "r2"], [["1.0", "2.0"], ["3.0", "4.0"]], [], "")
    # assert
    assert root.property("tableName") == "myTable"
    assert root.property("columnNames").toVariant() == ["r1", "r2"]
    assert root.property("rowValues").toVariant() == [["1.0", "2.0"], ["3.0", "4.0"]]
    assert root.property("validationError") == ""
    assert root.property("canAccept") is True


def test_initialize_sets_error_for_empty_block(root: QQuickItem):
    # act
    root.initialize("myTable", [], [], [], "")
    # assert
    assert root.property("validationError") == "At least one column is required"
    assert root.property("canAccept") is False
