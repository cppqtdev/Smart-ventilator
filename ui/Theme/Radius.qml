pragma Singleton
// -----------------------------------------------------------------------
// File: Radius.qml
// Description: Corner radius scale
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "."

QtObject {
    id: radius

    readonly property int none: 0
    readonly property int xs: Metrics.px(4)
    readonly property int small: Metrics.px(6)
    readonly property int medium: Metrics.px(8)
    readonly property int large: Metrics.px(12)

    readonly property int sm: radius.small
    readonly property int md: radius.medium
    readonly property int lg: radius.large
    readonly property int xl: Metrics.px(16)
    readonly property int pill: 999
}
