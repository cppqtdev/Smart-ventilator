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
    property string dateFormat: "dd/MM/yyyy"
    // A 24 hour clock is what the ward charts against, and it is also the
    // only format that leaves room for the date beside three indicators.
    property string timeFormat: "HH:mm"
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
        // The block is one rail wide. At the old spacing and icon sizes the
        // date and time were left 38 across and both read as an ellipsis.
        spacing: Spacing.sm

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Text {
                Layout.fillWidth: true
                text: Qt.formatDate(block.now, block.dateFormat)
                color: Colors.textSecondary
                font.family: Typography.family
                font.pixelSize: Typography.caption
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
            implicitWidth: Metrics.px(30)
            implicitHeight: Metrics.px(30)
            iconName: "lock"
            iconSize: Metrics.px(18)
            accessibleName: qsTr("Lock the screen")
            onClicked: block.lockRequested()
        }

        AppIcon {
            Layout.alignment: Qt.AlignVCenter
            name: block.acPower ? "power-plug" : "power-dc"
            size: Metrics.px(26)
            color: Colors.textPrimary
        }

        AppIcon {
            Layout.alignment: Qt.AlignVCenter
            source: Icons.battery(block.batteryPercent, block.batteryCharging)
            size: Metrics.px(30)
            color: block.batteryPercent <= 20 ? Colors.alarmHigh : Colors.success
        }
    }
}
