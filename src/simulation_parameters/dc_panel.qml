pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: panel

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
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    readonly property string sweepModeValue: (["LIN", "DEC", "OCT", "LIST", "DATA"])[sweepModeComboBox.currentIndex] || "LIN"
    readonly property bool isLogMode: sweepModeComboBox.currentIndex === 1 || sweepModeComboBox.currentIndex === 2
    readonly property bool isLinMode: sweepModeComboBox.currentIndex === 0
    readonly property bool isRangeMode: sweepModeComboBox.currentIndex <= 2
    readonly property bool isListMode: sweepModeComboBox.currentIndex === 3
    readonly property bool isDataMode: sweepModeComboBox.currentIndex === 4
    readonly property bool supportsSecondary: sweepModeComboBox.currentIndex <= 2
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 12

            // --- .DC section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: dcParamsColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: dcParamsColumn
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
                            id: sweepModeComboBox
                            Layout.fillWidth: true
                            model: ["LIN (linear)", "DEC (per decade)", "OCT (per octave)", "LIST (explicit)", "DATA (table-driven)"]
                        }

                        Label {
                            text: "Primary Variable *"
                            color: "#24292f"
                            visible: !panel.isDataMode
                        }
                        TextField {
                            id: primaryVariableField
                            placeholderText: "e.g. VIN, R1, TEMP"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: !panel.isDataMode
                        }

                        Label {
                            text: "Start *"
                            color: "#24292f"
                            visible: panel.isRangeMode
                        }
                        TextField {
                            id: startField
                            placeholderText: "e.g. 0"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: panel.isRangeMode
                        }

                        Label {
                            text: "Stop *"
                            color: "#24292f"
                            visible: panel.isRangeMode
                        }
                        TextField {
                            id: stopField
                            placeholderText: "e.g. 5"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: panel.isRangeMode
                        }

                        Label {
                            text: "Step *"
                            color: "#24292f"
                            visible: panel.isLinMode
                        }
                        TextField {
                            id: stepField
                            placeholderText: "e.g. 0.1"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: panel.isLinMode
                        }

                        Label {
                            text: "Points *"
                            color: "#24292f"
                            visible: panel.isLogMode
                        }
                        TextField {
                            id: pointsField
                            placeholderText: "e.g. 10"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: panel.isLogMode
                        }

                        Label {
                            text: "List Values *"
                            color: "#24292f"
                            visible: panel.isListMode
                        }
                        TextArea {
                            id: listValuesTextArea
                            placeholderText: "Enter values separated by spaces or commas.\nExample: 10 15 18 27 33"
                            selectByMouse: true
                            wrapMode: TextEdit.Wrap
                            Layout.fillWidth: true
                            Layout.preferredHeight: 72
                            visible: panel.isListMode
                        }

                        Label {
                            text: "Data Table Name *"
                            color: "#24292f"
                            visible: panel.isDataMode
                        }
                        TextField {
                            id: dataTableNameField
                            placeholderText: "e.g. resistorValues"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: panel.isDataMode
                        }
                    }

                    // secondary sweep (visible only for supported sweep modes)
                    ColumnLayout {
                        visible: panel.supportsSecondary
                        Layout.fillWidth: true
                        spacing: 6

                        CheckBox {
                            id: secondaryEnabledCheckBox
                            text: "Enable secondary (nested) sweep"
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            visible: secondaryEnabledCheckBox.checked
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: 10
                            columnSpacing: 12

                            Label {
                                text: "Secondary Variable *"
                                color: "#24292f"
                            }
                            TextField {
                                id: secondaryVariableField
                                placeholderText: "e.g. C1"
                                selectByMouse: true
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "Secondary Start *"
                                color: "#24292f"
                            }
                            TextField {
                                id: secondaryStartField
                                placeholderText: "e.g. 0"
                                selectByMouse: true
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "Secondary Stop *"
                                color: "#24292f"
                            }
                            TextField {
                                id: secondaryStopField
                                placeholderText: "e.g. 3.5"
                                selectByMouse: true
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "Secondary Step *"
                                color: "#24292f"
                                visible: panel.isLinMode
                            }
                            TextField {
                                id: secondaryStepField
                                placeholderText: "e.g. 0.5"
                                selectByMouse: true
                                Layout.fillWidth: true
                                visible: panel.isLinMode
                            }

                            Label {
                                text: "Secondary Points *"
                                color: "#24292f"
                                visible: panel.isLogMode
                            }
                            TextField {
                                id: secondaryPointsField
                                placeholderText: "e.g. 5"
                                selectByMouse: true
                                Layout.fillWidth: true
                                visible: panel.isLogMode
                            }
                        }
                    }
                }
            }

            // --- .PRINT DC section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: dcPrintColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: dcPrintColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    CheckBox {
                        id: dcPrintEnabledCheckBox
                        text: "Enable .PRINT DC output"
                        checked: panel.printEnabled
                        onCheckedChanged: panel.printEnabled = checked
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        enabled: dcPrintEnabledCheckBox.checked
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
                        enabled: dcPrintEnabledCheckBox.checked
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
                        enabled: dcPrintEnabledCheckBox.checked
                        columns: 4
                        Layout.fillWidth: true
                        rowSpacing: 6
                        columnSpacing: 8

                        Label {
                            text: "Additional"
                            color: "#24292f"
                        }
                        TextField {
                            id: dcPrintSpecificVarsField
                            placeholderText: "e.g. V(1) I(R1)"
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
                            id: dcPrintFileField
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
                implicitHeight: dcMeasureColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: dcMeasureColumn
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
                            id: dcMeasureParametersTextArea
                            placeholderText: "Enter one .MEASURE directive per line.\nExample: .MEASURE DC VIN_AT_2V FIND V(1) WHEN V(1)=2"
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
