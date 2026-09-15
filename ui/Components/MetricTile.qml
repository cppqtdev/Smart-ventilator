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

    // A Text reserves a whole line box, and for the 32 value that is nearly
    // twice what the digits occupy. Each row is given the height its glyphs
    // need rather than the line box the font asks for.
    readonly property int valueRowHeight: Math.round(Typography.tileValue * 1.08)
    readonly property int limitRowHeight: Math.round(Typography.tileLimit * 1.15)
    readonly property int unitRowHeight: Math.round(Typography.tileUnit * 1.15)

    implicitHeight: Metrics.tileHeightMin
    radius: Radius.medium
    color: Colors.surface
    opacity: tile.available ? 1.0 : 0.45
    border.width: tile.critical || tile.cautionary ? Metrics.focusWidth : 0
    border.color: tile.critical ? Colors.alarmHigh
                : tile.cautionary ? Colors.alarmMedium
                : tile.alarmTint

    // The label and the reading sit at the top; the two limit lines are
    // anchored to the bottom. Stacking all four in one column left the
    // bottom line 3 from the edge, inside the corner radius, because three
    // Text line boxes come to more than the tile holds. Anchoring the
    // bottom block makes the clearance the margin, whatever the metrics of
    // the font turn out to be.
    Item {
        anchors.fill: parent
        anchors.margins: Metrics.px(13)

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: tile.valueRowHeight
            spacing: Spacing.sm

            AppIcon {
                visible: tile.iconName.length > 0
                name: tile.iconName
                size: Math.round(Typography.tileLabel * 1.15)
                color: Colors.textSecondary
            }

            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: tile.label
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.tileLabel
                font.weight: Typography.bold
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                Layout.fillHeight: true
                text: String(tile.value)
                color: tile.critical ? Colors.alarmHigh
                     : tile.cautionary ? Colors.alarmMedium
                     : tile.derived ? Colors.textSecondary
                     : tile.valueColor
                font.family: Typography.monoFamily
                font.pixelSize: Typography.tileValue
                font.weight: Typography.bold
                verticalAlignment: Text.AlignVCenter
            }
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: 0

            Text {
                width: parent.width
                height: visible ? tile.limitRowHeight : 0
                text: tile.footnote
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.micro
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                visible: tile.footnote.length > 0
            }

            Text {
                width: parent.width
                height: visible ? tile.limitRowHeight : 0
                text: tile.upperText
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.tileLimit
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                visible: tile.upperText.length > 0
            }

            Item {
                width: parent.width
                height: tile.unitRowHeight

                // The lower limit keeps its place even when it is empty,
                // because the reference puts the unit against the right edge
                // on every tile and an absent limit must not pull it left.
                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - unitText.width - Spacing.sm
                    text: tile.lowerText
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.tileLimit
                    elide: Text.ElideRight
                }

                Text {
                    id: unitText
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: tile.unit
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.tileUnit
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: tile.activated()
    }
}
