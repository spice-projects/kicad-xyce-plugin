pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {

    id: panel
    spacing: 24

    property alias initialStep: initialStepField.text
    property alias finalTime: finalTimeField.text
    property alias startTime: startTimeField.text
    property alias stepCeiling: stepCeilingField.text
    property alias opModeIndex: opModeComboBox.currentIndex
    property alias scheduleEnabled: scheduleEnabledCheckBox.checked
    property alias schedulePairsText: schedulePairsTextArea.text
    property alias fftParametersText: fftParametersTextArea.text
    property alias fourParametersText: fourParametersTextArea.text
    property alias measureParametersText: measureParametersTextArea.text
    property bool printEnabled: false
    property bool printAllNodes: false
    property bool printAllCurrents: false
    property bool printPower: false
    property bool printBjtLeads: false
    property bool printFetLeads: false
    property bool hasBjtDevices: false
    property bool hasFetDevices: false
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
    readonly property string opKeywordValue: opModeComboBox.currentIndex === 1 ? "NOOP" : opModeComboBox.currentIndex === 2 ? "UIC" : ""
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""
    readonly property string printTypeValue: printTypeCombo.model[printTypeCombo.currentIndex]

    // --- .TRAN section ---
    SimulationCard {
        title: "Transient Parameters (.TRAN)"
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

                Label { text: "Initial step *"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: initialStepField
                    placeholderText: "e.g. 1u"
                    Layout.fillWidth: true
                    selectByMouse: true
                }

                Label { text: "Final time *"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: finalTimeField
                    placeholderText: "e.g. 10m"
                    Layout.fillWidth: true
                    selectByMouse: true
                }

                Label { text: "Start time"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: startTimeField
                    placeholderText: "e.g. 0"
                    Layout.fillWidth: true
                    selectByMouse: true
                }

                Label { text: "Step ceiling"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: stepCeilingField
                    placeholderText: "optional"
                    Layout.fillWidth: true
                    selectByMouse: true
                }

                Label { text: "Operating point"; color: "#6B6B66"; font.pixelSize: 12 }
                ComboBox {
                    id: opModeComboBox
                    Layout.fillWidth: true
                    model: ["Default (compute OP)", "NOOP (skip OP)", "UIC (skip OP)"]
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8
                CheckBox {
                    id: scheduleEnabledCheckBox
                    text: "Enable schedule(time, max_step, ...)"
                    font.weight: Font.Medium
                }

                TextArea {
                    id: schedulePairsTextArea
                    enabled: scheduleEnabledCheckBox.checked
                    placeholderText: "Enter pairs as time,max_step values.\nExample: 0.5e-3,0 1e-3,1e-6 2e-3,0"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    font.family: "monospace"
                    font.pixelSize: 11
                    selectByMouse: true
                }
            }
        }
    }

    // --- .PRINT TRAN section ---
    SimulationCard {
        title: "Output Variables (.PRINT TRAN)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                id: tranPrintEnabledCheckBox
                text: "Enable .PRINT output"
                checked: panel.printEnabled
                onCheckedChanged: panel.printEnabled = checked
                font.weight: Font.Medium
            }

            ColumnLayout {
                enabled: tranPrintEnabledCheckBox.checked
                Layout.fillWidth: true
                Layout.leftMargin: 28
                spacing: 12

                RowLayout {
                    spacing: 12
                    Label { text: "Print type"; color: "#6B6B66"; font.pixelSize: 12 }
                    ComboBox {
                        id: printTypeCombo
                        Layout.preferredWidth: 150
                        model: ["TRAN", "TRANADJOINT"]
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
                        id: tranPrintSpecificVarsField
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
                        id: tranPrintFileField
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

    // --- FFT / FOUR section ---
    SimulationCard {
        title: "Spectral Analysis (.FFT / .FOUR)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                Label { text: "Fast Fourier Transform directives (.FFT)"; font.pixelSize: 12; color: "#6B6B66" }
                TextArea {
                    id: fftParametersTextArea
                    placeholderText: "Example: .FFT V(OUT) WINDOW=HANN"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    font.family: "monospace"
                    font.pixelSize: 11
                    selectByMouse: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                Label { text: "Fourier Analysis directives (.FOUR)"; font.pixelSize: 12; color: "#6B6B66" }
                TextArea {
                    id: fourParametersTextArea
                    placeholderText: "Example: .FOUR 1k V(OUT)"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    font.family: "monospace"
                    font.pixelSize: 11
                    selectByMouse: true
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
            Label { text: "Transient analysis measurements"; font.pixelSize: 12; color: "#6B6B66" }
            TextArea {
                id: measureParametersTextArea
                placeholderText: "Example: .MEASURE TRAN RISE_TIME MAX V(OUT) RISE=1"
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
