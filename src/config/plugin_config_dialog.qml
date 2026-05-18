pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
	id: root
	implicitWidth: 640
	implicitHeight: 320

	property alias xyceExecutablePath: xycePathField.text
	property string errorText: ""

	signal browseRequested()
	signal submit(string xyceExecutablePath)
	signal cancelRequested()

	Rectangle {
		anchors.fill: parent
		color: "#efefe8"
	}

	ColumnLayout {
		anchors.fill: parent
		anchors.margins: 20
		spacing: 14

		Label {
			text: "Plugin Configuration"
			color: "#1b1f23"
			font.pixelSize: 22
			font.bold: true
			Layout.fillWidth: true
		}

		Label {
			text: "Set the Xyce executable location used by simulation runs"
			color: "#4a5560"
			font.pixelSize: 13
			wrapMode: Text.Wrap
			Layout.fillWidth: true
		}

		Rectangle {
			color: "#ffffff"
			radius: 8
			border.color: "#d0d7de"
			border.width: 1
			Layout.fillWidth: true
			Layout.fillHeight: true

			GridLayout {
				anchors.fill: parent
				anchors.margins: 16
				columns: 2
				rowSpacing: 10
				columnSpacing: 12

				Label {
					text: "Xyce Executable *"
					color: "#24292f"
				}

				RowLayout {
					spacing: 8
					Layout.fillWidth: true

					TextField {
						id: xycePathField
						placeholderText: "/path/to/Xyce"
						selectByMouse: true
						Layout.fillWidth: true
					}

					Button {
						text: "Browse"
						onClicked: root.browseRequested()
					}
				}

				Label {
					text: root.errorText
					visible: root.errorText.length > 0
					color: "#b42318"
					font.pixelSize: 12
					wrapMode: Text.Wrap
					Layout.columnSpan: 2
					Layout.fillWidth: true
				}

				Item {
					Layout.columnSpan: 2
					Layout.fillHeight: true
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
				onClicked: root.cancelRequested()
			}

			Button {
				text: "Save"
				highlighted: true
				onClicked: root.submit(root.xyceExecutablePath)
			}
		}
	}
}
