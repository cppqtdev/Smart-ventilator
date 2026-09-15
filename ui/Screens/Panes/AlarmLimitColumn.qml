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

    function field(name, fallback) {
        var value = column.entry[name]
        return value === undefined ? fallback : value
    }

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
        column.field("to", 100) - column.field("from", 0))

    spacing: Spacing.md

    Item { Layout.fillHeight: true }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: column.field("label", "")
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.readoutLabel
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: column.field("unit", "")
        color: Colors.textSecondary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.readoutUnit
    }

    AlarmLimitRing {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: column.gaugeSize
        Layout.preferredHeight: column.gaugeSize
        from: column.field("from", 0)
        to: column.field("to", 100)
        textWidth: column.gaugeTextWidth
        limitValue: column.field("high", 0)
        limitKey: column.field("highKey", "")
        crossed: column.field("highDisabled", false)
        onPressed: column.limitRequested(column.field("highKey", ""),
                                         column.field("high", 0))
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
                var current = column.field("current", 0)
                var base = column.field("from", 0)
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
            text: column.show(column.field("current", undefined))
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.micro
        }
    }

    AlarmLimitRing {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: column.gaugeSize
        Layout.preferredHeight: column.gaugeSize
        from: column.field("from", 0)
        to: column.field("to", 100)
        textWidth: column.gaugeTextWidth
        limitValue: column.field("low", 0)
        limitKey: column.field("lowKey", "")
        crossed: column.field("lowDisabled", false)
        onPressed: column.limitRequested(column.field("lowKey", ""),
                                         column.field("low", 0))
    }

    Item { Layout.fillHeight: true }
}
