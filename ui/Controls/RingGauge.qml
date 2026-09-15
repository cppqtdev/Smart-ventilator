// -----------------------------------------------------------------------
// File: RingGauge.qml
// Description: Circular progress ring drawn with QtQuick.Shapes
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Shapes
import "../Theme"

Item {
    id: gauge

    property real value: 0
    property real from: 0
    property real to: 100
    property real startAngle: -90
    property real sweepAngle: 360
    property color trackColor: Colors.controlTrack
    property color progressColor: Colors.accent

    // The reference dial is 103 across with a 7 wide ring. Holding that
    // ratio is what keeps a large gauge looking like the same control
    // rather than a thin wire circle.
    readonly property int diameter: Math.min(gauge.width, gauge.height)
    property int stroke: Math.max(Metrics.px(4), Math.round(gauge.diameter * 0.068))

    readonly property real span: Math.max(0.000001, gauge.to - gauge.from)
    readonly property real fraction:
        Math.max(0, Math.min(1, (gauge.value - gauge.from) / gauge.span))
    readonly property real radius: Math.max(1, gauge.diameter / 2 - gauge.stroke / 2)

    implicitWidth: Metrics.dialSize
    implicitHeight: Metrics.dialSize

    Shape {
        anchors.fill: parent

        // The default triangulating renderer steps the outline of a curve,
        // which reads as a ragged edge on a ring this size. The curve
        // renderer resolves the arc in the fragment shader instead.
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: gauge.trackColor
            strokeWidth: gauge.stroke
            fillColor: Colors.transparent
            capStyle: ShapePath.FlatCap

            PathAngleArc {
                centerX: gauge.width / 2
                centerY: gauge.height / 2
                radiusX: gauge.radius
                radiusY: gauge.radius
                startAngle: gauge.startAngle
                sweepAngle: gauge.sweepAngle
            }
        }

        ShapePath {
            // A zero length arc with a round cap still paints a dot, so an
            // empty gauge hides the progress path rather than shortening it.
            strokeColor: gauge.fraction > 0.001 ? gauge.progressColor : Colors.transparent
            strokeWidth: gauge.stroke
            fillColor: Colors.transparent
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: gauge.width / 2
                centerY: gauge.height / 2
                radiusX: gauge.radius
                radiusY: gauge.radius
                startAngle: gauge.startAngle
                sweepAngle: gauge.sweepAngle * gauge.fraction

                Behavior on sweepAngle {
                    NumberAnimation {
                        duration: Metrics.durationFast
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }
    }
}
