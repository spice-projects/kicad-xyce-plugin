pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {

    id: panel
    spacing: 24

    property alias sweepModeIndex: acSweepModeComboBox.currentIndex
    property alias points: acPointsField.text
    property alias start: acStartField.text
    property alias end: acEndField.text
    property alias dataTableName: acDataTableNameField.text
    property alias measureParametersText: acMeasureParametersTextArea.text
    property bool printEnabled: false
    property bool printAllNodes: false
    property bool printAllCurrents: false
    property bool replaceGround: true
    property alias sensEnabled: sensSection.active
    property alias sensObjectiveMode: sensSection.objectiveMode
    property alias sensObjectiveValues: sensSection.objectiveValues
    property alias sensParameters: sensSection.parameters
    property alias sensDirect: sensSection.direct
    property alias sensAdjoint: sensSection.adjoint
    property alias sensPrintEnabled: sensSection.printEnabled
    property alias sensPrintSpecificVars: sensSection.printSpecificVars
    property alias sensPrintFormatIndex: sensSection.printFormatIndex
    property alias sensPrintFormatValue: sensSection.printFormatValue
    property alias sensPrintFile: sensSection.printFile
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property alias printTypeIndex: printTypeCombo.currentIndex
    property string printFile: ""
    readonly property string sweepModeValue: (["LIN", "DEC", "OCT", "DATA"])[acSweepModeComboBox.currentIndex] || "LIN"
    readonly property bool isDataMode: acSweepModeComboBox.currentIndex === 3
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""
    readonly property string printTypeValue: printTypeCombo.model[printTypeCombo.currentIndex]

    // --- .AC section ---
    SimulationCard {
        title: "AC Analysis Parameters (.AC)"
        badge: "REQUIRED"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: 12
                columnSpacing: 20

                Label { text: "Sweep Mode *"; color: "#6B6B66"; font.pixelSize: 12 }
                ComboBox {
                    id: acSweepModeComboBox
                    Layout.fillWidth: true
                    model: ["LIN (linear)", "DEC (per decade)", "OCT (per octave)", "DATA (table-driven)"]
                }

                Label {
                    text: "Points *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: !panel.isDataMode
                }
                TextField {
                    id: acPointsField
                    placeholderText: "e.g. 100"
                    Layout.fillWidth: true
                    visible: !panel.isDataMode
                    selectByMouse: true
                }

                Label {
                    text: "Start Frequency *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: !panel.isDataMode
                }
                TextField {
                    id: acStartField
                    placeholderText: "e.g. 1"
                    Layout.fillWidth: true
                    visible: !panel.isDataMode
                    selectByMouse: true
                }

                Label {
                    text: "End Frequency *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: !panel.isDataMode
                }
                TextField {
                    id: acEndField
                    placeholderText: "e.g. 1MEG"
                    Layout.fillWidth: true
                    visible: !panel.isDataMode
                    selectByMouse: true
                }

                Label {
                    text: "Data Table Name *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: panel.isDataMode
                }
                TextField {
                    id: acDataTableNameField
                    placeholderText: "e.g. freqTable"
                    Layout.fillWidth: true
                    visible: panel.isDataMode
                    selectByMouse: true
                }
            }
        }
    }

    // --- .PRINT AC section ---
    SimulationCard {
        title: "Output Variables (.PRINT AC)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                id: acPrintEnabledCheckBox
                text: "Enable .PRINT output"
                checked: panel.printEnabled
                onCheckedChanged: panel.printEnabled = checked
                font.weight: Font.Medium
            }

            ColumnLayout {
                enabled: acPrintEnabledCheckBox.checked
                Layout.fillWidth: true
                Layout.leftMargin: 28
                spacing: 12

                RowLayout {
                    spacing: 12
                    Label { text: "Print type"; color: "#6B6B66"; font.pixelSize: 12 }
                    ComboBox {
                        id: printTypeCombo
                        Layout.preferredWidth: 150
                        model: ["AC", "AC_IC"]
                    }
                }

                RowLayout {
                    spacing: 20
                    CheckBox {
                        text: "All voltages V(*)"
                        checked: panel.printAllNodes
                        onCheckedChanged: panel.printAllNodes = checked
                    }
                    CheckBox {
                        text: "All currents I(*)"
                        checked: panel.printAllCurrents
                        onCheckedChanged: panel.printAllCurrents = checked
                    }
                }

                GridLayout {
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 12
                    Layout.fillWidth: true

                    Label { text: "Additional variables"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: acPrintSpecificVarsField
                        placeholderText: "e.g. VR(out) VM(1) IP(V1)"
                        text: panel.printSpecificVars
                        onTextChanged: panel.printSpecificVars = text
                        Layout.fillWidth: true
                        selectByMouse: true
                    }

                    Label { text: "Format"; color: "#6B6B66"; font.pixelSize: 12 }
                    ComboBox {
                        id: printFormatCombo
                        Layout.fillWidth: true
                        model: ["(default)", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]
                    }

                    Label { text: "Output file"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: acPrintFileField
                        placeholderText: "optional (e.g. output.raw)"
                        text: panel.printFile
                        onTextChanged: panel.printFile = text
                        Layout.fillWidth: true
                        selectByMouse: true
                    }
                }
            }
        }
    }

    // --- .MEASURE section ---
    SimulationCard {
        title: "Measurements (.MEASURE)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            Label { text: "AC analysis measurements"; font.pixelSize: 12; color: "#6B6B66" }
            TextArea {
                id: acMeasureParametersTextArea
                placeholderText: "Example: .MEASURE AC BANDWIDTH FIND V(OUT) WHEN V(OUT)=0.707"
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                font.family: "monospace"
                font.pixelSize: 11
                selectByMouse: true
            }
        }
    }

    SensitivitySection {
        id: sensSection
        replaceGround: panel.replaceGround
        Layout.fillWidth: true
    }

    // --- Global Settings ---
    SimulationCard {
        title: "Global Settings"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            CheckBox {
                text: "Replace ground (GND) with 0"
                checked: panel.replaceGround
                onCheckedChanged: panel.replaceGround = checked
                font.weight: Font.Medium
            }

            Label {
                text: "Matches KiCad's ground symbol to Xyce's expected 0 net"
                color: "#6B6B66"
                font.pixelSize: 11
                Layout.leftMargin: 28
            }
        }
    }
}
