// -----------------------------------------------------------------------
// File: SystemStatusBanner.qml
// Description: Technical fault strip - storage and backend, not patient alarms
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// This carries device faults, which are not alarm conditions about the
// patient and must not be dressed as one. It uses the caution colour as a
// flat strip with no flashing, so it reads as information beside the alarm
// banner rather than competing with it.
//
import QtQuick
import QtQuick.Layouts
import "../Theme"

Rectangle {
    id: root

    property var databaseData
    property var ventilatorData

    readonly property bool active: (databaseData && databaseData.degraded)
                                   || (databaseData && databaseData.readOnly)
                                   || (ventilatorData && !ventilatorData.backendConnected)

    readonly property string summary: {
        var messages = []
        if (root.databaseData && root.databaseData.degraded)
            messages.push(qsTr("Storage: %1").arg(root.databaseData.storageState))
        if (root.databaseData && root.databaseData.readOnly)
            messages.push(qsTr("Filesystem is read only"))
        if (root.ventilatorData && !root.ventilatorData.backendConnected)
            messages.push(qsTr("Backend: %1").arg(root.ventilatorData.backendState))
        return messages.join("   -   ")
    }

    visible: root.active
    radius: Radius.small
    color: Colors.warning

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Spacing.lg
        anchors.rightMargin: Spacing.lg
        spacing: Spacing.md

        Text {
            text: qsTr("SYSTEM")
            color: Colors.background
            font.family: Typography.monoFamily
            font.pixelSize: Typography.caption
            font.weight: Typography.bold
        }

        Text {
            Layout.fillWidth: true
            text: root.summary
            color: Colors.background
            font.family: Typography.family
            font.pixelSize: Typography.caption
            elide: Text.ElideRight
        }
    }
}
