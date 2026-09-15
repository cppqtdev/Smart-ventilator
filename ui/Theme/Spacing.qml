pragma Singleton
// -----------------------------------------------------------------------
// File: Spacing.qml
// Description: Layout spacing scale
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "."

QtObject {
    id: spacing

    readonly property int none: 0
    readonly property int xxs: Metrics.px(2)
    readonly property int xs: Metrics.px(4)
    readonly property int sm: Metrics.px(8)
    readonly property int md: Metrics.px(12)
    readonly property int lg: Metrics.px(16)
    readonly property int xl: Metrics.px(20)
    readonly property int xxl: Metrics.px(24)
    readonly property int xxxl: Metrics.px(32)
    readonly property int huge: Metrics.px(40)

    readonly property int screenMargin: Metrics.px(16)
    readonly property int panelGap: Metrics.px(16)
    readonly property int cardPadding: Metrics.px(12)
    readonly property int contentGap: Metrics.px(8)
    readonly property int inlineGap: Metrics.px(6)
    readonly property int sectionGap: Metrics.px(24)
    readonly property int touch: Metrics.touchTarget

    readonly property int screenMargin_10: Metrics.px(10)
    readonly property int screenMargin_8: Metrics.px(8)
    readonly property int screenMargin_6: Metrics.px(6)
    readonly property int screenMargin_4: Metrics.px(4)
}
