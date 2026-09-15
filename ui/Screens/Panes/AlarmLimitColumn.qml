// -----------------------------------------------------------------------
// File: AlarmLimitColumn.qml
// Description: High limit, current-value bar and low limit for one alarm
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

ColumnLayout {
    id: column

    property var entry: ({})

    signal limitRequested(string key, real value)

    // A four digit limit has to sit inside the ring, so the ring is sized
    // first and the value is fitted to the clear space inside the stroke.
    // The pane sets the size, because only the pane knows how many columns
    // share the width.
    property int gaugeSize: Metrics.px(126)

    // RingGauge sizes its stroke from its diameter, so the clear space
    // inside it has to be worked out the same way.
    readonly property int gaugeStroke:
        Math.max(Metrics.px(4), Math.round(column.gaugeSize * 0.068))
    readonly property int gaugeTextWidth:
        column.gaugeSize - column.gaugeStroke * 2 - Spacing.sm

    // Limits arrive as doubles. A raw one prints as 15.200000000000001,
    // which is unreadable on a ring this size.
    function show(value) {
        if (value === undefined || value === null)
            return "---"
        if (isNaN(value))
            return String(value)
        return Number.isInteger(value) ? String(value) : Number(value).toFixed(1)
    }

    readonly property real span: Math.max(0.000001,
        (column.entry.to !== undefined ? column.entry.to : 100)
        - (column.entry.from !== undefined ? column.entry.from : 0))

    spacing: Spacing.md

    Item { Layout.fillHeight: true }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: column.entry.label !== undefined ? column.entry.label : ""
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.readoutLabel
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: column.entry.unit !== undefined ? column.entry.unit : ""
        color: Colors.textSecondary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.readoutUnit
    }

    RingGauge {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: column.gaugeSize
        Layout.preferredHeight: column.gaugeSize
        value: column.entry.high !== undefined ? column.entry.high : 0
        from: column.entry.from !== undefined ? column.entry.from : 0
        to: column.entry.to !== undefined ? column.entry.to : 100

        Text {
            anchors.centerIn: parent
            width: column.gaugeTextWidth
            height: column.gaugeTextWidth
            text: column.show(column.entry.high)
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.readoutValue
            font.weight: Typography.bold
            fontSizeMode: Text.Fit
            minimumPixelSize: Typography.caption
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        MouseArea {
            anchors.fill: parent
            onClicked: column.limitRequested(
                column.entry.key !== undefined ? column.entry.key : "",
                column.entry.high !== undefined ? column.entry.high : 0)
        }
    }

    Item {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.round(column.gaugeSize * 0.22)
        Layout.preferredHeight: Math.round(column.gaugeSize * 0.78)

        Rectangle {
            anchors.fill: parent
            radius: Radius.xs
            color: Colors.controlTrack
        }

        Rectangle {
            readonly property real fraction: {
                var current = column.entry.current !== undefined ? column.entry.current : 0
                var base = column.entry.from !== undefined ? column.entry.from : 0
                return Math.max(0, Math.min(1, (current - base) / column.span))
            }

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Math.max(Metrics.px(6), parent.height * fraction)
            radius: Radius.xs
            color: Colors.accent
        }

        Text {
            anchors.left: parent.right
            anchors.leftMargin: Spacing.xs
            anchors.bottom: parent.bottom
            text: column.entry.current !== undefined ? column.show(column.entry.current) : ""
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.micro
        }
    }

    RingGauge {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: column.gaugeSize
        Layout.preferredHeight: column.gaugeSize
        value: column.entry.low !== undefined ? column.entry.low : 0
        from: column.entry.from !== undefined ? column.entry.from : 0
        to: column.entry.to !== undefined ? column.entry.to : 100

        Text {
            anchors.centerIn: parent
            width: column.gaugeTextWidth
            height: column.gaugeTextWidth
            text: column.show(column.entry.low)
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.readoutValue
            font.weight: Typography.bold
            fontSizeMode: Text.Fit
            minimumPixelSize: Typography.caption
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    Item { Layout.fillHeight: true }
}
