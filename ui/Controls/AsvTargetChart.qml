// -----------------------------------------------------------------------
// File: AsvTargetChart.qml
// Description: Adaptive target graph - rate against volume at one MinVol
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Rate on the bottom, tidal volume up the side, and the curve of every
// pair that delivers the same minute volume running between them. The box
// is the window the mode is allowed to choose inside; the dot is where it
// has settled. An operating point outside the box is the picture of a
// target the lung will not accept.
//
import QtQuick
import "../Theme"

Rectangle {
    id: chart

    // Keys minuteVolume, rate, tidalVolume, minRate, maxRate, minVolume,
    // maxVolume - as MonitoringPresenter.asvTarget supplies them.
    property var target: ({})

    readonly property real minuteVolume: chart.field("minuteVolume", 0)
    readonly property real rate: chart.field("rate", 0)
    readonly property real tidalVolume: chart.field("tidalVolume", 0)
    readonly property real minRate: chart.field("minRate", 15)
    readonly property real maxRate: chart.field("maxRate", 60)
    readonly property real minVolume: chart.field("minVolume", 200)
    readonly property real maxVolume: chart.field("maxVolume", 800)

    // A fixed ceiling of 1600 drew an eight kilo patient's whole curve as a
    // flat line along the bottom axis. The scale follows the window the mode
    // may choose inside, with room above it for a target that overshoots.
    readonly property real volumeCeiling:
        Math.max(60, chart.maxVolume * 1.8,
                 chart.tidalVolume * 1.3,
                 chart.minuteVolume > 0
                     ? chart.minuteVolume * 1000.0 / Math.max(1, chart.minRate) * 1.15
                     : 0)

    function field(name, fallback) {
        if (!chart.target)
            return fallback
        var value = chart.target[name]
        return (value === undefined || value === null || isNaN(value)) ? fallback : value
    }

    radius: Radius.medium
    color: Colors.surface

    onWidthChanged: plot.requestPaint()
    onHeightChanged: plot.requestPaint()
    onTargetChanged: plot.requestPaint()

    Text {
        id: heading
        anchors.left: plot.left
        anchors.top: parent.top
        anchors.topMargin: Spacing.sm
        text: qsTr("MinVol: %1 l/min").arg(chart.minuteVolume.toFixed(1))
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.caption
    }

    Text {
        id: volumeTop
        anchors.left: parent.left
        anchors.leftMargin: Spacing.sm
        anchors.top: plot.top
        text: Math.round(chart.volumeCeiling)
        color: Colors.textSecondary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.micro
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: Spacing.sm
        anchors.verticalCenter: plot.verticalCenter
        text: Math.round(chart.volumeCeiling / 2)
        color: Colors.textSecondary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.micro
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: Spacing.sm
        anchors.bottom: plot.bottom
        text: "1"
        color: Colors.textSecondary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.micro
    }

    Text {
        anchors.left: plot.left
        anchors.top: plot.bottom
        anchors.topMargin: Spacing.xs
        text: Math.round(chart.minRate)
        color: Colors.textSecondary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.micro
    }

    Text {
        anchors.right: plot.right
        anchors.top: plot.bottom
        anchors.topMargin: Spacing.xs
        text: Math.round(chart.maxRate)
        color: Colors.textSecondary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.micro
    }

    Text {
        anchors.left: plot.left
        anchors.leftMargin: Spacing.xs
        anchors.top: heading.bottom
        text: "V\nml"
        color: Colors.textSecondary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.micro
        lineHeight: 0.9
    }

    Canvas {
        id: plot

        anchors.fill: parent
        anchors.leftMargin: Metrics.px(34)
        anchors.rightMargin: Spacing.sm
        anchors.topMargin: Metrics.px(22)
        anchors.bottomMargin: Metrics.px(18)
        renderStrategy: Canvas.Cooperative

        function xFor(rateValue) {
            var span = Math.max(1, chart.maxRate - chart.minRate)
            return (rateValue - chart.minRate) / span * width
        }

        function yFor(volumeValue) {
            var span = Math.max(1, chart.volumeCeiling)
            return height - (volumeValue / span) * height
        }

        onPaint: {
            var context = getContext("2d")
            context.reset()

            context.fillStyle = Colors.surfaceRaised
            context.fillRect(0, 0, width, height)

            // The window the mode may choose inside.
            context.fillStyle = Colors.controlTrack
            var boxLeft = plot.xFor(chart.minRate)
            var boxRight = plot.xFor(chart.minRate
                                     + (chart.maxRate - chart.minRate) * 0.55)
            var boxTop = plot.yFor(chart.maxVolume)
            var boxBottom = plot.yFor(chart.minVolume)
            context.globalAlpha = 0.55
            context.fillRect(boxLeft, boxTop, boxRight - boxLeft, boxBottom - boxTop)
            context.globalAlpha = 1.0

            // Every rate and volume pair that gives the same minute volume.
            if (chart.minuteVolume > 0) {
                context.strokeStyle = Colors.accent
                context.lineWidth = Math.max(2, Metrics.px(2))
                context.beginPath()
                var started = false
                for (var r = chart.minRate; r <= chart.maxRate; r += 0.5) {
                    var volume = chart.minuteVolume * 1000.0 / r
                    var x = plot.xFor(r)
                    var y = plot.yFor(volume)
                    if (!started) {
                        context.moveTo(x, y)
                        started = true
                    } else {
                        context.lineTo(x, y)
                    }
                }
                context.stroke()
            }

            // Where it has settled.
            if (chart.rate > 0 && chart.tidalVolume > 0) {
                var px = plot.xFor(chart.rate)
                var py = plot.yFor(chart.tidalVolume)
                var radius = Math.max(4, Metrics.px(5))
                context.fillStyle = Colors.warning
                context.beginPath()
                context.arc(px, py, radius, 0, Math.PI * 2)
                context.fill()
                context.strokeStyle = Colors.textPrimary
                context.lineWidth = 1
                context.stroke()
            }
        }
    }
}
