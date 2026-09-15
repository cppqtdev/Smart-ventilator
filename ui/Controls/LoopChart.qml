// -----------------------------------------------------------------------
// File: LoopChart.qml
// Description: XY plot for pressure-volume and flow-volume respiratory loops
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------

import QtQuick
import "../Theme"
import "../Components"

Panel {
    id: root

    property string title: "P-V Loop"
    property string xLabel: "Volume (mL)"
    property string yLabel: "Pressure (cmH2O)"
    property var xSamples: []
    property var ySamples: []
    property real xMinimum: 0
    property real xMaximum: 800
    property real yMinimum: 0
    property real yMaximum: 60
    property color traceColor: Colors.cyan

    // Draggable cursors, as { value, label, active }. The chart owns the
    // plot margins, so it is the only thing that can turn a value on the x
    // axis into a position the operator can grab.
    property var cursors: []

    signal cursorMoved(int index, real value)

    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 16
        text: root.title
        color: root.traceColor
        font.pixelSize: Typography.subtitle
        font.weight: Font.DemiBold
    }

    Text {
        anchors.horizontalCenter: chart.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        text: root.xLabel
        color: Colors.textMuted
        font.pixelSize: Typography.caption
    }

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: chart.verticalCenter
        anchors.leftMargin: 6
        rotation: -90
        text: root.yLabel
        color: Colors.textMuted
        font.pixelSize: Typography.caption
    }

    Canvas {
        id: chart
        anchors.fill: parent
        anchors.leftMargin: 58
        anchors.rightMargin: 24
        anchors.topMargin: 52
        anchors.bottomMargin: 42
        renderStrategy: Canvas.Cooperative

        onPaint: {
            var ctx = getContext("2d")
            var w = width
            var h = height
            ctx.reset()
            ctx.clearRect(0, 0, w, h)

            ctx.lineWidth = 1
            for (var i = 0; i <= 5; ++i) {
                ctx.strokeStyle = i === 0 || i === 5
                    ? Qt.rgba(1, 1, 1, 0.16)
                    : Qt.rgba(1, 1, 1, 0.07)
                ctx.beginPath()
                ctx.moveTo(0, i * h / 5)
                ctx.lineTo(w, i * h / 5)
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(i * w / 5, 0)
                ctx.lineTo(i * w / 5, h)
                ctx.stroke()
            }

            var xs = root.xSamples
            var ys = root.ySamples
            var count = Math.min(xs ? xs.length : 0, ys ? ys.length : 0)
            if (count < 2)
                return

            var xr = Math.max(0.1, root.xMaximum - root.xMinimum)
            var yr = Math.max(0.1, root.yMaximum - root.yMinimum)
            function point(index) {
                var nx = (xs[index] - root.xMinimum) / xr
                var ny = (ys[index] - root.yMinimum) / yr
                return {
                    x: Math.max(0, Math.min(1, nx)) * w,
                    y: h - Math.max(0, Math.min(1, ny)) * h
                }
            }

            ctx.strokeStyle = root.traceColor
            ctx.lineWidth = 2.5
            ctx.lineJoin = "round"
            ctx.lineCap = "round"
            ctx.beginPath()
            var first = point(0)
            ctx.moveTo(first.x, first.y)
            for (var j = 1; j < count; ++j) {
                var p = point(j)
                ctx.lineTo(p.x, p.y)
            }
            ctx.stroke()

            var latest = point(count - 1)
            ctx.fillStyle = root.traceColor
            ctx.beginPath()
            ctx.arc(latest.x, latest.y, 4, 0, Math.PI * 2)
            ctx.fill()
        }

        function xForValue(value) {
            var span = Math.max(0.1, root.xMaximum - root.xMinimum)
            return Math.max(0, Math.min(1, (value - root.xMinimum) / span)) * width
        }

        function valueForX(x) {
            var span = Math.max(0.1, root.xMaximum - root.xMinimum)
            return root.xMinimum + Math.max(0, Math.min(1, x / Math.max(1, width))) * span
        }

        Repeater {
            model: root.cursors

            delegate: Item {
                id: cursor

                required property var modelData
                required property int index

                x: chart.xForValue(cursor.modelData.value) - width / 2
                y: 0
                width: Metrics.px(22)
                height: chart.height

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.max(1, Metrics.borderWidth)
                    height: parent.height
                    color: cursor.modelData.active ? Colors.warning : Colors.textSecondary
                    opacity: cursor.modelData.active ? 1.0 : 0.55
                }

                // The grab target is wider than the line, because a one pixel
                // line is not something a gloved finger can take hold of.
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    width: Metrics.px(18)
                    height: Metrics.px(18)
                    radius: width / 2
                    color: cursor.modelData.active ? Colors.warning : Colors.surfaceRaised
                    border.width: Metrics.borderWidth
                    border.color: Colors.line

                    Text {
                        anchors.centerIn: parent
                        text: cursor.modelData.label !== undefined
                              ? cursor.modelData.label : ""
                        color: Colors.textPrimary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.micro
                        font.weight: Typography.bold
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.SizeHorCursor

                    onPositionChanged: function (mouse) {
                        var centre = cursor.x + cursor.width / 2 + mouse.x - cursor.width / 2
                        root.cursorMoved(cursor.index, chart.valueForX(centre))
                    }
                    onPressed: root.cursorMoved(cursor.index,
                                                chart.valueForX(cursor.x + cursor.width / 2))
                }
            }
        }

        Connections {
            target: root
            function onXSamplesChanged() { chart.requestPaint() }
            function onYSamplesChanged() { chart.requestPaint() }
            function onTraceColorChanged() { chart.requestPaint() }
        }
    }

    Text {
        anchors.left: chart.left
        anchors.bottom: chart.top
        anchors.bottomMargin: 3
        text: root.yMaximum
        color: Colors.textUnit
        font.pixelSize: 10
    }

    Text {
        anchors.left: chart.left
        anchors.top: chart.bottom
        anchors.topMargin: 3
        text: root.xMinimum
        color: Colors.textUnit
        font.pixelSize: 10
    }

    Text {
        anchors.right: chart.right
        anchors.top: chart.bottom
        anchors.topMargin: 3
        text: root.xMaximum
        color: Colors.textUnit
        font.pixelSize: 10
    }
}
