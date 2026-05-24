import os
from pathlib import Path

import pytest
from PySide6.QtQuick import QQuickItem, QQuickView
from PySide6.QtCore import QUrl, qInstallMessageHandler, QtMsgType
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")


QML_PATH = Path(__file__).parent.parent.parent / "src" / "simulation_parameters" / "simulation_parameters_dialog.qml"


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
    # assert default values for top-level properties and aliases
    assert root.property("initialTabIndex") == 0
    assert root.property("currentTabIndex") == 0
    assert root.property("replaceGround") is False
    assert root.property("errorText") == ""
    assert root.property("opPrintEnabled") is False
    assert root.property("opPrintAllNodes") is False
    assert root.property("opPrintAllCurrents") is False
    assert root.property("opPrintPower") is False
    assert root.property("opPrintBjtLeads") is False
    assert root.property("opPrintFetLeads") is False
    assert root.property("opHasBjtDevices") is False
    assert root.property("opHasFetDevices") is False
    assert root.property("opPrintSpecificVars") == ""
    assert root.property("opPrintFormatIndex") == 0
    assert root.property("opPrintFile") == ""
    assert root.property("saveEnabled") is False
    assert root.property("saveType") == "NODESET"
    assert root.property("saveFile") == ""
    assert root.property("tranPrintEnabled") is False
    assert root.property("tranPrintAllNodes") is False
    assert root.property("tranPrintAllCurrents") is False
    assert root.property("tranPrintPower") is False
    assert root.property("tranPrintBjtLeads") is False
    assert root.property("tranPrintFetLeads") is False
    assert root.property("tranHasBjtDevices") is False
    assert root.property("tranHasFetDevices") is False
    assert root.property("tranPrintSpecificVars") == ""
    assert root.property("tranPrintFormatIndex") == 0
    assert root.property("tranPrintFile") == ""
    assert root.property("dcPrintEnabled") is False
    assert root.property("dcPrintAllNodes") is False
    assert root.property("dcPrintAllCurrents") is False
    assert root.property("dcPrintPower") is False
    assert root.property("dcPrintBjtLeads") is False
    assert root.property("dcPrintFetLeads") is False
    assert root.property("dcHasBjtDevices") is False
    assert root.property("dcHasFetDevices") is False
    assert root.property("dcPrintSpecificVars") == ""
    assert root.property("dcPrintFormatIndex") == 0
    assert root.property("dcPrintFile") == ""
    assert root.property("acSensEnabled") is False
    assert root.property("acSensObjectiveMode") == "objfunc"
    assert root.property("acSensObjectiveValues") == ""
    assert root.property("acSensParameters") == ""
    assert root.property("acSensDirect") is False
    assert root.property("acSensAdjoint") is False
    assert root.property("acSensPrintEnabled") is False
    assert root.property("acSensPrintSpecificVars") == ""
    assert root.property("acSensPrintFormatIndex") == 0
    assert root.property("acSensPrintFile") == ""
    assert root.property("acSweepModeIndex") == 0
    assert root.property("acPoints") == ""
    assert root.property("acStart") == ""
    assert root.property("acEnd") == ""
    assert root.property("acDataTableName") == ""
    assert root.property("acMeasureParametersText") == ""
    assert root.property("acPrintEnabled") is False
    assert root.property("acPrintAllNodes") is False
    assert root.property("acPrintAllCurrents") is False
    assert root.property("acPrintSpecificVars") == ""
    assert root.property("acPrintFormatIndex") == 0
    assert root.property("acPrintFile") == ""
    assert root.property("noiseOutputNode") == ""
    assert root.property("noiseRefNode") == ""
    assert root.property("noiseSourceName") == ""
    assert root.property("noiseSweepModeIndex") == 0
    assert root.property("noisePoints") == ""
    assert root.property("noiseStart") == ""
    assert root.property("noiseEnd") == ""
    assert root.property("noiseDataTableName") == ""
    assert root.property("noiseMeasureParametersText") == ""
    assert root.property("noisePrintEnabled") is False
    assert root.property("noisePrintAllNodes") is False
    assert root.property("noisePrintAllCurrents") is False
    assert root.property("noisePrintInoise") is False
    assert root.property("noisePrintOnoise") is False
    assert root.property("noisePrintSpecificVars") == ""
    assert root.property("noisePrintFormatIndex") == 0
    assert root.property("noisePrintFile") == ""
    assert root.property("noiseDeviceOperators").toVariant() == []
    assert root.property("hbFrequenciesText") == ""
    assert root.property("hbHarmonicsText") == ""
    assert root.property("hbTahbIndex") == 0
    assert root.property("hbSelectHarmsIndex") == 0
    assert root.property("hbStartupPeriodsText") == ""
    assert root.property("hbPrintEnabled") is False
    assert root.property("hbPrintAllNodes") is False
    assert root.property("hbPrintAllCurrents") is False
    assert root.property("hbPrintTypeIndex") == 0
    assert root.property("hbPrintSpecificVars") == ""
    assert root.property("hbPrintFormatIndex") == 0
    assert root.property("hbPrintFile") == ""
    assert root.property("linSparcalc") is True
    assert root.property("linFormat") == "TOUCHSTONE2"
    assert root.property("linType") == "S"
    assert root.property("linDataFormat") == "RI"
    assert root.property("linFile") == ""
    assert root.property("linWidth") == ""
    assert root.property("linPrecision") == ""
    assert root.property("linSweepModeIndex") == 0
    assert root.property("linPoints") == ""
    assert root.property("linStart") == ""
    assert root.property("linEnd") == ""
    assert root.property("linDataTableName") == ""
    assert root.property("linPrintEnabled") is False
    assert root.property("linPrintAllNodes") is False
    assert root.property("linPrintAllCurrents") is False
    assert root.property("linPrintSpecificVars") == ""
    assert root.property("linPrintFormatIndex") == 0
    assert root.property("linPrintFile") == ""
