pragma Singleton
// -----------------------------------------------------------------------
// File: Typography.qml
// Description: Font families and the type scale
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "."

QtObject {
    id: typography

    readonly property var sansCandidates: [
        "Inter",
        "Roboto",
        "Noto Sans",
        "Helvetica Neue",
        "Segoe UI",
        "DejaVu Sans",
        "Arial"
    ]

    readonly property var monoCandidates: [
        "JetBrains Mono",
        "Roboto Mono",
        "Noto Sans Mono",
        "SF Mono",
        "Menlo",
        "DejaVu Sans Mono",
        "Consolas",
        "Courier New"
    ]

    function firstAvailable(candidates) {
        var available = Qt.fontFamilies()
        for (var i = 0; i < candidates.length; ++i) {
            if (available.indexOf(candidates[i]) >= 0)
                return candidates[i]
        }
        return candidates[candidates.length - 1]
    }

    readonly property string family: typography.firstAvailable(typography.sansCandidates)
    readonly property string monoFamily: typography.firstAvailable(typography.monoCandidates)
    readonly property string numberFamily: typography.monoFamily

    readonly property var numericFeatures: ({ "tnum": 1 })

    readonly property int regular: Font.Normal
    readonly property int medium: Font.Medium
    readonly property int semibold: Font.DemiBold
    readonly property int bold: Font.Bold

    function px(referencePixels) {
        return Math.round(referencePixels * Metrics.scale)
    }

    readonly property int micro: typography.px(11)
    readonly property int caption: typography.px(13)
    readonly property int small: typography.px(14)
    readonly property int label: typography.px(16)
    readonly property int body: typography.px(18)
    readonly property int bodyLarge: typography.px(20)
    readonly property int subtitle: typography.px(24)
    readonly property int subtitleLarge: typography.px(26)
    readonly property int title: typography.px(32)
    readonly property int titleLarge: typography.px(36)
    readonly property int headline: typography.px(40)
    readonly property int display: typography.px(48)
    readonly property int displayLarge: typography.px(56)
    readonly property int value: typography.px(32)

    readonly property int tileLabel: typography.px(15)
    readonly property int tileValue: typography.px(32)
    readonly property int tileLimit: typography.px(17)
    readonly property int tileUnit: typography.px(18)
    readonly property int dialLabel: typography.px(18)
    readonly property int dialValue: typography.px(28)
    readonly property int dialUnit: typography.px(13)
    readonly property int readoutLabel: typography.px(16)
    readonly property int readoutValue: typography.px(32)
    readonly property int readoutUnit: typography.px(15)

    // The lung panel sets its readouts either side of the picture rather
    // than under it, and the reference draws them smaller there so the
    // picture keeps the middle of the panel.
    readonly property int readoutLabelCompact: typography.px(11)
    readonly property int readoutValueCompact: typography.px(21)
    readonly property int readoutUnitCompact: typography.px(9)
    readonly property int channelLabel: typography.px(17)
    readonly property int axisTick: typography.px(14)
    readonly property int tabLabel: typography.px(16)
    readonly property int bannerLabel: typography.px(17)

    readonly property real lineTight: 1.15
    readonly property real lineNormal: 1.35
    readonly property real lineRelaxed: 1.55
    readonly property real trackCaps: 0
    readonly property real trackNormal: 0
    readonly property real trackDisplay: 0
}
