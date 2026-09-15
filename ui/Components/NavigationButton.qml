// -----------------------------------------------------------------------
// File: NavigationButton.qml
// Description: Single icon + label destination button for the nav bar
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../Theme"
import "../Controls"

AbstractButton {
    id: navButton

    property string iconName: ""
    property string screen: ""
    property bool selected: false
    property bool showLabel: true
    property int badgeCount: 0
    property string badgePriority: "none"

    readonly property color badgeColor:
          navButton.badgePriority === "high"   ? Colors.alarmHigh
        : navButton.badgePriority === "medium" ? Colors.alarmMedium
        : navButton.badgePriority === "low"    ? Colors.alarmLow
        : Colors.accent

    implicitWidth: Math.round(112 * Metrics.scale)
    implicitHeight: Metrics.touchTarget
    hoverEnabled: true

    Accessible.role: Accessible.PageTab
    Accessible.name: navButton.text
    Accessible.checkable: true
    Accessible.checked: navButton.selected

    background: GlossSurface {
        radius: Radius.sm
        enabled: navButton.enabled
        selected: navButton.selected
        interaction: navButton.pressed ? 2 : (navButton.hovered ? 1 : 0)
        // Thirteen tabs sit in this row, so the resting bloom is kept small -
        // a full glow on every one turns the bar into a light strip and the
        // selected tab stops standing out, which is the whole point.
        bloom: navButton.selected ? Math.round(6 * Metrics.scale)
             : navButton.hovered ? Math.round(3 * Metrics.scale)
             : Math.round(1 * Metrics.scale)

        // A 3 px rule under the active destination. Selection is carried by
        // the rule, the icon tint and the label weight together - three
        // channels, so it survives both colour vision deficiency and the
        // washed-out contrast of a panel read at 1500 lux.
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Math.round(3 * Metrics.scale)
            width: navButton.selected ? parent.width * 0.55 : 0
            height: Math.round(3 * Metrics.scale)
            radius: height / 2
            color: Colors.accent

            Behavior on width {
                NumberAnimation {
                    duration: Metrics.durationNormal
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: Math.round(2 * Metrics.scale)

        Item {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: Math.round(24 * Metrics.scale)
            implicitHeight: Math.round(24 * Metrics.scale)

            AppIcon {
                anchors.centerIn: parent
                name: navButton.iconName
                size: Math.round(24 * Metrics.scale)
                color: !navButton.enabled ? Colors.textDisabled
                     : navButton.selected ? Colors.accent
                     : Colors.textSecondary
            }

            // Count badge, priority-coloured. Anchored to the icon rather
            // than the button so it stays put when the label hides on a
            // narrow panel.
            Rectangle {
                visible: navButton.badgeCount > 0
                anchors.left: parent.right
                anchors.leftMargin: Math.round(-8 * Metrics.scale)
                anchors.bottom: parent.top
                anchors.bottomMargin: Math.round(-8 * Metrics.scale)
                implicitWidth: Math.max(Math.round(18 * Metrics.scale),
                                        badgeText.implicitWidth + Spacing.xxs * 2)
                implicitHeight: Math.round(18 * Metrics.scale)
                radius: height / 2
                color: navButton.badgeColor

                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: navButton.badgeCount > 9 ? "9+" : navButton.badgeCount
                    color: navButton.badgePriority === "medium"
                        ? Colors.alarmMediumOn : "#FFFFFF"
                    font.family: Typography.numberFamily
                    font.pixelSize: Typography.micro
                    font.weight: Typography.bold
                }
            }
        }

        Text {
            visible: navButton.showLabel
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: navButton.width - Spacing.xs
            text: navButton.text
            color: !navButton.enabled ? Colors.textDisabled
                 : navButton.selected ? Colors.textPrimary
                 : Colors.textSecondary
            font.family: Typography.family
            font.pixelSize: Typography.micro
            font.weight: navButton.selected ? Typography.bold : Typography.medium
            font.letterSpacing: Typography.trackCaps
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }
}
