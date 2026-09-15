// -----------------------------------------------------------------------
// File: UtilitiesPane.qml
// Description: Tools > Utilities - the timed manoeuvres and alarm audio
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Every control here is timed and self-cancelling. A pause or a boost that
// has to be remembered by the operator is the failure mode these guard
// against, so each one shows the seconds it has left and ends on its own.
//
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"
import "../../Dialogs"

Item {
    id: pane

    property var ventilatorData
    property var alarmData

    readonly property bool running: pane.ventilatorData
                                    && pane.ventilatorData.running

    component UtilityRow: RowLayout {
        id: row

        property string title: ""
        property string detail: ""
        property string actionText: ""
        property string cancelText: ""
        property bool busy: false
        property bool actionEnabled: true
        property int variant: AppButton.Primary

        signal actioned()
        signal cancelled()

        Layout.fillWidth: true
        spacing: Spacing.lg

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Text {
                Layout.fillWidth: true
                text: row.title
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.readoutLabel
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: row.detail
                color: row.busy ? Colors.accent : Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.caption
                elide: Text.ElideRight
            }
        }

        AppButton {
            Layout.preferredHeight: Metrics.actionButtonHeight
            text: row.actionText
            buttonVariant: row.variant
            enabled: row.actionEnabled && !row.busy
            onClicked: row.actioned()
        }

        AppButton {
            Layout.preferredHeight: Metrics.actionButtonHeight
            text: row.cancelText
            buttonVariant: AppButton.Neutral
            visible: row.cancelText.length > 0
            enabled: row.busy
            onClicked: row.cancelled()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Spacing.xl

        UtilityRow {
            title: qsTr("Manual breath")
            detail: pane.running ? qsTr("Delivers one mandatory breath now")
                                 : qsTr("Needs ventilation running")
            actionText: qsTr("Deliver")
            actionEnabled: pane.running
                           && !(pane.ventilatorData && pane.ventilatorData.holdInProgress)
            onActioned: pane.ventilatorData.deliverManualBreath()
        }

        UtilityRow {
            title: qsTr("100 percent oxygen")
            busy: pane.ventilatorData && pane.ventilatorData.oxygenBoostActive
            detail: busy
                    ? qsTr("%1 s left, then the previous setting returns")
                          .arg(pane.ventilatorData.oxygenBoostRemaining)
                    : qsTr("Two minutes of pure oxygen before suctioning")
            actionText: qsTr("Start")
            cancelText: qsTr("End now")
            actionEnabled: pane.running
            onActioned: pane.ventilatorData.startOxygenBoost(120)
            onCancelled: pane.ventilatorData.cancelOxygenBoost()
        }

        UtilityRow {
            title: qsTr("Nebuliser")
            busy: pane.ventilatorData && pane.ventilatorData.nebuliserActive
            detail: busy
                    ? qsTr("%1 s left").arg(pane.ventilatorData.nebuliserRemaining)
                    : qsTr("Ten minutes of nebulised medication")
            actionText: qsTr("Start")
            cancelText: qsTr("Stop")
            actionEnabled: pane.running
            onActioned: pane.ventilatorData.startNebuliser(10)
            onCancelled: pane.ventilatorData.cancelNebuliser()
        }

        UtilityRow {
            title: qsTr("Alarm audio")
            busy: pane.alarmData && pane.alarmData.audioPaused
            detail: busy
                    ? qsTr("Paused, %1 s left").arg(pane.alarmData.audioPauseRemaining)
                    : qsTr("Pauses the alarm sound for up to %1 s")
                          .arg(pane.alarmData ? pane.alarmData.audioPauseMaxSeconds : 120)
            actionText: qsTr("Pause")
            cancelText: qsTr("Resume")
            variant: AppButton.Secondary
            actionEnabled: pane.alarmData !== undefined && pane.alarmData !== null
            onActioned: pane.alarmData.pauseAudio(pane.alarmData.audioPauseMaxSeconds)
            onCancelled: pane.alarmData.resumeAudio()
        }

        UtilityRow {
            title: qsTr("Latched alarms")
            detail: pane.alarmData && pane.alarmData.resettable
                    ? qsTr("%1 condition(s) can be reset").arg(pane.alarmData.activeCount)
                    : qsTr("Nothing to reset")
            actionText: qsTr("Reset")
            variant: AppButton.Secondary
            actionEnabled: pane.alarmData && pane.alarmData.resettable
            onActioned: resetLatchedConfirm.open()
        }

        Item { Layout.fillHeight: true }
    }

    ConfirmDialog {
        id: resetLatchedConfirm

        titleText: qsTr("Reset latched alarms")
        messageText: qsTr("Clear every latched alarm condition? A condition that is still present will latch again.")
        confirmText: qsTr("Reset")

        onConfirmed: {
            if (pane.alarmData)
                pane.alarmData.resetLatched()
        }
    }
}
