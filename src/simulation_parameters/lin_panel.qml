pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: panel

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
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    readonly property string sweepModeValue: (["LIN", "DEC", "OCT", "DATA"])[linSweepModeComboBox.currentIndex] || "LIN"
    readonly property bool isDataMode: linSweepModeComboBox.currentIndex === 3
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 12

            // --- .LIN section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: linParamsColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: linParamsColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    CheckBox {
                        id: linSparcalcCheckBox
                        text: "Enable SPARCALC (linearize into S/Y/Z parameters)"
                        checked: panel.sparcalc
                        onCheckedChanged: panel.sparcalc = checked
                        Layout.fillWidth: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 10
                        columnSpacing: 12

                        Label {
                            text: "Output Format"
                            color: "#24292f"
                        }
                        ComboBox {
                            id: linFormatComboBox
                            Layout.fillWidth: true
                            model: ["TOUCHSTONE2", "TOUCHSTONE"]
                            currentIndex: panel.linFormat === "TOUCHSTONE" ? 1 : 0
                            onCurrentIndexChanged: panel.linFormat = currentIndex === 1 ? "TOUCHSTONE" : "TOUCHSTONE2"
                        }

                        Label {
                            text: "Parameter Type"
                            color: "#24292f"
                        }
                        ComboBox {
                            id: linTypeComboBox
                            Layout.fillWidth: true
                            model: ["S", "Y", "Z"]
                            currentIndex: panel.linType === "Y" ? 1 : panel.linType === "Z" ? 2 : 0
                            onCurrentIndexChanged: panel.linType = currentIndex === 1 ? "Y" : currentIndex === 2 ? "Z" : "S"
                        }

                        Label {
                            text: "Data Format"
                            color: "#24292f"
                        }
                        ComboBox {
                            id: linDataFormatComboBox
                            Layout.fillWidth: true
                            model: ["RI", "MA", "DB"]
                            currentIndex: panel.linDataFormat === "MA" ? 1 : panel.linDataFormat === "DB" ? 2 : 0
                            onCurrentIndexChanged: panel.linDataFormat = currentIndex === 1 ? "MA" : currentIndex === 2 ? "DB" : "RI"
                        }

                        Label {
                            text: "Output File"
                            color: "#24292f"
                        }
                        TextField {
                            id: linFileField
                            placeholderText: "optional output file name"
                            selectByMouse: true
                            text: panel.linFile
                            onTextChanged: panel.linFile = text
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Width"
                            color: "#24292f"
                        }
                        TextField {
                            id: linWidthField
                            placeholderText: "optional"
                            selectByMouse: true
                            text: panel.linWidth
                            onTextChanged: panel.linWidth = text
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Precision"
                            color: "#24292f"
                        }
                        TextField {
                            id: linPrecisionField
                            placeholderText: "optional"
                            selectByMouse: true
                            text: panel.linPrecision
                            onTextChanged: panel.linPrecision = text
                            Layout.fillWidth: true
                        }
                    }

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
                            id: linSweepModeComboBox
                            Layout.fillWidth: true
                            model: ["LIN (linear)", "DEC (per decade)", "OCT (per octave)", "DATA (table-driven)"]
                        }

                        Label {
                            text: "Points *"
                            color: "#24292f"
                            visible: !panel.isDataMode
                        }
                        TextField {
                            id: linPointsField
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
                            id: linStartField
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
                            id: linEndField
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
                            id: linDataTableNameField
                            placeholderText: "e.g. freqTable"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: panel.isDataMode
                        }
                    }
                }
            }

            // --- .PRINT LIN section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: linPrintColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: linPrintColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    CheckBox {
                        id: linPrintEnabledCheckBox
                        text: "Enable .PRINT LIN output"
                        checked: panel.printEnabled
                        onCheckedChanged: panel.printEnabled = checked
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        enabled: linPrintEnabledCheckBox.checked
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
                        enabled: linPrintEnabledCheckBox.checked
                        columns: 4
                        Layout.fillWidth: true
                        rowSpacing: 6
                        columnSpacing: 8

                        Label {
                            text: "Additional"
                            color: "#24292f"
                        }
                        TextField {
                            id: linPrintSpecificVarsField
                            placeholderText: "e.g. SR(1,2) YP(2,1)"
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
                            id: linPrintFileField
                            placeholderText: "optional (e.g. output.raw)"
                            selectByMouse: true
                            text: panel.printFile
                            onTextChanged: panel.printFile = text
                            Layout.fillWidth: true
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
