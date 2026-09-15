// -----------------------------------------------------------------------
// File: MetricTile.qml
// Description: Sidebar tile - label, value, alarm limits and unit
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../Controls"
import "../Theme"

Rectangle {
    id: tile

    property string label: ""
    property var value: "---"
    property string unit: ""
    property color valueColor: Colors.textPrimary
    property int priority: 0

    // Upper and lower alarm limits. Four spellings reach this component from
    // screens written at different times; the first non-empty one wins.
    property string upperLimit: ""
    property string lowerLimit: ""
    property var highValue: undefined
    property var lowValue: undefined
    property var highLimit: undefined
    property var lowLimit: undefined

    property string footnote: ""
    property bool available: true
    property bool derived: false
    property string iconName: ""

    signal activated()

    readonly property string upperText: tile.firstText(tile.upperLimit,
                                                       tile.highValue,
                                                       tile.highLimit)
    readonly property string lowerText: tile.firstText(tile.lowerLimit,
                                                       tile.lowValue,
                                                       tile.lowLimit)

    readonly property color alarmTint:
          tile.priority === 3 ? Colors.alarmHigh
        : tile.priority === 2 ? Colors.alarmMedium
        : tile.priority === 1 ? Colors.alarmLow
        : Colors.transparent

    readonly property bool critical: tile.state === "critical" || tile.priority === 3
    readonly property bool cautionary: tile.state === "warning" || tile.priority === 2

    function firstText(preferred, second, third) {
        if (preferred !== undefined && String(preferred).length > 0)
            return String(preferred)
        if (second !== undefined && second !== null)
            return String(second)
        if (third !== undefined && third !== null)
            return String(third)
        return ""
    }

    implicitHeight: Metrics.tileHeightMin
    radius: Radius.medium
    color: Colors.surface
    opacity: tile.available ? 1.0 : 0.45
    border.width: tile.critical || tile.cautionary ? Metrics.focusWidth : 0
    border.color: tile.critical ? Colors.alarmHigh
                : tile.cautionary ? Colors.alarmMedium
                : tile.alarmTint

    ColumnLayout {
        anchors.fill: parent
        // The reference tile clears its text by the same 13 on every side.
        // Anything less puts the bottom row inside the corner radius.
        anchors.margins: Metrics.px(13)
        spacing: Spacing.xs

        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.sm

            AppIcon {
                visible: tile.iconName.length > 0
                name: tile.iconName
                size: Math.round(Typography.tileLabel * 1.15)
                color: Colors.textSecondary
            }

            Text {
                Layout.alignment: Qt.AlignTop
                text: tile.label
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.tileLabel
                font.weight: Typography.bold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                Layout.alignment: Qt.AlignTop
                text: String(tile.value)
                color: tile.critical ? Colors.alarmHigh
                     : tile.cautionary ? Colors.alarmMedium
                     : tile.derived ? Colors.textSecondary
                     : tile.valueColor
                font.family: Typography.monoFamily
                font.pixelSize: Typography.tileValue
                font.weight: Typography.bold
            }
        }

        Item { Layout.fillHeight: true }

        Text {
            Layout.fillWidth: true
            text: tile.upperText
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.tileLimit
            elide: Text.ElideRight
            visible: tile.upperText.length > 0
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.sm

            // The lower limit keeps its place in the row even when it is
            // empty, because the reference puts the unit against the right
            // edge on every tile and an absent limit must not pull it left.
            Text {
                Layout.fillWidth: true
                text: tile.lowerText
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.tileLimit
                elide: Text.ElideRight
            }

            Text {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                text: tile.unit
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.tileUnit
                horizontalAlignment: Text.AlignRight
            }
        }

        Text {
            Layout.fillWidth: true
            text: tile.footnote
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.micro
            elide: Text.ElideRight
            visible: tile.footnote.length > 0 && tile.height >= Metrics.px(96)
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: tile.activated()
    }
}
