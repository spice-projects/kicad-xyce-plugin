pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: panel

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
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    readonly property string opKeywordValue: opModeComboBox.currentIndex === 1 ? "NOOP" : opModeComboBox.currentIndex === 2 ? "UIC" : ""
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 12

            // --- .TRAN section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: tranParamsColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: tranParamsColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 4
                        rowSpacing: 10
                        columnSpacing: 12

                        Label {
                            text: "Initial Step *"
                            color: "#24292f"
                        }
                        TextField {
                            id: initialStepField
                            placeholderText: "e.g. 1u"
                            selectByMouse: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Final Time *"
                            color: "#24292f"
                        }
                        TextField {
                            id: finalTimeField
                            placeholderText: "e.g. 10m"
                            selectByMouse: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Start Time"
                            color: "#24292f"
                        }
                        TextField {
                            id: startTimeField
                            placeholderText: "e.g. 0"
                            selectByMouse: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Step Ceiling"
                            color: "#24292f"
                        }
                        TextField {
                            id: stepCeilingField
                            placeholderText: "optional"
                            selectByMouse: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Operating Point"
                            color: "#24292f"
                        }
                        ComboBox {
                            id: opModeComboBox
                            Layout.columnSpan: 3
                            Layout.fillWidth: true
                            model: ["Default (compute OP)", "NOOP (skip OP)", "UIC (skip OP)"]
                        }
                    }

                    CheckBox {
                        id: scheduleEnabledCheckBox
                        text: "Enable schedule(time, max_step, ...)"
                        Layout.fillWidth: true
                    }

                    TextArea {
                        id: schedulePairsTextArea
                        enabled: scheduleEnabledCheckBox.checked
                        placeholderText: "Enter pairs as time,max_step values.\nExample: 0.5e-3,0 1e-3,1e-6 2e-3,0"
                        selectByMouse: true
                        wrapMode: TextEdit.Wrap
                        Layout.fillWidth: true
                        Layout.preferredHeight: 72
                    }
                }
            }

            // --- .PRINT TRAN section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: tranPrintColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: tranPrintColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 10

                    CheckBox {
                        id: tranPrintEnabledCheckBox
                        text: "Enable .PRINT TRAN output"
                        checked: panel.printEnabled
                        onCheckedChanged: panel.printEnabled = checked
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        enabled: tranPrintEnabledCheckBox.checked
                        Layout.fillWidth: true
                        spacing: 16
                        CheckBox {
                            text: "All voltages  V(*)"
                            checked: panel.printAllNodes
                            onCheckedChanged: panel.printAllNodes = checked
                        }
                        CheckBox {
                            text: "All currents  I(*)"
                            checked: panel.printAllCurrents
                            onCheckedChanged: panel.printAllCurrents = checked
                        }
                        CheckBox {
                            text: "Power  P(*)"
                            checked: panel.printPower
                            onCheckedChanged: panel.printPower = checked
                        }
                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        enabled: tranPrintEnabledCheckBox.checked
                        visible: panel.hasBjtDevices || panel.hasFetDevices
                        Layout.fillWidth: true
                        spacing: 16
                        CheckBox {
                            text: "BJT leads  IB(*) IC(*) IE(*) IS(*)"
                            visible: panel.hasBjtDevices
                            checked: panel.printBjtLeads
                            onCheckedChanged: panel.printBjtLeads = checked
                        }
                        CheckBox {
                            text: "FET leads  IB(*) ID(*) IG(*) IS(*)"
                            visible: panel.hasFetDevices
                            checked: panel.printFetLeads
                            onCheckedChanged: panel.printFetLeads = checked
                        }
                        Item { Layout.fillWidth: true }
                    }

                    GridLayout {
                        enabled: tranPrintEnabledCheckBox.checked
                        columns: 4
                        Layout.fillWidth: true
                        rowSpacing: 6
                        columnSpacing: 8

                        Label {
                            text: "Additional"
                            color: "#24292f"
                        }
                        TextField {
                            id: tranPrintSpecificVarsField
                            placeholderText: "e.g. V(1) I(R1) — pre-filled from netlist"
                            selectByMouse: true
                            text: panel.printSpecificVars
                            onTextChanged: panel.printSpecificVars = text
                            Layout.columnSpan: 3
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Format"
                            color: "#24292f"
                        }
                        ComboBox {
                            id: printFormatCombo
                            Layout.fillWidth: true
                            model: ["(default)", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]
                        }

                        Label {
                            text: "Output File"
                            color: "#24292f"
                        }
                        TextField {
                            id: tranPrintFileField
                            placeholderText: "optional (e.g. output.raw)"
                            selectByMouse: true
                            text: panel.printFile
                            onTextChanged: panel.printFile = text
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // --- .FFT section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: fftParamsColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: fftParamsColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    Label {
                        text: "Fast Fourier Transform (.FFT)"
                        font.bold: true
                        color: "#24292f"
                        Layout.fillWidth: true
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        clip: true

                        TextArea {
                            id: fftParametersTextArea
                            placeholderText: "Enter one .FFT directive per line.\nExample: .FFT V(OUT) WINDOW=HANN"
                            selectByMouse: true
                            wrapMode: TextEdit.NoWrap
                        }
                    }
                }
            }

            // --- .FOUR section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: fourParamsColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: fourParamsColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    Label {
                        text: "Fourier Analysis (.FOUR)"
                        font.bold: true
                        color: "#24292f"
                        Layout.fillWidth: true
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        clip: true

                        TextArea {
                            id: fourParametersTextArea
                            placeholderText: "Enter one .FOUR directive per line.\nExample: .FOUR 1k V(OUT)"
                            selectByMouse: true
                            wrapMode: TextEdit.NoWrap
                        }
                    }
                }
            }

            // --- .MEASURE section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: tranMeasureColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: tranMeasureColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    Label {
                        text: "Measurements (.MEASURE)"
                        font.bold: true
                        color: "#24292f"
                        Layout.fillWidth: true
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        clip: true

                        TextArea {
                            id: measureParametersTextArea
                            placeholderText: "Enter one .MEASURE directive per line.\nExample: .MEASURE TRAN RISE_TIME MAX V(OUT) RISE=1"
                            selectByMouse: true
                            wrapMode: TextEdit.NoWrap
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
