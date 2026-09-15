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
// One look: the flat surface the reference draws - opaque, one hairline
// border. Tinted glass behind a waveform or a wall of numbers hurts
// legibility, and the waveform plot background is specified as pure black.
//
// There used to be a second look, `glass`, described as a frosted treatment
// for chrome. GlossSurface stopped drawing frost when the flat reference
// look landed, and what the property did instead was fill the whole panel
// with the accent blue - the dialog that asked for it came out a solid blue
// block. The property is gone rather than left as a trap; a caller that
// wants a lifted surface uses Colors.surfaceRaised.
//
import QtQuick
import "../Theme"

Rectangle {
    id: root
    property int elevation: 1
    property color accentEdge: Colors.transparent

    radius: Radius.medium
    color: Colors.surface
    border.color: Colors.line
    border.width: 1

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
