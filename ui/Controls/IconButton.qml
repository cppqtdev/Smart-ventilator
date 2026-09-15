// -----------------------------------------------------------------------
// File: IconButton.qml
// Description: Icon-only button with a guaranteed physical touch target
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The glyph and the target are sized independently on purpose. A 24 px
// icon inside a 56 px hit area reads as light and precise while still
// clearing the ~12 mm that ANSI/AAMI HE75 wants for a gloved finger on a
// safety-critical control. Shrinking the target to fit the glyph is the
// usual way this goes wrong.
//
// accessibleName is not optional. An icon-only control with no name is
// invisible to assistive technology and to automated UI tests.
//
import QtQuick
import QtQuick.Controls.Basic
import "../Theme"

AbstractButton {
    id: control

    /** Icon registry name, e.g. "alarm-audio-paused". */
    property string iconName: ""
    property int iconSize: Math.round(24 * Metrics.scale)
    property color iconColor: control.enabled ? Colors.textPrimary : Colors.textDisabled
    property color backgroundColor: Colors.transparent
    property int radius: Radius.sm
    property bool showBorder: false

    /** Spoken/queried label. Required for icon-only controls. */
    property string accessibleName: ""

    implicitWidth: Metrics.touchTarget
    implicitHeight: Metrics.touchTarget
    hoverEnabled: true

    Accessible.role: Accessible.Button
    Accessible.name: control.accessibleName
    Accessible.onPressAction: control.clicked()

    background: GlossSurface {
        radius: control.radius
        enabled: control.enabled
        selected: control.checked
        interaction: control.pressed ? 2 : (control.hovered ? 1 : 0)
        // backgroundColor is the caller's tint override - a danger icon
        // button passes the alarm red and gets the same glass in that hue.
        accentColor: control.backgroundColor.a > 0
            ? control.backgroundColor : Colors.glassRim
        glowColor: control.backgroundColor.a > 0
            ? control.backgroundColor : Colors.glassGlow
        bloom: control.hovered || control.pressed || control.checked
            ? Math.round(5 * Metrics.scale) : Math.round(2 * Metrics.scale)
    }

    contentItem: Item {
        implicitWidth: control.iconSize
        implicitHeight: control.iconSize

        AppIcon {
            anchors.centerIn: parent
            name: control.iconName
            size: control.iconSize
            color: control.iconColor
            opacity: control.enabled ? 1.0 : 0.45
        }
    }

    // Focus ring for keyboard and encoder-wheel navigation.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        visible: control.visualFocus
        radius: control.radius + 2
        color: Colors.transparent
        border.width: Metrics.focusWidth
        border.color: Colors.focusRing
    }
}
