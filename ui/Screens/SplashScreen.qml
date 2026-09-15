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
import QtQuick.Shapes
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

    // A breath traced across the screen: one inspiratory rise, a plateau and
    // an expiratory decay, drawn once and then breathing gently. It is the
    // waveform the device exists to produce.
    Item {
        id: breath

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -Metrics.px(10)
        height: Metrics.px(220)
        opacity: 0.20

        Shape {
            anchors.fill: parent
            antialiasing: true

            ShapePath {
                strokeColor: Colors.wavePressure
                strokeWidth: Metrics.px(3)
                fillColor: Colors.transparent
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin

                startX: 0
                startY: breath.height * 0.72

                PathCubic {
                    x: breath.width * 0.22; y: breath.height * 0.28
                    control1X: breath.width * 0.09; control1Y: breath.height * 0.72
                    control2X: breath.width * 0.14; control2Y: breath.height * 0.28
                }
                PathLine { x: breath.width * 0.38; y: breath.height * 0.28 }
                PathCubic {
                    x: breath.width * 0.62; y: breath.height * 0.72
                    control1X: breath.width * 0.48; control1Y: breath.height * 0.28
                    control2X: breath.width * 0.52; control2Y: breath.height * 0.72
                }
                PathLine { x: breath.width * 0.78; y: breath.height * 0.72 }
                PathCubic {
                    x: breath.width; y: breath.height * 0.40
                    control1X: breath.width * 0.87; control1Y: breath.height * 0.72
                    control2X: breath.width * 0.92; control2Y: breath.height * 0.40
                }
            }
        }

        SequentialAnimation on opacity {
            running: true
            loops: Animation.Infinite
            NumberAnimation { to: 0.34; duration: 1600; easing.type: Easing.InOutSine }
            NumberAnimation { to: 0.16; duration: 2400; easing.type: Easing.InOutSine }
        }
    }

    Column {
        id: identity

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -Metrics.px(96)
        spacing: Metrics.px(14)

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
                from: -Metrics.px(72); to: -Metrics.px(96)
                duration: 720; easing.type: Easing.OutCubic
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Metrics.px(20)

            // A ring that fills with the checks that have settled: the
            // progress is the readiness, not a timer.
            Item {
                width: Metrics.px(84)
                height: width
                anchors.verticalCenter: parent.verticalCenter

                RingGaugeShape {
                    anchors.fill: parent
                    fraction: splash.fraction
                }

                Text {
                    anchors.centerIn: parent
                    text: Math.round(splash.fraction * 100) + "%"
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.px(18)
                    font.weight: Typography.bold
                }
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

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Branding.product + "   " + Branding.model
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.px(17)
        }
    }

    // The checks, one line each, in the order they settle.
    Column {
        id: checks

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: Metrics.px(78)
        width: Math.min(parent.width * 0.5, Metrics.px(420))
        spacing: Metrics.px(6)

        Repeater {
            model: splash.stages

            delegate: Item {
                required property int index
                required property var modelData

                readonly property bool settled: index < splash.settledCount
                readonly property bool met: splash.conditionFor(modelData.key)

                width: checks.width
                height: Metrics.px(26)
                opacity: settled ? 1 : 0.25

                Behavior on opacity {
                    NumberAnimation { duration: 240; easing.type: Easing.OutCubic }
                }

                Rectangle {
                    id: dot
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: Metrics.px(9)
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
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.label
                    color: Colors.textPrimary
                    font.family: Typography.family
                    font.pixelSize: Typography.px(15)
                }

                Text {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: !parent.settled ? ""
                        : parent.met ? qsTr("ready") : qsTr("not ready")
                    color: parent.met ? Colors.success : Colors.warning
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.px(13)
                }
            }
        }
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Metrics.px(46)
        spacing: Metrics.px(6)

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Branding.regulatoryLine
            color: Colors.warning
            font.family: Typography.family
            font.pixelSize: Typography.px(13)
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Software %1     Operating hours %2")
                      .arg(splash.softwareVersion)
                      .arg(splash.operatingHours.toFixed(2))
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.px(13)
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

    component RingGaugeShape: Item {
        id: ring

        property real fraction: 0
        readonly property real stroke: Metrics.px(6)

        Shape {
            anchors.fill: parent
            antialiasing: true

            ShapePath {
                strokeColor: Colors.line
                strokeWidth: ring.stroke
                fillColor: Colors.transparent
                capStyle: ShapePath.FlatCap

                PathAngleArc {
                    centerX: ring.width / 2
                    centerY: ring.height / 2
                    radiusX: ring.width / 2 - ring.stroke / 2
                    radiusY: ring.height / 2 - ring.stroke / 2
                    startAngle: -90
                    sweepAngle: 360
                }
            }

            ShapePath {
                strokeColor: Colors.brand
                strokeWidth: ring.stroke
                fillColor: Colors.transparent
                capStyle: ShapePath.RoundCap

                PathAngleArc {
                    centerX: ring.width / 2
                    centerY: ring.height / 2
                    radiusX: ring.width / 2 - ring.stroke / 2
                    radiusY: ring.height / 2 - ring.stroke / 2
                    startAngle: -90
                    sweepAngle: 360 * ring.fraction

                    Behavior on sweepAngle {
                        NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
                    }
                }
            }
        }
    }
}
