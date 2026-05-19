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

    // --- OP simulation properties
    property bool opPrintEnabled: false
    property bool opPrintAllNodes: false
    property bool opPrintAllCurrents: false
    property bool opPrintPower: false
    property bool opPrintBjtLeads: false
    property bool opPrintFetLeads: false
    property bool opHasBjtDevices: false
    property bool opHasFetDevices: false
    property string opPrintSpecificVars: ""
    property alias opPrintFormatIndex: opPrintFormatCombo.currentIndex
    property string opPrintFile: ""
    property bool saveEnabled: false
    property string saveType: "NODESET"
    property string saveFile: ""
    property string nodesetEntries: ""

    // --- Transient print properties ---
    property bool tranPrintEnabled: false
    property bool tranPrintAllNodes: false
    property bool tranPrintAllCurrents: false
    property bool tranPrintPower: false
    property bool tranPrintBjtLeads: false
    property bool tranPrintFetLeads: false
    property bool tranHasBjtDevices: false
    property bool tranHasFetDevices: false
    property string tranPrintSpecificVars: ""
    property alias tranPrintFormatIndex: tranPrintFormatCombo.currentIndex
    property string tranPrintFile: ""

    // --- DC Sweep print properties ---
    property bool dcPrintEnabled: false
    property bool dcPrintAllNodes: false
    property bool dcPrintAllCurrents: false
    property bool dcPrintPower: false
    property bool dcPrintBjtLeads: false
    property bool dcPrintFetLeads: false
    property bool dcHasBjtDevices: false
    property bool dcHasFetDevices: false
    property string dcPrintSpecificVars: ""
    property alias dcPrintFormatIndex: dcPrintFormatCombo.currentIndex
    property string dcPrintFile: ""

    // --- Shared properties ---
    property bool replaceGround: false

    signal submitTransient(string initialStep, string finalTime, string startTime, string stepCeiling, string opKeyword, bool scheduleEnabled, string schedulePairsText, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitDC(string sweepMode, string primaryVariable, string startValue, string stopValue, string stepValue, string pointsValue, string listValuesText, string dataTableName, bool secondaryEnabled, string secondaryVariable, string secondaryStart, string secondaryStop, string secondaryStep, string secondaryPoints, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitOP(bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool saveEnabled, string saveType, string nodesetEntries, string saveFile, bool replaceGround)
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
                                    checked: root.opPrintEnabled
                                    onCheckedChanged: root.opPrintEnabled = checked
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    enabled: opPrintEnabledCheckBox.checked
                                    Layout.fillWidth: true
                                    spacing: 16
                                    CheckBox {
                                        text: "All voltages  V(*)"
                                        checked: root.opPrintAllNodes
                                        onCheckedChanged: root.opPrintAllNodes = checked
                                    }
                                    CheckBox {
                                        text: "All currents  I(*)"
                                        checked: root.opPrintAllCurrents
                                        onCheckedChanged: root.opPrintAllCurrents = checked
                                    }
                                    CheckBox {
                                        text: "Power  P(*)"
                                        checked: root.opPrintPower
                                        onCheckedChanged: root.opPrintPower = checked
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                RowLayout {
                                    enabled: opPrintEnabledCheckBox.checked
                                    visible: root.opHasBjtDevices || root.opHasFetDevices
                                    Layout.fillWidth: true
                                    spacing: 16
                                    CheckBox {
                                        text: "BJT leads  IB(*) IC(*) IE(*) IS(*)"
                                        visible: root.opHasBjtDevices
                                        checked: root.opPrintBjtLeads
                                        onCheckedChanged: root.opPrintBjtLeads = checked
                                    }
                                    CheckBox {
                                        text: "FET leads  IB(*) ID(*) IG(*) IS(*)"
                                        visible: root.opHasFetDevices
                                        checked: root.opPrintFetLeads
                                        onCheckedChanged: root.opPrintFetLeads = checked
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
                                        text: root.opPrintSpecificVars
                                        onTextChanged: root.opPrintSpecificVars = text
                                        Layout.columnSpan: 3
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: "Format"
                                        color: "#24292f"
                                    }
                                    ComboBox {
                                        id: opPrintFormatCombo
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
                                        text: root.opPrintFile
                                        onTextChanged: root.opPrintFile = text
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
                                    checked: root.saveEnabled
                                    onCheckedChanged: root.saveEnabled = checked
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    enabled: saveEnabledCheckBox.checked
                                    Layout.fillWidth: true
                                    spacing: 16
                                    RadioButton {
                                        text: "Save as .IC"
                                        checked: root.saveType === "IC"
                                        onClicked: root.saveType = "IC"
                                    }
                                    RadioButton {
                                        text: "Save as .NODESET"
                                        checked: root.saveType === "NODESET"
                                        onClicked: root.saveType = "NODESET"
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
                                        text: root.saveFile
                                        onTextChanged: root.saveFile = text
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
                                    text: root.nodesetEntries
                                    onTextChanged: root.nodesetEntries = text
                                    Layout.fillWidth: true
                                }
                            }
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

                        // --- .TRAN section ---
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: tranParamsColumn.implicitHeight + 16
                            color: "#f6f8fa"
                            radius: 6
                            border.color: "#d0d7de"
                            border.width: 1

                            ColumnLayout {
                                id: tranParamsColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 8
                                spacing: 6

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 4
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
                                        placeholderText: "optional"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: "Operating Point"
                                        color: "#24292f"
                                    }
                                    ComboBox {
                                        id: opModeComboBox
                                        Layout.columnSpan: 3
                                        Layout.fillWidth: true
                                        model: ["Default (compute OP)", "NOOP (skip OP)", "UIC (skip OP)"]
                                    }

                                    Label {
                                        text: root.transientErrorText
                                        visible: root.transientErrorText.length > 0
                                        color: "#b42318"
                                        font.pixelSize: 12
                                        wrapMode: Text.Wrap
                                        Layout.columnSpan: 4
                                        Layout.fillWidth: true
                                    }
                                }

                                CheckBox {
                                    id: scheduleEnabledCheckBox
                                    text: "Enable schedule(time, max_step, ...)"
                                    Layout.fillWidth: true
                                }

                                TextArea {
                                    id: schedulePairsTextArea
                                    enabled: scheduleEnabledCheckBox.checked
                                    placeholderText: "Enter pairs as time,max_step values.\nExample: 0.5e-3,0 1e-3,1e-6 2e-3,0"
                                    selectByMouse: true
                                    wrapMode: TextEdit.Wrap
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 72
                                }
                            }
                        }

                        // --- .PRINT TRAN section ---
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: tranPrintColumn.implicitHeight + 16
                            color: "#f6f8fa"
                            radius: 6
                            border.color: "#d0d7de"
                            border.width: 1

                            ColumnLayout {
                                id: tranPrintColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 8
                                spacing: 10

                                CheckBox {
                                    id: tranPrintEnabledCheckBox
                                    text: "Enable .PRINT TRAN output"
                                    checked: root.tranPrintEnabled
                                    onCheckedChanged: root.tranPrintEnabled = checked
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    enabled: tranPrintEnabledCheckBox.checked
                                    Layout.fillWidth: true
                                    spacing: 16
                                    CheckBox {
                                        text: "All voltages  V(*)"
                                        checked: root.tranPrintAllNodes
                                        onCheckedChanged: root.tranPrintAllNodes = checked
                                    }
                                    CheckBox {
                                        text: "All currents  I(*)"
                                        checked: root.tranPrintAllCurrents
                                        onCheckedChanged: root.tranPrintAllCurrents = checked
                                    }
                                    CheckBox {
                                        text: "Power  P(*)"
                                        checked: root.tranPrintPower
                                        onCheckedChanged: root.tranPrintPower = checked
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                RowLayout {
                                    enabled: tranPrintEnabledCheckBox.checked
                                    visible: root.tranHasBjtDevices || root.tranHasFetDevices
                                    Layout.fillWidth: true
                                    spacing: 16
                                    CheckBox {
                                        text: "BJT leads  IB(*) IC(*) IE(*) IS(*)"
                                        visible: root.tranHasBjtDevices
                                        checked: root.tranPrintBjtLeads
                                        onCheckedChanged: root.tranPrintBjtLeads = checked
                                    }
                                    CheckBox {
                                        text: "FET leads  IB(*) ID(*) IG(*) IS(*)"
                                        visible: root.tranHasFetDevices
                                        checked: root.tranPrintFetLeads
                                        onCheckedChanged: root.tranPrintFetLeads = checked
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                GridLayout {
                                    enabled: tranPrintEnabledCheckBox.checked
                                    columns: 4
                                    Layout.fillWidth: true
                                    rowSpacing: 6
                                    columnSpacing: 8

                                    Label {
                                        text: "Additional"
                                        color: "#24292f"
                                    }
                                    TextField {
                                        id: tranPrintSpecificVarsField
                                        placeholderText: "e.g. V(1) I(R1) — pre-filled from netlist"
                                        selectByMouse: true
                                        text: root.tranPrintSpecificVars
                                        onTextChanged: root.tranPrintSpecificVars = text
                                        Layout.columnSpan: 3
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: "Format"
                                        color: "#24292f"
                                    }
                                    ComboBox {
                                        id: tranPrintFormatCombo
                                        Layout.fillWidth: true
                                        model: ["(default)", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]
                                    }

                                    Label {
                                        text: "Output File"
                                        color: "#24292f"
                                    }
                                    TextField {
                                        id: tranPrintFileField
                                        placeholderText: "optional (e.g. output.raw)"
                                        selectByMouse: true
                                        text: root.tranPrintFile
                                        onTextChanged: root.tranPrintFile = text
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

                // --- Tab 1: DC Sweep ---
                ScrollView {
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

                                // secondary sweep (visible only for supported sweep modes)
                                ColumnLayout {
                                    visible: root.supportsSecondary()
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

                                Label {
                                    text: root.dcErrorText
                                    visible: root.dcErrorText.length > 0
                                    color: "#b42318"
                                    font.pixelSize: 12
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
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
                                    checked: root.dcPrintEnabled
                                    onCheckedChanged: root.dcPrintEnabled = checked
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    enabled: dcPrintEnabledCheckBox.checked
                                    Layout.fillWidth: true
                                    spacing: 16
                                    CheckBox {
                                        text: "All voltages  V(*)"
                                        checked: root.dcPrintAllNodes
                                        onCheckedChanged: root.dcPrintAllNodes = checked
                                    }
                                    CheckBox {
                                        text: "All currents  I(*)"
                                        checked: root.dcPrintAllCurrents
                                        onCheckedChanged: root.dcPrintAllCurrents = checked
                                    }
                                    CheckBox {
                                        text: "Power  P(*)"
                                        checked: root.dcPrintPower
                                        onCheckedChanged: root.dcPrintPower = checked
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                RowLayout {
                                    enabled: dcPrintEnabledCheckBox.checked
                                    visible: root.dcHasBjtDevices || root.dcHasFetDevices
                                    Layout.fillWidth: true
                                    spacing: 16
                                    CheckBox {
                                        text: "BJT leads  IB(*) IC(*) IE(*) IS(*)"
                                        visible: root.dcHasBjtDevices
                                        checked: root.dcPrintBjtLeads
                                        onCheckedChanged: root.dcPrintBjtLeads = checked
                                    }
                                    CheckBox {
                                        text: "FET leads  IB(*) ID(*) IG(*) IS(*)"
                                        visible: root.dcHasFetDevices
                                        checked: root.dcPrintFetLeads
                                        onCheckedChanged: root.dcPrintFetLeads = checked
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
                                        text: root.dcPrintSpecificVars
                                        onTextChanged: root.dcPrintSpecificVars = text
                                        Layout.columnSpan: 3
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: "Format"
                                        color: "#24292f"
                                    }
                                    ComboBox {
                                        id: dcPrintFormatCombo
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
                                        text: root.dcPrintFile
                                        onTextChanged: root.dcPrintFile = text
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
        }

        CheckBox {
            id: replaceGroundCheckBox
            text: "Replace ground node (.PREPROCESS REPLACEGROUND TRUE)"
            checked: root.replaceGround
            onCheckedChanged: root.replaceGround = checked
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: "Cancel"
                onClicked: root.cancelRequested()
            }

            Button {
                text: "Apply"
                highlighted: true
                onClicked: {
                    if (simTabBar.currentIndex === 0) {
                        root.submitOP(root.opPrintEnabled, root.opPrintAllNodes, root.opPrintAllCurrents, root.opPrintPower, root.opPrintBjtLeads, root.opPrintFetLeads, root.opPrintSpecificVars, opPrintFormatCombo.currentIndex > 0 ? opPrintFormatCombo.model[opPrintFormatCombo.currentIndex] : "", root.opPrintFile, root.saveEnabled, root.saveType, root.nodesetEntries, root.saveFile, root.replaceGround)
                    } else if (simTabBar.currentIndex === 1) {
                        root.submitTransient(root.initialStep, root.finalTime, root.startTime, root.stepCeiling, root.opKeywordValue(), root.scheduleEnabled, root.schedulePairsText, root.tranPrintEnabled, root.tranPrintAllNodes, root.tranPrintAllCurrents, root.tranPrintPower, root.tranPrintBjtLeads, root.tranPrintFetLeads, root.tranPrintSpecificVars, tranPrintFormatCombo.currentIndex > 0 ? tranPrintFormatCombo.model[tranPrintFormatCombo.currentIndex] : "", root.tranPrintFile, root.replaceGround)
                    } else {
                        root.submitDC(root.sweepModeValue(), root.primaryVariable, root.startValue, root.stopValue, root.stepValue, root.pointsValue, root.listValuesText, root.dataTableName, root.secondaryEnabled, root.secondaryVariable, root.secondaryStart, root.secondaryStop, root.secondaryStep, root.secondaryPoints, root.dcPrintEnabled, root.dcPrintAllNodes, root.dcPrintAllCurrents, root.dcPrintPower, root.dcPrintBjtLeads, root.dcPrintFetLeads, root.dcPrintSpecificVars, dcPrintFormatCombo.currentIndex > 0 ? dcPrintFormatCombo.model[dcPrintFormatCombo.currentIndex] : "", root.dcPrintFile, root.replaceGround)
                    }
                }
            }
        }
    }
}
