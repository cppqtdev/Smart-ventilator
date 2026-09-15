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
    // Measured off reference/controls-basic.png, all as a fraction of the
    // ring so the shape holds at any size. The button stops just short of
    // the ring; the carve disc reaches further out than the button does, so
    // the disc's arc bites the button's inner edge. That bite is the curve,
    // and it only lands on the edge - the glyph stays clear of it.
    //
    //   ring 89, button 34.3 wide and 63 tall, button edge 3.2 short of the
    //   ring, carve disc 11.9 beyond it.
    readonly property real assemblyRatio: 1.842

    readonly property int ringSize:
        Math.max(Metrics.px(56),
                 Math.min(Metrics.dialSize,
                          Math.floor(dial.width / dial.assemblyRatio)))

    readonly property int stepWidth: Math.round(dial.ringSize * 0.385)
    readonly property int stepHeight: Math.round(dial.ringSize * 0.71)
    readonly property int stepGap: Math.max(2, Math.round(dial.ringSize * 0.036))
    readonly property int carveGap: Math.max(4, Math.round(dial.ringSize * 0.134))

    // The disc eats the inner edge of the button, so the middle of what is
    // left is not the middle of the rectangle. The glyph moves outward by
    // half the bite to sit in the centre of the part that still shows.
    readonly property int glyphShift:
        Math.round((dial.carveGap - dial.stepGap) / 2)

    readonly property real sizeRatio: dial.ringSize / Math.max(1, Metrics.dialSize)

    implicitWidth: Math.round(Metrics.dialSize * dial.assemblyRatio)
    implicitHeight: labelText.implicitHeight + Spacing.sm + dial.ringSize

    // The ring is anchored to the bottom of the item, so a caller that asks
    // for less height than the dial needs would draw the ring over whatever
    // sits above it. Layouts honour the minimum, which stops that.
    //
    // Width is different: the ring shrinks with the width it is given, so
    // the minimum is the whole assembly around the smallest ring.
    Layout.minimumWidth: Math.round(Metrics.px(56) * dial.assemblyRatio)
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
            anchors.rightMargin: dial.stepGap
            anchors.verticalCenter: ring.verticalCenter
            buttonWidth: dial.stepWidth
            buttonHeight: dial.stepHeight
            glyphOffset: -dial.glyphShift
            symbol: "−"
            onStepped: dial.propose(-1)
        }

        StepButton {
            id: plusButton
            anchors.left: ring.right
            anchors.leftMargin: dial.stepGap
            anchors.verticalCenter: ring.verticalCenter
            buttonWidth: dial.stepWidth
            buttonHeight: dial.stepHeight
            glyphOffset: dial.glyphShift
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
        property int glyphOffset: 0

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
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: step.glyphOffset
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
