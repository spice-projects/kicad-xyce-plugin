pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root
    anchors.fill: parent

    property var expressions: []
    property var selectedIndices: []
    property var selectionState: ({})
    property string filterText: ""
    property string expressionError: ""
    property bool allowCustomExpressions: true
    property var expressionColors: {
        "voltage": "#5b9bd5",
        "current": "#7cb342",
        "frequency": "#e57373",
        "time": "#ba68c8",
        "power": "#ffb74d",
        "misc": "#3a3d4a"
    }

    function getIndicatorColor(type) {
        var color = root.expressionColors[type.toLowerCase()]
        return color ? color : root.expressionColors["misc"]
    }

    property var filteredExpressions: {
        var text = filterText.toLowerCase()
        if (text === "") return root.expressions
        return root.expressions.filter(function(e) {
            return String(e[0]).toLowerCase().indexOf(text) !== -1
        })
    }

    signal dialogAccepted()
    signal dialogRejected()
    signal selectionChanged(string expression, bool selected)
    signal customExpressionRequested(string text)

    function initialize(expressions) {
        root.expressions = expressions
        var state = {}
        for (var i = 0; i < expressions.length; i++) {
            state[expressions[i][0]] = expressions[i][1]
        }
        root.selectionState = state
    }

    function addExpression(name, selected) {
        // update selection state for the new expression
        var state = Object.assign({}, root.selectionState)
        state[name] = selected
        root.selectionState = state
        // only append to expressions list if not already present
        for (var i = 0; i < root.expressions.length; i++) {
            if (String(root.expressions[i][0]) === name) {
                exprInput.text = ""
                return
            }
        }
        var updated = root.expressions.slice()
        updated.push([name, selected, "Misc"])
        root.expressions = updated
        exprInput.text = ""
    }

    function setExpressionError(msg) {
        root.expressionError = msg
    }

    Rectangle {
        anchors.fill: parent
        color: "#1a1b1e"
    }

    Text {
        id: titleLabel
        anchors { top: parent.top; left: parent.left; right: parent.right; topMargin: 12; leftMargin: 14 }
        text: "Select one or more expressions to plot:"
        color: "#b0b8c8"
        font.pixelSize: 13
    }

    // filter input bar
    Rectangle {
        id: filterBar
        anchors { top: titleLabel.bottom; left: parent.left; right: parent.right; topMargin: 8; leftMargin: 10; rightMargin: 10 }
        height: 28
        radius: 4
        color: "#23252e"
        border.color: filterInput.activeFocus ? "#5b9bd5" : "#3a3d4a"
        border.width: 1

        TextInput {
            id: filterInput
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; right: clearBtn.left; leftMargin: 8; rightMargin: 4 }
            color: "#dce8f8"
            font.pixelSize: 12
            clip: true
            onTextChanged: root.filterText = filterInput.text

            // placeholder text shown when empty
            Text {
                anchors.fill: parent
                text: "Filter expressions..."
                color: "#555b6a"
                font.pixelSize: 12
                visible: filterInput.text === ""
            }
        }

        // clear button — only visible while filter text is non-empty
        Rectangle {
            id: clearBtn
            anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 6 }
            width: 16; height: 16
            radius: 8
            color: clearMouse.containsMouse ? "#3a3d4a" : "transparent"
            visible: filterInput.text !== ""

            Text {
                anchors.centerIn: parent
                text: "\u00D7"
                color: "#b0b8c8"
                font.pixelSize: 13
            }

            MouseArea {
                id: clearMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: filterInput.text = ""
            }
        }
    }

    GridView {
        id: grid
        anchors {
            top: filterBar.bottom
            left: parent.left; right: parent.right; bottom: customExprRow.top
            margins: 10
            topMargin: 8
        }
        cellWidth: 180
        cellHeight: 28
        clip: true

        model: root.filteredExpressions

        delegate: Item {

            id: cellItem
            width: grid.cellWidth
            height: grid.cellHeight

            required property int index
            required property var modelData

            property string expression: String(modelData[0])
            property string exprType: (modelData.length > 2) ? String(modelData[2]) : "Misc"
            // read selection state from the root map so toggling persists across filter changes
            property bool selected: root.selectionState[cellItem.expression] === true

            Rectangle {
                anchors { fill: parent; margins: 3 }
                radius: 4
                color: cellItem.selected ? "#2a4a7a" : "#23252e"
                border.color: "#3a3d4a"
                border.width: 1

                // indicator circle
                Rectangle {
                    anchors { left: parent.left; leftMargin: 6; verticalCenter: parent.verticalCenter }
                    width: 6; height: 6
                    radius: 3
                    color: root.getIndicatorColor(cellItem.exprType)
                }

                Text {
                    anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 18; right: parent.right; rightMargin: 4 }
                    text: cellItem.expression
                    color: cellItem.selected ? "#dce8f8" : "#b0b8c8"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.color = cellItem.selected ? "#2a4a7a" : "#2e3040"
                    onExited:  parent.color = cellItem.selected ? "#2a4a7a" : "#23252e"
                    onClicked: {
                        // compute new selection value by inverting the current state
                        var newSelected = !root.selectionState[cellItem.expression]
                        // replace the entire map object so QML detects the change and re-evaluates bindings
                        var updated = Object.assign({}, root.selectionState)
                        updated[cellItem.expression] = newSelected
                        root.selectionState = updated
                        // emit signal to notify of selection change
                        root.selectionChanged(cellItem.expression, newSelected)
                    }
                }
            }
        }
    }

    // custom expression input section
    Rectangle {
        id: customExprRow
        visible: root.allowCustomExpressions
        anchors { bottom: buttonBar.top; left: parent.left; right: parent.right }
        height: root.expressionError !== "" ? 72 : 50
        color: "#16171e"

        // top separator
        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 1
            color: "#3a3d4a"
        }

        // section label
        Text {
            id: customExprLabel
            anchors { left: parent.left; verticalCenter: addExprBtn.verticalCenter; leftMargin: 12 }
            text: "Expression:"
            color: "#808898"
            font.pixelSize: 12
        }

        // add button
        Rectangle {
            id: addExprBtn
            anchors { right: parent.right; top: parent.top; topMargin: 11; rightMargin: 10 }
            width: 52; height: 28
            radius: 4
            color: addExprMouse.containsMouse ? "#3a6aaa" : "#2a5090"
            border.color: "#5b9bd5"
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: "Add"
                color: "#ffffff"
                font.pixelSize: 12
            }
            MouseArea {
                id: addExprMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.customExpressionRequested(exprInput.text.trim())
            }
        }

        // expression text input
        Rectangle {
            id: exprInputRect
            anchors { left: customExprLabel.right; right: addExprBtn.left; top: addExprBtn.top; leftMargin: 6; rightMargin: 6 }
            height: 28
            radius: 4
            color: "#23252e"
            border.color: exprInput.activeFocus ? "#5b9bd5" : (root.expressionError !== "" ? "#c05050" : "#3a3d4a")
            border.width: 1

            TextInput {
                id: exprInput
                anchors { verticalCenter: parent.verticalCenter; left: parent.left; right: parent.right; leftMargin: 8; rightMargin: 8 }
                color: "#dce8f8"
                font.pixelSize: 12
                clip: true
                onTextChanged: root.expressionError = ""
                Keys.onReturnPressed: root.customExpressionRequested(exprInput.text.trim())
                Keys.onEnterPressed: root.customExpressionRequested(exprInput.text.trim())

                // placeholder text shown when empty
                Text {
                    anchors.fill: parent
                    text: "e.g. V(net1) / I(R1)"
                    color: "#555b6a"
                    font.pixelSize: 12
                    visible: exprInput.text === ""
                }
            }
        }

        // error label — only visible when expression evaluation fails
        Text {
            anchors { top: exprInputRect.bottom; left: exprInputRect.left; topMargin: 5 }
            text: root.expressionError
            color: "#c05050"
            font.pixelSize: 11
            visible: root.expressionError !== ""
        }
    }

    // -------------------------------------------------------------------------
    // Button bar — Legend, OK and Cancel
    // -------------------------------------------------------------------------
    Rectangle {
        id: buttonBar
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 60
        color: "#16171e"

        // thin top divider
        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 1
            color: "#3a3d4a"
        }

        // legend
        Row {
            anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
            spacing: 12
            z: 10

            Row { spacing: 4; Rectangle { width: 8; height: 8; color: root.expressionColors.voltage; radius: 4; z: 11 } Text { text: "Voltage"; color: "#b0b8c8"; font.pixelSize: 10 } }
            Row { spacing: 4; Rectangle { width: 8; height: 8; color: root.expressionColors.current; radius: 4; z: 11 } Text { text: "Current"; color: "#b0b8c8"; font.pixelSize: 10 } }
            Row { spacing: 4; Rectangle { width: 8; height: 8; color: root.expressionColors.frequency; radius: 4; z: 11 } Text { text: "Freq"; color: "#b0b8c8"; font.pixelSize: 10 } }
            Row { spacing: 4; Rectangle { width: 8; height: 8; color: root.expressionColors.time; radius: 4; z: 11 } Text { text: "Time"; color: "#b0b8c8"; font.pixelSize: 10 } }
            Row { spacing: 4; Rectangle { width: 8; height: 8; color: root.expressionColors.power; radius: 4; z: 11 } Text { text: "Power"; color: "#b0b8c8"; font.pixelSize: 10 } }
            Row { spacing: 4; Rectangle { width: 8; height: 8; color: root.expressionColors.misc; radius: 4; z: 11 } Text { text: "Misc"; color: "#b0b8c8"; font.pixelSize: 10 } }
        }

        Row {
            anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 12 }
            spacing: 8

            // cancel button
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

            // ok button
            Rectangle {
                id: okBtn
                width: 80; height: 28
                radius: 4
                color: okMouse.containsMouse ? "#3a6aaa" : "#2a5090"
                border.color: "#5b9bd5"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "OK"
                    color: "#ffffff"
                    font.pixelSize: 12
                    font.bold: true
                }
                MouseArea {
                    id: okMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.dialogAccepted()
                }
            }
        }
    }
}
