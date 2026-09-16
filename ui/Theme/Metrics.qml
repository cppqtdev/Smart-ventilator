pragma Singleton
// -----------------------------------------------------------------------
// File: Metrics.qml
// Description: Responsive scale, physical touch geometry and layout sizes
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick

QtObject {
    id: metrics

    property real windowWidth: 1920
    property real windowHeight: 1080

    // Screen is an attached property and cannot be read from a QtObject
    // singleton, so main.qml pushes the real density in at startup.
    property real pixelsPerMm: 5.9

    // reference/home-monitoring.png measures 1024 x 768 in design pixels.
    readonly property real referenceWidth: 1024
    readonly property real referenceHeight: 768

    readonly property real rawScale: metrics.windowHeight / metrics.referenceHeight
    readonly property real scale: Math.max(0.85, Math.min(1.70, metrics.rawScale))

    readonly property bool compact: metrics.windowWidth < 1400
    readonly property bool medium: metrics.windowWidth >= 1400 && metrics.windowWidth < 1700
    readonly property bool expanded: metrics.windowWidth >= 1700

    function px(referencePixels) {
        return Math.round(referencePixels * metrics.scale)
    }

    function mm(millimetres) {
        return Math.round(millimetres * metrics.pixelsPerMm)
    }

    function toMm(pixels) {
        return pixels / metrics.pixelsPerMm
    }

    // ANSI/AAMI HE75 floors: 9-10 mm for a gloved finger, 12-15 mm for a
    // safety-critical control. These are floors, never ceilings.
    readonly property int touchMinimum: Math.max(44, metrics.mm(9))
    readonly property int touchTarget: Math.max(48, metrics.mm(10))
    readonly property int touchPrimary: Math.max(64, metrics.mm(13))
    readonly property int touchSpacing: metrics.px(6)

    readonly property int screenPadding: metrics.px(16)
    readonly property int gutter: metrics.px(16)

    readonly property int headerHeight: metrics.px(68)
    readonly property int navHeight: metrics.px(45)
    readonly property int navGap: metrics.px(9)

    readonly property int sidebarWidth: metrics.px(222)
    readonly property int railWidth: metrics.px(210)

    readonly property int alarmBarHeight: metrics.px(34)
    readonly property int rowHeight: metrics.px(46)
    readonly property int rowHeightCompact: metrics.px(34)
    readonly property int tileHeightMin: metrics.px(84)
    readonly property int tileGap: metrics.px(19)
    readonly property int actionButtonWidth: metrics.px(108)
    readonly property int actionButtonHeight: metrics.px(44)

    // Three button heights, and nothing else. Nine were in use - 28, 30, 32,
    // 34, 36, 44, 45, 56 and 64 - which is why an odd-sized button kept
    // turning up on a screen: there was no rule for it to be the odd one out
    // from. Small is a chip in a dense row, standard is an action, primary is
    // one the operator must not miss under pressure.
    readonly property int buttonHeightSmall: metrics.px(34)
    readonly property int buttonHeightStandard: metrics.actionButtonHeight
    readonly property int buttonHeightPrimary: metrics.touchPrimary
    readonly property int pageTabWidth: metrics.px(183)
    readonly property int waveformHeightMin: metrics.px(100)

    readonly property int dialSize: metrics.px(103)
    readonly property int dialStroke: metrics.px(7)
    readonly property int dialStepWidth: metrics.px(38)
    readonly property int dialStepHeight: metrics.px(66)

    readonly property real hairline: 1
    readonly property real borderWidth: 1
    readonly property real emphasisWidth: 2
    readonly property real waveformStroke: Math.max(2, metrics.px(2))

    readonly property int durationInstant: 90
    readonly property int durationFast: 140
    readonly property int durationHold: 420
    readonly property int durationRepeat: 110
    readonly property int durationNormal: 200
    readonly property int durationSlow: 320

    // IEC 60601-1-8 Table 2: high 1.4-2.8 Hz, medium 0.4-0.8 Hz.
    readonly property int alarmFlashHighMs: 250
    readonly property int alarmFlashMediumMs: 833

    // ISO 80601-2-12 caps AUDIO PAUSED at 120 s for a ventilator; this
    // overrides the more permissive general allowance in 60601-1-8.
    readonly property int audioPauseMaxSeconds: 120
}
