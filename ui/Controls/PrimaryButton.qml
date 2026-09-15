// -----------------------------------------------------------------------
// File: PrimaryButton.qml
// Description: Standard action button
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// One button, four intents. The variant decides the treatment, so a caller
// never has to reach for a raw colour and the meaning of "this is the
// dangerous one" stays consistent across every screen.
//
// The default height is a real touch target computed in millimetres, not a
// pixel constant - see Metrics.qml.
//
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../Theme"

Button {
    id: root

    property string variant: "primary"
    property string iconName: ""
    property int iconSize: Math.round(18 * Metrics.scale)
    // Escape hatch for a bespoke tint. Transparent by default so `variant`
    // governs - defaulting this to a real colour made every button that only
    // set `variant` come out in that colour regardless.
    property color buttonColor: Colors.transparent

    implicitHeight: Metrics.touchTarget
    implicitWidth: Math.max(Math.round(120 * Metrics.scale),
                            root.implicitContentWidth + Spacing.xl * 2)
    hoverEnabled: true

    font.family: Typography.family
    font.pixelSize: Typography.label
    font.weight: Typography.semibold

    // The glass body is dark whatever the variant, so the label is light in
    // all of them. Only a caller-supplied pale tint needs the dark treatment.
    readonly property color resolvedForeground:
        root.buttonColor.a > 0 && root.buttonColor.hslLightness > 0.6
        ? Colors.textInverse : Colors.glassTextOn

    background: GlossSurface {
        radius: Radius.sm
        enabled: root.enabled
        interaction: root.pressed ? 2 : (root.hovered ? 1 : 0)
        // A ghost button is the outline and nothing else.
        hollow: root.variant === "ghost"
        selected: root.variant === "primary" || root.variant === "danger"
        accentColor: root.variant === "danger" ? Colors.alarmHigh
                   : root.buttonColor.a > 0 ? root.buttonColor
                   : Colors.accent
        glowColor: root.variant === "danger" ? Colors.alarmHigh
                 : root.buttonColor.a > 0 ? root.buttonColor
                 : Colors.accentSubtle
    }

    contentItem: RowLayout {
        id: contentRow
        spacing: root.iconName.length > 0 ? Spacing.xs : 0

        Item { Layout.fillWidth: true }

        AppIcon {
            visible: root.iconName.length > 0
            name: root.iconName
            size: root.iconSize
            color: root.enabled ? root.resolvedForeground : Colors.textDisabled
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            text: root.text
            color: root.enabled ? root.resolvedForeground : Colors.textDisabled
            font: root.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            Layout.alignment: Qt.AlignVCenter
        }

        Item { Layout.fillWidth: true }
    }
}
