import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {

    id: root
    width: parent ? parent.width : mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    color: "#FFFFFF"
    border.color: "#E0E0DC"
    border.width: 1
    radius: 8
    clip: true

    property string title: ""
    property string badge: ""
    property alias spacing: mainContent.spacing
    
    // The content will be placed inside this ColumnLayout
    default property alias content: mainContent.data

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 0

        // Section Header
        Rectangle {
            id: headerRect
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: "#F5F5F3"
            
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#E0E0DC"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Label {
                    text: root.title
                    font.family: "Arial"
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    font.letterSpacing: 0.5
                    color: "#4A4A45"
                    Layout.fillWidth: true
                }

                Rectangle {
                    visible: root.badge !== ""
                    Layout.preferredWidth: badgeLabel.implicitWidth + 12
                    Layout.preferredHeight: 18
                    radius: 4
                    color: "#E0E0DC"
                    
                    Label {
                        id: badgeLabel
                        anchors.centerIn: parent
                        text: root.badge
                        font.family: "Arial"
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: "#6B6B66"
                    }
                }
            }
        }

        // Section Body
        ColumnLayout {
            id: mainContent
            Layout.fillWidth: true
            Layout.margins: 20
            spacing: 16
        }
    }
}
