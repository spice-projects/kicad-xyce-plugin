pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

Item {
    id: root
    implicitWidth: 800
    implicitHeight: 600

    // --- Properties from original dialog for compatibility ---
    property alias initialTabIndex: root.currentTabIndex
    property int currentTabIndex: 0
    onCurrentTabIndexChanged: root.errorText = ""

    // --- Transient tab properties ---
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

    // --- DC Sweep tab properties ---
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

    // --- OP simulation properties ---
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

    // --- Transient print properties ---
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

    // --- DC Sweep print properties ---
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

    // --- Sensitivity properties ---
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

    // --- AC sweep properties ---
    property alias acSweepModeIndex: acPanel.sweepModeIndex
    property alias acPoints: acPanel.points
    property alias acStart: acPanel.start
    property alias acEnd: acPanel.end
    property alias acDataTableName: acPanel.dataTableName
    property alias acMeasureParametersText: acPanel.measureParametersText

    // --- AC print properties ---
    property alias acPrintEnabled: acPanel.printEnabled
    property alias acPrintAllNodes: acPanel.printAllNodes
    property alias acPrintAllCurrents: acPanel.printAllCurrents
    property alias acPrintSpecificVars: acPanel.printSpecificVars
    property alias acPrintFormatIndex: acPanel.printFormatIndex
    property alias acPrintFile: acPanel.printFile

    // --- NOISE sweep properties ---
    property alias noiseOutputNode: noisePanel.outputNode
    property alias noiseRefNode: noisePanel.refNode
    property alias noiseSourceName: noisePanel.sourceName
    property alias noiseSweepModeIndex: noisePanel.sweepModeIndex
    property alias noisePoints: noisePanel.points
    property alias noiseStart: noisePanel.start
    property alias noiseEnd: noisePanel.end
    property alias noiseDataTableName: noisePanel.dataTableName
    property alias noiseMeasureParametersText: noisePanel.measureParametersText

    // --- NOISE print properties ---
    property alias noisePrintEnabled: noisePanel.printEnabled
    property alias noisePrintAllNodes: noisePanel.printAllNodes
    property alias noisePrintAllCurrents: noisePanel.printAllCurrents
    property alias noisePrintInoise: noisePanel.printInoise
    property alias noisePrintOnoise: noisePanel.printOnoise
    property alias noisePrintSpecificVars: noisePanel.printSpecificVars
    property alias noisePrintFormatIndex: noisePanel.printFormatIndex
    property alias noisePrintFile: noisePanel.printFile
    property alias noiseDeviceOperators: noisePanel.deviceOperators

    // --- HB properties ---
    property alias hbFrequenciesText: hbPanel.frequenciesText
    property alias hbHarmonicsText: hbPanel.harmonicsText
    property alias hbTahbIndex: hbPanel.tahbIndex
    property alias hbSelectHarmsIndex: hbPanel.selectHarmsIndex
    property alias hbStartupPeriodsText: hbPanel.startupPeriodsText

    // --- HB print properties ---
    property alias hbPrintEnabled: hbPanel.printEnabled
    property alias hbPrintAllNodes: hbPanel.printAllNodes
    property alias hbPrintAllCurrents: hbPanel.printAllCurrents
    property alias hbPrintTypeIndex: hbPanel.printTypeIndex
    property alias hbPrintSpecificVars: hbPanel.printSpecificVars
    property alias hbPrintFormatIndex: hbPanel.printFormatIndex
    property alias hbPrintFile: hbPanel.printFile

    // --- LIN properties ---
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

    // --- LIN print properties ---
    property alias linPrintEnabled: linPanel.printEnabled
    property alias linPrintAllNodes: linPanel.printAllNodes
    property alias linPrintAllCurrents: linPanel.printAllCurrents
    property alias linPrintSpecificVars: linPanel.printSpecificVars
    property alias linPrintFormatIndex: linPanel.printFormatIndex
    property alias linPrintFile: linPanel.printFile

    // --- Shared properties ---
    property bool replaceGround: false
    property string errorText: ""

    // --- Signals ---
    signal submitTransient(string initialStep, string finalTime, string startTime, string stepCeiling, string opKeyword, bool scheduleEnabled, string schedulePairsText, string fftParametersText, string fourParametersText, string measureParametersText, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitDC(string sweepMode, string primaryVariable, string startValue, string stopValue, string stepValue, string pointsValue, string listValuesText, string dataTableName, bool secondaryEnabled, string secondaryVariable, string secondaryStart, string secondaryStop, string secondaryStep, string secondaryPoints, string measureParametersText, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitOP(bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool saveEnabled, string saveType, string nodesetEntries, string saveFile, bool replaceGround)
    signal submitAC(string sweepMode, string points, string start, string end, string dataTableName, string measureParametersText, bool printEnabled, bool printAllNodes, bool printAllCurrents, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitNoise(string outputNode, string refNode, string sourceName, string sweepMode, string points, string start, string end, string dataTableName, string measureParametersText, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printInoise, bool printOnoise, string printSpecificVars, string printFormat, string printFile, bool replaceGround, var deviceOperatorsList)
    signal submitHB(string frequenciesText, string harmonicsText, int tahb, string selectharms, int startupPeriods, bool printEnabled, bool printAllNodes, bool printAllCurrents, string printType, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitLIN(bool sparcalc, string format, string lintype, string dataformat, string file, string width, string precision, string sweepMode, string points, string start, string end, string dataTableName, bool printEnabled, bool printAllNodes, bool printAllCurrents, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitSens(string objectiveMode, string objectiveValues, string parameters, bool direct, bool adjoint, bool replaceGround, bool printEnabled, string printSpecificVars, string printFormat, string printFile)
    signal cancelRequested()

    // --- Internal Logic ---
    function handleApply() {
        if (root.currentTabIndex === 0) {
            root.submitOP(opPanel.printEnabled, opPanel.printAllNodes, opPanel.printAllCurrents, opPanel.printPower, opPanel.printBjtLeads, opPanel.printFetLeads, opPanel.printSpecificVars, opPanel.printFormatValue, opPanel.printFile, opPanel.saveEnabled, opPanel.saveType, opPanel.nodesetEntries, opPanel.saveFile, root.replaceGround)
        } else if (root.currentTabIndex === 1) {
            root.submitTransient(tranPanel.initialStep, tranPanel.finalTime, tranPanel.startTime, tranPanel.stepCeiling, tranPanel.opKeywordValue, tranPanel.scheduleEnabled, tranPanel.schedulePairsText, tranPanel.fftParametersText, tranPanel.fourParametersText, tranPanel.measureParametersText, tranPanel.printEnabled, tranPanel.printAllNodes, tranPanel.printAllCurrents, tranPanel.printPower, tranPanel.printBjtLeads, tranPanel.printFetLeads, tranPanel.printSpecificVars, tranPanel.printFormatValue, tranPanel.printFile, root.replaceGround)
        } else if (root.currentTabIndex === 2) {
            root.submitDC(dcPanel.sweepModeValue, dcPanel.primaryVariable, dcPanel.startValue, dcPanel.stopValue, dcPanel.stepValue, dcPanel.pointsValue, dcPanel.listValuesText, dcPanel.dataTableName, dcPanel.secondaryEnabled, dcPanel.secondaryVariable, dcPanel.secondaryStart, dcPanel.secondaryStop, dcPanel.secondaryStep, dcPanel.secondaryPoints, dcPanel.measureParametersText, dcPanel.printEnabled, dcPanel.printAllNodes, dcPanel.printAllCurrents, dcPanel.printPower, dcPanel.printBjtLeads, dcPanel.printFetLeads, dcPanel.printSpecificVars, dcPanel.printFormatValue, dcPanel.printFile, root.replaceGround)
        } else if (root.currentTabIndex === 3) {
            root.submitAC(acPanel.sweepModeValue, acPanel.points, acPanel.start, acPanel.end, acPanel.dataTableName, acPanel.measureParametersText, acPanel.printEnabled, acPanel.printAllNodes, acPanel.printAllCurrents, acPanel.printSpecificVars, acPanel.printFormatValue, acPanel.printFile, root.replaceGround)
        } else if (root.currentTabIndex === 4) {
            root.submitSens(sensPanel.objectiveMode, sensPanel.objectiveValues, sensPanel.parameters, sensPanel.direct, sensPanel.adjoint, root.replaceGround, sensPanel.printEnabled, sensPanel.printSpecificVars, sensPanel.printFormatValue, sensPanel.printFile)
        } else if (root.currentTabIndex === 5) {
            root.submitNoise(noisePanel.outputNode, noisePanel.refNode, noisePanel.sourceName, noisePanel.sweepModeValue, noisePanel.points, noisePanel.start, noisePanel.end, noisePanel.dataTableName, noisePanel.measureParametersText, noisePanel.printEnabled, noisePanel.printAllNodes, noisePanel.printAllCurrents, noisePanel.printInoise, noisePanel.printOnoise, noisePanel.printSpecificVars, noisePanel.printFormatValue, noisePanel.printFile, root.replaceGround, noisePanel.deviceOperators)
        } else if (root.currentTabIndex === 6) {
            root.submitHB(hbPanel.frequenciesText, hbPanel.harmonicsText, hbPanel.tahbIndex, hbPanel.selectHarmsValue, parseInt(hbPanel.startupPeriodsText) || 0, hbPanel.printEnabled, hbPanel.printAllNodes, hbPanel.printAllCurrents, hbPanel.printTypeValue, hbPanel.printSpecificVars, hbPanel.printFormatValue, hbPanel.printFile, root.replaceGround)
        } else if (root.currentTabIndex === 7) {
            root.submitLIN(linPanel.sparcalc, linPanel.linFormat, linPanel.linType, linPanel.linDataFormat, linPanel.linFile, linPanel.linWidth, linPanel.linPrecision, linPanel.sweepModeValue, linPanel.points, linPanel.start, linPanel.end, linPanel.dataTableName, linPanel.printEnabled, linPanel.printAllNodes, linPanel.printAllCurrents, linPanel.printSpecificVars, linPanel.printFormatValue, linPanel.printFile, root.replaceGround)
        }
    }

    // --- Styling Constants ---
    readonly property color bgColor: "#FFFFFF"
    readonly property color sidebarColor: "#F5F5F3"
    readonly property color borderColor: "#E0E0DC"
    readonly property color textColor: "#1A1A18"
    readonly property color mutedTextColor: "#6B6B66"
    readonly property color accentBlue: "#185FA5"

    Rectangle {
        anchors.fill: parent
        color: root.bgColor
        border.color: root.borderColor
        border.width: 1
        radius: 12
        clip: true

        RowLayout {
            anchors.fill: parent
            anchors.bottomMargin: 48 // Footer height
            spacing: 0

            // --- Sidebar ---
            Rectangle {
                id: sidebar
                width: 220
                Layout.fillHeight: true
                color: root.sidebarColor
                
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: 1
                    color: root.borderColor
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    Label {
                        text: "SIMULATION TYPE"
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 1.0
                        color: root.mutedTextColor
                        Layout.fillWidth: true
                        topPadding: 16
                        leftPadding: 12
                        bottomPadding: 8
                    }

                    Repeater {
                        model: [
                            {directive: ".OP", name: "Operating point"},
                            {directive: ".TRAN", name: "Transient"},
                            {directive: ".DC", name: "DC sweep"},
                            {directive: ".AC", name: "AC small-signal"},
                            {directive: ".SENS", name: "Sensitivity"},
                            {directive: ".NOISE", name: "Noise"},
                            {directive: ".HB", name: "Harmonic balance"},
                            {directive: ".LIN", name: "Linear network"}
                        ]
                        delegate: Rectangle {
                            id: sidebarButton
                            Layout.fillWidth: true
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8
                            height: 34
                            radius: 6

                            required property int index
                            required property var modelData

                            color: (root.currentTabIndex === index) ? root.bgColor : (mouseArea.containsMouse ? "#EBEAE6" : "transparent")
                            border.color: (root.currentTabIndex === index) ? root.borderColor : "transparent"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                spacing: 10

                                Rectangle {
                                    id: badgeRect
                                    width: 38
                                    height: 18
                                    radius: 4
                                    color: (root.currentTabIndex === index) ? root.accentBlue : "#E6F1FB"
                                    
                                    Text {
                                        id: badgeText
                                        anchors.centerIn: parent
                                        text: modelData.directive
                                        font.family: "monospace"
                                        font.pixelSize: 10
                                        font.weight: Font.Bold
                                        color: (root.currentTabIndex === index) ? root.bgColor : root.accentBlue
                                    }
                                }
                                
                                Label {
                                    id: labelText
                                    text: modelData.name
                                    font.pixelSize: 13
                                    verticalAlignment: Text.AlignVCenter
                                    color: (root.currentTabIndex === index) ? root.textColor : root.mutedTextColor
                                    font.weight: (root.currentTabIndex === index) ? Font.Medium : Font.Normal
                                    Layout.fillWidth: true
                                }
                            }
                            
                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: root.currentTabIndex = index
                            }
                        }
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }

            // --- Main Content Area ---
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: root.bgColor
                
                ScrollView {
                    anchors.fill: parent
                    contentWidth: availableWidth
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    clip: true
                    
                    ColumnLayout {
                        width: parent.width
                        spacing: 0
                        
                        StackLayout {
                            Layout.fillWidth: true
                            Layout.margins: 24
                            currentIndex: root.currentTabIndex

                            OpPanel { id: opPanel }
                            TranPanel { id: tranPanel }
                            DcPanel { id: dcPanel }
                            AcPanel { id: acPanel }
                            SensPanel { id: sensPanel }
                            NoisePanel { id: noisePanel }
                            HbPanel { id: hbPanel }
                            LinPanel { id: linPanel }
                        }
                        
                        // Spacer to ensure bottom margin if content is short
                        Item { Layout.fillHeight: true; Layout.preferredHeight: 24 }
                    }
                }
            }
        }
        
        // --- Footer ---
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 60
            color: root.sidebarColor
            
            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: root.borderColor
            }
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 20
                
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.topMargin: 12
                    Layout.bottomMargin: 12
                    color: "#EBEAE6"
                    radius: 6
                    border.color: root.borderColor
                    border.width: 1
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        
                        Label {
                            text: "DIRECTIVE"
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            color: root.mutedTextColor
                        }
                        
                        Label {
                            text: (root.currentTabIndex === 0 ? ".OP" : 
                                   root.currentTabIndex === 1 ? ".TRAN " + root.initialStep + " " + root.finalTime :
                                   root.currentTabIndex === 2 ? ".DC " + root.primaryVariable + " " + root.startValue + " " + root.stopValue :
                                   root.currentTabIndex === 3 ? ".AC " + root.acStart + " " + root.acEnd :
                                   root.currentTabIndex === 4 ? ".SENS" :
                                   root.currentTabIndex === 5 ? ".NOISE" :
                                   root.currentTabIndex === 6 ? ".HB" :
                                   root.currentTabIndex === 7 ? ".LIN" : "")
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: root.textColor
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }
                }
                
                RowLayout {
                    spacing: 12
                    Button { 
                        text: "Cancel"
                        implicitWidth: 100
                        onClicked: root.cancelRequested()
                    }
                    Button { 
                        text: "Apply"
                        highlighted: true
                        implicitWidth: 100
                        onClicked: root.handleApply()
                    }
                }
            }
        }
    }
}
