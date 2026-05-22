pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: panel

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
    property string printSpecificVars: ""
    property alias printFormatIndex: printFormatCombo.currentIndex
    property string printFile: ""
    property var deviceOperators: []
    readonly property string sweepModeValue: (["LIN", "DEC", "OCT", "DATA"])[noiseSweepModeComboBox.currentIndex] || "LIN"
    readonly property bool isDataMode: noiseSweepModeComboBox.currentIndex === 3
    readonly property string printFormatValue: printFormatCombo.currentIndex > 0 ? printFormatCombo.model[printFormatCombo.currentIndex] : ""

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 12

            // --- .NOISE section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: noiseParamsColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: noiseParamsColumn
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
                            text: "Output Node *"
                            color: "#24292f"
                        }
                        TextField {
                            id: noiseOutputNodeField
                            placeholderText: "e.g. out"
                            selectByMouse: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Reference Node"
                            color: "#24292f"
                        }
                        TextField {
                            id: noiseRefNodeField
                            placeholderText: "optional (default = ground)"
                            selectByMouse: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Input Source *"
                            color: "#24292f"
                        }
                        TextField {
                            id: noiseSourceNameField
                            placeholderText: "e.g. VIN"
                            selectByMouse: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Sweep Mode *"
                            color: "#24292f"
                        }
                        ComboBox {
                            id: noiseSweepModeComboBox
                            Layout.fillWidth: true
                            model: ["LIN (linear)", "DEC (per decade)", "OCT (per octave)", "DATA (table-driven)"]
                        }

                        Label {
                            text: "Points *"
                            color: "#24292f"
                            visible: !panel.isDataMode
                        }
                        TextField {
                            id: noisePointsField
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
                            id: noiseStartField
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
                            id: noiseEndField
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
                            id: noiseDataTableNameField
                            placeholderText: "e.g. freqTable"
                            selectByMouse: true
                            Layout.fillWidth: true
                            visible: panel.isDataMode
                        }
                    }
                }
            }

            // --- .PRINT NOISE section ---
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: noisePrintColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: noisePrintColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    CheckBox {
                        id: noisePrintEnabledCheckBox
                        text: "Enable .PRINT NOISE output"
                        checked: panel.printEnabled
                        onCheckedChanged: panel.printEnabled = checked
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        enabled: noisePrintEnabledCheckBox.checked
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
                            text: "INOISE"
                            checked: panel.printInoise
                            onCheckedChanged: panel.printInoise = checked
                        }
                        CheckBox {
                            text: "ONOISE"
                            checked: panel.printOnoise
                            onCheckedChanged: panel.printOnoise = checked
                        }
                        Item { Layout.fillWidth: true }
                    }

                    GridLayout {
                        enabled: noisePrintEnabledCheckBox.checked
                        columns: 4
                        Layout.fillWidth: true
                        rowSpacing: 6
                        columnSpacing: 8

                        Label {
                            text: "Device Noise Operators"
                            color: "#24292f"
                            Layout.columnSpan: 4
                        }

                        Column {
                            Layout.columnSpan: 4
                            Layout.fillWidth: true
                            spacing: 6

                            Repeater {
                                model: panel.deviceOperators.length

                                RowLayout {
                                    required property int index

                                    width: parent.width
                                    spacing: 8

                                    TextField {
                                        Layout.fillWidth: true
                                        placeholderText: "Device name"
                                        text: panel.deviceOperators[parent.index].deviceName || ""
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

                            RowLayout {
                                Layout.fillWidth: true

                                Button {
                                    text: "Add Device Operator"
                                    onClicked: {
                                        var updatedOperators = panel.deviceOperators.slice();
                                        updatedOperators.push({deviceName: "", operatorType: "DNI", noiseSource: ""});
                                        panel.deviceOperators = updatedOperators;
                                    }
                                }

                                Item { Layout.fillWidth: true }
                            }
                        }

                        Label {
                            text: "Additional"
                            color: "#24292f"
                        }
                        TextField {
                            id: noisePrintSpecificVarsField
                            placeholderText: "e.g. V(1) I(V1)"
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
                            id: noisePrintFileField
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
                implicitHeight: noiseMeasureColumn.implicitHeight + 16
                color: "#f6f8fa"
                radius: 6
                border.color: "#d0d7de"
                border.width: 1

                ColumnLayout {
                    id: noiseMeasureColumn
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
                            id: noiseMeasureParametersTextArea
                            placeholderText: "Enter one .MEASURE directive per line.\nExample: .MEASURE NOISE TOTAL_INOISE INTEG INOISE"
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
