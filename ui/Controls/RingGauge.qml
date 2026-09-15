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
    property int stroke: Metrics.dialStroke
    property color trackColor: Colors.controlTrack
    property color progressColor: Colors.accent

    readonly property real span: Math.max(0.000001, gauge.to - gauge.from)
    readonly property real fraction:
        Math.max(0, Math.min(1, (gauge.value - gauge.from) / gauge.span))
    readonly property real radius:
        Math.max(1, Math.min(width, height) / 2 - gauge.stroke / 2)

    implicitWidth: Metrics.dialSize
    implicitHeight: Metrics.dialSize

    Shape {
        anchors.fill: parent
        antialiasing: true
        layer.enabled: false

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
            strokeColor: gauge.progressColor
            strokeWidth: gauge.stroke
            fillColor: Colors.transparent
            capStyle: ShapePath.FlatCap

            PathAngleArc {
                centerX: gauge.width / 2
                centerY: gauge.height / 2
                radiusX: gauge.radius
                radiusY: gauge.radius
                startAngle: gauge.startAngle
                sweepAngle: gauge.sweepAngle * gauge.fraction
            }
        }
    }
}
