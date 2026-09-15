// -----------------------------------------------------------------------
// File: ChipButton.qml
// Description: Light grey option chip with dark text
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "../Theme"

AppButton {
    id: chip

    buttonVariant: AppButton.Neutral
    radius: Radius.xs
    fontSize: Typography.readoutLabel
    fontWeight: Typography.regular
    implicitHeight: Metrics.px(34)

    background: Rectangle {
        radius: chip.radius
        color: !chip.enabled ? Colors.controlDisabled
             : chip.active ? Colors.accent
             : chip.down ? Qt.darker(Colors.neutral, 1.15)
             : Colors.neutral

        Behavior on color {
            ColorAnimation { duration: Metrics.durationFast }
        }
    }

    textColor: !chip.enabled ? Colors.textInverse
             : chip.active ? Colors.textPrimary
             : Colors.textInverse
}
