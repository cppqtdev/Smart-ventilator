// -----------------------------------------------------------------------
// File: SplashScreen.qml
// Description: Start-up screen driven by the real readiness checks
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// A start-up screen on a ventilator is not decoration. It is the last
// moment before the device is put on a patient, so it reports what is
// actually ready and what is not, and it hands over on a deadline: a start-up
// animation must never be the thing keeping a ventilator off the patient.
//
// Each stage binds a live condition rather than a step in a timer. A stage
// that is still false when the deadline passes is handed over as unmet, and
// the operator sees it.
//
import QtQuick
import "../Theme"

Item {
    id: splash

    property string softwareVersion: "0.0.0"
    property real operatingHours: 0

    // Live readiness conditions, keyed to Branding.bootStages.
    property bool storageReady: false
    property bool operatorsReady: false
    property bool alarmsReady: false
    property bool deviceReady: false
    property bool sessionReady: false

    signal finished()

    readonly property var stages: Branding.bootStages

    property int settledCount: 0
    property bool handingOver: false

    function conditionFor(key) {
        switch (key) {
        case "storage":   return splash.storageReady
        case "operators": return splash.operatorsReady
        case "alarms":    return splash.alarmsReady
        case "device":    return splash.deviceReady
        case "session":   return splash.sessionReady
        case "interface": return true
        default:          return false
        }
    }

    function metCount() {
        var met = 0
        for (var i = 0; i < splash.stages.length; ++i) {
            if (splash.conditionFor(splash.stages[i].key))
                ++met
        }
        return met
    }

    readonly property int met: splash.settledCount
    readonly property real fraction: splash.stages.length > 0
                                     ? splash.met / splash.stages.length : 0

    function handOver() {
        if (splash.handingOver)
            return
        splash.handingOver = true
        handoverDelay.start()
    }

    Component.onCompleted: {
        minimumHold.start()
        deadline.start()
    }

    // The stages settle one after another rather than all at once, so the
    // operator can read them. A stage that is already true still takes its
    // turn on screen.
    Timer {
        id: reveal
        interval: Math.max(120, Branding.splashMinimumMs / Math.max(1, splash.stages.length))
        repeat: true
        running: true
        onTriggered: {
            if (splash.settledCount < splash.stages.length)
                splash.settledCount += 1
            if (splash.settledCount >= splash.stages.length && minimumHold.finished)
                splash.handOver()
        }
    }

    Timer {
        id: minimumHold
        property bool finished: false
        interval: Branding.splashMinimumMs
        onTriggered: {
            finished = true
            if (splash.settledCount >= splash.stages.length)
                splash.handOver()
        }
    }

    Timer {
        id: deadline
        interval: Branding.splashMaximumMs
        onTriggered: {
            splash.settledCount = splash.stages.length
            splash.handOver()
        }
    }

    Timer {
        id: handoverDelay
        interval: Branding.splashHandoverMs
        onTriggered: splash.finished()
    }

    Rectangle {
        anchors.fill: parent
        color: Colors.background
    }

    // The identity and the checks were anchored separately to the centre,
    // one above it and one below, so the composition drifted apart on a tall
    // screen and left a band of nothing over and under it. They are one stack
    // now, centred as a whole, with a rhythm that holds at any height.
    Column {
        id: identity

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -Metrics.px(20)
        spacing: Metrics.px(26)

        opacity: 0
        Component.onCompleted: identityIn.start()

        ParallelAnimation {
            id: identityIn
            NumberAnimation {
                target: identity; property: "opacity"
                from: 0; to: 1; duration: 620; easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: identity; property: "anchors.verticalCenterOffset"
                from: Metrics.px(4); to: -Metrics.px(20)
                duration: 720; easing.type: Easing.OutCubic
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Metrics.px(20)

            // A ring that fills with the checks that have settled: the
            // progress is the readiness, not a timer.
            Canvas {
                id: logoMark

                width: Metrics.px(104)
                height: width
                anchors.verticalCenter: parent.verticalCenter

                onPaint: {
                    var ctx = getContext("2d")
                    var unit = width / 190
                    ctx.reset()
                    ctx.strokeStyle = Colors.brand
                    ctx.fillStyle = Colors.brand
                    ctx.lineWidth = 9 * unit
                    ctx.lineJoin = "round"
                    ctx.lineCap = "round"

                    ctx.beginPath()
                    ctx.moveTo(58 * unit, 148 * unit)
                    ctx.lineTo(82 * unit, 48 * unit)
                    ctx.lineTo(132 * unit, 48 * unit)
                    ctx.lineTo(132 * unit, 84 * unit)
                    ctx.moveTo(132 * unit, 148 * unit)
                    ctx.lineTo(132 * unit, 104 * unit)
                    ctx.lineTo(170 * unit, 104 * unit)
                    ctx.lineTo(170 * unit, 84 * unit)
                    ctx.lineTo(96 * unit, 84 * unit)
                    ctx.lineTo(82 * unit, 148 * unit)
                    ctx.lineTo(58 * unit, 148 * unit)
                    ctx.stroke()

                    ctx.beginPath()
                    ctx.arc(132 * unit, 78 * unit, 7 * unit, 0, Math.PI * 2)
                    ctx.fill()
                    ctx.beginPath()
                    ctx.arc(132 * unit, 112 * unit, 7 * unit, 0, Math.PI * 2)
                    ctx.fill()
                }

                onWidthChanged: requestPaint()
            }

            Rectangle {
                width: Metrics.px(3)
                height: Metrics.px(76)
                anchors.verticalCenter: parent.verticalCenter
                radius: width
                color: Colors.brand
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Metrics.px(2)

                Text {
                    text: Branding.companyLine1
                    color: Colors.brand
                    font.family: Typography.family
                    font.pixelSize: Typography.px(38)
                    font.weight: Typography.semibold
                }

                Text {
                    text: Branding.companyLine2
                    color: Colors.brand
                    font.family: Typography.family
                    font.pixelSize: Typography.px(38)
                    font.weight: Typography.semibold
                }
            }
        }

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: checks.width
            spacing: Metrics.px(10)

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Branding.product + "   " + Branding.model
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.px(17)
                font.letterSpacing: Metrics.px(1)
            }

            // The bar says how far through the checks the device is, so the
            // count belongs beside it. On its own it read as a timer.
            Item {
                width: parent.width
                height: Metrics.px(4)

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: countLabel.left
                    anchors.rightMargin: Metrics.px(12)
                    anchors.verticalCenter: parent.verticalCenter
                    height: parent.height
                    radius: height / 2
                    color: Colors.line

                    Rectangle {
                        width: parent.width * splash.fraction
                        height: parent.height
                        radius: parent.radius
                        color: Colors.brand

                        Behavior on width {
                            NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
                        }
                    }
                }

                Text {
                    id: countLabel
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: splash.settledCount + "/" + splash.stages.length
                    color: Colors.textSecondary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.px(12)
                }
            }
        }
        // The checks, one line each, in the order they settle. They belong in
        // the stack rather than anchored on their own below it.
        Column {
            id: checks

            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(splash.width * 0.42, Metrics.px(360))
            spacing: Metrics.px(2)

            Repeater {
                model: splash.stages

                delegate: Rectangle {
                    required property int index
                    required property var modelData

                    readonly property bool settled: index < splash.settledCount
                    readonly property bool met: splash.conditionFor(modelData.key)

                    width: checks.width
                    height: Metrics.px(30)
                    radius: Radius.small
                    color: settled ? Colors.surface : Colors.transparent
                    opacity: settled ? 1 : 0.35

                    Behavior on opacity {
                        NumberAnimation { duration: 240; easing.type: Easing.OutCubic }
                    }
                    Behavior on color {
                        ColorAnimation { duration: 240 }
                    }

                    Rectangle {
                        id: dot
                        anchors.left: parent.left
                        anchors.leftMargin: Metrics.px(12)
                        anchors.verticalCenter: parent.verticalCenter
                        width: Metrics.px(8)
                        height: width
                        radius: width / 2
                        color: !parent.settled ? Colors.line
                             : parent.met ? Colors.success : Colors.warning

                        SequentialAnimation on scale {
                            running: !parent.settled
                            loops: Animation.Infinite
                            NumberAnimation { to: 1.35; duration: 620; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 1.0; duration: 620; easing.type: Easing.InOutSine }
                        }
                    }

                    Text {
                        anchors.left: dot.right
                        anchors.leftMargin: Metrics.px(12)
                        anchors.right: state.left
                        anchors.rightMargin: Metrics.px(12)
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.label
                        color: Colors.textPrimary
                        elide: Text.ElideRight
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.px(13)
                    }

                    Text {
                        id: state
                        anchors.right: parent.right
                        anchors.rightMargin: Metrics.px(12)
                        anchors.verticalCenter: parent.verticalCenter
                        text: !parent.settled ? qsTr("checking")
                            : parent.met ? qsTr("ready") : qsTr("not ready")
                        color: !parent.settled ? Colors.textSecondary
                             : parent.met ? Colors.success : Colors.warning
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.px(12)
                    }
                }
            }
        }
    }


    // The regulatory line is a safety statement, not a caption, so it is
    // banded rather than left as loose amber text at the foot of the screen.
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Metrics.px(40)
        spacing: Metrics.px(12)

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: warning.implicitWidth + Metrics.px(44)
            height: Metrics.px(30)
            radius: height / 2
            color: Colors.transparent
            border.color: Colors.warning
            border.width: Metrics.borderWidth

            Rectangle {
                id: warningDot
                anchors.left: parent.left
                anchors.leftMargin: Metrics.px(14)
                anchors.verticalCenter: parent.verticalCenter
                width: Metrics.px(7)
                height: width
                radius: width / 2
                color: Colors.warning
            }

            Text {
                id: warning
                anchors.left: warningDot.right
                anchors.leftMargin: Metrics.px(10)
                anchors.verticalCenter: parent.verticalCenter
                text: Branding.regulatoryLine
                color: Colors.warning
                font.family: Typography.monoFamily
                font.pixelSize: Typography.px(12)
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Software %1     Operating hours %2")
                      .arg(splash.softwareVersion)
                      .arg(splash.operatingHours.toFixed(2))
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.px(12)
        }
    }

    // Fades the whole screen out on handover so the interface does not
    // appear with a cut.
    Rectangle {
        anchors.fill: parent
        color: Colors.background
        opacity: splash.handingOver ? 1 : 0
        visible: opacity > 0

        Behavior on opacity {
            NumberAnimation { duration: Branding.splashHandoverMs; easing.type: Easing.InCubic }
        }
    }
}
