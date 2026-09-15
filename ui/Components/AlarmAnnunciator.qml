// -----------------------------------------------------------------------
// File: AlarmAnnunciator.qml
// Description: Highest-priority alarm message bar (the 1 m indicator)
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The second indicator required by IEC 60601-1-8 clause 6.3.2.2: legible
// at 1 m, and it must identify the specific alarm condition and its
// priority - not merely that something is wrong.
//
// When nothing is alarming this does not go blank. A silent area that only
// fills during an emergency trains the eye to ignore it; instead it shows
// the ventilation state, so the region is always worth reading.
//
import QtQuick
import QtQuick.Layouts
import "../Theme"
import "../Controls"

Rectangle {
    id: annunciator

    /** "none" | "low" | "medium" | "high" */
    property string priority: "none"
    property string headline: ""
    property string detail: ""

    /** Count of active conditions beyond the one displayed. */
    property int additionalCount: 0

    /** True when the condition has cleared but the operator has not reset. */
    property bool latched: false

    property bool audioPaused: false
    property int audioPauseRemaining: 0

    /** Fallback content when nothing is alarming. */
    property string idleStatus: "Standby"
    property string idleDetail: ""

    signal activated()

    readonly property bool active: annunciator.priority !== "none"

    readonly property color priorityColor:
          annunciator.priority === "high"   ? Colors.alarmHigh
        : annunciator.priority === "medium" ? Colors.alarmMedium
        : annunciator.priority === "low"    ? Colors.alarmLow
        : Colors.line

    readonly property color priorityOn:
          annunciator.priority === "high"   ? Colors.alarmHighOn
        : annunciator.priority === "medium" ? Colors.alarmMediumOn
        : annunciator.priority === "low"    ? Colors.alarmLowOn
        : Colors.textPrimary

    implicitHeight: Metrics.alarmBarHeight
    radius: Radius.md
    color: annunciator.active ? annunciator.priorityColor : Colors.surfaceRaised
    border.width: Metrics.borderWidth
    border.color: annunciator.active ? annunciator.priorityColor : Colors.line

        ? (annunciator.priority + " priority alarm: " + annunciator.headline)
        : annunciator.idleStatus

    Behavior on color { ColorAnimation { duration: Metrics.durationNormal } }

    // The bar's own fill flashes, but the text layer above it does not -
    // see the opacity binding on contentRow. A message that fades in and
    // out is measurably slower to read.
    SequentialAnimation on opacity {
        running: annunciator.priority === "high" || annunciator.priority === "medium"
        loops: Animation.Infinite
        NumberAnimation {
            to: 0.55
            duration: annunciator.priority === "high" ? Metrics.alarmFlashHighMs
                                                      : Metrics.alarmFlashMediumMs
        }
        NumberAnimation {
            to: 1.0
            duration: annunciator.priority === "high" ? Metrics.alarmFlashHighMs
                                                      : Metrics.alarmFlashMediumMs
        }
    }

    onPriorityChanged: if (!active || priority === "low") opacity = 1.0

    RowLayout {
        id: contentRow
        anchors.fill: parent
        anchors.leftMargin: Spacing.md
        anchors.rightMargin: Spacing.md
        spacing: Spacing.sm

        // Counter-animate so the message stays at full contrast while the
        // background flashes underneath it.
        opacity: annunciator.opacity > 0 ? 1.0 / annunciator.opacity : 1.0

        AppIcon {
            source: annunciator.active
                ? Icons.alarmPriority(annunciator.priority)
                : Icons.shieldCheck
            size: Math.round(28 * Metrics.scale)
            color: annunciator.active ? annunciator.priorityOn : Colors.success
            Layout.alignment: Qt.AlignVCenter
        }

        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true

            Text {
                text: annunciator.active ? annunciator.headline : annunciator.idleStatus
                color: annunciator.active ? annunciator.priorityOn : Colors.textPrimary
                font.family: Typography.family
                font.pixelSize: Typography.bodyLarge
                font.weight: Typography.bold
                font.letterSpacing: Typography.trackCaps
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                visible: text.length > 0
                text: annunciator.active ? annunciator.detail : annunciator.idleDetail
                color: annunciator.active
                    ? Qt.rgba(annunciator.priorityOn.r, annunciator.priorityOn.g,
                              annunciator.priorityOn.b, 0.85)
                    : Colors.textSecondary
                font.family: Typography.family
                font.pixelSize: Typography.caption
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        // "Cleared - awaiting reset". A latched high-priority alarm must
        // stay visible after the condition resolves, or a transient
        // disconnection leaves no trace.
        StatusChip {
            visible: annunciator.latched
            iconName: "alarm-resolved"
            text: qsTr("CLEARED")
            accentColor: annunciator.priorityOn
            surfaceColor: Qt.rgba(0, 0, 0, 0.28)
            Layout.alignment: Qt.AlignVCenter
        }

        StatusChip {
            visible: annunciator.additionalCount > 0
            text: "+" + annunciator.additionalCount
            accentColor: annunciator.priorityOn
            surfaceColor: Qt.rgba(0, 0, 0, 0.28)
            Layout.alignment: Qt.AlignVCenter
        }

        // ISO 80601-2-12 caps AUDIO PAUSED at 120 s. Showing the remaining
        // time, rather than just a crossed bell, is what makes the cap
        // meaningful to the operator.
        StatusChip {
            visible: annunciator.audioPaused
            iconName: "alarm-audio-paused"
            text: annunciator.audioPauseRemaining > 0
                ? Math.floor(annunciator.audioPauseRemaining / 60) + ":"
                  + String(annunciator.audioPauseRemaining % 60).padStart(2, "0")
                : ""
            accentColor: annunciator.active ? annunciator.priorityOn : Colors.warning
            surfaceColor: Qt.rgba(0, 0, 0, 0.28)
            Layout.alignment: Qt.AlignVCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: annunciator.activated()
    }
}
