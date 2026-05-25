pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root
    anchors.fill: parent

    property var parameterNames: []
    property var stepRows: []
    property var initialSelectedSteps: []
    property var selectedState: ({})
    readonly property int maxVisibleColumns: 3
    readonly property var displayedParameterNames: parameterNames.slice(0, maxVisibleColumns)
    readonly property real headerHeight: 28
    readonly property real rowHeight: 30

    readonly property real selectorWidth: 92
    readonly property real valueColumnWidth: {
        var count = Math.max(1, displayedParameterNames.length)
        var available = (tableArea.width - selectorWidth - 12) / count
        if (displayedParameterNames.length === 1)
            return Math.min(260, Math.max(160, available))
        return Math.max(120, available)
    }

    signal selectionChanged(int stepIndex, bool selected)
    signal dialogAccepted(var selectedSteps)
    signal dialogRejected()

    function initialize() {
        // build selection map from the initial selected-step list
        var selection = {}
        for (var i = 0; i < initialSelectedSteps.length; i++) {
            selection[String(initialSelectedSteps[i])] = true
        }
        root.selectedState = selection
    }

    function setSelectionForAll(selected) {
        var updated = Object.assign({}, selectedState)
        for (var i = 0; i < stepRows.length; i++) {
            var step = stepRows[i].stepIndex
            var key = String(step)
            updated[key] = selected
            root.selectionChanged(step, selected)
        }
        root.selectedState = updated
    }

    function invertSelectionForAll() {
        var updated = Object.assign({}, selectedState)
        for (var i = 0; i < stepRows.length; i++) {
            var key = String(stepRows[i].stepIndex)
            var newSelected = updated[key] !== true
            updated[key] = newSelected
            root.selectionChanged(stepRows[i].stepIndex, newSelected)
        }
        root.selectedState = updated
    }

    function selectedCount() {
        var count = 0
        for (var i = 0; i < stepRows.length; i++) {
            if (selectedState[String(stepRows[i].stepIndex)] === true) {
                count++
            }
        }
        return count
    }

    function selectedStepIndices() {
        var steps = []
        for (var i = 0; i < stepRows.length; i++) {
            var step = stepRows[i].stepIndex
            if (selectedState[String(step)] === true) {
                steps.push(step)
            }
        }
        return steps
    }

    Rectangle {
        anchors.fill: parent
        color: "#1a1b1e"
    }

    Column {
        id: contentColumn

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: buttonBar.top
            topMargin: 12
            leftMargin: 12
            rightMargin: 12
            bottomMargin: 12
        }
        spacing: 10

        Text {
            id: titleLabel

            text: "Step Tool"
            color: "#dce8f8"
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            id: subtitleLabel

            text: "Select one or more parameter combinations to keep active for this chart"
            color: "#8f98ab"
            font.pixelSize: 12
        }

        Rectangle {
            id: actionsBar

            width: parent.width
            height: 32
            radius: 4
            color: "#20232b"
            border.color: "#303544"
            border.width: 1

            Row {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                SmallButton {
                    text: "Select All"
                    onClicked: root.setSelectionForAll(true)
                }

                SmallButton {
                    text: "Clear All"
                    onClicked: root.setSelectionForAll(false)
                }

                SmallButton {
                    text: "Invert"
                    onClicked: root.invertSelectionForAll()
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Selected " + root.selectedCount() + " / " + root.stepRows.length
                    color: "#9ca7bd"
                    font.pixelSize: 12
                }
            }
        }

        Rectangle {
            id: tableArea
            width: parent.width
            height: Math.max(root.headerHeight + root.rowHeight + 14, contentColumn.height - titleLabel.height - subtitleLabel.height - actionsBar.height - (contentColumn.spacing * 3))
            radius: 6
            color: "#20232b"
            border.color: "#303544"
            border.width: 1

            Column {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 2

                Rectangle {
                    width: parent.width
                    height: root.headerHeight
                    color: "#252a34"
                    radius: 4

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        spacing: 0

                        Text {
                            width: root.selectorWidth
                            height: parent.height
                            text: "Selected"
                            color: "#9ca7bd"
                            font.pixelSize: 11
                            font.bold: true
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Repeater {
                            model: root.displayedParameterNames
                            delegate: Text {
                                required property string modelData
                                width: root.valueColumnWidth
                                height: parent.height
                                text: modelData
                                color: "#9ca7bd"
                                font.pixelSize: 11
                                font.bold: true
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignRight
                                rightPadding: 8
                                elide: Text.ElideNone
                            }
                        }
                    }

                    // vertical separators
                    Rectangle {
                        x: 6 + root.selectorWidth
                        width: 1
                        height: parent.height - 8
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#3a4255"
                    }

                    Repeater {
                        model: Math.max(0, root.displayedParameterNames.length - 1)
                        delegate: Rectangle {
                            required property int index

                            x: 6 + root.selectorWidth + (index + 1) * root.valueColumnWidth
                            width: 1
                            height: parent.height - 8
                            anchors.verticalCenter: parent.verticalCenter
                            color: "#3a4255"
                        }
                    }

                    Rectangle {
                        x: 6 + root.selectorWidth + root.displayedParameterNames.length * root.valueColumnWidth
                        width: 1
                        height: parent.height - 8
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#3a4255"
                    }

                    Rectangle {
                        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                        height: 1
                        color: "#333b4d"
                    }
                }

                ListView {
                    id: rowsView
                    width: parent.width
                    height: Math.max(root.rowHeight, parent.height - root.headerHeight - parent.spacing)
                    clip: true
                    model: root.stepRows
                    spacing: 0

                    delegate: Rectangle {
                        id: rowDelegate

                        required property var modelData
                        required property int index

                        property int stepIndex: modelData.stepIndex
                        property bool selected: root.selectedState[String(stepIndex)] === true

                        width: rowsView.width
                        height: root.rowHeight
                        color: rowDelegate.index % 2 === 0 ? "#222733" : "#202636"
                        radius: 0

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            spacing: 0

                            Rectangle {
                                width: root.selectorWidth
                                height: parent.height
                                color: "transparent"

                                Rectangle {
                                    id: checkboxBox

                                    anchors.centerIn: parent
                                    width: 16
                                    height: 16
                                    radius: 3
                                    color: rowDelegate.selected ? "#3a79bf" : "#2a2e39"
                                    border.color: rowDelegate.selected ? "#8ec2ff" : "#4a5060"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: rowDelegate.selected ? "\u2713" : ""
                                        color: "#ffffff"
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                }

                                MouseArea {
                                    anchors.fill: checkboxBox
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        var updated = Object.assign({}, root.selectedState)
                                        var key = String(rowDelegate.stepIndex)
                                        var newSelected = updated[key] !== true
                                        updated[key] = newSelected
                                        root.selectedState = updated
                                        root.selectionChanged(rowDelegate.stepIndex, newSelected)
                                    }
                                }
                            }

                            Repeater {
                                model: rowDelegate.modelData.values.slice(0, root.displayedParameterNames.length)
                                delegate: Text {
                                    required property string modelData
                                    width: root.valueColumnWidth
                                    height: parent.height
                                    text: modelData
                                    color: "#b8c1d2"
                                    font.pixelSize: 12
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignRight
                                    rightPadding: 8
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        // vertical separators
                        Rectangle {
                            x: 6 + root.selectorWidth
                            width: 1
                            height: parent.height - 6
                            anchors.verticalCenter: parent.verticalCenter
                            color: "#343c4f"
                        }

                        Repeater {
                            model: Math.max(0, root.displayedParameterNames.length - 1)
                            delegate: Rectangle {
                                required property int index

                                x: 6 + root.selectorWidth + (index + 1) * root.valueColumnWidth
                                width: 1
                                height: parent.height - 6
                                anchors.verticalCenter: parent.verticalCenter
                                color: "#343c4f"
                            }
                        }

                        Rectangle {
                            x: 6 + root.selectorWidth + root.displayedParameterNames.length * root.valueColumnWidth
                            width: 1
                            height: parent.height - 6
                            anchors.verticalCenter: parent.verticalCenter
                            color: "#343c4f"
                        }

                        Rectangle {
                            anchors { left: parent.left; right: parent.right; top: parent.top }
                            height: 1
                            color: "#2a3142"
                            visible: rowDelegate.index > 0
                        }
                    }
                }
            }
        }

    }

    Rectangle {
        id: buttonBar

        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
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

            ActionButton {
                text: "Cancel"
                primary: false
                onClicked: root.dialogRejected()
            }

            ActionButton {
                text: "Apply"
                primary: true
                onClicked: root.dialogAccepted(root.selectedStepIndices())
            }
        }
    }

    component SmallButton: Rectangle {
        id: smallBtn

        required property string text

        signal clicked()

        width: label.implicitWidth + 20
        height: 20
        radius: 10
        color: mouse.containsMouse ? "#3a3f4d" : "#2c303b"
        border.color: "#4b5365"
        border.width: 1

        Text {
            id: label
            anchors.centerIn: parent
            text: smallBtn.text
            color: "#cfd7e8"
            font.pixelSize: 11
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: smallBtn.clicked()
        }
    }

    component ActionButton: Rectangle {
        id: actionBtn

        required property string text
        required property bool primary

        signal clicked()

        width: 88
        height: 28
        radius: 4
        color: actionBtn.primary ? (mouse.containsMouse ? "#3a6aaa" : "#2a5090") : (mouse.containsMouse ? "#2e3040" : "#23252e")
        border.color: actionBtn.primary ? "#5b9bd5" : "#3a3d4a"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: actionBtn.text
            color: actionBtn.primary ? "#ffffff" : "#b0b8c8"
            font.pixelSize: 12
            font.bold: actionBtn.primary
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: actionBtn.clicked()
        }
    }
}
