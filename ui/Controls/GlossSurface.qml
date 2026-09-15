// -----------------------------------------------------------------------
// File: GlossSurface.qml
// Description: Flat button background matching the reference screens
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The reference design is flat: a solid fill, a corner radius, and a colour
// change for the state. No gradient, no rim light, no bloom. Every button,
// tab and icon button in the project draws through this one item, so keeping
// the old property surface here is what makes the whole set flat at once
// without touching a single call site.
//
// The frosted treatment this file used to carry is gone. Panel.glass, which
// claimed to still offer it, only filled the surface with the accent blue,
// and has been removed.
//
import QtQuick
import "../Theme"

Item {
    id: surface

    /** 0 = resting, 1 = hovered, 2 = pressed. */
    property int interaction: 0

    /** Lit fill, for the active tab or a checked button. */
    property bool selected: false

    /** Overrides the fill hue - danger buttons, alarm states. */
    property color accentColor: Colors.accent

    /** Kept for call sites that tint the resting state separately. */
    property color glowColor: Colors.accentSubtle

    /** Suppresses the fill so only the outline shows - ghost buttons. */
    property bool hollow: false

    property int radius: Radius.small

    /** No longer draws anything. Retained so existing call sites still load. */
    property int bloom: 0

    readonly property color fillColor: {
        if (!surface.enabled)
            return surface.hollow ? Colors.transparent : Colors.controlDisabled
        if (surface.interaction === 2)
            return Qt.darker(surface.selected ? surface.accentColor
                                              : surface.glowColor, 1.18)
        if (surface.selected)
            return surface.accentColor
        if (surface.interaction === 1)
            return Qt.lighter(surface.glowColor, 1.12)
        return surface.hollow ? Colors.transparent : surface.glowColor
    }

    Rectangle {
        anchors.fill: parent
        radius: surface.radius
        antialiasing: true
        color: surface.fillColor
        border.width: surface.hollow ? Metrics.borderWidth : 0
        border.color: surface.selected ? surface.accentColor : Colors.line

        Behavior on color {
            ColorAnimation { duration: Metrics.durationFast }
        }
    }
}
