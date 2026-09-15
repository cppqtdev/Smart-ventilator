// -----------------------------------------------------------------------
// File: ParameterControl.qml
// Description: Adjust-preview-confirm control for one ventilation parameter
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The interaction model is the one every ICU ventilator uses, and it exists
// because IEC 62366-1 requires that an unintended single action cannot
// produce a hazardous output:
//
//   adjust    stepping changes a PENDING value, never the delivered one
//   preview   pending and active are shown together, visually distinct
//   confirm   an explicit, separate press applies it
//
// Nothing here is applied on touch-down, on release, or on a timer. The
// previous implementation called requestParameterChange() straight from the
// +/- MouseArea, so a brush against the screen changed delivered therapy.
//
// The control takes its whole specification - label, unit, range, step,
// decimals, advisory band - from a ParameterCatalog::describe() map supplied
// by `ventilationCatalog`. It does not know what a PEEP is, which is why the
// same component serves every parameter and why changing patient category
// silently re-ranges all of them.
//
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../Theme"
import "../Components"

Item {
    id: control

    // -- Specification -----------------------------------------------------
    /** A ParameterCatalog::describe() map. Everything below defaults from it. */
    property var spec: ({})

    property string label: control.spec.label !== undefined ? control.spec.label : ""
    property string shortLabel: control.spec.shortLabel !== undefined
        ? control.spec.shortLabel : control.label
    property string unit: control.spec.unit !== undefined ? control.spec.unit : ""
    property real minimumValue: control.spec.minimum !== undefined ? control.spec.minimum : 0
    property real maximumValue: control.spec.maximum !== undefined ? control.spec.maximum : 100
    property real stepSize: control.spec.step !== undefined ? control.spec.step : 1
    property int decimals: control.spec.decimals !== undefined ? control.spec.decimals : 0
    property bool hazardous: control.spec.hazardous === true
    property string helpText: control.spec.help !== undefined ? control.spec.help : ""

    property real advisoryLow: control.spec.advisoryLow !== undefined ? control.spec.advisoryLow : 0
    property real advisoryHigh: control.spec.advisoryHigh !== undefined ? control.spec.advisoryHigh : 0
    property bool hasAdvisory: control.spec.hasAdvisory === true
    property string advisoryRationale: control.spec.advisoryRationale !== undefined
        ? control.spec.advisoryRationale : ""

    // -- State -------------------------------------------------------------
    /** The value currently being delivered. Owned by the controller. */
    property real activeValue: 0

    /** The value being dialled. Equal to activeValue when nothing is pending. */
    property real pendingValue: control.activeValue

    /** Read-only presentation, for values the active mode derives. */
    property bool derived: false

    /** Disables interaction without implying the parameter is derived. */
    property bool editable: true

    readonly property bool hasPendingChange:
        Math.abs(control.pendingValue - control.activeValue) > 1e-9

    readonly property bool pendingOutsideAdvisory:
        control.hasAdvisory
        && (control.pendingValue < control.advisoryLow
            || control.pendingValue > control.advisoryHigh)

    /**
     * A change large enough to deserve a modal rather than an inline confirm.
     * Twenty per cent is a defensible default: big enough not to fire on
     * ordinary titration, small enough to catch a mis-dial. Document the
     * threshold as a risk control if you change it.
     */
    readonly property bool isLargeChange:
        control.activeValue !== 0
        && Math.abs(control.pendingValue - control.activeValue)
           / Math.abs(control.activeValue) > 0.20

    signal confirmed(real value)
    signal cancelled()
    signal keypadRequested()
    signal helpRequested()

    implicitWidth: Math.round(260 * Metrics.scale)
    implicitHeight: Math.round(190 * Metrics.scale)

    function format(value) {
        return Number(value).toFixed(control.decimals)
    }

    function step(direction) {
        if (!control.editable || control.derived)
            return
        var next = control.pendingValue + direction * control.stepSize
        // Snap to the step grid so repeated presses cannot accumulate a
        // floating-point offset that shows as 7.999999.
        next = Math.round(next / control.stepSize) * control.stepSize
        control.pendingValue = Math.max(control.minimumValue,
                                        Math.min(control.maximumValue, next))
    }

    function applyChange() {
        if (!control.hasPendingChange)
            return
        control.confirmed(control.pendingValue)
    }

    function discardChange() {
        control.pendingValue = control.activeValue
        control.cancelled()
    }

    // A change to the delivered value - the operator's own confirm, a clamp
    // applied by the controller, a mode switch, the backend - always wins over
    // an uncommitted local edit. Resyncing unconditionally is what makes the
    // displayed pending value trustworthy: if the controller accepted 42 when
    // 45 was asked for, the control must show 42, not keep offering 45.
    onActiveValueChanged: control.pendingValue = control.activeValue

    Panel {
        id: surface
        anchors.fill: parent
        elevation: 1
        border.color: control.hasPendingChange ? Colors.pendingValue
                    : control.derived ? Colors.line
                    : Colors.line
        border.width: control.hasPendingChange
            ? Math.max(2, Metrics.borderWidth * 2) : Metrics.borderWidth
        opacity: control.editable || control.derived ? 1.0 : 0.5

        Behavior on border.color { ColorAnimation { duration: Metrics.durationFast } }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.sm
        spacing: Spacing.xxs

        // -- Header: label, unit, derived marker, help ---------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.xxs

            Text {
                text: control.label
                color: Colors.textLabel
                font.family: Typography.family
                font.pixelSize: Typography.caption
                font.weight: Typography.semibold
                font.letterSpacing: Typography.trackCaps
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            StatusChip {
                visible: control.derived
                text: qsTr("DERIVED")
                accentColor: Colors.textMuted
                surfaceColor: Colors.transparent
            }

            IconButton {
                visible: control.helpText.length > 0
                iconName: "help"
                iconSize: Math.round(16 * Metrics.scale)
                iconColor: Colors.textMuted
                implicitWidth: Math.round(28 * Metrics.scale)
                implicitHeight: Math.round(28 * Metrics.scale)
                accessibleName: qsTr("About %1").arg(control.label)
                onClicked: control.helpRequested()
            }
        }

        // -- Value row: minus, value, plus ---------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Spacing.xs

            StepButton {
                direction: -1
                parameterLabel: control.label
                enabled: control.editable && !control.derived
                         && control.pendingValue > control.minimumValue
                onStepped: control.step(-1)
            }

            // Tapping the numeral opens the keypad - faster than stepping 40
            // times, and the keypad has its own confirm step.
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 0

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: Spacing.xs

                        Text {
                            text: control.format(control.pendingValue)
                            color: control.hasPendingChange ? Colors.pendingValue
                                 : control.derived ? Colors.textSecondary
                                 : Colors.textValue
                            font.family: Typography.numberFamily
                            font.features: Typography.numericFeatures
                            font.pixelSize: Typography.titleLarge
                            font.weight: Typography.bold
                            font.letterSpacing: Typography.trackDisplay
                        }

                        Text {
                            visible: control.unit.length > 0
                            text: control.unit
                            color: Colors.textUnit
                            font.family: Typography.family
                            font.pixelSize: Typography.micro
                            Layout.alignment: Qt.AlignBottom
                            Layout.bottomMargin: Math.round(6 * Metrics.scale)
                        }
                    }

                    // The delivered value stays on screen while a change is
                    // pending. Replacing it would leave the operator unable to
                    // see what they are moving away from.
                    RowLayout {
                        visible: control.hasPendingChange
                        Layout.alignment: Qt.AlignHCenter
                        spacing: Spacing.xxs

                        Text {
                            text: qsTr("now")
                            color: Colors.textMuted
                            font.family: Typography.family
                            font.pixelSize: Typography.micro
                        }

                        Text {
                            text: control.format(control.activeValue)
                            color: Colors.textSecondary
                            font.family: Typography.numberFamily
                            font.features: Typography.numericFeatures
                            font.pixelSize: Typography.caption
                            font.weight: Typography.semibold
                        }

                        AppIcon {
                            name: "arrow-right"
                            size: Math.round(12 * Metrics.scale)
                            color: Colors.pendingValue
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: control.editable && !control.derived
                    onClicked: control.keypadRequested()
                }
            }

            StepButton {
                direction: 1
                parameterLabel: control.label
                enabled: control.editable && !control.derived
                         && control.pendingValue < control.maximumValue
                onStepped: control.step(1)
            }
        }

        // -- Advisory strip -------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            visible: control.pendingOutsideAdvisory && !control.derived
            spacing: Spacing.xxs

            AppIcon {
                name: "alarm-medium"
                size: Math.round(13 * Metrics.scale)
                color: Colors.warning
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                text: qsTr("Outside %1-%2 %3")
                    .arg(control.format(control.advisoryLow))
                    .arg(control.format(control.advisoryHigh))
                    .arg(control.unit)
                color: Colors.warning
                font.family: Typography.family
                font.pixelSize: Typography.micro
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        // -- Range footer, or the confirm row when a change is pending ------
        Item {
            Layout.fillWidth: true
            implicitHeight: Math.round(34 * Metrics.scale)

            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: !control.hasPendingChange
                text: control.derived
                    ? qsTr("Set by the ventilator in this mode")
                    : control.format(control.minimumValue) + " - "
                      + control.format(control.maximumValue) + " " + control.unit
                color: Colors.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.micro
                elide: Text.ElideRight
                width: parent.width
            }

            RowLayout {
                anchors.fill: parent
                visible: control.hasPendingChange
                spacing: Spacing.xs

                IconButton {
                    iconName: "close"
                    iconSize: Math.round(16 * Metrics.scale)
                    iconColor: Colors.textSecondary
                    showBorder: true
                    implicitWidth: Math.round(44 * Metrics.scale)
                    implicitHeight: Math.round(32 * Metrics.scale)
                    accessibleName: qsTr("Cancel change to %1").arg(control.label)
                    onClicked: control.discardChange()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.round(32 * Metrics.scale)
                    text: control.isLargeChange || control.hazardous
                        ? qsTr("CONFIRM") : qsTr("APPLY")
                    onClicked: control.applyChange()

                    background: GlossSurface {
                        radius: Radius.sm
                        interaction: parent.pressed ? 2 : (parent.hovered ? 1 : 0)
                        // Lit, because this is the button that commits a
                        // therapy change - it should be the obvious target
                        // once a value is pending.
                        selected: true
                        accentColor: control.hazardous ? Colors.warning
                                                       : Colors.glassRimBright
                        glowColor: control.hazardous ? Colors.warning : Colors.glassGlow
                        bloom: Math.round(5 * Metrics.scale)
                    }

                    contentItem: Text {
                        text: parent.text
                        color: Colors.glassTextOn
                        font.family: Typography.family
                        font.pixelSize: Typography.micro
                        font.weight: Typography.bold
                        font.letterSpacing: Typography.trackCaps
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    // -- Stepper -----------------------------------------------------------
    // Inline components do not share scope with the file they are declared
    // in, so this cannot reach the root `control` id - everything it needs is
    // passed in as a property instead.
    component StepButton: AbstractButton {
        id: stepper
        property int direction: 1
        property string parameterLabel: ""
        signal stepped()

        implicitWidth: Metrics.touchTarget
        implicitHeight: Metrics.touchTarget
        Layout.preferredWidth: Metrics.touchTarget
        Layout.fillHeight: true
        Layout.maximumHeight: Metrics.touchPrimary
        hoverEnabled: true

        Accessible.role: Accessible.Button
        Accessible.name: stepper.direction > 0
            ? qsTr("Increase %1").arg(stepper.parameterLabel)
            : qsTr("Decrease %1").arg(stepper.parameterLabel)

        onPressed: stepper.stepped()

        // Press-and-hold accelerates, but only ever moves the pending value -
        // it can never cross into applying the change.
        Timer {
            interval: 420
            running: stepper.pressed && stepper.enabled
            onTriggered: repeatTimer.start()
        }

        Timer {
            id: repeatTimer
            interval: 90
            repeat: true
            running: false
            onTriggered: {
                if (!stepper.pressed || !stepper.enabled) {
                    stop()
                    return
                }
                stepper.stepped()
            }
        }

        background: GlossSurface {
            radius: Radius.sm
            enabled: stepper.enabled
            interaction: stepper.pressed ? 2 : (stepper.hovered ? 1 : 0)
            bloom: stepper.pressed || stepper.hovered
                ? Math.round(5 * Metrics.scale) : Math.round(2 * Metrics.scale)
        }

        contentItem: Item {
            AppIcon {
                anchors.centerIn: parent
                name: stepper.direction > 0 ? "plus" : "minus"
                size: Math.round(20 * Metrics.scale)
                color: stepper.enabled ? Colors.textPrimary : Colors.textDisabled
            }
        }
    }
}
