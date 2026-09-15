// -----------------------------------------------------------------------
// File: AppButton.qml
// Description: Flat button base for every pressable surface
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import "../Theme"

T.AbstractButton {
    id: control

    enum Variant {
        Primary,
        Secondary,
        Success,
        Neutral,
        Danger,
        Ghost
    }

    property int buttonVariant: AppButton.Primary
    property color textColor: control.resolvedTextColor
    property int radius: Radius.small
    property int fontSize: Typography.tabLabel
    property int fontWeight: Typography.bold
    property string fontFamily: Typography.monoFamily
    property bool uppercaseLabel: false
    property int labelPadding: Spacing.lg

    // Selection the caller owns.
    //
    // checkable is not the way to show it: T.AbstractButton flips checked
    // itself on release, and that write lands on top of the caller's
    // binding, so a button goes on showing a selection the model refused -
    // two layouts lit at once, because the setting clamped the change away.
    //
    // A control whose selection lives elsewhere binds this instead and
    // leaves checkable alone. Nothing writes it, so the binding is the only
    // thing that moves the button. checkable stays for a control that owns
    // its own toggle and reports it through onToggled.
    property bool selected: false

    readonly property bool active: control.checkable
                                   ? control.checked
                                   : (control.selected || control.down)

    readonly property color baseColor: {
        switch (control.buttonVariant) {
        case AppButton.Primary:   return control.active ? Colors.accent : Colors.accentSubtle
        case AppButton.Secondary: return control.active ? Colors.accent : Colors.surfaceRaised
        case AppButton.Success:   return control.active ? Colors.success : Colors.successSubtle
        case AppButton.Neutral:   return control.active ? Colors.accent : Colors.neutral
        case AppButton.Danger:    return Colors.danger
        default:                          return Colors.transparent
        }
    }

    readonly property color resolvedTextColor: {
        if (!control.enabled)
            return Colors.textDisabled
        if (control.buttonVariant === AppButton.Neutral)
            return control.active ? Colors.textPrimary : Colors.textInverse
        if (control.buttonVariant === AppButton.Ghost)
            return control.active ? Colors.accent : Colors.textPrimary
        return Colors.textPrimary
    }

    readonly property color surfaceColor: {
        if (!control.enabled)
            return control.buttonVariant === AppButton.Ghost ? Colors.transparent
                                                               : Colors.controlDisabled
        if (control.down)
            return Qt.darker(control.baseColor, 1.18)
        return control.baseColor
    }

    implicitWidth: Math.max(Metrics.touchMinimum,
                            control.implicitContentWidth + control.labelPadding * 2)
    implicitHeight: Math.max(Metrics.touchMinimum, Metrics.rowHeight)

    // A caller that asks for a narrower preferred width than the label needs
    // gets the label width instead. Layouts honour the minimum over the
    // preferred size, so this is what stops a button from clipping its text.
    Layout.minimumWidth: control.implicitWidth

    background: Rectangle {
        radius: control.radius
        color: control.surfaceColor
        border.width: control.buttonVariant === AppButton.Ghost ? Metrics.borderWidth : 0
        border.color: control.active ? Colors.accent : Colors.line

        Behavior on color {
            ColorAnimation { duration: Metrics.durationFast }
        }
    }

    contentItem: Text {
        id: label
        text: control.uppercaseLabel ? control.text.toUpperCase() : control.text
        color: control.textColor
        font.family: control.fontFamily
        font.pixelSize: control.fontSize
        font.weight: control.fontWeight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
