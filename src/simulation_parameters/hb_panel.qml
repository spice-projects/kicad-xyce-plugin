pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {

    id: panel
    spacing: 24

    property alias frequenciesText: hbFrequenciesField.text
    property alias harmonicsText: hbHarmonicsField.text
    property alias tahbIndex: hbTahbCombo.currentIndex
    property alias selectHarmsIndex: hbSelectHarmsCombo.currentIndex
    property alias startupPeriodsText: hbStartupPeriodsField.text
    property alias nonlinOptionsText: hbNonlinOptionsField.text
    property alias linsolOptionsText: hbLinsolOptionsField.text
    property bool printEnabled: false
    property bool printAllNodes: false
    property bool printAllCurrents: false
    property bool replaceGround: true
    property alias printTypeIndex: hbPrintTypeComboBox.currentIndex
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    readonly property string printTypeValue: (["HB", "HB_FD", "HB_TD"])[hbPrintTypeComboBox.currentIndex] || "HB"
    readonly property string selectHarmsValue: hbSelectHarmsCombo.model[hbSelectHarmsCombo.currentIndex].toLowerCase()
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    // --- .HB section ---
    SimulationCard {
        title: "Harmonic Balance Analysis Parameters (.HB)"
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

                Label { text: "Fundamental Frequencies *"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: hbFrequenciesField
                    placeholderText: "e.g. 1MEG 2MEG 500K"
                    Layout.fillWidth: true
                }

                Label { text: "Harmonics (NUMFREQ)"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: hbHarmonicsField
                    placeholderText: "e.g. 10 10 (defaults to 10 if empty)"
                    Layout.fillWidth: true
                }

                Label { text: "Transient Assistance (TAHB)"; color: "#6B6B66"; font.pixelSize: 12 }
                ComboBox {
                    id: hbTahbCombo
                    Layout.fillWidth: true
                    model: ["Off (0)", "Transient (1)", "DC (2)"]
                }

                Label { text: "Truncation (SELECTHARMS)"; color: "#6B6B66"; font.pixelSize: 12 }
                ComboBox {
                    id: hbSelectHarmsCombo
                    Layout.fillWidth: true
                    model: ["Hybrid", "Box", "Diamond"]
                }

                Label { text: "Startup Periods"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: hbStartupPeriodsField
                    placeholderText: "e.g. 0"
                    Layout.fillWidth: true
                    validator: IntValidator { bottom: 0 }
                    selectByMouse: true
                }

                Label { text: "Nonlinear solver options (.OPTIONS NONLIN-HB)"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: hbNonlinOptionsField
                    placeholderText: "e.g. ABSTOL=1e-9 MAXIT=50"
                    Layout.fillWidth: true
                }

                Label { text: "Linear solver options (.OPTIONS LINSOL-HB)"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: hbLinsolOptionsField
                    placeholderText: "e.g. TYPE=AZTECOO NPRE=1"
                    Layout.fillWidth: true
                }
            }
        }
    }

    // --- .PRINT HB section ---
    SimulationCard {
        title: "Output Variables (.PRINT HB)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                id: hbPrintEnabledCheckBox
                text: "Enable .PRINT HB output"
                checked: panel.printEnabled
                onCheckedChanged: panel.printEnabled = checked
                font.weight: Font.Medium
            }

            ColumnLayout {
                enabled: hbPrintEnabledCheckBox.checked
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
                        id: hbPrintSpecificVarsField
                        placeholderText: "e.g. VR(out) VM(1) IP(V1)"
                        text: panel.printSpecificVars
                        onTextChanged: panel.printSpecificVars = text
                        Layout.fillWidth: true
                    }

                    Label { text: "Print Type"; color: "#6B6B66"; font.pixelSize: 12 }
                    ComboBox {
                        id: hbPrintTypeComboBox
                        Layout.fillWidth: true
                        model: ["HB", "HB_FD", "HB_TD"]
                    }

                    Label { text: "Format"; color: "#6B6B66"; font.pixelSize: 12 }
                    ComboBox {
                        id: printFormatCombo
                        Layout.fillWidth: true
                        model: ["(default)", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]
                    }

                    Label { text: "Output file"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: hbPrintFileField
                        placeholderText: "optional (e.g. output.raw)"
                        text: panel.printFile
                        onTextChanged: panel.printFile = text
                        Layout.fillWidth: true
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
