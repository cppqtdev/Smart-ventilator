// -----------------------------------------------------------------------
// File: AlarmStrip.qml
// Description: Full-width highest-priority alarm bar (the 4 m indicator)
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// IEC 60601-1-8 clause 6.3.2.2 requires TWO distinct visual alarm
// indicators. This is the first of them: perceivable at 4 m, carrying only
// the highest active priority and nothing else - no text, no detail, just
// a large field of the priority colour flashing at the Table 2 rate.
//
// AlarmAnnunciator is the second one: legible at 1 m, identifying the
// specific condition. Do not merge them.
//
import QtQuick
import "../Theme"

Rectangle {
    id: strip

    /** "none" | "low" | "medium" | "high" */
    property string priority: "none"

    /** True while audio is paused - the strip stays lit, the pattern differs. */
    property bool audioPaused: false

    readonly property bool active: strip.priority !== "none"

    implicitHeight: Math.round(8 * Metrics.scale)
    height: strip.active ? implicitHeight : 0
    visible: height > 0

    color: strip.priority === "high"   ? Colors.alarmHigh
         : strip.priority === "medium" ? Colors.alarmMedium
         : strip.priority === "low"    ? Colors.alarmLow
         : Colors.transparent

    Behavior on height {
        NumberAnimation { duration: Metrics.durationFast }
    }

    // High flashes at 2.0 Hz, medium at 0.6 Hz, low is steady - Table 2.
    // Duty cycle sits at 50 %, inside the permitted 20-60 % band.
    SequentialAnimation on opacity {
        running: strip.priority === "high" || strip.priority === "medium"
        loops: Animation.Infinite
        NumberAnimation {
            to: 0.15
            duration: strip.priority === "high" ? Metrics.alarmFlashHighMs
                                                : Metrics.alarmFlashMediumMs
        }
        NumberAnimation {
            to: 1.0
            duration: strip.priority === "high" ? Metrics.alarmFlashHighMs
                                                : Metrics.alarmFlashMediumMs
        }
    }

    onPriorityChanged: {
        if (priority === "none" || priority === "low")
            opacity = 1.0
    }

    // Hatched overlay while audio is paused, so the paused state is
    // readable at a distance as well as from the header countdown.
    Row {
        anchors.fill: parent
        visible: strip.audioPaused && strip.active
        spacing: Math.round(10 * Metrics.scale)
        clip: true

        Repeater {
            model: Math.ceil(strip.width / Math.round(20 * Metrics.scale)) + 2
            Rectangle {
                width: Math.round(10 * Metrics.scale)
                height: strip.height
                color: Qt.rgba(0, 0, 0, 0.45)
            }
        }
    }
}
