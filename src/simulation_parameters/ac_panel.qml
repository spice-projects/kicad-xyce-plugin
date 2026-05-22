pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: panel

    property alias sweepModeIndex: acSweepModeComboBox.currentIndex
    property alias points: acPointsField.text
    property alias start: acStartField.text
    property alias end: acEndField.text
    property alias dataTableName: acDataTableNameField.text
    property alias measureParametersText: acMeasureParametersTextArea.text
    property bool printEnabled: false
    property bool printAllNodes: false
    property bool printAllCurrents: false
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    readonly property string sweepModeValue: (["LIN", "DEC", "OCT", "DATA"])[acSweepModeComboBox.currentIndex] || "LIN"
    readonly property bool isDataMode: acSweepModeComboBox.currentIndex === 3
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 12

            // --- .AC section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: acParamsColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: acParamsColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 10
                        columnSpacing: 12

                        Label {
                            text: "Sweep Mode *"
                            color: "#24292f"
                        }
                        ComboBox {
                            id: acSweepModeComboBox
                            Layout.fillWidth: true
                            model: ["LIN (linear)", "DEC (per decade)", "OCT (per octave)", "DATA (table-driven)"]
                        }

                        Label {
                            text: "Points *"
                            color: "#24292f"
                            visible: !panel.isDataMode
                        }
                        TextField {
                            id: acPointsField
                            placeholderText: "e.g. 100"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: !panel.isDataMode
                        }

                        Label {
                            text: "Start Frequency *"
                            color: "#24292f"
                            visible: !panel.isDataMode
                        }
                        TextField {
                            id: acStartField
                            placeholderText: "e.g. 1"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: !panel.isDataMode
                        }

                        Label {
                            text: "End Frequency *"
                            color: "#24292f"
                            visible: !panel.isDataMode
                        }
                        TextField {
                            id: acEndField
                            placeholderText: "e.g. 1MEG"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: !panel.isDataMode
                        }

                        Label {
                            text: "Data Table Name *"
                            color: "#24292f"
                            visible: panel.isDataMode
                        }
                        TextField {
                            id: acDataTableNameField
                            placeholderText: "e.g. freqTable"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: panel.isDataMode
                        }
                    }
                }
            }

            // --- .PRINT AC section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: acPrintColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: acPrintColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    CheckBox {
                        id: acPrintEnabledCheckBox
                        text: "Enable .PRINT AC output"
                        checked: panel.printEnabled
                        onCheckedChanged: panel.printEnabled = checked
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        enabled: acPrintEnabledCheckBox.checked
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
                        Item { Layout.fillWidth: true }
                    }

                    GridLayout {
                        enabled: acPrintEnabledCheckBox.checked
                        columns: 4
                        Layout.fillWidth: true
                        rowSpacing: 6
                        columnSpacing: 8

                        Label {
                            text: "Additional"
                            color: "#24292f"
                        }
                        TextField {
                            id: acPrintSpecificVarsField
                            placeholderText: "e.g. VR(out) VM(1) IP(V1)"
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
                            id: acPrintFileField
                            placeholderText: "optional (e.g. output.raw)"
                            selectByMouse: true
                            text: panel.printFile
                            onTextChanged: panel.printFile = text
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // --- .MEASURE section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: acMeasureColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: acMeasureColumn
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
                            id: acMeasureParametersTextArea
                            placeholderText: "Enter one .MEASURE directive per line.\nExample: .MEASURE AC BANDWIDTH FIND V(OUT) WHEN V(OUT)=0.707"
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
