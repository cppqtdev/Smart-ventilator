// -----------------------------------------------------------------------
// File: StatusChip.qml
// Description: Compact icon + label status pill
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../Theme"
import "../Controls"

Rectangle {
    id: chip

    property string iconName: ""
    property string text: ""
    property color accentColor: Colors.textSecondary
    property color surfaceColor: Colors.surfaceRaised
    property bool outlined: true
    property bool emphasised: false

    implicitWidth: row.implicitWidth + Spacing.sm * 2
    implicitHeight: Math.round(32 * Metrics.scale)
    radius: Radius.sm
    color: chip.emphasised ? chip.accentColor : chip.surfaceColor
    border.width: chip.outlined && !chip.emphasised ? Metrics.borderWidth : 0
    border.color: Qt.rgba(chip.accentColor.r, chip.accentColor.g, chip.accentColor.b, 0.45)

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: Spacing.xs

        AppIcon {
            visible: chip.iconName.length > 0
            name: chip.iconName
            size: Math.round(16 * Metrics.scale)
            color: chip.emphasised ? Colors.textInverse : chip.accentColor
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            visible: chip.text.length > 0
            text: chip.text
            color: chip.emphasised ? Colors.textInverse : chip.accentColor
            font.family: Typography.family
            font.pixelSize: Typography.caption
            font.weight: Typography.semibold
            font.letterSpacing: Typography.trackCaps
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
