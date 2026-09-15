// -----------------------------------------------------------------------
// File: DialControl.qml
// Description: Ring gauge flanked by carved step buttons
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../Theme"

Item {
    id: dial

    property string label: ""
    property real value: 0
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property string unit: ""
    property int decimals: 0
    property bool editable: true
    property color progressColor: Colors.accent

    signal stepRequested(real proposedValue)
    signal valueClicked()

    // The dial appears both in the rail, where it gets its full size, and in
    // the Controls grid, where three sit side by side in one column. Sizing
    // the ring from the width it is actually given is what keeps the grid
    // from overflowing at 1024 x 768 while still filling the rail at 1920.
    readonly property int stepWidth:
        Math.max(Metrics.px(24),
                 Math.min(Metrics.dialStepWidth, Math.round(dial.width * 0.23)))

    // The carve disc is the panel colour painted over the buttons, so the
    // edge the operator sees on the inside of each one is the disc's arc.
    // That is where the reference's crescent comes from, and it only appears
    // if the button reaches further in than the disc does.
    // Both are sized from the step button rather than from the ring, because
    // the ring is sized from them and the other way round would be a loop.
    readonly property int carveGap: Math.max(Metrics.px(4),
                                             Math.round(dial.stepWidth * 0.22))

    readonly property int stepOverlap:
        dial.carveGap + Math.max(Metrics.px(3), Math.round(dial.stepWidth * 0.16))

    readonly property int ringSize:
        Math.max(Metrics.px(56),
                 Math.min(Metrics.dialSize,
                          Math.round(dial.width
                                     - (dial.stepWidth - dial.stepOverlap) * 2)))

    readonly property real sizeRatio: dial.ringSize / Math.max(1, Metrics.dialSize)

    // The reference button is 66 tall against a 103 ring. Holding that ratio
    // keeps the button inside the row when the ring has had to shrink.
    readonly property int stepHeight:
        Math.min(Metrics.dialStepHeight, Math.round(dial.ringSize * 0.64))

    implicitWidth: Metrics.dialSize + Metrics.dialStepWidth * 2
    implicitHeight: labelText.implicitHeight + Spacing.sm + dial.ringSize

    // The ring is anchored to the bottom of the item, so a caller that asks
    // for less height than the dial needs would draw the ring over whatever
    // sits above it. Layouts honour the minimum, which stops that.
    //
    // Width is different: the ring already shrinks with the width it is
    // given, so the minimum is the two step buttons plus the smallest ring.
    // Asking for the full reference width here overflowed the rail, which
    // is one pixel narrower than the dial draws at.
    Layout.minimumWidth: Metrics.px(56)
                         + (Metrics.dialStepWidth - Metrics.px(12)) * 2
    Layout.minimumHeight: dial.implicitHeight

    function propose(delta) {
        var next = dial.value + delta * dial.stepSize
        next = Math.max(dial.from, Math.min(dial.to, next))
        if (next !== dial.value)
            dial.stepRequested(next)
    }

    Text {
        id: labelText
        anchors.left: parent.left
        anchors.top: parent.top
        text: dial.label
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.dialLabel
        font.weight: Typography.bold
    }

    Item {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: dial.ringSize

        StepButton {
            id: minusButton
            anchors.right: ring.left
            anchors.rightMargin: -dial.stepOverlap
            anchors.verticalCenter: ring.verticalCenter
            buttonWidth: dial.stepWidth
            buttonHeight: dial.stepHeight
            symbol: "−"
            onStepped: dial.propose(-1)
        }

        StepButton {
            id: plusButton
            anchors.left: ring.right
            anchors.leftMargin: -dial.stepOverlap
            anchors.verticalCenter: ring.verticalCenter
            buttonWidth: dial.stepWidth
            buttonHeight: dial.stepHeight
            symbol: "+"
            onStepped: dial.propose(1)
        }

        Rectangle {
            id: carve
            anchors.centerIn: ring
            width: dial.ringSize + dial.carveGap * 2
            height: width
            radius: width / 2
            color: Colors.surface
        }

        RingGauge {
            id: ring
            anchors.centerIn: parent
            width: dial.ringSize
            height: dial.ringSize
            value: dial.value
            from: dial.from
            to: dial.to
            progressColor: dial.editable ? dial.progressColor : Colors.controlDisabled
        }

        Column {
            anchors.centerIn: ring
            spacing: 0

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: dial.value.toFixed(dial.decimals)
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Math.max(Metrics.px(14),
                                         Math.round(Typography.dialValue * dial.sizeRatio))
                font.weight: Typography.bold
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: dial.unit
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Math.max(Metrics.px(9),
                                         Math.round(Typography.dialUnit * dial.sizeRatio))
            }
        }

        MouseArea {
            anchors.fill: ring
            enabled: dial.editable
            onClicked: dial.valueClicked()
        }
    }

    component StepButton: Item {
        id: step

        property string symbol: "+"
        property int buttonWidth: Metrics.dialStepWidth
        property int buttonHeight: Metrics.dialStepHeight
        property int glyphSize: Typography.subtitle

        signal stepped()

        width: step.buttonWidth
        height: step.buttonHeight

        Rectangle {
            anchors.fill: parent
            radius: Radius.small
            color: !step.enabled ? Colors.controlDisabled
                 : stepArea.pressed ? Colors.accentPressed
                 : Colors.accent

            Behavior on color {
                ColorAnimation { duration: Metrics.durationInstant }
            }
        }

        Text {
            anchors.centerIn: parent
            text: step.symbol
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: step.glyphSize
            font.weight: Typography.bold
        }

        MouseArea {
            id: stepArea
            anchors.fill: parent
            onClicked: step.stepped()
            onPressAndHold: repeatTimer.start()
            onReleased: repeatTimer.stop()
            onCanceled: repeatTimer.stop()
        }

        Timer {
            id: repeatTimer
            interval: 90
            repeat: true
            onTriggered: step.stepped()
        }
    }
}
