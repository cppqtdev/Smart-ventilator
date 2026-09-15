// -----------------------------------------------------------------------
// File: AlarmLimitRing.qml
// Description: One alarm limit ring, editable or a fixed bound
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// A ring with a limit key is pressable and opens the editor for that limit.
// One without is a fixed bound the operator cannot move, which is how the
// reference draws the apnea floor. A crossed ring is a limit that does not
// apply at all, which is how the reference draws the upper saturation one.
//
import QtQuick
import "../../Controls"
import "../../Theme"

RingGauge {
    id: ring

    property real limitValue: 0
    property string limitKey: ""
    property bool crossed: false
    property int textWidth: Metrics.px(90)

    signal pressed()

    value: ring.crossed ? ring.from : ring.limitValue
    progressColor: ring.limitKey.length > 0 ? Colors.accent : Colors.controlDisabled

    function show(value) {
        if (value === undefined || value === null)
            return "---"
        if (isNaN(value))
            return String(value)
        return Number.isInteger(value) ? String(value) : Number(value).toFixed(1)
    }

    Text {
        anchors.centerIn: parent
        width: ring.textWidth
        height: width
        visible: !ring.crossed
        text: ring.show(ring.limitValue)
        color: ring.limitKey.length > 0 ? Colors.textPrimary : Colors.textSecondary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.readoutValue
        font.weight: Typography.bold
        fontSizeMode: Text.Fit
        minimumPixelSize: Typography.caption
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Canvas {
        id: cross
        anchors.centerIn: parent
        width: Math.round(ring.textWidth * 0.68)
        height: width
        visible: ring.crossed

        onPaint: {
            var context = getContext("2d")
            context.reset()
            context.strokeStyle = Colors.textSecondary
            context.lineWidth = Math.max(2, Math.round(cross.width * 0.07))
            context.beginPath()
            context.moveTo(0, 0)
            context.lineTo(cross.width, cross.height)
            context.moveTo(cross.width, 0)
            context.lineTo(0, cross.height)
            context.stroke()
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: ring.limitKey.length > 0
        onClicked: ring.pressed()
    }
}
