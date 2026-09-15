// -----------------------------------------------------------------------
// File: HomeHeader.qml
// Description: Home screen header - mode block, alarm banner, clock block
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../Controls"
import "../Theme"

Item {
    id: header

    property string mode: "---"
    property string patientCategory: "adult"
    property bool spontaneous: false
    property bool nonInvasive: false
    property var alarms: []
    property date now: new Date()
    property bool acPower: true
    property int batteryPercent: 100
    property bool batteryCharging: false

    signal modeClicked()
    signal alarmActivated(int index)
    signal patientClicked()
    signal lockRequested()

    implicitHeight: Metrics.headerHeight

    RowLayout {
        anchors.fill: parent
        spacing: Metrics.gutter

        Rectangle {
            Layout.preferredWidth: Metrics.sidebarWidth
            Layout.fillHeight: true
            radius: Radius.medium
            color: Colors.surface

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Metrics.px(12)
                anchors.rightMargin: Metrics.px(12)
                spacing: Metrics.px(9)

                ModeBadge {
                    mode: header.mode
                    onClicked: header.modeClicked()
                }

                Item { Layout.fillWidth: true }

                // The breath type and the interface type are both decided on
                // the mode screen, so both indicators open it.
                AppIcon {
                    name: "mode-spont"
                    size: Metrics.px(29)
                    color: Colors.textPrimary
                    opacity: header.spontaneous ? 1.0 : 0.45

                    MouseArea {
                        anchors.fill: parent
                        onClicked: header.modeClicked()
                    }
                }

                AppIcon {
                    source: Icons.patientCategory(header.patientCategory)
                    size: Metrics.px(29)
                    color: Colors.textPrimary

                    MouseArea {
                        anchors.fill: parent
                        onClicked: header.patientClicked()
                    }
                }

                AppIcon {
                    name: "ventilator-mask"
                    size: Metrics.px(29)
                    color: Colors.textPrimary
                    opacity: header.nonInvasive ? 1.0 : 0.45

                    MouseArea {
                        anchors.fill: parent
                        onClicked: header.modeClicked()
                    }
                }
            }
        }

        AlarmBanner {
            Layout.fillWidth: true
            Layout.fillHeight: true
            alarms: header.alarms
            onAlarmActivated: function (index) { header.alarmActivated(index) }
        }

        ClockBlock {
            Layout.preferredWidth: Metrics.railWidth
            Layout.fillHeight: true
            now: header.now
            acPower: header.acPower
            batteryPercent: header.batteryPercent
            batteryCharging: header.batteryCharging
            onLockRequested: header.lockRequested()
        }
    }
}
