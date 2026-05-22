pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: panel

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
    property bool saveEnabled: false
    property string saveType: "NODESET"
    property string saveFile: ""
    property string nodesetEntries: ""

    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 12

            // --- .PRINT DC section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: opPrintColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: opPrintColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    CheckBox {
                        id: opPrintEnabledCheckBox
                        text: "Enable .PRINT DC output"
                        checked: panel.printEnabled
                        onCheckedChanged: panel.printEnabled = checked
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        enabled: opPrintEnabledCheckBox.checked
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
                        enabled: opPrintEnabledCheckBox.checked
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
                        enabled: opPrintEnabledCheckBox.checked
                        columns: 4
                        Layout.fillWidth: true
                        rowSpacing: 6
                        columnSpacing: 8

                        Label {
                            text: "Additional"
                            color: "#24292f"
                        }
                        TextField {
                            id: opPrintSpecificVarsField
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
                            id: opPrintFileField
                            placeholderText: "optional (e.g. output.raw)"
                            selectByMouse: true
                            text: panel.printFile
                            onTextChanged: panel.printFile = text
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // --- .SAVE section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: saveColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: saveColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    CheckBox {
                        id: saveEnabledCheckBox
                        text: "Enable .SAVE operating point"
                        checked: panel.saveEnabled
                        onCheckedChanged: panel.saveEnabled = checked
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        enabled: saveEnabledCheckBox.checked
                        Layout.fillWidth: true
                        spacing: 16
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
                        Item { Layout.fillWidth: true }
                    }

                    GridLayout {
                        enabled: saveEnabledCheckBox.checked
                        columns: 2
                        Layout.fillWidth: true
                        rowSpacing: 6
                        columnSpacing: 8

                        Label {
                            text: "Save File"
                            color: "#24292f"
                        }
                        TextField {
                            id: saveFileField
                            placeholderText: "optional output file path"
                            selectByMouse: true
                            text: panel.saveFile
                            onTextChanged: panel.saveFile = text
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // --- .NODESET section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: nodesetColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: nodesetColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    Label {
                        text: "Convergence Hints (.NODESET)"
                        font.bold: true
                        color: "#24292f"
                        Layout.fillWidth: true
                    }

                    TextField {
                        placeholderText: "e.g. V(1)=5.0 V(2)=3.3"
                        selectByMouse: true
                        text: panel.nodesetEntries
                        onTextChanged: panel.nodesetEntries = text
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
