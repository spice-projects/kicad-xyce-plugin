pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {

    id: panel
    spacing: 24

    property bool sparcalc: true
    property string linFormat: "TOUCHSTONE2"
    property string linType: "S"
    property string linDataFormat: "RI"
    property string linFile: ""
    property string linWidth: ""
    property string linPrecision: ""
    property alias sweepModeIndex: linSweepModeComboBox.currentIndex
    property alias points: linPointsField.text
    property alias start: linStartField.text
    property alias end: linEndField.text
    property alias dataTableName: linDataTableNameField.text
    property bool printEnabled: false
    property bool printAllNodes: false
    property bool printAllCurrents: false
    property bool replaceGround: true
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    readonly property string sweepModeValue: (["LIN", "DEC", "OCT", "DATA"])[linSweepModeComboBox.currentIndex] || "LIN"
    readonly property bool isDataMode: linSweepModeComboBox.currentIndex === 3
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    // --- .LIN section ---
    SimulationCard {
        title: "Linear Network Analysis Parameters (.LIN)"
        badge: "REQUIRED"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                id: linSparcalcCheckBox
                text: "Enable SPARCALC (linearize into S/Y/Z parameters)"
                checked: panel.sparcalc
                onCheckedChanged: panel.sparcalc = checked
                font.weight: Font.Medium
            }

            ColumnLayout {
                enabled: linSparcalcCheckBox.checked
                Layout.fillWidth: true
                Layout.leftMargin: 28
                spacing: 12

                GridLayout {
                    columns: 2
                    rowSpacing: 12
                    columnSpacing: 20
                    Layout.fillWidth: true

                    Label { text: "Output Format"; color: "#6B6B66"; font.pixelSize: 12 }
                    ComboBox {
                        id: linFormatComboBox
                        Layout.fillWidth: true
                        model: ["TOUCHSTONE2", "TOUCHSTONE"]
                        currentIndex: panel.linFormat === "TOUCHSTONE" ? 1 : 0
                        onCurrentIndexChanged: panel.linFormat = currentIndex === 1 ? "TOUCHSTONE" : "TOUCHSTONE2"
                    }

                    Label { text: "Parameter Type"; color: "#6B6B66"; font.pixelSize: 12 }
                    ComboBox {
                        id: linTypeComboBox
                        Layout.fillWidth: true
                        model: ["S", "Y", "Z"]
                        currentIndex: panel.linType === "Y" ? 1 : panel.linType === "Z" ? 2 : 0
                        onCurrentIndexChanged: panel.linType = currentIndex === 1 ? "Y" : currentIndex === 2 ? "Z" : "S"
                    }

                    Label { text: "Data Format"; color: "#6B6B66"; font.pixelSize: 12 }
                    ComboBox {
                        id: linDataFormatComboBox
                        Layout.fillWidth: true
                        model: ["RI", "MA", "DB"]
                        currentIndex: panel.linDataFormat === "MA" ? 1 : panel.linDataFormat === "DB" ? 2 : 0
                        onCurrentIndexChanged: panel.linDataFormat = currentIndex === 1 ? "MA" : currentIndex === 2 ? "DB" : "RI"
                    }

                    Label { text: "Output File"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: linFileField
                        placeholderText: "optional output file name"
                        text: panel.linFile
                        onTextChanged: panel.linFile = text
                        Layout.fillWidth: true
                    }

                    Label { text: "Width"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: linWidthField
                        placeholderText: "optional"
                        text: panel.linWidth
                        onTextChanged: panel.linWidth = text
                        Layout.fillWidth: true
                    }

                    Label { text: "Precision"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: linPrecisionField
                        placeholderText: "optional"
                        text: panel.linPrecision
                        onTextChanged: panel.linPrecision = text
                        Layout.fillWidth: true
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: 12
                columnSpacing: 20

                Label { text: "Sweep Mode *"; color: "#6B6B66"; font.pixelSize: 12 }
                ComboBox {
                    id: linSweepModeComboBox
                    Layout.fillWidth: true
                    model: ["LIN (linear)", "DEC (per decade)", "OCT (per octave)", "DATA (table-driven)"]
                }

                Label {
                    text: "Points *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: !panel.isDataMode
                }
                TextField {
                    id: linPointsField
                    placeholderText: "e.g. 100"
                    Layout.fillWidth: true
                    visible: !panel.isDataMode
                }

                Label {
                    text: "Start Frequency *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: !panel.isDataMode
                }
                TextField {
                    id: linStartField
                    placeholderText: "e.g. 1"
                    Layout.fillWidth: true
                    visible: !panel.isDataMode
                }

                Label {
                    text: "End Frequency *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: !panel.isDataMode
                }
                TextField {
                    id: linEndField
                    placeholderText: "e.g. 1MEG"
                    Layout.fillWidth: true
                    visible: !panel.isDataMode
                }

                Label {
                    text: "Data Table Name *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: panel.isDataMode
                }
                TextField {
                    id: linDataTableNameField
                    placeholderText: "e.g. freqTable"
                    Layout.fillWidth: true
                    visible: panel.isDataMode
                }
            }
        }
    }

    // --- .PRINT LIN section ---
    SimulationCard {
        title: "Output Variables (.PRINT LIN)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                id: linPrintEnabledCheckBox
                text: "Enable .PRINT LIN output"
                checked: panel.printEnabled
                onCheckedChanged: panel.printEnabled = checked
                font.weight: Font.Medium
            }

            ColumnLayout {
                enabled: linPrintEnabledCheckBox.checked
                Layout.fillWidth: true
                Layout.leftMargin: 28
                spacing: 12

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
                        id: linPrintSpecificVarsField
                        placeholderText: "e.g. SR(1,2) YP(2,1)"
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
                        id: linPrintFileField
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
