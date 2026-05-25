import os
from pathlib import Path

import pytest
from PySide6.QtQuick import QQuickItem, QQuickView
from PySide6.QtCore import QUrl, qInstallMessageHandler, QtMsgType
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")


QML_PATH = Path(__file__).parent.parent.parent / "src" / "kicad_xyce_plugin" / "add_plot_dialog.qml"


@pytest.fixture
def view(qapp: QApplication):
    # collect warnings loading the qml file
    warnings = []

    def message_handler(msg_type, context, message):
        # check for warning messages
        if msg_type == QtMsgType.QtWarningMsg:
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
    # assert default values for all properties exposed by the QML panel
    assert root.property("expressions").toVariant() == []
    assert root.property("selectedIndices").toVariant() == []
    assert root.property("selectionState").toVariant() == {}
    assert root.property("filterText") == ""
    assert root.property("expressionError") == ""
    assert root.property("allowCustomExpressions") is True
    assert root.property("expressionColors").toVariant() == {
        "voltage": "#5b9bd5",
        "current": "#7cb342",
        "frequency": "#e57373",
        "time": "#ba68c8",
        "power": "#ffb74d",
        "misc": "#3a3d4a"
    }
    assert root.property("filteredExpressions").toVariant() == []
