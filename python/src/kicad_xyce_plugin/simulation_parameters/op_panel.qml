pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {

    id: panel
    spacing: 24

    property bool printEnabled: false
    property bool printAllNodes: false
    property bool printAllCurrents: false
    property bool printPower: false
    property bool printBjtLeads: false
    property bool printFetLeads: false
    property bool hasBjtDevices: false
    property bool hasFetDevices: false
    property bool replaceGround: true
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    property bool saveEnabled: false
    property string saveType: "NODESET"
    property string saveFile: ""
    property string nodesetEntries: ""
    property string initialConditionEntries: ""

    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    // --- .PRINT section ---
    SimulationCard {
        title: "Output Variables (.PRINT)"
        badge: "REQUIRED FOR PLOTTING"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                id: opPrintEnabledCheckBox
                text: "Enable .PRINT output"
                checked: panel.printEnabled
                onCheckedChanged: panel.printEnabled = checked
                font.weight: Font.Medium
            }

            ColumnLayout {
                enabled: opPrintEnabledCheckBox.checked
                Layout.fillWidth: true
                Layout.leftMargin: 28 // Indent relative to main checkbox
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
                        id: opPrintSpecificVarsField
                        placeholderText: "e.g. V(1) I(R1)"
                        text: panel.printSpecificVars
                        onTextChanged: panel.printSpecificVars = text
                        Layout.fillWidth: true
                    }

                    Label { text: "Format"; color: "#6B6B66"; font.pixelSize: 12 }
                    ComboBox {
                        id: printFormatCombo
                        Layout.fillWidth: true
                        model: ["(default)", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]
                    }

                    Label { text: "Output file"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: opPrintFileField
                        placeholderText: "optional (e.g. output.raw)"
                        text: panel.printFile
                        onTextChanged: panel.printFile = text
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    // --- .SAVE section ---
    SimulationCard {
        title: "Save Results (.SAVE)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                id: saveEnabledCheckBox
                text: "Enable .SAVE operating point"
                checked: panel.saveEnabled
                onCheckedChanged: panel.saveEnabled = checked
                font.weight: Font.Medium
            }

            ColumnLayout {
                enabled: saveEnabledCheckBox.checked
                Layout.fillWidth: true
                Layout.leftMargin: 28
                spacing: 12

                RowLayout {
                    spacing: 24
                    RadioButton {
                        text: "Save as .IC"
                        checked: panel.saveType === "IC"
                        onClicked: panel.saveType = "IC"
                    }
                    RadioButton {
                        text: "Save as .NODESET"
                        checked: panel.saveType === "NODESET"
                        onClicked: panel.saveType = "NODESET"
                    }
                }

                RowLayout {
                    spacing: 12
                    Label { text: "Save file"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: saveFileField
                        placeholderText: "optional output file path"
                        text: panel.saveFile
                        onTextChanged: panel.saveFile = text
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    // --- .NODESET section ---
    SimulationCard {
        title: "Convergence Hints (.NODESET)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "Initial voltage/current guesses to help DC convergence"
                font.pixelSize: 11
                color: "#6B6B66"
            }

            TextField {
                placeholderText: "e.g. V(1)=5.0 V(2)=3.3"
                text: panel.nodesetEntries
                onTextChanged: panel.nodesetEntries = text
                Layout.fillWidth: true
            }
        }
    }

    // --- .IC / .DCVOLT section ---
    SimulationCard {
        title: "Initial Conditions (.IC / .DCVOLT)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "Initial conditions for operating-point analysis"
                font.pixelSize: 11
                color: "#6B6B66"
            }

            TextField {
                placeholderText: "e.g. V(out)=1.0 V(in)=0"
                text: panel.initialConditionEntries
                onTextChanged: panel.initialConditionEntries = text
                Layout.fillWidth: true
            }
        }
    }

    // --- Global Settings ---
    SimulationCard {
        title: "Global Settings"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            CheckBox {
                text: "Replace ground (GND) with 0"
                checked: panel.replaceGround
                onCheckedChanged: panel.replaceGround = checked
                font.weight: Font.Medium
            }

            Label {
                text: "Matches KiCad's ground symbol to Xyce's expected 0 net"
                font.pixelSize: 11
                color: "#6B6B66"
                Layout.leftMargin: 28
            }
        }
    }
}
