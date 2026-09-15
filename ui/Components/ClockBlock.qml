// -----------------------------------------------------------------------
// File: ClockBlock.qml
// Description: Date, time, mains and battery indicator
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../Controls"
import "../Theme"

Rectangle {
    id: block

    property date now: new Date()
    property string dateFormat: "MMM-dd-yyyy"
    property string timeFormat: "hh:mm AP"
    property bool acPower: true
    property int batteryPercent: 100
    property bool batteryCharging: false

    signal lockRequested()

    radius: Radius.medium
    color: Colors.surface

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Metrics.px(14)
        anchors.rightMargin: Metrics.px(11)
        spacing: Metrics.px(16)

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Text {
                Layout.fillWidth: true
                text: Qt.formatDate(block.now, block.dateFormat)
                color: Colors.textSecondary
                font.family: Typography.family
                font.pixelSize: Typography.small
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: Qt.formatTime(block.now, block.timeFormat)
                color: Colors.textPrimary
                font.family: Typography.family
                font.pixelSize: Typography.label
                font.weight: Typography.bold
                elide: Text.ElideRight
            }
        }

        // Locking is a deliberate act before stepping away from the bedside,
        // so it has its own control rather than only an inactivity timeout.
        IconButton {
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: Metrics.px(34)
            implicitHeight: Metrics.px(34)
            iconName: "lock"
            iconSize: Metrics.px(20)
            accessibleName: qsTr("Lock the screen")
            onClicked: block.lockRequested()
        }

        AppIcon {
            Layout.alignment: Qt.AlignVCenter
            name: block.acPower ? "power-plug" : "power-dc"
            size: Metrics.px(29)
            color: Colors.textPrimary
        }

        AppIcon {
            Layout.alignment: Qt.AlignVCenter
            source: Icons.battery(block.batteryPercent, block.batteryCharging)
            size: Metrics.px(36)
            color: block.batteryPercent <= 20 ? Colors.alarmHigh : Colors.success
        }
    }
}
