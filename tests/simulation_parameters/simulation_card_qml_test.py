import os
from pathlib import Path

import pytest
from PySide6.QtQuick import QQuickView
from PySide6.QtCore import QUrl
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")


QML_PATH = Path(__file__).parent.parent.parent / "src" / "simulation_parameters" / "SimulationCard.qml"


@pytest.fixture
def view(qapp: QApplication):
    # create view
    v = QQuickView()
    # set qml source
    v.setSource(QUrl.fromLocalFile(str(QML_PATH)))
    # wait for qml loading
    QTest.qWait(100)
    # use view in tests
    yield v
    # close it
    v.close()


@pytest.fixture
def root(view):
    # extract root component
    r = view.rootObject()
    # ensure it is valid
    assert r is not None, "QML failed to load"
    # use it
    return r


def test_loads_without_errors(view):
    # act
    errors = view.errors()
    # assert
    assert errors == [], [e.description() for e in errors]
