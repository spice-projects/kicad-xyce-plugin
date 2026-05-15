pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 640
    implicitHeight: 680

    // --- Tab persistence ---
    property int initialTabIndex: 0
    onInitialTabIndexChanged: simTabBar.currentIndex = initialTabIndex

    // --- Transient tab properties ---
    property alias initialStep: initialStepField.text
    property alias finalTime: finalTimeField.text
    property alias startTime: startTimeField.text
    property alias stepCeiling: stepCeilingField.text
    property alias opModeIndex: opModeComboBox.currentIndex
    property alias scheduleEnabled: scheduleEnabledCheckBox.checked
    property alias schedulePairsText: schedulePairsTextArea.text
    property string transientErrorText: ""

    // --- DC Sweep tab properties ---
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
    property string dcErrorText: ""

    signal submitTransient(string initialStep, string finalTime, string startTime, string stepCeiling, string opKeyword, bool scheduleEnabled, string schedulePairsText)
    signal submitDC(string sweepMode, string primaryVariable, string startValue, string stopValue, string stepValue, string pointsValue, string listValuesText, string dataTableName, bool secondaryEnabled, string secondaryVariable, string secondaryStart, string secondaryStop, string secondaryStep, string secondaryPoints)
    signal submitOP()
    signal cancelRequested()

    function opKeywordValue() {
        // map combo box index to the exact transient keyword emitted in the netlist
        if (opModeComboBox.currentIndex === 1)
            return "NOOP";
        // map combo box index to the exact transient keyword emitted in the netlist
        if (opModeComboBox.currentIndex === 2)
            return "UIC";
        // use empty token for default transient behavior
        return "";
    }

    function sweepModeValue() {
        // map combo box index to the sweep mode keyword emitted in the netlist
        var modes = ["LIN", "DEC", "OCT", "LIST", "DATA"]
        return modes[sweepModeComboBox.currentIndex] || "LIN"
    }

    function isLogMode() {
        // true when the selected mode uses a logarithmic point count
        return sweepModeComboBox.currentIndex === 1 || sweepModeComboBox.currentIndex === 2
    }

    function isLinMode() {
        // true when the selected mode is linear
        return sweepModeComboBox.currentIndex === 0
    }

    function isRangeMode() {
        // true when start/stop fields apply (LIN, DEC, OCT)
        return sweepModeComboBox.currentIndex <= 2
    }

    function isListMode() {
        // true when the selected mode is LIST
        return sweepModeComboBox.currentIndex === 3
    }

    function isDataMode() {
        // true when the selected mode is DATA
        return sweepModeComboBox.currentIndex === 4
    }

    function supportsSecondary() {
        // true when the selected mode supports a secondary sweep (LIN, DEC, OCT)
        return sweepModeComboBox.currentIndex <= 2
    }

    Rectangle {
        anchors.fill: parent
        color: "#efefe8"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        Label {
            text: "Xyce Simulation"
            font.pixelSize: 22
            font.bold: true
            color: "#1b1f23"
            Layout.fillWidth: true
        }

        Label {
            text: "Select a simulation type and configure its parameters"
            font.pixelSize: 13
            color: "#4a5560"
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        TabBar {
            id: simTabBar
            Layout.fillWidth: true

            TabButton {
                text: "Operating Point (.OP)"
                // bind to root width (minus margins) to avoid circular dependency with TabBar's own width
                width: (root.width - 40) / 3
            }

            TabButton {
                text: "Transient (.TRAN)"
                // bind to root width (minus margins) to avoid circular dependency with TabBar's own width
                width: (root.width - 40) / 3
            }

            TabButton {
                text: "DC Sweep (.DC)"
                // bind to root width (minus margins) to avoid circular dependency with TabBar's own width
                width: (root.width - 40) / 3
            }
        }

        Rectangle {
            color: "#ffffff"
            radius: 8
            border.color: "#d0d7de"
            border.width: 1
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                anchors.fill: parent
                anchors.margins: 16
                currentIndex: simTabBar.currentIndex

                // --- Tab 0: Operating Point ---
                ScrollView {
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        Label {
                            text: "Operating Point (.OP)"
                            font.pixelSize: 16
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Compute the DC operating point of the circuit. No additional parameters are required for this analysis."
                            color: "#4a5560"
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        Item {
                            Layout.fillHeight: true
                        }
                    }
                }

                // --- Tab 1: Transient ---
                ScrollView {
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
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
                                placeholderText: "optional (defaults to (final-start)/10)"
                                selectByMouse: true
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "Operating Point"
                                color: "#24292f"
                            }
                            ComboBox {
                                id: opModeComboBox
                                Layout.fillWidth: true
                                model: ["Default (compute OP)", "NOOP (skip OP)", "UIC (skip OP)"]
                            }

                            CheckBox {
                                id: scheduleEnabledCheckBox
                                text: "Enable schedule(time, max_step, ...)"
                                Layout.columnSpan: 2
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "Schedule Pairs"
                                color: "#24292f"
                                enabled: scheduleEnabledCheckBox.checked
                            }
                            TextArea {
                                id: schedulePairsTextArea
                                placeholderText: "Enter pairs as time,max_step values.\nExample: 0.5e-3,0 1e-3,1e-6 2e-3,0"
                                selectByMouse: true
                                enabled: scheduleEnabledCheckBox.checked
                                wrapMode: TextEdit.Wrap
                                Layout.fillWidth: true
                                Layout.preferredHeight: 96
                            }

                            Label {
                                text: root.transientErrorText
                                visible: root.transientErrorText.length > 0
                                color: "#b42318"
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                                Layout.columnSpan: 2
                                Layout.fillWidth: true
                            }
                        }

                        Item {
                            Layout.fillHeight: true
                        }
                    }
                }

                // --- Tab 1: DC Sweep ---
                ScrollView {
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

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
                                visible: !root.isDataMode()
                            }
                            TextField {
                                id: primaryVariableField
                                placeholderText: "e.g. VIN, R1, TEMP"
                                selectByMouse: true
                                Layout.fillWidth: true
                                visible: !root.isDataMode()
                            }

                            Label {
                                text: "Start *"
                                color: "#24292f"
                                visible: root.isRangeMode()
                            }
                            TextField {
                                id: startField
                                placeholderText: "e.g. 0"
                                selectByMouse: true
                                Layout.fillWidth: true
                                visible: root.isRangeMode()
                            }

                            Label {
                                text: "Stop *"
                                color: "#24292f"
                                visible: root.isRangeMode()
                            }
                            TextField {
                                id: stopField
                                placeholderText: "e.g. 5"
                                selectByMouse: true
                                Layout.fillWidth: true
                                visible: root.isRangeMode()
                            }

                            Label {
                                text: "Step *"
                                color: "#24292f"
                                visible: root.isLinMode()
                            }
                            TextField {
                                id: stepField
                                placeholderText: "e.g. 0.1"
                                selectByMouse: true
                                Layout.fillWidth: true
                                visible: root.isLinMode()
                            }

                            Label {
                                text: "Points *"
                                color: "#24292f"
                                visible: root.isLogMode()
                            }
                            TextField {
                                id: pointsField
                                placeholderText: "e.g. 10"
                                selectByMouse: true
                                Layout.fillWidth: true
                                visible: root.isLogMode()
                            }

                            Label {
                                text: "List Values *"
                                color: "#24292f"
                                visible: root.isListMode()
                            }
                            TextArea {
                                id: listValuesTextArea
                                placeholderText: "Enter values separated by spaces or commas.\nExample: 10 15 18 27 33"
                                selectByMouse: true
                                wrapMode: TextEdit.Wrap
                                Layout.fillWidth: true
                                Layout.preferredHeight: 72
                                visible: root.isListMode()
                            }

                            Label {
                                text: "Data Table Name *"
                                color: "#24292f"
                                visible: root.isDataMode()
                            }
                            TextField {
                                id: dataTableNameField
                                placeholderText: "e.g. resistorValues"
                                selectByMouse: true
                                Layout.fillWidth: true
                                visible: root.isDataMode()
                            }
                        }

                        Rectangle {
                            visible: root.supportsSecondary()
                            Layout.fillWidth: true
                            implicitHeight: secondaryColumn.implicitHeight + 16
                            color: "#f6f8fa"
                            radius: 6
                            border.color: "#d0d7de"
                            border.width: 1

                            ColumnLayout {
                                id: secondaryColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 8
                                spacing: 10

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
                                        visible: root.isLinMode()
                                    }
                                    TextField {
                                        id: secondaryStepField
                                        placeholderText: "e.g. 0.5"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: root.isLinMode()
                                    }

                                    Label {
                                        text: "Secondary Points *"
                                        color: "#24292f"
                                        visible: root.isLogMode()
                                    }
                                    TextField {
                                        id: secondaryPointsField
                                        placeholderText: "e.g. 5"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: root.isLogMode()
                                    }
                                }
                            }
                        }

                        Label {
                            text: root.dcErrorText
                            visible: root.dcErrorText.length > 0
                            color: "#b42318"
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        Item {
                            Layout.fillHeight: true
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: "Cancel"
                icon.source: "kicad-icons/cancel_24.png"
                icon.color: "transparent"
                icon.width: 24
                icon.height: 24
                onClicked: root.cancelRequested()
            }

            Button {
                text: "Apply"
                highlighted: true
                icon.source: "kicad-icons/checked_ok_24.png"
                icon.color: "transparent"
                icon.width: 24
                icon.height: 24
                onClicked: {
                    if (simTabBar.currentIndex === 0) {
                        root.submitOP()
                    } else if (simTabBar.currentIndex === 1) {
                        root.submitTransient(root.initialStep, root.finalTime, root.startTime, root.stepCeiling, root.opKeywordValue(), root.scheduleEnabled, root.schedulePairsText)
                    } else {
                        root.submitDC(root.sweepModeValue(), root.primaryVariable, root.startValue, root.stopValue, root.stepValue, root.pointsValue, root.listValuesText, root.dataTableName, root.secondaryEnabled, root.secondaryVariable, root.secondaryStart, root.secondaryStop, root.secondaryStep, root.secondaryPoints)
                    }
                }
            }
        }
    }
}
