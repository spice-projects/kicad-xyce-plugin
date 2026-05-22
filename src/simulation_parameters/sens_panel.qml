pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: panel

    property bool active: false
    property string objectiveMode: "objfunc"
    property string objectiveValues: ""
    property string parameters: ""
    property bool direct: false
    property bool adjoint: false
    property bool printEnabled: false
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: sensParamsColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: sensParamsColumn
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

                        Label { text: "Objective Mode"; color: "#24292f" }
                        ComboBox {
                            id: sensObjectiveModeCombo
                            model: ["objfunc", "objvars", "acobjfunc"]
                            currentIndex: ["objfunc", "objvars", "acobjfunc"].indexOf(panel.objectiveMode)
                            onCurrentIndexChanged: panel.objectiveMode = model[currentIndex]
                            Layout.fillWidth: true
                        }

                        Label { text: "Objective Values *"; color: "#24292f" }
                        TextField {
                            id: sensObjectiveValuesField
                            placeholderText: "e.g. V(2)"
                            text: panel.objectiveValues
                            onTextChanged: panel.objectiveValues = text
                            Layout.fillWidth: true
                        }

                        Label { text: "Parameters *"; color: "#24292f" }
                        TextField {
                            id: sensParametersField
                            placeholderText: "e.g. R1:R,C1:C"
                            text: panel.parameters
                            onTextChanged: panel.parameters = text
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        CheckBox {
                            id: sensDirectCheckBox
                            text: "Direct Method"
                            checked: panel.direct
                            onCheckedChanged: panel.direct = checked
                        }
                        CheckBox {
                            id: sensAdjointCheckBox
                            text: "Adjoint Method"
                            checked: panel.adjoint
                            onCheckedChanged: panel.adjoint = checked
                        }
                    }
                }
            }

            // --- .PRINT SENS section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: sensPrintColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: sensPrintColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    CheckBox {
                        id: sensPrintEnabledCheckBox
                        text: "Enable .PRINT SENS output"
                        checked: panel.printEnabled
                        onCheckedChanged: panel.printEnabled = checked
                        Layout.fillWidth: true
                    }

                    GridLayout {
                        enabled: sensPrintEnabledCheckBox.checked
                        columns: 4
                        Layout.fillWidth: true
                        rowSpacing: 6
                        columnSpacing: 8

                        Label { text: "Additional Vars"; color: "#24292f" }
                        TextField {
                            id: sensPrintSpecificVarsField
                            placeholderText: "e.g. dSdP(V(2):R1:R)"
                            selectByMouse: true
                            text: panel.printSpecificVars
                            onTextChanged: panel.printSpecificVars = text
                            Layout.columnSpan: 3
                            Layout.fillWidth: true
                        }

                        Label { text: "Format"; color: "#24292f" }
                        ComboBox {
                            id: printFormatCombo
                            Layout.fillWidth: true
                            model: ["(default)", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]
                        }

                        Label { text: "Output File"; color: "#24292f" }
                        TextField {
                            id: sensPrintFileField
                            placeholderText: "optional (e.g. output.raw)"
                            selectByMouse: true
                            text: panel.printFile
                            onTextChanged: panel.printFile = text
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}
