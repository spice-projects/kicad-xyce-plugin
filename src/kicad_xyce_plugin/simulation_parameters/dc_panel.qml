pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {

    id: panel
    spacing: 24

    property alias sweepModeIndex: sweepModeComboBox.currentIndex
    property alias primaryVariable: primaryVariableField.text
    property alias startValue: startField.text
    property alias stopValue: stopField.text
    property alias stepValue: stepField.text
    property alias pointsValue: pointsField.text
    property alias listValuesText: listValuesTextArea.text
    property alias dataTableName: dataTableNameField.text
    property alias secondaryEnabled: secondaryEnabledCheckBox.checked
    property alias secondaryVariable: secondaryVariableField.text
    property alias secondaryStart: secondaryStartField.text
    property alias secondaryStop: secondaryStopField.text
    property alias secondaryStep: secondaryStepField.text
    property alias secondaryPoints: secondaryPointsField.text
    property alias measureParametersText: dcMeasureParametersTextArea.text
    property bool printEnabled: false
    property bool printAllNodes: false
    property bool printAllCurrents: false
    property bool printPower: false
    property bool printBjtLeads: false
    property bool printFetLeads: false
    property bool hasBjtDevices: false
    property bool hasFetDevices: false
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
    property bool replaceGround: true
    property alias printFormatIndex: printFormatCombo.currentIndex
    property alias printTypeIndex: printTypeCombo.currentIndex
    property string printFile: ""
    readonly property string sweepModeValue: (["LIN", "DEC", "OCT", "LIST", "DATA"])[sweepModeComboBox.currentIndex] || "LIN"
    readonly property bool isLogMode: sweepModeComboBox.currentIndex === 1 || sweepModeComboBox.currentIndex === 2
    readonly property bool isLinMode: sweepModeComboBox.currentIndex === 0
    readonly property bool isRangeMode: sweepModeComboBox.currentIndex <= 2
    readonly property bool isListMode: sweepModeComboBox.currentIndex === 3
    readonly property bool isDataMode: sweepModeComboBox.currentIndex === 4
    readonly property bool supportsSecondary: sweepModeComboBox.currentIndex <= 2
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""
    readonly property string printTypeValue: printTypeCombo.model[printTypeCombo.currentIndex]

    // --- .DC section ---
    SimulationCard {

        title: "DC Sweep Parameters (.DC)"
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
                    id: sweepModeComboBox
                    Layout.fillWidth: true
                    model: ["LIN (linear)", "DEC (per decade)", "OCT (per octave)", "LIST (explicit)", "DATA (table-driven)"]
                }

                Label {
                    text: "Primary Variable *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: !panel.isDataMode
                }
                TextField {
                    id: primaryVariableField
                    placeholderText: "e.g. VIN, R1, TEMP"
                    Layout.fillWidth: true
                    visible: !panel.isDataMode
                    selectByMouse: true
                }

                Label {
                    text: "Start *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: panel.isRangeMode
                }
                TextField {
                    id: startField
                    placeholderText: "e.g. 0"
                    Layout.fillWidth: true
                    visible: panel.isRangeMode
                    selectByMouse: true
                }

                Label {
                    text: "Stop *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: panel.isRangeMode
                }
                TextField {
                    id: stopField
                    placeholderText: "e.g. 5"
                    Layout.fillWidth: true
                    visible: panel.isRangeMode
                    selectByMouse: true
                }

                Label {
                    text: "Step *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: panel.isLinMode
                }
                TextField {
                    id: stepField
                    placeholderText: "e.g. 0.1"
                    Layout.fillWidth: true
                    visible: panel.isLinMode
                    selectByMouse: true
                }

                Label {
                    text: "Points *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: panel.isLogMode
                }
                TextField {
                    id: pointsField
                    placeholderText: "e.g. 10"
                    Layout.fillWidth: true
                    visible: panel.isLogMode
                    selectByMouse: true
                }

                Label {
                    text: "List Values *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: panel.isListMode
                }
                TextArea {
                    id: listValuesTextArea
                    placeholderText: "Enter values separated by spaces or commas.\nExample: 10 15 18 27 33"
                    wrapMode: TextEdit.Wrap
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    font.family: "monospace"
                    font.pixelSize: 11
                    visible: panel.isListMode
                    selectByMouse: true
                }

                Label {
                    text: "Data Table Name *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: panel.isDataMode
                }
                TextField {
                    id: dataTableNameField
                    placeholderText: "e.g. resistorValues"
                    Layout.fillWidth: true
                    visible: panel.isDataMode
                    selectByMouse: true
                }
            }

            // secondary sweep
            ColumnLayout {
                visible: panel.supportsSecondary
                Layout.fillWidth: true
                spacing: 12

                CheckBox {
                    id: secondaryEnabledCheckBox
                    text: "Enable secondary (nested) sweep"
                    font.weight: Font.Medium
                }

                GridLayout {
                    visible: secondaryEnabledCheckBox.checked
                    Layout.fillWidth: true
                    Layout.leftMargin: 28
                    columns: 2
                    rowSpacing: 12
                    columnSpacing: 20

                    Label { text: "Secondary Variable *"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: secondaryVariableField
                        placeholderText: "e.g. C1"
                        Layout.fillWidth: true
                        selectByMouse: true
                    }

                    Label { text: "Secondary Start *"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: secondaryStartField
                        placeholderText: "e.g. 0"
                        Layout.fillWidth: true
                        selectByMouse: true
                    }

                    Label { text: "Secondary Stop *"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: secondaryStopField
                        placeholderText: "e.g. 3.5"
                        Layout.fillWidth: true
                        selectByMouse: true
                    }

                    Label {
                        text: "Secondary Step *"
                        color: "#6B6B66"; font.pixelSize: 12
                        visible: panel.isLinMode
                    }
                    TextField {
                        id: secondaryStepField
                        placeholderText: "e.g. 0.5"
                        Layout.fillWidth: true
                        visible: panel.isLinMode
                        selectByMouse: true
                    }

                    Label {
                        text: "Secondary Points *"
                        color: "#6B6B66"; font.pixelSize: 12
                        visible: panel.isLogMode
                    }
                    TextField {
                        id: secondaryPointsField
                        placeholderText: "e.g. 5"
                        Layout.fillWidth: true
                        visible: panel.isLogMode
                        selectByMouse: true
                    }
                }
            }
        }
    }

    // --- .PRINT DC section ---
    SimulationCard {
        title: "Output Variables (.PRINT DC)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                id: dcPrintEnabledCheckBox
                text: "Enable .PRINT output"
                checked: panel.printEnabled
                onCheckedChanged: panel.printEnabled = checked
                font.weight: Font.Medium
            }

            ColumnLayout {
                enabled: dcPrintEnabledCheckBox.checked
                Layout.fillWidth: true
                Layout.leftMargin: 28
                spacing: 12

                RowLayout {
                    spacing: 12
                    Label { text: "Print type"; color: "#6B6B66"; font.pixelSize: 12 }
                    ComboBox {
                        id: printTypeCombo
                        Layout.preferredWidth: 150
                        model: ["DC", "HOMOTOPY"]
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
                    CheckBox {
                        text: "Power P(*)"
                        checked: panel.printPower
                        onCheckedChanged: panel.printPower = checked
                    }
                }

                RowLayout {
                    visible: panel.hasBjtDevices || panel.hasFetDevices
                    spacing: 20
                    CheckBox {
                        text: "BJT leads"
                        visible: panel.hasBjtDevices
                        checked: panel.printBjtLeads
                        onCheckedChanged: panel.printBjtLeads = checked
                    }
                    CheckBox {
                        text: "FET leads"
                        visible: panel.hasFetDevices
                        checked: panel.printFetLeads
                        onCheckedChanged: panel.printFetLeads = checked
                    }
                }

                GridLayout {
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 12
                    Layout.fillWidth: true

                    Label { text: "Additional variables"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: dcPrintSpecificVarsField
                        placeholderText: "e.g. V(1) I(R1)"
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
                        id: dcPrintFileField
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
            Label { text: "DC analysis measurements"; font.pixelSize: 12; color: "#6B6B66" }
            TextArea {
                id: dcMeasureParametersTextArea
                placeholderText: "Example: .MEASURE DC VIN_AT_2V FIND V(1) WHEN V(1)=2"
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
