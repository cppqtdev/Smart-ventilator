// -----------------------------------------------------------------------
// File: BreathingLung.qml
// Description: The lung picture, breathing with the delivered breath
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The lung breathes with the delivered breath rather than on a decorative
// loop: it fills over the inspiratory time and empties over what is left of
// the cycle. A lung that breathes when the ventilator is stopped would be
// telling the operator something that is not true, so it rests.
//
// Three screens drew this picture and only two of them animated it, so the
// cycle lives here once and every screen gets the same one.
//
import QtQuick
import "../Theme"

Item {
    id: lung

    property var presenter
    property bool frozen: false

    readonly property bool ventilating: lung.presenter ? lung.presenter.ventilating : false

    readonly property int breathRate: {
        var rate = lung.presenter && lung.presenter.measuredRate !== undefined
                   ? lung.presenter.measuredRate : 0
        return rate > 0 ? rate : 14
    }

    // 0 at end expiration, 1 at end inspiration.
    property real inflation: 0

    SequentialAnimation {
        id: breathCycle
        running: lung.ventilating && !lung.frozen
        loops: Animation.Infinite

        NumberAnimation {
            target: lung
            property: "inflation"
            to: 1
            duration: breathCycle.inspiratoryMs
            easing.type: Easing.OutQuad
        }

        NumberAnimation {
            target: lung
            property: "inflation"
            to: 0
            duration: breathCycle.expiratoryMs
            easing.type: Easing.InQuad
        }

        readonly property int cycleMs: 60000 / Math.max(4, lung.breathRate)
        readonly property int inspiratoryMs: Math.round(cycleMs * 0.33)
        readonly property int expiratoryMs: Math.max(200, cycleMs - inspiratoryMs)

        onRunningChanged: {
            if (!running)
                lung.inflation = 0
        }
    }

    Image {
        id: picture

        anchors.fill: parent
        source: "qrc:/ui/Assets/lungs.png"
        fillMode: Image.PreserveAspectFit
        smooth: true
        asynchronous: true

        // Inflation is mostly downward and outward, the way a chest moves,
        // so the scale is not uniform.
        transform: Scale {
            origin.x: picture.width / 2
            origin.y: picture.height * 0.18
            xScale: 1.0 + lung.inflation * 0.035
            yScale: 1.0 + lung.inflation * 0.075
        }
    }
}
