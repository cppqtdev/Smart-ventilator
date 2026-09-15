// -----------------------------------------------------------------------
// File: RingSpinner.qml
// Description: Ring gauge that sets its own value, with a caption below
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The date and time page in the reference has no step buttons beside its
// rings, so the ring itself is the control: the upper half counts up, the
// lower half counts down, holding repeats and the wheel works too. The whole
// ring is one touch target, which is larger than the two buttons a dial
// would carve out of the same space.
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
    property int ringSize: Metrics.px(103)

    signal valueSet(int newValue)

    implicitWidth: spinner.ringSize
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
        if (next === spinner.value)
            return
        spinner.value = next
        spinner.valueSet(next)
    }

    RingGauge {
        id: ring

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: spinner.ringSize
        height: spinner.ringSize

        value: spinner.value
        from: spinner.from
        to: spinner.to

        Text {
            anchors.centerIn: parent
            width: ring.width - Metrics.dialStroke * 2 - Spacing.sm
            height: width
            text: String(spinner.value)
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.dialValue
            font.weight: Typography.bold
            fontSizeMode: Text.Fit
            minimumPixelSize: Typography.caption
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        MouseArea {
            id: tap
            anchors.fill: parent
            hoverEnabled: true

            property int direction: 0

            onPressed: function (mouse) {
                tap.direction = mouse.y < tap.height / 2 ? 1 : -1
                spinner.apply(tap.direction)
                repeatDelay.restart()
            }

            onReleased: {
                repeatDelay.stop()
                repeatTick.stop()
            }

            onCanceled: {
                repeatDelay.stop()
                repeatTick.stop()
            }

            onWheel: function (wheel) {
                spinner.apply(wheel.angleDelta.y > 0 ? 1 : -1)
            }
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Metrics.px(6)
            width: Metrics.px(14)
            height: Metrics.px(2)
            radius: height
            color: tap.containsMouse ? Colors.textPrimary : Colors.textSecondary
            opacity: tap.containsMouse ? 1.0 : 0.5
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Metrics.px(6)
            width: Metrics.px(14)
            height: Metrics.px(2)
            radius: height
            color: tap.containsMouse ? Colors.textPrimary : Colors.textSecondary
            opacity: tap.containsMouse ? 1.0 : 0.5
        }
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
        onTriggered: spinner.apply(tap.direction)
    }

    Text {
        id: caption
        anchors.top: ring.bottom
        anchors.topMargin: Spacing.sm
        anchors.horizontalCenter: parent.horizontalCenter
        text: spinner.label
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.dialLabel
    }

    Accessible.role: Accessible.SpinBox
    Accessible.name: spinner.label
    Accessible.description: qsTr("%1, currently %2").arg(spinner.label).arg(spinner.value)
    Accessible.onIncreaseAction: spinner.apply(1)
    Accessible.onDecreaseAction: spinner.apply(-1)
}
