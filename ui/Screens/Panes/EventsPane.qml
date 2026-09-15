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
    property int pageIndex: 0

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
