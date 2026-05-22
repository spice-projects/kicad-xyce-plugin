pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: panel

    property alias frequenciesText: hbFrequenciesField.text
    property alias harmonicsText: hbHarmonicsField.text
    property alias tahbIndex: hbTahbCombo.currentIndex
    property alias selectHarmsIndex: hbSelectHarmsCombo.currentIndex
    property alias startupPeriodsText: hbStartupPeriodsField.text
    property bool printEnabled: false
    property bool printAllNodes: false
    property bool printAllCurrents: false
    property alias printTypeIndex: hbPrintTypeComboBox.currentIndex
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    readonly property string printTypeValue: (["HB", "HB_FD", "HB_TD"])[hbPrintTypeComboBox.currentIndex] || "HB"
    readonly property string selectHarmsValue: hbSelectHarmsCombo.model[hbSelectHarmsCombo.currentIndex].toLowerCase()
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 12

            // --- .HB section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: hbParamsColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: hbParamsColumn
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
                            text: "Fundamental Frequencies *"
                            color: "#24292f"
                        }
                        TextField {
                            id: hbFrequenciesField
                            placeholderText: "e.g. 1MEG 2MEG 500K"
                            selectByMouse: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Harmonics (NUMFREQ)"
                            color: "#24292f"
                        }
                        TextField {
                            id: hbHarmonicsField
                            placeholderText: "e.g. 10 10 (defaults to 10 if empty)"
                            selectByMouse: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Transient Assistance (TAHB)"
                            color: "#24292f"
                        }
                        ComboBox {
                            id: hbTahbCombo
                            Layout.fillWidth: true
                            model: ["Off (0)", "Transient (1)", "DC (2)"]
                        }

                        Label {
                            text: "Truncation (SELECTHARMS)"
                            color: "#24292f"
                        }
                        ComboBox {
                            id: hbSelectHarmsCombo
                            Layout.fillWidth: true
                            model: ["Hybrid", "Box", "Diamond"]
                        }

                        Label {
                            text: "Startup Periods"
                            color: "#24292f"
                        }
                        TextField {
                            id: hbStartupPeriodsField
                            placeholderText: "e.g. 0"
                            selectByMouse: true
                            Layout.fillWidth: true
                            validator: IntValidator { bottom: 0 }
                        }
                    }
                }
            }

            // --- .PRINT HB section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: hbPrintColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: hbPrintColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    CheckBox {
                        id: hbPrintEnabledCheckBox
                        text: "Enable .PRINT HB output"
                        checked: panel.printEnabled
                        onCheckedChanged: panel.printEnabled = checked
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        enabled: hbPrintEnabledCheckBox.checked
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
                        enabled: hbPrintEnabledCheckBox.checked
                        columns: 4
                        Layout.fillWidth: true
                        rowSpacing: 6
                        columnSpacing: 8

                        Label {
                            text: "Additional"
                            color: "#24292f"
                        }
                        TextField {
                            id: hbPrintSpecificVarsField
                            placeholderText: "e.g. VR(out) VM(1) IP(V1)"
                            selectByMouse: true
                            text: panel.printSpecificVars
                            onTextChanged: panel.printSpecificVars = text
                            Layout.columnSpan: 3
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Print Type"
                            color: "#24292f"
                        }
                        ComboBox {
                            id: hbPrintTypeComboBox
                            Layout.fillWidth: true
                            model: ["HB", "HB_FD", "HB_TD"]
                        }
                        Item { Layout.columnSpan: 2 }

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
                            id: hbPrintFileField
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
