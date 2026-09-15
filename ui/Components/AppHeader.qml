// -----------------------------------------------------------------------
// File: AppHeader.qml
// Description: Application-wide header for every screen except the home screen
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The home screen carries its own header (HomeHeader) because the reference
// design puts the mode block, alarm banner and clock on one row aligned with
// the three content columns. This is the header the remaining screens use.
//
import QtQuick
import QtQuick.Layouts
import "../Controls"
import "../Theme"

Rectangle {
    id: header

    property var alarmData
    property var clockData
    property var batteryData
    property var patientData

    property string alarmPriority: "none"
    property string mode: "---"
    property string modeDescription: ""
    property bool nonInvasive: false
    property string patientCategory: "adult"
    property string automationStatus: ""
    property bool ventilating: false
    property string elapsedText: ""
    property bool screenLocked: false

    signal alarmCentreRequested()
    signal audioPauseRequested()
    signal alarmResetRequested()
    signal lockRequested()
    signal modeRequested()
    signal patientRequested()
    signal emergencyRequested()
    signal shutdownRequested()

    implicitHeight: Metrics.headerHeight + Spacing.md
    color: Colors.background

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Metrics.screenPadding
        anchors.rightMargin: Metrics.screenPadding
        anchors.topMargin: Spacing.sm
        anchors.bottomMargin: Spacing.sm
        spacing: Metrics.gutter

        Rectangle {
            Layout.preferredWidth: Metrics.sidebarWidth
            Layout.fillHeight: true
            radius: Radius.medium
            color: Colors.surface

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Spacing.md
                anchors.rightMargin: Spacing.md
                spacing: Spacing.md

                ModeBadge {
                    mode: header.mode
                    onClicked: header.modeRequested()
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Text {
                        Layout.fillWidth: true
                        text: header.modeDescription
                        color: Colors.textSecondary
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        elide: Text.ElideRight
                        visible: header.modeDescription.length > 0
                    }

                    Text {
                        Layout.fillWidth: true
                        text: header.automationStatus
                        color: Colors.accent
                        font.family: Typography.family
                        font.pixelSize: Typography.caption
                        elide: Text.ElideRight
                        visible: header.automationStatus.length > 0
                    }
                }

                AppIcon {
                    source: Icons.patientCategory(header.patientCategory)
                    size: Metrics.px(26)
                    color: Colors.textPrimary

                    MouseArea {
                        anchors.fill: parent
                        onClicked: header.patientRequested()
                    }
                }

                AppIcon {
                    name: "ventilator-mask"
                    size: Metrics.px(26)
                    color: Colors.textPrimary
                    opacity: header.nonInvasive ? 1.0 : 0.4
                }
            }
        }

        AlarmAnnunciator {
            Layout.fillWidth: true
            Layout.fillHeight: true

            priority: header.alarmPriority
            headline: header.alarmData ? header.alarmData.headline : ""
            detail: header.alarmData ? header.alarmData.detail : ""
            additionalCount: header.alarmData
                             ? Math.max(0, header.alarmData.activeCount - 1) : 0
            latched: header.alarmData ? header.alarmData.latched : false
            audioPaused: header.alarmData ? header.alarmData.audioPaused : false
            audioPauseRemaining: header.alarmData
                                 ? header.alarmData.audioPauseRemaining : 0
            idleStatus: header.ventilating ? qsTr("Ventilating") : qsTr("Standby")
            idleDetail: header.elapsedText

            onActivated: header.alarmCentreRequested()
        }

        Rectangle {
            Layout.preferredWidth: Metrics.railWidth
            Layout.fillHeight: true
            radius: Radius.medium
            color: Colors.surface

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Spacing.md
                anchors.rightMargin: Spacing.md
                spacing: Spacing.sm

                ClockDisplay {
                    Layout.fillWidth: true
                    timeText: header.clockData ? header.clockData.timeText : "--:--"
                    dateText: header.clockData ? header.clockData.dateText : ""
                    elapsedText: header.elapsedText
                    ventilating: header.ventilating
                }

                PowerIndicator {
                    percentage: header.batteryData ? header.batteryData.percentage : 100
                    acConnected: header.batteryData ? header.batteryData.acPower : true
                    charging: header.batteryData ? header.batteryData.isCharging : false
                    runtimeMinutes: header.batteryData
                                    ? header.batteryData.runtimeMinutes : -1
                }

                IconButton {
                    iconName: header.alarmData && header.alarmData.audioPaused
                              ? "alarm-audio-paused" : "alarm-bell"
                    iconSize: Metrics.px(22)
                    accessibleName: qsTr("Pause alarm audio")
                    onClicked: header.audioPauseRequested()
                }

                IconButton {
                    iconName: "alarm-reset"
                    iconSize: Metrics.px(22)
                    enabled: header.alarmData ? header.alarmData.resettable : false
                    accessibleName: qsTr("Reset latched alarms")
                    onClicked: header.alarmResetRequested()
                }

                IconButton {
                    iconName: header.screenLocked ? "lock" : "unlock"
                    iconSize: Metrics.px(22)
                    accessibleName: qsTr("Lock the screen")
                    onClicked: header.lockRequested()
                }
            }
        }
    }
}
