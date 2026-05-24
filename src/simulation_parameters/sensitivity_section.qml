pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {

    id: root
    spacing: 24

    readonly property bool active: objectiveValues.trim() !== "" && parameters.trim() !== ""
    property string objectiveMode: "objfunc"
    property string objectiveValues: ""
    property string parameters: ""
    property bool direct: false
    property bool adjoint: false
    property bool printEnabled: false
    property bool replaceGround: true
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    // --- .SENS section ---
    SimulationCard {
        title: "Sensitivity Analysis Parameters (.SENS)"
        badge: "OPTIONAL"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: 12
                columnSpacing: 20

                Label { text: "Objective Mode"; color: "#6B6B66"; font.pixelSize: 12 }
                ComboBox {
                    id: sensObjectiveModeCombo
                    Layout.fillWidth: true
                    model: ["objfunc", "objvars", "acobjfunc"]
                    currentIndex: ["objfunc", "objvars", "acobjfunc"].indexOf(root.objectiveMode)
                    onCurrentIndexChanged: root.objectiveMode = model[currentIndex]
                }

                Label { text: "Objective Values *"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: sensObjectiveValuesField
                    placeholderText: "e.g. V(2)"
                    text: root.objectiveValues
                    onTextChanged: root.objectiveValues = text
                    Layout.fillWidth: true
                    selectByMouse: true
                }

                Label { text: "Parameters *"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: sensParametersField
                    placeholderText: "e.g. R1:R,C1:C"
                    text: root.parameters
                    onTextChanged: root.parameters = text
                    Layout.fillWidth: true
                    selectByMouse: true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 20
                CheckBox {
                    id: sensDirectCheckBox
                    text: "Direct Method"
                    checked: root.direct
                    onCheckedChanged: root.direct = checked
                }
                CheckBox {
                    id: sensAdjointCheckBox
                    text: "Adjoint Method"
                    checked: root.adjoint
                    onCheckedChanged: root.adjoint = checked
                }
            }
        }
    }

    // --- .PRINT SENS section ---
    SimulationCard {
        title: "Output Variables (.PRINT SENS)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                id: sensPrintEnabledCheckBox
                text: "Enable .PRINT SENS output"
                checked: root.printEnabled
                onCheckedChanged: root.printEnabled = checked
                font.weight: Font.Medium
            }

            ColumnLayout {
                enabled: sensPrintEnabledCheckBox.checked
                Layout.fillWidth: true
                Layout.leftMargin: 28
                spacing: 12

                GridLayout {
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 12
                    Layout.fillWidth: true

                    Label { text: "Additional variables"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: sensPrintSpecificVarsField
                        placeholderText: "e.g. dSdP(V(2):R1:R)"
                        text: root.printSpecificVars
                        onTextChanged: root.printSpecificVars = text
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
                        id: sensPrintFileField
                        placeholderText: "optional (e.g. output.raw)"
                        text: root.printFile
                        onTextChanged: root.printFile = text
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
