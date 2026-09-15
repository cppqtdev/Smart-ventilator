// -----------------------------------------------------------------------
// File: WaveformChart.qml
// Description: Labelled real-time waveform plot
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Layout follows the reference ventilator UI:
//
//   the unit sits above the plot, top-left, in small plain text
//   the plot field is pure black with a fine grid
//   axis tick values are drawn inside the field, hard against the left edge
//   the trace has a gradient fill from the curve down to the baseline
//   the live numeric readout sits top-right, in the trace colour
//
// Nothing here rasterises on the CPU. The trace, grid, fill and limit lines
// are all scene-graph geometry built by WaveformView - see
// src/render/src/WaveformView.cpp for why that matters on OpenGL ES 2.0.
//
// Vertical order across a stack of these is fixed by convention and must not
// be rearranged: pressure, then flow, then volume. It is the one waveform
// convention that is genuinely universal across manufacturers.
//
import QtQuick
import QtQuick.Layouts
import SmartVentilator.Render
import "../Theme"

Item {
    id: chart

    // -- Signal -----------------------------------------------------------
    property string title: ""
    property string unit: ""
    property var samples: []
    property color traceColor: Colors.wavePressure
    property real minimumValue: 0
    property real maximumValue: 40
    property real baselineValue: 0
    property bool frozen: false

    /** Samples across the full plot width. Sets the visible time window. */
    property int capacity: 600

    // -- Alarm limit reference lines --------------------------------------
    property real highLimit: 0
    property real lowLimit: 0
    property bool showLimits: false

    // -- Axis ticks --------------------------------------------------------
    /** Values labelled inside the plot field. Defaults to max / mid / min. */
    property var tickValues: chart.minimumValue < 0
        ? [chart.maximumValue, 0, chart.minimumValue]
        : [chart.maximumValue, chart.minimumValue]

    property int decimals: 0

    readonly property real latestValue: plot.latestValue

    implicitHeight: Metrics.waveformHeightMin

    ColumnLayout {
        anchors.fill: parent
        spacing: Math.round(4 * Metrics.scale)

        // -- Unit caption above the plot, as in the reference --------------
        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.xs

            Text {
                text: chart.unit
                color: Colors.textSecondary
                font.family: Typography.family
                font.pixelSize: Typography.caption
            }

            Text {
                visible: chart.title.length > 0
                text: chart.title
                color: chart.traceColor
                font.family: Typography.family
                font.pixelSize: Typography.caption
                font.weight: Typography.semibold
                font.letterSpacing: Typography.trackCaps
            }

            Item { Layout.fillWidth: true }

            // Frozen is a display state, not a therapy state. Saying so
            // explicitly stops a stopped trace being read as an apnoea.
            Rectangle {
                visible: chart.frozen
                implicitWidth: frozenLabel.implicitWidth + Spacing.xs * 2
                implicitHeight: Math.round(18 * Metrics.scale)
                radius: Radius.xs
                color: Colors.infoSubtle
                border.width: 1
                border.color: Colors.info

                Text {
                    id: frozenLabel
                    anchors.centerIn: parent
                    text: qsTr("FROZEN")
                    color: Colors.info
                    font.family: Typography.family
                    font.pixelSize: Typography.micro
                    font.weight: Typography.bold
                }
            }
        }

        // -- Plot field -----------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Canvas {
                id: idlePlot
                anchors.fill: parent
                visible: !chart.samples || chart.samples.length === 0
                z: 1

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.globalAlpha = 1.0
                    ctx.setLineDash([])
                    ctx.fillStyle = Colors.wavePlotBackground.toString()
                    ctx.fillRect(0, 0, width, height)

                    ctx.strokeStyle = Colors.waveGrid.toString()
                    ctx.lineWidth = 1
                    for (var col = 0; col <= 16; ++col) {
                        var x = Math.round(col * width / 16) + 0.5
                        ctx.beginPath()
                        ctx.moveTo(x, 0)
                        ctx.lineTo(x, height)
                        ctx.stroke()
                    }
                    for (var row = 0; row <= 8; ++row) {
                        var y = Math.round(row * height / 8) + 0.5
                        ctx.beginPath()
                        ctx.moveTo(0, y)
                        ctx.lineTo(width, y)
                        ctx.stroke()
                    }

                    ctx.strokeStyle = chart.traceColor.toString()
                    ctx.globalAlpha = 0.55
                    ctx.setLineDash([8, 8])
                    ctx.beginPath()
                    ctx.moveTo(0, height * 0.58)
                    ctx.lineTo(width, height * 0.58)
                    ctx.stroke()
                    ctx.setLineDash([])
                    ctx.globalAlpha = 1.0
                }

                Connections {
                    target: Colors
                    function onNightModeChanged() { idlePlot.requestPaint() }
                }

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                onVisibleChanged: if (visible) requestPaint()
            }

            WaveformView {
                id: plot
                anchors.fill: parent
                visible: chart.samples && chart.samples.length > 0

                samples: chart.samples
                capacity: chart.capacity
                minimumValue: chart.minimumValue
                maximumValue: chart.maximumValue
                baselineValue: chart.baselineValue
                frozen: chart.frozen

                lineColor: chart.traceColor
                fillColor: chart.traceColor
                backgroundColor: Colors.wavePlotBackground
                gridColor: Colors.waveGrid
                gridColorMajor: Colors.waveGridMajor
                baselineColor: Colors.waveBaseline

                lineWidth: Metrics.waveformStroke
                fillOpacity: 0.30
                showFill: true
                showGrid: true
                showBaseline: true

                gridRows: 4
                gridColumns: 8
                gridMinorDivisions: 2

                showLimits: chart.showLimits
                highLimit: chart.highLimit
                lowLimit: chart.lowLimit
                highLimitColor: Colors.waveLimitHigh
                lowLimitColor: Colors.waveLimitLow
            }

            // Axis tick values, inside the field against the left edge.
            Repeater {
                model: chart.tickValues

                Text {
                    required property var modelData
                    required property int index

                    x: Math.round(6 * Metrics.scale)
                    y: {
                        var span = chart.maximumValue - chart.minimumValue
                        if (span <= 0)
                            return 0
                        var norm = (modelData - chart.minimumValue) / span
                        var py = plot.height - norm * plot.height
                        // Nudge the extreme labels inward so they are not
                        // clipped by the plot border.
                        if (index === 0)
                            return Math.max(2, py + 2)
                        if (index === chart.tickValues.length - 1)
                            return Math.min(plot.height - height - 2, py - height - 2)
                        return py - height / 2
                    }
                    text: Number(modelData).toFixed(chart.decimals)
                    color: Colors.textMuted
                    font.family: Typography.numberFamily
                    font.features: Typography.numericFeatures
                    font.pixelSize: Typography.micro
                }
            }

            // Live value, top-right, in the trace colour.
            Text {
                z: 2
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Math.round(8 * Metrics.scale)
                text: chart.samples && chart.samples.length > 0
                    ? Number(plot.latestValue).toFixed(chart.decimals)
                    : "--"
                color: chart.traceColor
                font.family: Typography.numberFamily
                font.features: Typography.numericFeatures
                font.pixelSize: Typography.subtitle
                font.weight: Typography.semibold
            }

            // Hairline frame, so a stack of plots reads as separate fields
            // rather than one tall black area.
            Rectangle {
                z: 3
                anchors.fill: parent
                color: Colors.transparent
                border.width: 1
                border.color: Colors.line
                radius: Radius.xs
            }
        }
    }
}
