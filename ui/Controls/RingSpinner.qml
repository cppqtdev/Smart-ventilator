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
    // The reference ring measures 90 across. Three of these have to sit in
    // a content column 484 wide, so the step buttons are narrower than the
    // rail ones: 3 * (90 + 56) + two gutters comes to 470.
    property int ringSize: Metrics.px(90)
    property int stepWidth: Metrics.px(28)
    property int stepHeight: Metrics.px(58)

    signal valueSet(int newValue)

    readonly property int gaugeStroke:
        Math.max(Metrics.px(4), Math.round(spinner.ringSize * 0.068))

    implicitWidth: spinner.ringSize + spinner.stepWidth * 2
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
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            buttonWidth: spinner.stepWidth
            buttonHeight: spinner.stepHeight
            symbol: "−"
            onStepped: spinner.apply(-1)
        }

        StepButton {
            id: plusButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            buttonWidth: spinner.stepWidth
            buttonHeight: spinner.stepHeight
            symbol: "+"
            onStepped: spinner.apply(1)
        }

        // The step buttons sit under the ring, so the surface colour is cut
        // back in behind it and the ring reads as one piece.
        Rectangle {
            anchors.centerIn: ring
            width: ring.width + Metrics.px(8)
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
                font.pixelSize: Typography.dialValue
                font.weight: Typography.bold
                fontSizeMode: Text.Fit
                minimumPixelSize: Typography.caption
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                anchors.fill: parent
                onWheel: function (wheel) {
                    spinner.apply(wheel.angleDelta.y > 0 ? 1 : -1)
                }
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
        font.pixelSize: Typography.dialLabel
    }

    Accessible.role: Accessible.SpinBox
    Accessible.name: spinner.label
    Accessible.description: qsTr("%1, currently %2").arg(spinner.label).arg(spinner.value)
    Accessible.onIncreaseAction: spinner.apply(1)
    Accessible.onDecreaseAction: spinner.apply(-1)

    component StepButton: Item {
        id: step

        property string symbol: "+"
        property int buttonWidth: Metrics.px(28)
        property int buttonHeight: Metrics.px(58)

        signal stepped()

        width: step.buttonWidth
        height: step.buttonHeight

        Rectangle {
            anchors.fill: parent
            radius: Radius.small
            color: stepArea.pressed ? Colors.accentPressed : Colors.accent

            Behavior on color {
                ColorAnimation { duration: Metrics.durationInstant }
            }
        }

        Text {
            anchors.centerIn: parent
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
