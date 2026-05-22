pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

Item {
    id: root
    implicitWidth: 640
    implicitHeight: 680

    // --- Tab persistence ---
    property int initialTabIndex: 0
    onInitialTabIndexChanged: simTabBar.currentIndex = initialTabIndex

    // --- Transient tab properties (delegated to TranPanel) ---
    property alias initialStep: tranPanel.initialStep
    property alias finalTime: tranPanel.finalTime
    property alias startTime: tranPanel.startTime
    property alias stepCeiling: tranPanel.stepCeiling
    property alias opModeIndex: tranPanel.opModeIndex
    property alias scheduleEnabled: tranPanel.scheduleEnabled
    property alias schedulePairsText: tranPanel.schedulePairsText
    property alias fftParametersText: tranPanel.fftParametersText
    property alias fourParametersText: tranPanel.fourParametersText
    property alias tranMeasureParametersText: tranPanel.measureParametersText

    // --- DC Sweep tab properties (delegated to DcPanel) ---
    property alias sweepModeIndex: dcPanel.sweepModeIndex
    property alias primaryVariable: dcPanel.primaryVariable
    property alias startValue: dcPanel.startValue
    property alias stopValue: dcPanel.stopValue
    property alias stepValue: dcPanel.stepValue
    property alias pointsValue: dcPanel.pointsValue
    property alias listValuesText: dcPanel.listValuesText
    property alias dataTableName: dcPanel.dataTableName
    property alias secondaryEnabled: dcPanel.secondaryEnabled
    property alias secondaryVariable: dcPanel.secondaryVariable
    property alias secondaryStart: dcPanel.secondaryStart
    property alias secondaryStop: dcPanel.secondaryStop
    property alias secondaryStep: dcPanel.secondaryStep
    property alias secondaryPoints: dcPanel.secondaryPoints
    property alias dcMeasureParametersText: dcPanel.measureParametersText

    // --- OP simulation properties (delegated to OpPanel) ---
    property alias opPrintEnabled: opPanel.printEnabled
    property alias opPrintAllNodes: opPanel.printAllNodes
    property alias opPrintAllCurrents: opPanel.printAllCurrents
    property alias opPrintPower: opPanel.printPower
    property alias opPrintBjtLeads: opPanel.printBjtLeads
    property alias opPrintFetLeads: opPanel.printFetLeads
    property alias opHasBjtDevices: opPanel.hasBjtDevices
    property alias opHasFetDevices: opPanel.hasFetDevices
    property alias opPrintSpecificVars: opPanel.printSpecificVars
    property alias opPrintFormatIndex: opPanel.printFormatIndex
    property alias opPrintFile: opPanel.printFile
    property alias saveEnabled: opPanel.saveEnabled
    property alias saveType: opPanel.saveType
    property alias saveFile: opPanel.saveFile
    property alias nodesetEntries: opPanel.nodesetEntries

    // --- Transient print properties (delegated to TranPanel) ---
    property alias tranPrintEnabled: tranPanel.printEnabled
    property alias tranPrintAllNodes: tranPanel.printAllNodes
    property alias tranPrintAllCurrents: tranPanel.printAllCurrents
    property alias tranPrintPower: tranPanel.printPower
    property alias tranPrintBjtLeads: tranPanel.printBjtLeads
    property alias tranPrintFetLeads: tranPanel.printFetLeads
    property alias tranHasBjtDevices: tranPanel.hasBjtDevices
    property alias tranHasFetDevices: tranPanel.hasFetDevices
    property alias tranPrintSpecificVars: tranPanel.printSpecificVars
    property alias tranPrintFormatIndex: tranPanel.printFormatIndex
    property alias tranPrintFile: tranPanel.printFile

    // --- DC Sweep print properties (delegated to DcPanel) ---
    property alias dcPrintEnabled: dcPanel.printEnabled
    property alias dcPrintAllNodes: dcPanel.printAllNodes
    property alias dcPrintAllCurrents: dcPanel.printAllCurrents
    property alias dcPrintPower: dcPanel.printPower
    property alias dcPrintBjtLeads: dcPanel.printBjtLeads
    property alias dcPrintFetLeads: dcPanel.printFetLeads
    property alias dcHasBjtDevices: dcPanel.hasBjtDevices
    property alias dcHasFetDevices: dcPanel.hasFetDevices
    property alias dcPrintSpecificVars: dcPanel.printSpecificVars
    property alias dcPrintFormatIndex: dcPanel.printFormatIndex
    property alias dcPrintFile: dcPanel.printFile

    // --- Sensitivity tab properties (delegated to SensPanel) ---
    property alias sensEnabled: sensPanel.active
    property alias sensObjectiveMode: sensPanel.objectiveMode
    property alias sensObjectiveValues: sensPanel.objectiveValues
    property alias sensParameters: sensPanel.parameters
    property alias sensDirect: sensPanel.direct
    property alias sensAdjoint: sensPanel.adjoint
    property alias sensPrintEnabled: sensPanel.printEnabled
    property alias sensPrintSpecificVars: sensPanel.printSpecificVars
    property alias sensPrintFormatIndex: sensPanel.printFormatIndex
    property alias sensPrintFile: sensPanel.printFile

    // --- AC sweep tab properties (delegated to AcPanel) ---
    property alias acSweepModeIndex: acPanel.sweepModeIndex
    property alias acPoints: acPanel.points
    property alias acStart: acPanel.start
    property alias acEnd: acPanel.end
    property alias acDataTableName: acPanel.dataTableName
    property alias acMeasureParametersText: acPanel.measureParametersText

    // --- AC print properties (delegated to AcPanel) ---
    property alias acPrintEnabled: acPanel.printEnabled
    property alias acPrintAllNodes: acPanel.printAllNodes
    property alias acPrintAllCurrents: acPanel.printAllCurrents
    property alias acPrintSpecificVars: acPanel.printSpecificVars
    property alias acPrintFormatIndex: acPanel.printFormatIndex
    property alias acPrintFile: acPanel.printFile

    // --- NOISE sweep tab properties (delegated to NoisePanel) ---
    property alias noiseOutputNode: noisePanel.outputNode
    property alias noiseRefNode: noisePanel.refNode
    property alias noiseSourceName: noisePanel.sourceName
    property alias noiseSweepModeIndex: noisePanel.sweepModeIndex
    property alias noisePoints: noisePanel.points
    property alias noiseStart: noisePanel.start
    property alias noiseEnd: noisePanel.end
    property alias noiseDataTableName: noisePanel.dataTableName
    property alias noiseMeasureParametersText: noisePanel.measureParametersText

    // --- NOISE print properties (delegated to NoisePanel) ---
    property alias noisePrintEnabled: noisePanel.printEnabled
    property alias noisePrintAllNodes: noisePanel.printAllNodes
    property alias noisePrintAllCurrents: noisePanel.printAllCurrents
    property alias noisePrintInoise: noisePanel.printInoise
    property alias noisePrintOnoise: noisePanel.printOnoise
    property alias noisePrintSpecificVars: noisePanel.printSpecificVars
    property alias noisePrintFormatIndex: noisePanel.printFormatIndex
    property alias noisePrintFile: noisePanel.printFile
    property alias noiseDeviceOperators: noisePanel.deviceOperators

    // --- HB tab properties (delegated to HbPanel) ---
    property alias hbFrequenciesText: hbPanel.frequenciesText
    property alias hbHarmonicsText: hbPanel.harmonicsText
    property alias hbTahbIndex: hbPanel.tahbIndex
    property alias hbSelectHarmsIndex: hbPanel.selectHarmsIndex
    property alias hbStartupPeriodsText: hbPanel.startupPeriodsText

    // --- HB print properties (delegated to HbPanel) ---
    property alias hbPrintEnabled: hbPanel.printEnabled
    property alias hbPrintAllNodes: hbPanel.printAllNodes
    property alias hbPrintAllCurrents: hbPanel.printAllCurrents
    property alias hbPrintTypeIndex: hbPanel.printTypeIndex
    property alias hbPrintSpecificVars: hbPanel.printSpecificVars
    property alias hbPrintFormatIndex: hbPanel.printFormatIndex
    property alias hbPrintFile: hbPanel.printFile

    // --- LIN tab properties (delegated to LinPanel) ---
    property alias linSparcalc: linPanel.sparcalc
    property alias linFormat: linPanel.linFormat
    property alias linType: linPanel.linType
    property alias linDataFormat: linPanel.linDataFormat
    property alias linFile: linPanel.linFile
    property alias linWidth: linPanel.linWidth
    property alias linPrecision: linPanel.linPrecision
    property alias linSweepModeIndex: linPanel.sweepModeIndex
    property alias linPoints: linPanel.points
    property alias linStart: linPanel.start
    property alias linEnd: linPanel.end
    property alias linDataTableName: linPanel.dataTableName

    // --- LIN print properties (delegated to LinPanel) ---
    property alias linPrintEnabled: linPanel.printEnabled
    property alias linPrintAllNodes: linPanel.printAllNodes
    property alias linPrintAllCurrents: linPanel.printAllCurrents
    property alias linPrintSpecificVars: linPanel.printSpecificVars
    property alias linPrintFormatIndex: linPanel.printFormatIndex
    property alias linPrintFile: linPanel.printFile

    // --- Shared properties ---
    property bool replaceGround: false
    property string errorText: ""

    signal submitTransient(string initialStep, string finalTime, string startTime, string stepCeiling, string opKeyword, bool scheduleEnabled, string schedulePairsText, string fftParametersText, string fourParametersText, string measureParametersText, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitDC(string sweepMode, string primaryVariable, string startValue, string stopValue, string stepValue, string pointsValue, string listValuesText, string dataTableName, bool secondaryEnabled, string secondaryVariable, string secondaryStart, string secondaryStop, string secondaryStep, string secondaryPoints, string measureParametersText, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitOP(bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool saveEnabled, string saveType, string nodesetEntries, string saveFile, bool replaceGround)
    signal submitAC(string sweepMode, string points, string start, string end, string dataTableName, string measureParametersText, bool printEnabled, bool printAllNodes, bool printAllCurrents, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitNoise(string outputNode, string refNode, string sourceName, string sweepMode, string points, string start, string end, string dataTableName, string measureParametersText, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printInoise, bool printOnoise, string printSpecificVars, string printFormat, string printFile, bool replaceGround, var deviceOperatorsList)
    signal submitHB(string frequenciesText, string harmonicsText, int tahb, string selectharms, int startupPeriods, bool printEnabled, bool printAllNodes, bool printAllCurrents, string printType, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitLIN(bool sparcalc, string format, string lintype, string dataformat, string file, string width, string precision, string sweepMode, string points, string start, string end, string dataTableName, bool printEnabled, bool printAllNodes, bool printAllCurrents, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitSens(string objectiveMode, string objectiveValues, string parameters, bool direct, bool adjoint, bool replaceGround, bool printEnabled, string printSpecificVars, string printFormat, string printFile)
    signal cancelRequested()


    Rectangle {
        anchors.fill: parent
        color: "#efefe8"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        Label {
            text: "Xyce Simulation"
            font.pixelSize: 22
            font.bold: true
            color: "#1b1f23"
            Layout.fillWidth: true
        }

        Label {
            text: "Select a simulation type and configure its parameters"
            font.pixelSize: 13
            color: "#4a5560"
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        TabBar {
            id: simTabBar
            Layout.fillWidth: true
            onCurrentIndexChanged: root.errorText = ""

            TabButton {
                text: ".OP"
                width: (root.width - 45) / 8
            }

            TabButton {
                text: ".TRAN"
                width: (root.width - 45) / 8
            }

            TabButton {
                text: ".DC"
                width: (root.width - 45) / 8
            }

            TabButton {
                text: ".AC"
                width: (root.width - 45) / 8
            }

            TabButton {
                text: ".SENS"
                width: (root.width - 45) / 8
            }

            TabButton {
                text: ".NOISE"
                width: (root.width - 45) / 8
            }

            TabButton {
                text: ".HB"
                width: (root.width - 45) / 8
            }

            TabButton {
                text: ".LIN"
                width: (root.width - 45) / 8
            }
        }

        Rectangle {
            color: "#ffffff"
            radius: 8
            border.color: "#d0d7de"
            border.width: 1
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                anchors.fill: parent
                anchors.margins: 16
                currentIndex: simTabBar.currentIndex

                // --- Tab 0: Operating Point ---
                OpPanel {
                    id: opPanel
                }

                // --- Tab 1: Transient ---
                TranPanel {
                    id: tranPanel
                }

                // --- Tab 2: DC Sweep ---
                DcPanel {
                    id: dcPanel
                }

                // --- Tab 3: AC Sweep ---
                AcPanel {
                    id: acPanel
                }

                // --- Tab 4: Sensitivity ---
                SensPanel {
                    id: sensPanel
                }

                // --- Tab 5: Noise ---
                NoisePanel {
                    id: noisePanel
                }

                // --- Tab 6: HB ---
                HbPanel {
                    id: hbPanel
                }

                // --- Tab 7: LIN ---
                LinPanel {
                    id: linPanel
                }
            }
        }

        Label {
            text: root.errorText
            visible: root.errorText.length > 0
            color: "#b42318"
            font.pixelSize: 12
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        CheckBox {
            id: replaceGroundCheckBox
            text: "Replace ground node (.PREPROCESS REPLACEGROUND TRUE)"
            checked: root.replaceGround
            onCheckedChanged: root.replaceGround = checked
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: "Cancel"
                onClicked: root.cancelRequested()
            }

            Button {
                text: "Apply"
                highlighted: true
                onClicked: {
                    if (simTabBar.currentIndex === 0) {
                        root.submitOP(opPanel.printEnabled, opPanel.printAllNodes, opPanel.printAllCurrents, opPanel.printPower, opPanel.printBjtLeads, opPanel.printFetLeads, opPanel.printSpecificVars, opPanel.printFormatValue, opPanel.printFile, opPanel.saveEnabled, opPanel.saveType, opPanel.nodesetEntries, opPanel.saveFile, root.replaceGround)
                    } else if (simTabBar.currentIndex === 1) {
                        root.submitTransient(tranPanel.initialStep, tranPanel.finalTime, tranPanel.startTime, tranPanel.stepCeiling, tranPanel.opKeywordValue, tranPanel.scheduleEnabled, tranPanel.schedulePairsText, tranPanel.fftParametersText, tranPanel.fourParametersText, tranPanel.measureParametersText, tranPanel.printEnabled, tranPanel.printAllNodes, tranPanel.printAllCurrents, tranPanel.printPower, tranPanel.printBjtLeads, tranPanel.printFetLeads, tranPanel.printSpecificVars, tranPanel.printFormatValue, tranPanel.printFile, root.replaceGround)
                    } else if (simTabBar.currentIndex === 2) {
                        root.submitDC(dcPanel.sweepModeValue, dcPanel.primaryVariable, dcPanel.startValue, dcPanel.stopValue, dcPanel.stepValue, dcPanel.pointsValue, dcPanel.listValuesText, dcPanel.dataTableName, dcPanel.secondaryEnabled, dcPanel.secondaryVariable, dcPanel.secondaryStart, dcPanel.secondaryStop, dcPanel.secondaryStep, dcPanel.secondaryPoints, dcPanel.measureParametersText, dcPanel.printEnabled, dcPanel.printAllNodes, dcPanel.printAllCurrents, dcPanel.printPower, dcPanel.printBjtLeads, dcPanel.printFetLeads, dcPanel.printSpecificVars, dcPanel.printFormatValue, dcPanel.printFile, root.replaceGround)
                    } else if (simTabBar.currentIndex === 3) {
                        root.submitAC(acPanel.sweepModeValue, acPanel.points, acPanel.start, acPanel.end, acPanel.dataTableName, acPanel.measureParametersText, acPanel.printEnabled, acPanel.printAllNodes, acPanel.printAllCurrents, acPanel.printSpecificVars, acPanel.printFormatValue, acPanel.printFile, root.replaceGround)
                    } else if (simTabBar.currentIndex === 4) {
                        root.submitSens(sensPanel.objectiveMode, sensPanel.objectiveValues, sensPanel.parameters, sensPanel.direct, sensPanel.adjoint, root.replaceGround, sensPanel.printEnabled, sensPanel.printSpecificVars, sensPanel.printFormatValue, sensPanel.printFile)
                    } else if (simTabBar.currentIndex === 5) {
                        root.submitNoise(noisePanel.outputNode, noisePanel.refNode, noisePanel.sourceName, noisePanel.sweepModeValue, noisePanel.points, noisePanel.start, noisePanel.end, noisePanel.dataTableName, noisePanel.measureParametersText, noisePanel.printEnabled, noisePanel.printAllNodes, noisePanel.printAllCurrents, noisePanel.printInoise, noisePanel.printOnoise, noisePanel.printSpecificVars, noisePanel.printFormatValue, noisePanel.printFile, root.replaceGround, noisePanel.deviceOperators)
                    } else if (simTabBar.currentIndex === 6) {
                        root.submitHB(hbPanel.frequenciesText, hbPanel.harmonicsText, hbPanel.tahbIndex, hbPanel.selectHarmsValue, parseInt(hbPanel.startupPeriodsText) || 0, hbPanel.printEnabled, hbPanel.printAllNodes, hbPanel.printAllCurrents, hbPanel.printTypeValue, hbPanel.printSpecificVars, hbPanel.printFormatValue, hbPanel.printFile, root.replaceGround)
                    } else if (simTabBar.currentIndex === 7) {
                        root.submitLIN(linPanel.sparcalc, linPanel.linFormat, linPanel.linType, linPanel.linDataFormat, linPanel.linFile, linPanel.linWidth, linPanel.linPrecision, linPanel.sweepModeValue, linPanel.points, linPanel.start, linPanel.end, linPanel.dataTableName, linPanel.printEnabled, linPanel.printAllNodes, linPanel.printAllCurrents, linPanel.printSpecificVars, linPanel.printFormatValue, linPanel.printFile, root.replaceGround)
                    }
                }
            }
        }
    }
}
