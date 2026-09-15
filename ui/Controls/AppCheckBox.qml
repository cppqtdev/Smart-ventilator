// -----------------------------------------------------------------------
// File: AppCheckBox.qml
// Description: Square checkable box with a tick indicator
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Templates as T
import "../Theme"

T.CheckBox {
    id: control

    property int boxSize: Metrics.px(28)

    implicitWidth: control.boxSize + control.spacing + control.implicitContentWidth
    implicitHeight: Math.max(Metrics.touchMinimum, control.boxSize)

    hoverEnabled: true
    spacing: Spacing.sm

    indicator: Rectangle {
        implicitWidth: control.boxSize
        implicitHeight: control.boxSize
        x: 0
        y: (control.height - height) / 2
        radius: Radius.xs
        color: control.checked ? Colors.success : Colors.surfaceRaised
        border.width: Metrics.borderWidth
        border.color: control.checked ? Colors.success : Colors.line

        AppIcon {
            anchors.centerIn: parent
            name: "check"
            size: Math.round(control.boxSize * 0.66)
            color: Colors.textPrimary
            visible: control.checked
        }

        Behavior on color {
            ColorAnimation { duration: Metrics.durationFast }
        }
    }

    contentItem: Text {
        id: label
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        color: control.enabled ? Colors.textPrimary : Colors.textDisabled
        font.family: Typography.monoFamily
        font.pixelSize: Typography.readoutLabel
        verticalAlignment: Text.AlignVCenter
    }
}
