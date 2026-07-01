pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root
    anchors.fill: parent

    // context properties injected by Python
    property var windowFunctions: []
    property var outputTypes: []
    property var fftPointOptions: []
    property real abscissaMin: 0
    property real abscissaMax: 0
    property real zoomFromTime: 0
    property real zoomToTime: 0
    property int defaultWindowIndex: 2
    property int defaultFftPointIndex: 3

    // expression multi-select state
    property var expressionItems: []
    property var selectionState: ({})
    property string filterText: ""
    readonly property var filteredItems: {
        var text = filterText.toLowerCase()
        if (text === "") return root.expressionItems
        return root.expressionItems.filter(function(e) {
            return String(e[0]).toLowerCase().indexOf(text) !== -1
        })
    }

    function initializeExpressions(items) {
        root.expressionItems = items
        var state = {}
        for (var i = 0; i < items.length; i++) {
            state[items[i][0]] = items[i][1]
        }
        root.selectionState = state
    }

    signal dialogAccepted(string windowFn, int npPoints, string output, bool normalize, string rangeMode, real customFrom, real customTo, bool keepDc)
    signal dialogRejected()
    signal selectionChanged(string expressionName, bool selected)

    // selected range mode: "all" | "zoom" | "custom"
    property string rangeMode: "all"
    property var rangeOptions: [
        { label: "All abscissa points", value: "all" },
        { label: "Current zoom / visible range", value: "zoom" },
        { label: "Custom time range", value: "custom" }
    ]

    Rectangle {
        anchors.fill: parent
        color: "#1a1b1e"
    }

    // -----------------------------------------------------------------------
    // Scrollable content area
    // -----------------------------------------------------------------------
    Flickable {
        id: scrollArea
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: buttonBar.top }
        contentHeight: formColumn.implicitHeight + 24
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: formColumn
            anchors { top: parent.top; left: parent.left; right: parent.right; topMargin: 16; leftMargin: 16; rightMargin: 16 }
            spacing: 14

            // ---------------------------------------------------------------
            // Section: Expressions
            // ---------------------------------------------------------------
            Text {
                text: "Expressions"
                color: "#7a8599"
                font.pixelSize: 11
                font.capitalization: Font.AllUppercase
                leftPadding: 2
            }

            // filter input bar
            Rectangle {
                id: exprFilterBar
                width: parent.width
                height: 28
                radius: 4
                color: "#23252e"
                border.color: exprFilterInput.activeFocus ? "#5b9bd5" : "#3a3d4a"
                border.width: 1

                TextInput {
                    id: exprFilterInput
                    anchors { verticalCenter: parent.verticalCenter; left: parent.left; right: exprClearBtn.left; leftMargin: 8; rightMargin: 4 }
                    color: "#dce8f8"
                    font.pixelSize: 12
                    clip: true
                    onTextChanged: root.filterText = exprFilterInput.text
                    Text {
                        anchors.fill: parent
                        text: "Filter expressions..."
                        color: "#555b6a"
                        font.pixelSize: 12
                        visible: exprFilterInput.text === ""
                    }
                }

                Rectangle {
                    id: exprClearBtn
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 6 }
                    width: 16; height: 16
                    radius: 8
                    color: exprClearMouse.containsMouse ? "#3a3d4a" : "transparent"
                    visible: exprFilterInput.text !== ""
                    Text {
                        anchors.centerIn: parent
                        text: "\u00D7"
                        color: "#b0b8c8"
                        font.pixelSize: 13
                    }
                    MouseArea {
                        id: exprClearMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: exprFilterInput.text = ""
                    }
                }
            }

            // expression selection grid
            GridView {
                id: exprGrid
                width: parent.width
                height: Math.max(28, Math.min(root.expressionItems.length * 28, 112))
                cellWidth: 220
                cellHeight: 28
                clip: true
                model: root.filteredItems
                delegate: Item {
                    id: exprCell
                    width: exprGrid.cellWidth
                    height: exprGrid.cellHeight
                    required property int index
                    required property var modelData
                    property string exprName: String(modelData[0])
                    property bool selected: root.selectionState[exprCell.exprName] === true

                    Rectangle {
                        anchors { fill: parent; margins: 3 }
                        radius: 4
                        color: exprCell.selected ? "#2a4a7a" : "#23252e"
                        border.color: exprCell.selected ? "#5b9bd5" : "#3a3d4a"
                        border.width: 1
                        Text {
                            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 8; right: parent.right; rightMargin: 4 }
                            text: exprCell.exprName
                            color: exprCell.selected ? "#dce8f8" : "#b0b8c8"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.color = exprCell.selected ? "#2a4a7a" : "#2e3040"
                            onExited:  parent.color = exprCell.selected ? "#2a4a7a" : "#23252e"
                            onClicked: {
                                var newSelected = !root.selectionState[exprCell.exprName]
                                var updated = Object.assign({}, root.selectionState)
                                updated[exprCell.exprName] = newSelected
                                root.selectionState = updated
                                root.selectionChanged(exprCell.exprName, newSelected)
                            }
                        }
                    }
                }
            }

            // ---------------------------------------------------------------
            // Section: Data Range
            // ---------------------------------------------------------------
            Text {
                text: "Data Range"
                color: "#7a8599"
                font.pixelSize: 11
                font.capitalization: Font.AllUppercase
                leftPadding: 2
            }

            Column {
                width: parent.width
                spacing: 6

                Rectangle {
                    width: parent.width
                    height: 32
                    radius: 4
                    color: "#23252e"
                    border.color: "#3a3d4a"
                    border.width: 1

                    ComboBox {
                        id: rangeCombo
                        anchors { fill: parent; margins: 2 }
                        model: root.rangeOptions
                        textRole: "label"
                        onCurrentIndexChanged: root.rangeMode = rangeCombo.currentIndex >= 0 ? root.rangeOptions[rangeCombo.currentIndex].value : "all"
                        background: Rectangle { color: "transparent" }
                        contentItem: Text {
                            leftPadding: 8
                            text: rangeCombo.displayText
                            color: "#dce8f8"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                        delegate: ItemDelegate {
                            id: rangeDelegate
                            required property var modelData
                            required property int index
                            width: rangeCombo.width
                            contentItem: Text {
                                text: rangeDelegate.modelData.label
                                color: "#b0b8c8"
                                font.pixelSize: 12
                            }
                            background: Rectangle {
                                color: rangeCombo.highlightedIndex === rangeDelegate.index ? "#3a3d4a" : "#252730"
                            }
                        }
                        popup.background: Rectangle {
                            color: "#252730"
                            border.color: "#3a3d4a"
                            border.width: 1
                            radius: 4
                        }
                    }
                }

                Row {
                    visible: root.rangeMode === "custom"
                    width: parent.width
                    spacing: 8

                    Column {
                        width: (parent.width - 8) / 2
                        spacing: 4
                        Text { text: "From (s)"; color: "#7a8599"; font.pixelSize: 11 }
                        Rectangle {
                            width: parent.width; height: 30
                            radius: 4
                            color: "#23252e"
                            border.color: fromField.activeFocus ? "#5b9bd5" : "#3a3d4a"
                            border.width: 1
                            TextInput {
                                id: fromField
                                anchors { fill: parent; leftMargin: 8; rightMargin: 4 }
                                text: root.zoomFromTime.toFixed(9)
                                color: "#dce8f8"
                                font.pixelSize: 12
                                verticalAlignment: Text.AlignVCenter
                                validator: DoubleValidator { bottom: root.abscissaMin; top: root.abscissaMax }
                                selectByMouse: true
                            }
                        }
                    }

                    Column {
                        width: (parent.width - 8) / 2
                        spacing: 4
                        Text { text: "To (s)"; color: "#7a8599"; font.pixelSize: 11 }
                        Rectangle {
                            width: parent.width; height: 30
                            radius: 4
                            color: "#23252e"
                            border.color: toField.activeFocus ? "#5b9bd5" : "#3a3d4a"
                            border.width: 1
                            TextInput {
                                id: toField
                                anchors { fill: parent; leftMargin: 8; rightMargin: 4 }
                                text: root.zoomToTime.toFixed(9)
                                color: "#dce8f8"
                                font.pixelSize: 12
                                verticalAlignment: Text.AlignVCenter
                                validator: DoubleValidator { bottom: root.abscissaMin; top: root.abscissaMax }
                                selectByMouse: true
                            }
                        }
                    }
                }
            }

            // ---------------------------------------------------------------
            // Section: Window Function
            // ---------------------------------------------------------------
            Text {
                text: "Window Function"
                color: "#7a8599"
                font.pixelSize: 11
                font.capitalization: Font.AllUppercase
                leftPadding: 2
            }

            Rectangle {
                width: parent.width
                height: 32
                radius: 4
                color: "#23252e"
                border.color: "#3a3d4a"
                border.width: 1

                ComboBox {
                    id: windowCombo
                    anchors { fill: parent; margins: 2 }
                    model: root.windowFunctions
                    currentIndex: root.defaultWindowIndex
                    onModelChanged: currentIndex = root.defaultWindowIndex
                    background: Rectangle { color: "transparent" }
                    contentItem: Text {
                        leftPadding: 8
                        text: windowCombo.displayText
                        color: "#dce8f8"
                        font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter
                    }
                    delegate: ItemDelegate {
                        id: windowDelegate
                        required property string modelData
                        required property int index
                        width: windowCombo.width
                        contentItem: Text {
                            text: windowDelegate.modelData
                            color: "#b0b8c8"
                            font.pixelSize: 12
                        }
                        background: Rectangle {
                            color: windowCombo.highlightedIndex === windowDelegate.index ? "#3a3d4a" : "#252730"
                        }
                    }
                    popup.background: Rectangle {
                        color: "#252730"
                        border.color: "#3a3d4a"
                        border.width: 1
                        radius: 4
                    }
                }
            }

            // ---------------------------------------------------------------
            // Section: FFT Points
            // ---------------------------------------------------------------
            Text {
                text: "FFT Points"
                color: "#7a8599"
                font.pixelSize: 11
                font.capitalization: Font.AllUppercase
                leftPadding: 2
            }

            Rectangle {
                width: parent.width
                height: 32
                radius: 4
                color: "#23252e"
                border.color: "#3a3d4a"
                border.width: 1

                ComboBox {
                    id: fftPointCombo
                    anchors { fill: parent; margins: 2 }
                    model: root.fftPointOptions
                    currentIndex: root.defaultFftPointIndex
                    onModelChanged: currentIndex = root.defaultFftPointIndex
                    background: Rectangle { color: "transparent" }
                    contentItem: Text {
                        leftPadding: 8
                        text: fftPointCombo.displayText
                        color: "#dce8f8"
                        font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter
                    }
                    delegate: ItemDelegate {
                        id: fftPointDelegate
                        required property var modelData
                        required property int index
                        width: fftPointCombo.width
                        contentItem: Text {
                            text: String(fftPointDelegate.modelData)
                            color: "#b0b8c8"
                            font.pixelSize: 12
                        }
                        background: Rectangle {
                            color: fftPointCombo.highlightedIndex === fftPointDelegate.index ? "#3a3d4a" : "#252730"
                        }
                    }
                    popup.background: Rectangle {
                        color: "#252730"
                        border.color: "#3a3d4a"
                        border.width: 1
                        radius: 4
                    }
                }
            }

            // ---------------------------------------------------------------
            // Section: Output Type
            // ---------------------------------------------------------------
            Text {
                text: "Output"
                color: "#7a8599"
                font.pixelSize: 11
                font.capitalization: Font.AllUppercase
                leftPadding: 2
            }

            Rectangle {
                width: parent.width
                height: 32
                radius: 4
                color: "#23252e"
                border.color: "#3a3d4a"
                border.width: 1

                ComboBox {
                    id: outputCombo
                    anchors { fill: parent; margins: 2 }
                    model: root.outputTypes
                    background: Rectangle { color: "transparent" }
                    contentItem: Text {
                        leftPadding: 8
                        text: outputCombo.displayText
                        color: "#dce8f8"
                        font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter
                    }
                    delegate: ItemDelegate {
                        id: outputDelegate
                        required property string modelData
                        required property int index
                        width: outputCombo.width
                        contentItem: Text {
                            text: outputDelegate.modelData
                            color: "#b0b8c8"
                            font.pixelSize: 12
                        }
                        background: Rectangle {
                            color: outputCombo.highlightedIndex === outputDelegate.index ? "#3a3d4a" : "#252730"
                        }
                    }
                    popup.background: Rectangle {
                        color: "#252730"
                        border.color: "#3a3d4a"
                        border.width: 1
                        radius: 4
                    }
                }
            }

            // ---------------------------------------------------------------
            // Section: Options
            // ---------------------------------------------------------------
            Text {
                text: "Options"
                color: "#7a8599"
                font.pixelSize: 11
                font.capitalization: Font.AllUppercase
                leftPadding: 2
            }

            // Normalize checkbox
            Rectangle {
                width: parent.width
                height: 32
                radius: 4
                color: normalizeCheck.checked ? "#2a3a2a" : "#23252e"
                border.color: normalizeCheck.checked ? "#4a9a4a" : "#3a3d4a"
                border.width: 1

                Row {
                    anchors { fill: parent; leftMargin: 10 }
                    spacing: 8

                    CheckBox {
                        id: normalizeCheck
                        anchors.verticalCenter: parent.verticalCenter
                        checked: true
                        indicator: Rectangle {
                            width: 16; height: 16
                            radius: 3
                            anchors.verticalCenter: parent.verticalCenter
                            color: normalizeCheck.checked ? "#4a9a4a" : "#23252e"
                            border.color: normalizeCheck.checked ? "#4a9a4a" : "#5a6070"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "✓"
                                color: "#ffffff"
                                font.pixelSize: 11
                                visible: normalizeCheck.checked
                            }
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Normalize output (peak = 1)"
                        color: "#b0b8c8"
                        font.pixelSize: 12
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: normalizeCheck.checked = !normalizeCheck.checked
                }
            }

            // Keep DC component checkbox
            Rectangle {
                width: parent.width
                height: 32
                radius: 4
                color: keepDcCheck.checked ? "#2a3a2a" : "#23252e"
                border.color: keepDcCheck.checked ? "#4a9a4a" : "#3a3d4a"
                border.width: 1

                Row {
                    anchors { fill: parent; leftMargin: 10 }
                    spacing: 8

                    CheckBox {
                        id: keepDcCheck
                        anchors.verticalCenter: parent.verticalCenter
                        checked: false
                        indicator: Rectangle {
                            width: 16; height: 16
                            radius: 3
                            anchors.verticalCenter: parent.verticalCenter
                            color: keepDcCheck.checked ? "#4a9a4a" : "#23252e"
                            border.color: keepDcCheck.checked ? "#4a9a4a" : "#5a6070"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "✓"
                                color: "#ffffff"
                                font.pixelSize: 11
                                visible: keepDcCheck.checked
                            }
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Keep DC component"
                        color: "#b0b8c8"
                        font.pixelSize: 12
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: keepDcCheck.checked = !keepDcCheck.checked
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Button bar
    // -----------------------------------------------------------------------
    Rectangle {
        id: buttonBar
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 44
        color: "#16171e"

        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 1
            color: "#3a3d4a"
        }

        Row {
            anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 12 }
            spacing: 8

            Rectangle {
                id: cancelBtn
                width: 80; height: 28
                radius: 4
                color: cancelMouse.containsMouse ? "#2e3040" : "#23252e"
                border.color: "#3a3d4a"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "Cancel"
                    color: "#b0b8c8"
                    font.pixelSize: 12
                }
                MouseArea {
                    id: cancelMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.dialogRejected()
                }
            }

            Rectangle {
                id: runBtn
                width: 80; height: 28
                radius: 4
                color: runMouse.containsMouse ? "#3a6aaa" : "#2a5090"
                border.color: "#5b9bd5"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "Run"
                    color: "#ffffff"
                    font.pixelSize: 12
                    font.bold: true
                }
                MouseArea {
                    id: runMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        var winFn = windowCombo.currentText
                        var fftPts = parseInt(fftPointCombo.currentText)
                        var out = outputCombo.currentText
                        var norm = normalizeCheck.checked
                        var from = parseFloat(fromField.text)
                        var to = parseFloat(toField.text)
                        var keepDc = keepDcCheck.checked
                        root.dialogAccepted(winFn, fftPts, out, norm, root.rangeMode, from, to, keepDc)
                    }
                }
            }
        }
    }
}
