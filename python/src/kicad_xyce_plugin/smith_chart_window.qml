pragma ComponentBehavior: Bound

import QtQuick
import SmithChart 1.0

Item {
    id: root
    anchors.fill: parent

    property int _seriesCount: 0
    property bool stepToolVisible: false

    signal menuAddRemovePlots()
    signal menuDeleteAllPlots()
    signal menuStepTool()

    SmithGridItem {
        id: grid
        anchors.fill: parent
    }

    SmithTraceItem {
        id: trace
        anchors.fill: parent
    }

    // right-button click — asks root to open the shared context menu at this location
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: mouse => {
            // map from panel-local coordinates to root-local coordinates
            var pt = mapToItem(root, mouse.x, mouse.y);
            // clamp so the menu never overflows the root boundary
            contextMenu.x = Math.min(pt.x, root.width - contextMenu.width - 2);
            contextMenu.y = Math.min(pt.y, root.height - contextMenu.height - 2);
            // show menu
            contextMenu.visible = true;
        }
    }

    MouseArea {
        anchors.fill: parent
        visible: contextMenu.visible
        z: 998
        acceptedButtons: Qt.AllButtons
        onPressed: contextMenu.visible = false
    }

    Rectangle {
        id: contextMenu
        visible: false
        z: 999
        width: 210
        height: menuColumn.implicitHeight + 8
        color: "#252730"
        border.color: "#3a3d4a"
        border.width: 1
        radius: 4

        Column {
            id: menuColumn
            anchors {
                top: parent.top
                left: parent.left
                topMargin: 4
            }

            ContextMenuItem {
                itemText: "Add/Remove Plots"
                onTriggered: root.menuAddRemovePlots()
            }
            ContextMenuItem {
                itemText: "Delete All Plots"
                enabled: root._seriesCount > 0
                onTriggered: root.menuDeleteAllPlots()
            }
            ContextMenuSeparator { 
                visible: root.stepToolVisible 
            }
            ContextMenuItem {
                itemText: "Step Tool..."
                visible: root.stepToolVisible
                onTriggered: root.menuStepTool()
            }
        }
    }

    component ContextMenuItem: Rectangle {
        id: itemRoot
        required property string itemText
        signal triggered
        width: contextMenu.width
        implicitHeight: 26
        color: itemMouse.containsMouse ? "#3a3d4a" : "transparent"
        radius: 2
        Text {
            anchors.fill: parent
            text: itemRoot.itemText
            color: itemRoot.enabled ? "#b0b8c8" : "#4a5060"
            font.pixelSize: 12
            leftPadding: 12
            verticalAlignment: Text.AlignVCenter
        }
        MouseArea {
            id: itemMouse
            anchors.fill: parent
            hoverEnabled: true
            enabled: itemRoot.enabled
            onClicked: {
                contextMenu.visible = false;
                itemRoot.triggered();
            }
        }
    }

    // reusable separator
    component ContextMenuSeparator: Rectangle {
        width: contextMenu.width
        implicitHeight: 9
        color: "transparent"
        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 8
            height: 1
            color: "#3a3d4a"
        }
    }

    function plot(series) {
        // trace series
        trace.plot(series);
    }
}
