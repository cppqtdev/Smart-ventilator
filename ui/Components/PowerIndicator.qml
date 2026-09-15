// -----------------------------------------------------------------------
// File: PowerIndicator.qml
// Description: Mains/battery source and state-of-charge indicator
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Source and charge are two separate facts and are drawn as two separate
// glyphs. "On mains, pack at 40 %" and "on battery, pack at 40 %" are very
// different situations and must not share a representation.
//
// ISO 80601-2-12 makes internal power source depletion a MEDIUM priority
// alarm that escalates to HIGH within 5 minutes of depletion, so the
// runtime estimate is shown, not just the percentage.
//
import QtQuick
import QtQuick.Layouts
import "../Theme"
import "../Controls"

Rectangle {
    id: indicator

    property int percentage: 0
    property bool acConnected: true
    property bool charging: false
    property int runtimeMinutes: -1

    readonly property bool low: indicator.percentage < 20 && !indicator.acConnected
    readonly property bool critical: indicator.percentage < 10 && !indicator.acConnected

    readonly property color chargeColor:
          indicator.critical ? Colors.alarmHigh
        : indicator.low ? Colors.alarmMedium
        : indicator.percentage > 50 ? Colors.success
        : Colors.warning

    implicitWidth: row.implicitWidth + Spacing.sm * 2
    implicitHeight: Math.round(48 * Metrics.scale)
    radius: Radius.sm
    color: indicator.critical ? Colors.alarmHighSurface
         : indicator.low ? Colors.alarmMediumSurface
         : Colors.transparent
    border.width: indicator.low ? Metrics.borderWidth : 0
    border.color: indicator.chargeColor

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: Spacing.xs

        AppIcon {
            source: indicator.acConnected ? Icons.powerAc : Icons.powerDc
            size: Math.round(20 * Metrics.scale)
            color: indicator.acConnected ? Colors.success : Colors.textSecondary
            Layout.alignment: Qt.AlignVCenter
        }

        AppIcon {
            source: Icons.battery(indicator.percentage, indicator.charging)
            size: Math.round(24 * Metrics.scale)
            color: indicator.chargeColor
            Layout.alignment: Qt.AlignVCenter

            // A depleting pack flashes at the medium-priority rate, which
            // is the priority ISO 80601-2-12 assigns the condition.
            SequentialAnimation on opacity {
                running: indicator.low
                loops: Animation.Infinite
                NumberAnimation { to: 0.3; duration: Metrics.alarmFlashMediumMs }
                NumberAnimation { to: 1.0; duration: Metrics.alarmFlashMediumMs }
            }
            onVisibleChanged: if (!indicator.low) opacity = 1.0
        }

        ColumnLayout {
            spacing: 0

            Text {
                text: indicator.percentage + "%"
                color: indicator.chargeColor
                font.family: Typography.numberFamily
                font.features: Typography.numericFeatures
                font.pixelSize: Typography.small
                font.weight: Typography.semibold
            }

            Text {
                visible: indicator.runtimeMinutes >= 0 && !indicator.acConnected
                text: Math.floor(indicator.runtimeMinutes / 60) + "h "
                      + String(indicator.runtimeMinutes % 60).padStart(2, "0") + "m"
                color: Colors.textMuted
                font.family: Typography.numberFamily
                font.features: Typography.numericFeatures
                font.pixelSize: Typography.micro
            }
        }
    }
}
