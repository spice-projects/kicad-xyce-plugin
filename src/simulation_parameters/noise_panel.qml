pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {

    id: panel
    spacing: 24

    property alias outputNode: noiseOutputNodeField.text
    property alias refNode: noiseRefNodeField.text
    property alias sourceName: noiseSourceNameField.text
    property alias sweepModeIndex: noiseSweepModeComboBox.currentIndex
    property alias points: noisePointsField.text
    property alias start: noiseStartField.text
    property alias end: noiseEndField.text
    property alias dataTableName: noiseDataTableNameField.text
    property alias measureParametersText: noiseMeasureParametersTextArea.text
    property bool printEnabled: false
    property bool printAllNodes: false
    property bool printAllCurrents: false
    property bool printInoise: false
    property bool printOnoise: false
    property bool replaceGround: true
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    property var deviceOperators: []
    readonly property string sweepModeValue: (["LIN", "DEC", "OCT", "DATA"])[noiseSweepModeComboBox.currentIndex] || "LIN"
    readonly property bool isDataMode: noiseSweepModeComboBox.currentIndex === 3
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    // --- .NOISE section ---
    SimulationCard {
        title: "Noise Analysis Parameters (.NOISE)"
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

                Label { text: "Output Node *"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: noiseOutputNodeField
                    placeholderText: "e.g. out"
                    Layout.fillWidth: true
                    selectByMouse: true
                }

                Label { text: "Reference Node"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: noiseRefNodeField
                    placeholderText: "optional (default = ground)"
                    Layout.fillWidth: true
                    selectByMouse: true
                }

                Label { text: "Input Source *"; color: "#6B6B66"; font.pixelSize: 12 }
                TextField {
                    id: noiseSourceNameField
                    placeholderText: "e.g. VIN"
                    Layout.fillWidth: true
                    selectByMouse: true
                }

                Label { text: "Sweep Mode *"; color: "#6B6B66"; font.pixelSize: 12 }
                ComboBox {
                    id: noiseSweepModeComboBox
                    Layout.fillWidth: true
                    model: ["LIN (linear)", "DEC (per decade)", "OCT (per octave)", "DATA (table-driven)"]
                }

                Label {
                    text: "Points *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: !panel.isDataMode
                }
                TextField {
                    id: noisePointsField
                    placeholderText: "e.g. 100"
                    Layout.fillWidth: true
                    visible: !panel.isDataMode
                    selectByMouse: true
                }

                Label {
                    text: "Start Frequency *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: !panel.isDataMode
                }
                TextField {
                    id: noiseStartField
                    placeholderText: "e.g. 1"
                    Layout.fillWidth: true
                    visible: !panel.isDataMode
                    selectByMouse: true
                }

                Label {
                    text: "End Frequency *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: !panel.isDataMode
                }
                TextField {
                    id: noiseEndField
                    placeholderText: "e.g. 1MEG"
                    Layout.fillWidth: true
                    visible: !panel.isDataMode
                    selectByMouse: true
                }

                Label {
                    text: "Data Table Name *"
                    color: "#6B6B66"; font.pixelSize: 12
                    visible: panel.isDataMode
                }
                TextField {
                    id: noiseDataTableNameField
                    placeholderText: "e.g. freqTable"
                    Layout.fillWidth: true
                    visible: panel.isDataMode
                    selectByMouse: true
                }
            }
        }
    }

    // --- .PRINT NOISE section ---
    SimulationCard {
        title: "Output Variables (.PRINT NOISE)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                id: noisePrintEnabledCheckBox
                text: "Enable .PRINT NOISE output"
                checked: panel.printEnabled
                onCheckedChanged: panel.printEnabled = checked
                font.weight: Font.Medium
            }

            ColumnLayout {
                enabled: noisePrintEnabledCheckBox.checked
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
                    CheckBox {
                        text: "INOISE"
                        checked: panel.printInoise
                        onCheckedChanged: panel.printInoise = checked
                    }
                    CheckBox {
                        text: "ONOISE"
                        checked: panel.printOnoise
                        onCheckedChanged: panel.printOnoise = checked
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    Label { text: "Device Noise Operators"; color: "#6B6B66"; font.pixelSize: 12 }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: panel.deviceOperators.length

                            RowLayout {
                                required property int index
                                Layout.fillWidth: true
                                spacing: 8

                                TextField {
                                    Layout.fillWidth: true
                                    placeholderText: "Device name"
                                    text: panel.deviceOperators[parent.index].deviceName || ""
                                    selectByMouse: true
                                    onTextChanged: {
                                        if (text !== panel.deviceOperators[parent.index].deviceName) {
                                            var updatedOperators = panel.deviceOperators.slice();
                                            updatedOperators[parent.index].deviceName = text;
                                            panel.deviceOperators = updatedOperators;
                                        }
                                    }
                                }

                                ComboBox {
                                    Layout.preferredWidth: 80
                                    model: ["DNI", "DNO"]
                                    currentIndex: panel.deviceOperators[parent.index].operatorType === "DNI" ? 0 : 1
                                    onActivated: function(activatedIndex) {
                                        var updatedOperators = panel.deviceOperators.slice();
                                        updatedOperators[parent.index].operatorType = model[activatedIndex];
                                        panel.deviceOperators = updatedOperators;
                                    }
                                }

                                TextField {
                                    Layout.preferredWidth: 120
                                    placeholderText: "Noise source (optional)"
                                    text: panel.deviceOperators[parent.index].noiseSource || ""
                                    selectByMouse: true
                                    onTextChanged: {
                                        if (text !== (panel.deviceOperators[parent.index].noiseSource || "")) {
                                            var updatedOperators = panel.deviceOperators.slice();
                                            updatedOperators[parent.index].noiseSource = text;
                                            panel.deviceOperators = updatedOperators;
                                        }
                                    }
                                }

                                Button {
                                    Layout.preferredWidth: 30
                                    text: "\u00d7"
                                    onClicked: {
                                        var updatedOperators = panel.deviceOperators.slice();
                                        updatedOperators.splice(parent.index, 1);
                                        panel.deviceOperators = updatedOperators;
                                    }
                                }
                            }
                        }

                        Button {
                            text: "+ Add Device Operator"
                            onClicked: {
                                var updatedOperators = panel.deviceOperators.slice();
                                updatedOperators.push({deviceName: "", operatorType: "DNI", noiseSource: ""});
                                panel.deviceOperators = updatedOperators;
                            }
                        }
                    }
                }

                GridLayout {
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 12
                    Layout.fillWidth: true

                    Label { text: "Additional variables"; color: "#6B6B66"; font.pixelSize: 12 }
                    TextField {
                        id: noisePrintSpecificVarsField
                        placeholderText: "e.g. V(1) I(V1)"
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
                        id: noisePrintFileField
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

    // --- .MEASURE section ---
    SimulationCard {
        title: "Measurements (.MEASURE)"
        Layout.fillWidth: true

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            Label { text: "Noise analysis measurements"; font.pixelSize: 12; color: "#6B6B66" }
            TextArea {
                id: noiseMeasureParametersTextArea
                placeholderText: "Example: .MEASURE NOISE TOTAL_INOISE INTEG INOISE"
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                font.family: "monospace"
                font.pixelSize: 11
                selectByMouse: true
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
