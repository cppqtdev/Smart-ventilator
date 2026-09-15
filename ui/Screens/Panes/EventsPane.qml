// -----------------------------------------------------------------------
// File: EventsPane.qml
// Description: Events screen - the chronological log
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: pane

    property var eventData
    property var logData
    property var database
    property int pageIndex: 0

    // Where the last export went, so the operator can be told rather than
    // left wondering whether the press did anything.
    property string lastExportPath: ""
    property bool lastExportFailed: false

    readonly property var filters: [
        { key: "all",     label: qsTr("All") },
        { key: "alarm",   label: qsTr("Alarms") },
        { key: "setting", label: qsTr("Settings") },
        { key: "log",     label: qsTr("System Log") }
    ]

    function priorityColour(severity) {
        switch (String(severity).toLowerCase()) {
        case "high":
        case "critical":
        case "alarm":    return Colors.alarmHigh
        case "medium":
        case "warning":  return Colors.alarmMedium
        case "low":      return Colors.alarmLow
        default:         return Colors.transparent
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Metrics.gutter

        RowLayout {
            Layout.fillWidth: true
            spacing: Metrics.gutter

            SubTabStrip {
                Layout.fillWidth: true
                model: pane.filters
                currentIndex: pane.pageIndex
                onActivated: function (index) {
                    pane.pageIndex = index
                    if (pane.eventData && index !== 3)
                        pane.eventData.filter = pane.filters[index].key
                }
            }

            // The export existed and nothing called it, so an audit trail
            // that cannot leave the device is an audit trail nobody can
            // review after an incident.
            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(96), implicitWidth)
                Layout.preferredHeight: Metrics.px(34)
                text: qsTr("Export")
                buttonVariant: AppButton.Primary
                enabled: pane.database !== null && pane.database !== undefined
                onClicked: {
                    if (!pane.database)
                        return
                    var written = pane.database.exportAuditTrail("")
                    pane.lastExportPath = written
                    pane.lastExportFailed = written.length === 0
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: pane.lastExportPath.length > 0 || pane.lastExportFailed
            text: pane.lastExportFailed
                  ? qsTr("Export failed. The audit trail was not written.")
                  : qsTr("Exported to %1").arg(pane.lastExportPath)
            color: pane.lastExportFailed ? Colors.critical : Colors.textSecondary
            elide: Text.ElideMiddle
            font.family: Typography.monoFamily
            font.pixelSize: Typography.micro
        }

        LogPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: pane.pageIndex === 3
            logData: pane.logData
        }

        ListView {
            id: list
            visible: pane.pageIndex !== 3
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Metrics.px(2)
            model: pane.eventData

            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                text: qsTr("No entries.")
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.readoutLabel
            }

            delegate: Rectangle {
                // EventController is a QAbstractListModel, so the delegate
                // reads its roles rather than a modelData object.
                required property string time
                required property string source
                required property string description
                required property string severity

                width: ListView.view.width
                height: Metrics.px(23)
                color: pane.priorityColour(severity)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Spacing.md
                    anchors.rightMargin: Spacing.md
                    spacing: Spacing.lg

                    Text {
                        Layout.preferredWidth: Metrics.px(72)
                        text: time
                        color: Colors.textPrimary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.caption
                    }

                    Text {
                        Layout.preferredWidth: Metrics.px(78)
                        text: source
                        color: Colors.textPrimary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.caption
                    }

                    Text {
                        Layout.fillWidth: true
                        text: description
                        color: Colors.textPrimary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.caption
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
