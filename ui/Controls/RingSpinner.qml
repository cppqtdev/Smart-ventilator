// -----------------------------------------------------------------------
// File: RingSpinner.qml
// Description: Spin box - minus, ring value and plus, with a caption below
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The reference draws the date and time values as plain rings, and the rail
// on the same screen draws a settable value as minus, ring, plus. A ring on
// its own gives the operator nothing to press, so this control keeps the
// reference ring and borrows the step buttons the reference already uses.
//
// The control never writes its own value. The owner binds value and updates
// it in valueSet, so a revert or a clamp from elsewhere still reaches the
// ring. Assigning to value here would break that binding on the first press.
//
import QtQuick
import "../Theme"

Item {
    id: spinner

    property string label: ""
    property int value: 0
    property int from: 0
    property int to: 59
    property int step: 1
    property bool wrap: true
    // The owner sets the ring; everything else follows it, so the control
    // keeps the proportions of the rail dial at whatever size it is given.
    property int ringSize: Metrics.px(90)

    // The proportions measured off the reference dial: the button stops
    // just short of the ring and the carve disc reaches further out than
    // the button does, so the disc's arc bites the button's inner edge.
    // The glyph sits outside the bite and stays readable.
    readonly property int stepWidth: Math.round(spinner.ringSize * 0.385)
    readonly property int stepHeight: Math.round(spinner.ringSize * 0.71)
    readonly property int stepGap: Math.max(2, Math.round(spinner.ringSize * 0.036))
    readonly property int carveGap: Math.max(4, Math.round(spinner.ringSize * 0.134))

    // The disc eats the inner edge of the button, so the middle of what is
    // left is not the middle of the rectangle. The glyph moves outward by
    // half the bite to sit in the centre of the part that still shows.
    readonly property int glyphShift:
        Math.round((spinner.carveGap - spinner.stepGap) / 2)

    // Text.Fit only ever shrinks, so a bigger ring would keep the reference
    // sized number in the middle of it. The value grows with the ring and
    // Fit is left to handle the four digit year.
    readonly property real sizeRatio: spinner.ringSize / Math.max(1, Metrics.px(90))

    signal valueSet(int newValue)

    readonly property int gaugeStroke:
        Math.max(Metrics.px(4), Math.round(spinner.ringSize * 0.068))

    implicitWidth: spinner.ringSize
                   + (spinner.stepWidth + spinner.stepGap) * 2
    implicitHeight: spinner.ringSize + Spacing.sm + caption.implicitHeight

    function clampValue(candidate) {
        if (candidate > spinner.to)
            return spinner.wrap ? spinner.from : spinner.to
        if (candidate < spinner.from)
            return spinner.wrap ? spinner.to : spinner.from
        return candidate
    }

    function apply(delta) {
        var next = spinner.clampValue(spinner.value + delta * spinner.step)
        if (next !== spinner.value)
            spinner.valueSet(next)
    }

    Item {
        id: row

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: spinner.implicitWidth
        height: spinner.ringSize

        StepButton {
            id: minusButton
            anchors.right: ring.left
            anchors.rightMargin: spinner.stepGap
            anchors.verticalCenter: ring.verticalCenter
            buttonWidth: spinner.stepWidth
            buttonHeight: spinner.stepHeight
            glyphOffset: -spinner.glyphShift
            symbol: "−"
            onStepped: spinner.apply(-1)
        }

        StepButton {
            id: plusButton
            anchors.left: ring.right
            anchors.leftMargin: spinner.stepGap
            anchors.verticalCenter: ring.verticalCenter
            buttonWidth: spinner.stepWidth
            buttonHeight: spinner.stepHeight
            glyphOffset: spinner.glyphShift
            symbol: "+"
            onStepped: spinner.apply(1)
        }

        Rectangle {
            anchors.centerIn: ring
            width: ring.width + spinner.carveGap * 2
            height: width
            radius: width / 2
            color: Colors.surface
        }

        RingGauge {
            id: ring
            anchors.centerIn: parent
            width: spinner.ringSize
            height: spinner.ringSize
            value: spinner.value
            from: spinner.from
            to: spinner.to

            Text {
                anchors.centerIn: parent
                width: ring.width - spinner.gaugeStroke * 2 - Spacing.sm
                height: width
                text: String(spinner.value)
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Math.max(Metrics.px(14),
                                         Math.round(Typography.dialValue * spinner.sizeRatio))
                font.weight: Typography.bold
                fontSizeMode: Text.Fit
                minimumPixelSize: Typography.caption
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Text {
        id: caption
        anchors.top: row.bottom
        anchors.topMargin: Spacing.sm
        anchors.horizontalCenter: parent.horizontalCenter
        text: spinner.label
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: Math.max(Typography.caption,
                                 Math.round(Typography.dialLabel * spinner.sizeRatio))
    }

    component StepButton: Item {
        id: step

        property string symbol: "+"
        property int buttonWidth: Metrics.px(30)
        property int buttonHeight: Metrics.px(58)
        property int glyphOffset: 0

        signal stepped()

        width: step.buttonWidth
        height: step.buttonHeight

        Rectangle {
            anchors.fill: parent
            // 4, not a scaled token: a corner this small reads as a corner at
            // every scale, and Radius.xs came out at 6 to 8 on a real panel.
            radius: 4
            color: stepArea.pressed ? Colors.accentPressed : Colors.accent

            Behavior on color {
                ColorAnimation { duration: Metrics.durationInstant }
            }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: step.glyphOffset
            text: step.symbol
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.subtitle
            font.weight: Typography.bold
        }

        MouseArea {
            id: stepArea
            anchors.fill: parent
            onClicked: step.stepped()
            onPressAndHold: repeatDelay.start()
            onReleased: { repeatDelay.stop(); repeatTick.stop() }
            onCanceled: { repeatDelay.stop(); repeatTick.stop() }
        }

        Timer {
            id: repeatDelay
            interval: Metrics.durationHold
            onTriggered: repeatTick.start()
        }

        Timer {
            id: repeatTick
            interval: Metrics.durationRepeat
            repeat: true
            onTriggered: step.stepped()
        }
    }
}
