// -----------------------------------------------------------------------
// File: LogPane.qml
// Description: Live application log, newest first
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: pane

    property var logData
    property int minimumLevel: 0

    readonly property var levels: [
        { key: "all",     label: qsTr("All"),      level: 0 },
        { key: "notice",  label: qsTr("Controls"), level: 2 },
        { key: "warning", label: qsTr("Problems"), level: 3 }
    ]

    function levelColour(level) {
        switch (level) {
        case 4:  return Colors.alarmHigh
        case 3:  return Colors.alarmMedium
        case 2:  return Colors.accent
        default: return Colors.textSecondary
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Spacing.md

        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.md

            Repeater {
                model: pane.levels

                delegate: ChipButton {
                    required property var modelData

                    Layout.preferredWidth: Math.max(Metrics.px(96), implicitWidth)
                    text: modelData.label
                    selected: pane.minimumLevel === modelData.level
                    onClicked: pane.minimumLevel = modelData.level
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: pane.logData ? qsTr("%1 records").arg(pane.logData.count) : ""
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.caption
            }

            ChipButton {
                Layout.preferredWidth: Math.max(Metrics.px(78), implicitWidth)
                text: qsTr("Clear")
                onClicked: {
                    if (pane.logData)
                        pane.logData.clear()
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            // A filtered-out row collapses to nothing, so the list spacing is
            // carried by the row itself rather than by the view.
            spacing: 0
            model: pane.logData

            delegate: Item {
                required property string time
                required property int level
                required property string levelName
                required property string category
                required property string message

                width: ListView.view.width
                height: visible ? Metrics.px(25) : 0
                visible: level >= pane.minimumLevel

                RowLayout {
                    anchors.fill: parent
                    anchors.bottomMargin: Metrics.px(2)
                    anchors.leftMargin: Spacing.sm
                    anchors.rightMargin: Spacing.sm
                    spacing: Spacing.md

                    Text {
                        Layout.preferredWidth: Metrics.px(84)
                        text: time
                        color: Colors.textSecondary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.caption
                    }

                    Text {
                        Layout.preferredWidth: Metrics.px(62)
                        text: levelName
                        color: pane.levelColour(level)
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.caption
                    }

                    Text {
                        Layout.preferredWidth: Metrics.px(96)
                        text: category
                        color: Colors.textSecondary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.caption
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: message
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
