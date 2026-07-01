import os
from pathlib import Path

import pytest
from PySide6.QtQuick import QQuickItem, QQuickView
from PySide6.QtCore import QUrl, qInstallMessageHandler, QtMsgType
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")


QML_PATH = Path(__file__).parent.parent.parent / "src" / "kicad_xyce_plugin" / "fft_dialog.qml"


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
    # assert default values for top-level properties exposed by the FFT dialog
    assert root.property("windowFunctions").toVariant() == []
    assert root.property("outputTypes").toVariant() == []
    assert root.property("fftPointOptions").toVariant() == []
    assert root.property("abscissaMin") == 0
    assert root.property("abscissaMax") == 0
    assert root.property("zoomFromTime") == 0
    assert root.property("zoomToTime") == 0
    assert root.property("defaultWindowIndex") == 2
    assert root.property("defaultFftPointIndex") == 3
    assert root.property("expressionItems").toVariant() == []
    assert root.property("selectionState").toVariant() == {}
    assert root.property("filterText") == ""
    assert root.property("rangeMode") == "all"
    assert root.property("rangeOptions").toVariant() == [
        {"label": "All abscissa points", "value": "all"},
        {"label": "Current zoom / visible range", "value": "zoom"},
        {"label": "Custom time range", "value": "custom"}
    ]
