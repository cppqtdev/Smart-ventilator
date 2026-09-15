// -----------------------------------------------------------------------
// File: AlarmBanner.qml
// Description: Stacked alarm bars, highest priority on top
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../Theme"

Item {
    id: banner

    // Each entry: { priority: 3|2|1, text: string }
    property var alarms: []
    property int maximumRows: 2

    /** Shown in place of the bars when nothing is active. */
    property string idleText: qsTr("No active alarms")

    signal alarmActivated(int index)

    readonly property var visibleAlarms: banner.alarms.slice(0, banner.maximumRows)

    implicitHeight: Metrics.alarmBarHeight * banner.maximumRows

    function surfaceFor(priority) {
        switch (priority) {
        case 3: return Colors.alarmHigh
        case 2: return Colors.alarmMedium
        case 1: return Colors.alarmLow
        default: return Colors.alarmNone
        }
    }

    function foregroundFor(priority) {
        switch (priority) {
        case 3: return Colors.alarmHighOn
        case 2: return Colors.alarmMediumOn
        case 1: return Colors.alarmLowOn
        default: return Colors.textSecondary
        }
    }

    // With no alarms the banner used to draw nothing at all, leaving a hole
    // across the middle of the header.
    Rectangle {
        anchors.fill: parent
        visible: banner.visibleAlarms.length === 0
        radius: Radius.small
        color: Colors.surface

        Text {
            anchors.centerIn: parent
            text: banner.idleText
            color: Colors.textSecondary
            font.family: Typography.family
            font.pixelSize: Typography.bannerLabel
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Repeater {
            model: banner.visibleAlarms

            delegate: Rectangle {
                required property int index
                required property var modelData

                Layout.fillWidth: true
                Layout.fillHeight: true

                color: banner.surfaceFor(modelData.priority)
                topLeftRadius: index === 0 ? Radius.small : 0
                topRightRadius: index === 0 ? Radius.small : 0
                bottomLeftRadius: index === banner.visibleAlarms.length - 1 ? Radius.small : 0
                bottomRightRadius: index === banner.visibleAlarms.length - 1 ? Radius.small : 0

                Text {
                    anchors.centerIn: parent
                    width: parent.width - Spacing.xl * 2
                    text: modelData.text
                    color: banner.foregroundFor(modelData.priority)
                    font.family: Typography.family
                    font.pixelSize: Typography.bannerLabel
                    font.weight: Typography.bold
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: banner.alarmActivated(index)
                }

                SequentialAnimation on opacity {
                    running: modelData.priority >= 2
                    loops: Animation.Infinite
                    NumberAnimation {
                        to: 0.45
                        duration: modelData.priority === 3 ? Metrics.alarmFlashHighMs
                                                           : Metrics.alarmFlashMediumMs
                    }
                    NumberAnimation {
                        to: 1.0
                        duration: modelData.priority === 3 ? Metrics.alarmFlashHighMs
                                                           : Metrics.alarmFlashMediumMs
                    }
                }
            }
        }
    }
}
