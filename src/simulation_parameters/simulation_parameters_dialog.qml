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
    property alias fftParametersText: fftParametersTextArea.text
    property alias fourParametersText: fourParametersTextArea.text
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

    // --- AC sweep tab properties ---
    property alias acSweepModeIndex: acSweepModeComboBox.currentIndex
    property alias acPoints: acPointsField.text
    property alias acStart: acStartField.text
    property alias acEnd: acEndField.text
    property alias acDataTableName: acDataTableNameField.text
    property string acErrorText: ""

    // --- AC print properties ---
    property bool acPrintEnabled: false
    property bool acPrintAllNodes: false
    property bool acPrintAllCurrents: false
    property string acPrintSpecificVars: ""
    property alias acPrintFormatIndex: acPrintFormatCombo.currentIndex
    property string acPrintFile: ""

    // --- NOISE sweep tab properties ---
    property alias noiseOutputNode: noiseOutputNodeField.text
    property alias noiseRefNode: noiseRefNodeField.text
    property alias noiseSourceName: noiseSourceNameField.text
    property alias noiseSweepModeIndex: noiseSweepModeComboBox.currentIndex
    property alias noisePoints: noisePointsField.text
    property alias noiseStart: noiseStartField.text
    property alias noiseEnd: noiseEndField.text
    property alias noiseDataTableName: noiseDataTableNameField.text
    property string noiseErrorText: ""

    // --- NOISE print properties ---
    property bool noisePrintEnabled: false
    property bool noisePrintAllNodes: false
    property bool noisePrintAllCurrents: false
    property bool noisePrintInoise: false
    property bool noisePrintOnoise: false
    property string noisePrintSpecificVars: ""
    property alias noisePrintFormatIndex: noisePrintFormatCombo.currentIndex
    property string noisePrintFile: ""
    property var noiseDeviceOperators: []

    // --- HB tab properties ---
    property alias hbFrequenciesText: hbFrequenciesField.text
    property string hbErrorText: ""

    // --- HB print properties ---
    property bool hbPrintEnabled: false
    property bool hbPrintAllNodes: false
    property bool hbPrintAllCurrents: false
    property alias hbPrintTypeIndex: hbPrintTypeComboBox.currentIndex
    property string hbPrintSpecificVars: ""
    property alias hbPrintFormatIndex: hbPrintFormatCombo.currentIndex
    property string hbPrintFile: ""

    // --- LIN tab properties ---
    property bool linSparcalc: true
    property string linFormat: "TOUCHSTONE2"
    property string linType: "S"
    property string linDataFormat: "RI"
    property string linFile: ""
    property string linWidth: ""
    property string linPrecision: ""
    property alias linSweepModeIndex: linSweepModeComboBox.currentIndex
    property alias linPoints: linPointsField.text
    property alias linStart: linStartField.text
    property alias linEnd: linEndField.text
    property alias linDataTableName: linDataTableNameField.text
    property string linErrorText: ""

    // --- LIN print properties ---
    property bool linPrintEnabled: false
    property bool linPrintAllNodes: false
    property bool linPrintAllCurrents: false
    property string linPrintSpecificVars: ""
    property alias linPrintFormatIndex: linPrintFormatCombo.currentIndex
    property string linPrintFile: ""

    // --- Shared properties ---
    property bool replaceGround: false

    signal submitTransient(string initialStep, string finalTime, string startTime, string stepCeiling, string opKeyword, bool scheduleEnabled, string schedulePairsText, string fftParametersText, string fourParametersText, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitDC(string sweepMode, string primaryVariable, string startValue, string stopValue, string stepValue, string pointsValue, string listValuesText, string dataTableName, bool secondaryEnabled, string secondaryVariable, string secondaryStart, string secondaryStop, string secondaryStep, string secondaryPoints, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitOP(bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printPower, bool printBjtLeads, bool printFetLeads, string printSpecificVars, string printFormat, string printFile, bool saveEnabled, string saveType, string nodesetEntries, string saveFile, bool replaceGround)
    signal submitAC(string sweepMode, string points, string start, string end, string dataTableName, bool printEnabled, bool printAllNodes, bool printAllCurrents, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitNoise(string outputNode, string refNode, string sourceName, string sweepMode, string points, string start, string end, string dataTableName, bool printEnabled, bool printAllNodes, bool printAllCurrents, bool printInoise, bool printOnoise, string printSpecificVars, string printFormat, string printFile, bool replaceGround, var deviceOperatorsList)
    signal submitHB(string frequenciesText, bool printEnabled, bool printAllNodes, bool printAllCurrents, string printType, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
    signal submitLIN(bool sparcalc, string format, string lintype, string dataformat, string file, string width, string precision, string sweepMode, string points, string start, string end, string dataTableName, bool printEnabled, bool printAllNodes, bool printAllCurrents, string printSpecificVars, string printFormat, string printFile, bool replaceGround)
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

    function acSweepModeValue() {
        // map ac combo box index to the sweep mode keyword emitted in the netlist
        var modes = ["LIN", "DEC", "OCT", "DATA"]
        return modes[acSweepModeComboBox.currentIndex] || "LIN"
    }

    function acIsDataMode() {
        // true when the selected ac sweep mode is DATA
        return acSweepModeComboBox.currentIndex === 3
    }

    function noiseSweepModeValue() {
        // map noise combo box index to the sweep mode keyword emitted in the netlist
        var modes = ["LIN", "DEC", "OCT", "DATA"]
        return modes[noiseSweepModeComboBox.currentIndex] || "LIN"
    }

    function noiseIsDataMode() {
        // true when the selected noise sweep mode is DATA
        return noiseSweepModeComboBox.currentIndex === 3
    }

    function hbPrintTypeValue() {
        // map hb combo box index to the print subtype emitted in the netlist
        var types = ["HB", "HB_FD", "HB_TD"]
        return types[hbPrintTypeComboBox.currentIndex] || "HB"
    }

    function linSweepModeValue() {
        // map lin combo box index to the sweep mode keyword emitted in the netlist
        var modes = ["LIN", "DEC", "OCT", "DATA"]
        return modes[linSweepModeComboBox.currentIndex] || "LIN"
    }

    function linIsDataMode() {
        // true when the selected lin sweep mode is DATA
        return linSweepModeComboBox.currentIndex === 3
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
                text: ".OP"
                // bind to root width (minus margins) to avoid circular dependency with TabBar's own width
                width: (root.width - 45) / 7
            }

            TabButton {
                text: ".TRAN"
                // bind to root width (minus margins) to avoid circular dependency with TabBar's own width
                width: (root.width - 45) / 7
            }

            TabButton {
                text: ".DC"
                // bind to root width (minus margins) to avoid circular dependency with TabBar's own width
                width: (root.width - 45) / 7
            }

            TabButton {
                text: ".AC"
                // bind to root width (minus margins) to avoid circular dependency with TabBar's own width
                width: (root.width - 45) / 7
            }

            TabButton {
                text: ".NOISE"
                // bind to root width (minus margins) to avoid circular dependency with TabBar's own width
                width: (root.width - 45) / 7
            }

            TabButton {
                text: ".HB"
                // bind to root width (minus margins) to avoid circular dependency with TabBar's own width
                width: (root.width - 45) / 7
            }

            TabButton {
                text: ".LIN"
                // bind to root width (minus margins) to avoid circular dependency with TabBar's own width
                width: (root.width - 45) / 7
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

                        // --- .FFT section ---
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: fftParamsColumn.implicitHeight + 16
                            color: "#f6f8fa"
                            radius: 6
                            border.color: "#d0d7de"
                            border.width: 1

                            ColumnLayout {
                                id: fftParamsColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 8
                                spacing: 6

                                Label {
                                    text: "Fast Fourier Transform (.FFT)"
                                    font.bold: true
                                    color: "#24292f"
                                    Layout.fillWidth: true
                                }

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 120
                                    clip: true

                                    TextArea {
                                        id: fftParametersTextArea
                                        placeholderText: "Enter one .FFT directive per line.\nExample: .FFT V(OUT) WINDOW=HANN"
                                        selectByMouse: true
                                        wrapMode: TextEdit.NoWrap
                                    }
                                }
                            }
                        }

                        // --- .FOUR section ---
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: fourParamsColumn.implicitHeight + 16
                            color: "#f6f8fa"
                            radius: 6
                            border.color: "#d0d7de"
                            border.width: 1

                            ColumnLayout {
                                id: fourParamsColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 8
                                spacing: 6

                                Label {
                                    text: "Fourier Analysis (.FOUR)"
                                    font.bold: true
                                    color: "#24292f"
                                    Layout.fillWidth: true
                                }

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 120
                                    clip: true

                                    TextArea {
                                        id: fourParametersTextArea
                                        placeholderText: "Enter one .FOUR directive per line.\nExample: .FOUR 1k V(OUT)"
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

                // --- Tab 3: AC Sweep ---
                ScrollView {
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        // --- .AC section ---
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: acParamsColumn.implicitHeight + 16
                            color: "#f6f8fa"
                            radius: 6
                            border.color: "#d0d7de"
                            border.width: 1

                            ColumnLayout {
                                id: acParamsColumn
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
                                        id: acSweepModeComboBox
                                        Layout.fillWidth: true
                                        model: ["LIN (linear)", "DEC (per decade)", "OCT (per octave)", "DATA (table-driven)"]
                                    }

                                    Label {
                                        text: "Points *"
                                        color: "#24292f"
                                        visible: !root.acIsDataMode()
                                    }
                                    TextField {
                                        id: acPointsField
                                        placeholderText: "e.g. 100"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: !root.acIsDataMode()
                                    }

                                    Label {
                                        text: "Start Frequency *"
                                        color: "#24292f"
                                        visible: !root.acIsDataMode()
                                    }
                                    TextField {
                                        id: acStartField
                                        placeholderText: "e.g. 1"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: !root.acIsDataMode()
                                    }

                                    Label {
                                        text: "End Frequency *"
                                        color: "#24292f"
                                        visible: !root.acIsDataMode()
                                    }
                                    TextField {
                                        id: acEndField
                                        placeholderText: "e.g. 1MEG"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: !root.acIsDataMode()
                                    }

                                    Label {
                                        text: "Data Table Name *"
                                        color: "#24292f"
                                        visible: root.acIsDataMode()
                                    }
                                    TextField {
                                        id: acDataTableNameField
                                        placeholderText: "e.g. freqTable"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: root.acIsDataMode()
                                    }
                                }

                                Label {
                                    text: root.acErrorText
                                    visible: root.acErrorText.length > 0
                                    color: "#b42318"
                                    font.pixelSize: 12
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        // --- .PRINT AC section ---
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: acPrintColumn.implicitHeight + 16
                            color: "#f6f8fa"
                            radius: 6
                            border.color: "#d0d7de"
                            border.width: 1

                            ColumnLayout {
                                id: acPrintColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 8
                                spacing: 6

                                CheckBox {
                                    id: acPrintEnabledCheckBox
                                    text: "Enable .PRINT AC output"
                                    checked: root.acPrintEnabled
                                    onCheckedChanged: root.acPrintEnabled = checked
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    enabled: acPrintEnabledCheckBox.checked
                                    Layout.fillWidth: true
                                    spacing: 16
                                    CheckBox {
                                        text: "All voltages  V(*)"
                                        checked: root.acPrintAllNodes
                                        onCheckedChanged: root.acPrintAllNodes = checked
                                    }
                                    CheckBox {
                                        text: "All currents  I(*)"
                                        checked: root.acPrintAllCurrents
                                        onCheckedChanged: root.acPrintAllCurrents = checked
                                    }
                                    Item { Layout.fillWidth: true }
                                }

                                GridLayout {
                                    enabled: acPrintEnabledCheckBox.checked
                                    columns: 4
                                    Layout.fillWidth: true
                                    rowSpacing: 6
                                    columnSpacing: 8

                                    Label {
                                        text: "Additional"
                                        color: "#24292f"
                                    }
                                    TextField {
                                        id: acPrintSpecificVarsField
                                        placeholderText: "e.g. VR(out) VM(1) IP(V1)"
                                        selectByMouse: true
                                        text: root.acPrintSpecificVars
                                        onTextChanged: root.acPrintSpecificVars = text
                                        Layout.columnSpan: 3
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: "Format"
                                        color: "#24292f"
                                    }
                                    ComboBox {
                                        id: acPrintFormatCombo
                                        Layout.fillWidth: true
                                        model: ["(default)", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]
                                    }

                                    Label {
                                        text: "Output File"
                                        color: "#24292f"
                                    }
                                    TextField {
                                        id: acPrintFileField
                                        placeholderText: "optional (e.g. output.raw)"
                                        selectByMouse: true
                                        text: root.acPrintFile
                                        onTextChanged: root.acPrintFile = text
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

                // --- Tab 4: Noise ---
                ScrollView {
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
                                        visible: !root.noiseIsDataMode()
                                    }
                                    TextField {
                                        id: noisePointsField
                                        placeholderText: "e.g. 100"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: !root.noiseIsDataMode()
                                    }

                                    Label {
                                        text: "Start Frequency *"
                                        color: "#24292f"
                                        visible: !root.noiseIsDataMode()
                                    }
                                    TextField {
                                        id: noiseStartField
                                        placeholderText: "e.g. 1"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: !root.noiseIsDataMode()
                                    }

                                    Label {
                                        text: "End Frequency *"
                                        color: "#24292f"
                                        visible: !root.noiseIsDataMode()
                                    }
                                    TextField {
                                        id: noiseEndField
                                        placeholderText: "e.g. 1MEG"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: !root.noiseIsDataMode()
                                    }

                                    Label {
                                        text: "Data Table Name *"
                                        color: "#24292f"
                                        visible: root.noiseIsDataMode()
                                    }
                                    TextField {
                                        id: noiseDataTableNameField
                                        placeholderText: "e.g. freqTable"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: root.noiseIsDataMode()
                                    }
                                }

                                Label {
                                    text: root.noiseErrorText
                                    visible: root.noiseErrorText.length > 0
                                    color: "#b42318"
                                    font.pixelSize: 12
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
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
                                    checked: root.noisePrintEnabled
                                    onCheckedChanged: root.noisePrintEnabled = checked
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    enabled: noisePrintEnabledCheckBox.checked
                                    Layout.fillWidth: true
                                    spacing: 16
                                    CheckBox {
                                        text: "All voltages  V(*)"
                                        checked: root.noisePrintAllNodes
                                        onCheckedChanged: root.noisePrintAllNodes = checked
                                    }
                                    CheckBox {
                                        text: "All currents  I(*)"
                                        checked: root.noisePrintAllCurrents
                                        onCheckedChanged: root.noisePrintAllCurrents = checked
                                    }
                                    CheckBox {
                                        text: "INOISE"
                                        checked: root.noisePrintInoise
                                        onCheckedChanged: root.noisePrintInoise = checked
                                    }
                                    CheckBox {
                                        text: "ONOISE"
                                        checked: root.noisePrintOnoise
                                        onCheckedChanged: root.noisePrintOnoise = checked
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
                                            model: root.noiseDeviceOperators.length

                                            RowLayout {
                                                required property int index

                                                width: parent.width
                                                spacing: 8

                                                TextField {
                                                    Layout.fillWidth: true
                                                    placeholderText: "Device name"
                                                    text: root.noiseDeviceOperators[index].deviceName || ""
                                                    onTextChanged: {
                                                        if (text !== root.noiseDeviceOperators[index].deviceName) {
                                                            var updatedOperators = root.noiseDeviceOperators.slice();
                                                            updatedOperators[index].deviceName = text;
                                                            root.noiseDeviceOperators = updatedOperators;
                                                        }
                                                    }
                                                }

                                                ComboBox {
                                                    Layout.preferredWidth: 80
                                                    model: ["DNI", "DNO"]
                                                    currentIndex: root.noiseDeviceOperators[index].operatorType === "DNI" ? 0 : 1
                                                    onActivated: function(activatedIndex) {
                                                        var updatedOperators = root.noiseDeviceOperators.slice();
                                                        updatedOperators[index].operatorType = model[activatedIndex];
                                                        root.noiseDeviceOperators = updatedOperators;
                                                    }
                                                }

                                                TextField {
                                                    Layout.preferredWidth: 120
                                                    placeholderText: "Noise source (optional)"
                                                    text: root.noiseDeviceOperators[index].noiseSource || ""
                                                    onTextChanged: {
                                                        if (text !== (root.noiseDeviceOperators[index].noiseSource || "")) {
                                                            var updatedOperators = root.noiseDeviceOperators.slice();
                                                            updatedOperators[index].noiseSource = text;
                                                            root.noiseDeviceOperators = updatedOperators;
                                                        }
                                                    }
                                                }

                                                Button {
                                                    Layout.preferredWidth: 30
                                                    text: "×"
                                                    onClicked: {
                                                        var updatedOperators = root.noiseDeviceOperators.slice();
                                                        updatedOperators.splice(index, 1);
                                                        root.noiseDeviceOperators = updatedOperators;
                                                    }
                                                }
                                            }
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true

                                            Button {
                                                text: "Add Device Operator"
                                                onClicked: {
                                                    var updatedOperators = root.noiseDeviceOperators.slice();
                                                    updatedOperators.push({deviceName: "", operatorType: "DNI", noiseSource: ""});
                                                    root.noiseDeviceOperators = updatedOperators;
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
                                        text: root.noisePrintSpecificVars
                                        onTextChanged: root.noisePrintSpecificVars = text
                                        Layout.columnSpan: 3
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: "Format"
                                        color: "#24292f"
                                    }
                                    ComboBox {
                                        id: noisePrintFormatCombo
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
                                        text: root.noisePrintFile
                                        onTextChanged: root.noisePrintFile = text
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

                // --- Tab 5: HB ---
                ScrollView {
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
                                        text: root.hbErrorText
                                        visible: root.hbErrorText.length > 0
                                        color: "#b42318"
                                        font.pixelSize: 12
                                        wrapMode: Text.Wrap
                                        Layout.columnSpan: 2
                                        Layout.fillWidth: true
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
                                    checked: root.hbPrintEnabled
                                    onCheckedChanged: root.hbPrintEnabled = checked
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    enabled: hbPrintEnabledCheckBox.checked
                                    Layout.fillWidth: true
                                    spacing: 16
                                    CheckBox {
                                        text: "All voltages  V(*)"
                                        checked: root.hbPrintAllNodes
                                        onCheckedChanged: root.hbPrintAllNodes = checked
                                    }
                                    CheckBox {
                                        text: "All currents  I(*)"
                                        checked: root.hbPrintAllCurrents
                                        onCheckedChanged: root.hbPrintAllCurrents = checked
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
                                        text: root.hbPrintSpecificVars
                                        onTextChanged: root.hbPrintSpecificVars = text
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
                                        id: hbPrintFormatCombo
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
                                        text: root.hbPrintFile
                                        onTextChanged: root.hbPrintFile = text
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

                // --- Tab 6: LIN ---
                ScrollView {
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
                                    checked: root.linSparcalc
                                    onCheckedChanged: root.linSparcalc = checked
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
                                        currentIndex: root.linFormat === "TOUCHSTONE" ? 1 : 0
                                        onCurrentIndexChanged: root.linFormat = currentIndex === 1 ? "TOUCHSTONE" : "TOUCHSTONE2"
                                    }

                                    Label {
                                        text: "Parameter Type"
                                        color: "#24292f"
                                    }
                                    ComboBox {
                                        id: linTypeComboBox
                                        Layout.fillWidth: true
                                        model: ["S", "Y", "Z"]
                                        currentIndex: root.linType === "Y" ? 1 : root.linType === "Z" ? 2 : 0
                                        onCurrentIndexChanged: root.linType = currentIndex === 1 ? "Y" : currentIndex === 2 ? "Z" : "S"
                                    }

                                    Label {
                                        text: "Data Format"
                                        color: "#24292f"
                                    }
                                    ComboBox {
                                        id: linDataFormatComboBox
                                        Layout.fillWidth: true
                                        model: ["RI", "MA", "DB"]
                                        currentIndex: root.linDataFormat === "MA" ? 1 : root.linDataFormat === "DB" ? 2 : 0
                                        onCurrentIndexChanged: root.linDataFormat = currentIndex === 1 ? "MA" : currentIndex === 2 ? "DB" : "RI"
                                    }

                                    Label {
                                        text: "Output File"
                                        color: "#24292f"
                                    }
                                    TextField {
                                        id: linFileField
                                        placeholderText: "optional output file name"
                                        selectByMouse: true
                                        text: root.linFile
                                        onTextChanged: root.linFile = text
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
                                        text: root.linWidth
                                        onTextChanged: root.linWidth = text
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
                                        text: root.linPrecision
                                        onTextChanged: root.linPrecision = text
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
                                        visible: !root.linIsDataMode()
                                    }
                                    TextField {
                                        id: linPointsField
                                        placeholderText: "e.g. 100"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: !root.linIsDataMode()
                                    }

                                    Label {
                                        text: "Start Frequency *"
                                        color: "#24292f"
                                        visible: !root.linIsDataMode()
                                    }
                                    TextField {
                                        id: linStartField
                                        placeholderText: "e.g. 1"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: !root.linIsDataMode()
                                    }

                                    Label {
                                        text: "End Frequency *"
                                        color: "#24292f"
                                        visible: !root.linIsDataMode()
                                    }
                                    TextField {
                                        id: linEndField
                                        placeholderText: "e.g. 1MEG"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: !root.linIsDataMode()
                                    }

                                    Label {
                                        text: "Data Table Name *"
                                        color: "#24292f"
                                        visible: root.linIsDataMode()
                                    }
                                    TextField {
                                        id: linDataTableNameField
                                        placeholderText: "e.g. freqTable"
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        visible: root.linIsDataMode()
                                    }
                                }

                                Label {
                                    text: root.linErrorText
                                    visible: root.linErrorText.length > 0
                                    color: "#b42318"
                                    font.pixelSize: 12
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
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
                                    text: "Enable .PRINT AC output"
                                    checked: root.linPrintEnabled
                                    onCheckedChanged: root.linPrintEnabled = checked
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    enabled: linPrintEnabledCheckBox.checked
                                    Layout.fillWidth: true
                                    spacing: 16
                                    CheckBox {
                                        text: "All voltages  V(*)"
                                        checked: root.linPrintAllNodes
                                        onCheckedChanged: root.linPrintAllNodes = checked
                                    }
                                    CheckBox {
                                        text: "All currents  I(*)"
                                        checked: root.linPrintAllCurrents
                                        onCheckedChanged: root.linPrintAllCurrents = checked
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
                                        text: root.linPrintSpecificVars
                                        onTextChanged: root.linPrintSpecificVars = text
                                        Layout.columnSpan: 3
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: "Format"
                                        color: "#24292f"
                                    }
                                    ComboBox {
                                        id: linPrintFormatCombo
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
                                        text: root.linPrintFile
                                        onTextChanged: root.linPrintFile = text
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
                        root.submitTransient(root.initialStep, root.finalTime, root.startTime, root.stepCeiling, root.opKeywordValue(), root.scheduleEnabled, root.schedulePairsText, root.fftParametersText, root.fourParametersText, root.tranPrintEnabled, root.tranPrintAllNodes, root.tranPrintAllCurrents, root.tranPrintPower, root.tranPrintBjtLeads, root.tranPrintFetLeads, root.tranPrintSpecificVars, tranPrintFormatCombo.currentIndex > 0 ? tranPrintFormatCombo.model[tranPrintFormatCombo.currentIndex] : "", root.tranPrintFile, root.replaceGround)
                    } else if (simTabBar.currentIndex === 2) {
                        root.submitDC(root.sweepModeValue(), root.primaryVariable, root.startValue, root.stopValue, root.stepValue, root.pointsValue, root.listValuesText, root.dataTableName, root.secondaryEnabled, root.secondaryVariable, root.secondaryStart, root.secondaryStop, root.secondaryStep, root.secondaryPoints, root.dcPrintEnabled, root.dcPrintAllNodes, root.dcPrintAllCurrents, root.dcPrintPower, root.dcPrintBjtLeads, root.dcPrintFetLeads, root.dcPrintSpecificVars, dcPrintFormatCombo.currentIndex > 0 ? dcPrintFormatCombo.model[dcPrintFormatCombo.currentIndex] : "", root.dcPrintFile, root.replaceGround)
                    } else if (simTabBar.currentIndex === 3) {
                        root.submitAC(root.acSweepModeValue(), root.acPoints, root.acStart, root.acEnd, root.acDataTableName, root.acPrintEnabled, root.acPrintAllNodes, root.acPrintAllCurrents, root.acPrintSpecificVars, acPrintFormatCombo.currentIndex > 0 ? acPrintFormatCombo.model[acPrintFormatCombo.currentIndex] : "", root.acPrintFile, root.replaceGround)
                    } else if (simTabBar.currentIndex === 4) {
                        root.submitNoise(root.noiseOutputNode, root.noiseRefNode, root.noiseSourceName, root.noiseSweepModeValue(), root.noisePoints, root.noiseStart, root.noiseEnd, root.noiseDataTableName, root.noisePrintEnabled, root.noisePrintAllNodes, root.noisePrintAllCurrents, root.noisePrintInoise, root.noisePrintOnoise, root.noisePrintSpecificVars, noisePrintFormatCombo.currentIndex > 0 ? noisePrintFormatCombo.model[noisePrintFormatCombo.currentIndex] : "", root.noisePrintFile, root.replaceGround, root.noiseDeviceOperators)
                    } else if (simTabBar.currentIndex === 5) {
                        root.submitHB(root.hbFrequenciesText, root.hbPrintEnabled, root.hbPrintAllNodes, root.hbPrintAllCurrents, root.hbPrintTypeValue(), root.hbPrintSpecificVars, hbPrintFormatCombo.currentIndex > 0 ? hbPrintFormatCombo.model[hbPrintFormatCombo.currentIndex] : "", root.hbPrintFile, root.replaceGround)
                    } else if (simTabBar.currentIndex === 6) {
                        root.submitLIN(root.linSparcalc, root.linFormat, root.linType, root.linDataFormat, root.linFile, root.linWidth, root.linPrecision, root.linSweepModeValue(), root.linPoints, root.linStart, root.linEnd, root.linDataTableName, root.linPrintEnabled, root.linPrintAllNodes, root.linPrintAllCurrents, root.linPrintSpecificVars, linPrintFormatCombo.currentIndex > 0 ? linPrintFormatCombo.model[linPrintFormatCombo.currentIndex] : "", root.linPrintFile, root.replaceGround)
                    }
                }
            }
        }
    }
}
