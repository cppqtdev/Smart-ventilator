// -----------------------------------------------------------------------
// File: Panel.qml
// Description: Base surface container for grouped content
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The single surface primitive. Elevation is expressed with border and
// surface tone rather than drop shadows: a blurred shadow costs a
// full-screen offscreen pass on OpenGL ES 2.0 hardware, and buys nothing
// on a matte medical panel viewed at 100-1500 lux.
//
// Two looks, chosen with `glass`:
//
//   glass: false  the default flat surface - opaque, one hairline border.
//                 This is what data-dense areas use. Tinted glass behind a
//                 waveform or a wall of numbers hurts legibility, and the
//                 waveform plot background is specified as pure black.
//
//   glass: true   the frosted treatment from the reference sheet: a blue
//                 tint at about a quarter alpha with specular streaks on the
//                 top and bottom edges. Used for chrome - dialog shells,
//                 the control rail, mode cards - where the background is
//                 allowed to read through.
//
// The glass look is not re-implemented here. It is the same GlossSurface the
// buttons use, so the panels and the buttons can never drift apart.
//
import QtQuick
import "../Controls"
import "../Theme"

Rectangle {
    id: root
    property int elevation: 1
    property color accentEdge: Colors.transparent

    /** Switches from the flat surface to the frosted glass treatment. */
    property bool glass: false

    radius: Radius.medium
    color: root.glass ? Colors.transparent : Colors.surface
    border.color: root.glass ? Colors.transparent : Colors.line
    border.width: root.glass ? 0 : 1

    // Declared first so it sits behind whatever the caller puts inside.
    GlossSurface {
        anchors.fill: parent
        visible: root.glass
        radius: root.radius
        // A panel is large and static, so the streaks sit at their resting
        // strength - a panel that lights up on hover would compete with the
        // buttons on top of it.
        interaction: 0
        selected: false
        bloom: Math.round(4 * Metrics.scale)
    }

    Rectangle {
        visible: root.accentEdge.a > 0
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: Math.max(1, root.border.width)
        width: 3
        color: root.accentEdge
    }
}
