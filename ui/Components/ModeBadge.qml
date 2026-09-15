// -----------------------------------------------------------------------
// File: ModeBadge.qml
// Description: Ventilation mode badge with a MODE caption strip
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "../Theme"

Rectangle {
    id: badge

    property string mode: "---"
    property string caption: qsTr("MODE")

    signal clicked()

    implicitWidth: Metrics.px(63)
    implicitHeight: Metrics.px(37)

    radius: Radius.small
    color: Colors.transparent
    border.width: Metrics.borderWidth
    border.color: Colors.neutral

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Metrics.px(2)
        text: badge.mode
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.px(19)
        font.weight: Typography.bold
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Metrics.borderWidth
        height: Metrics.px(12)
        radius: Radius.xs
        color: Colors.neutral

        Text {
            anchors.centerIn: parent
            text: badge.caption
            color: Colors.textInverse
            font.family: Typography.monoFamily
            font.pixelSize: Typography.px(9)
            font.weight: Typography.bold
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: badge.clicked()
    }
}
