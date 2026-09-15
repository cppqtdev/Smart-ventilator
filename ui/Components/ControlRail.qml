// -----------------------------------------------------------------------
// File: ControlRail.qml
// Description: Ventilation and freeze actions over the setting dials
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../Controls"
import "../Theme"

Rectangle {
    id: rail

    // Each entry: { key, label, value, from, to, step, unit, decimals }
    property var dials: []
    property bool frozen: false
    property bool ventilating: false

    signal freezeToggled(bool frozen)
    signal ventilationToggled(bool start)
    signal settingRequested(string key, real value)
    signal settingActivated(string key)

    radius: Radius.medium
    color: Colors.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.lg
        spacing: Spacing.lg

        // Start and stop belong on the rail because the rail is on every
        // screen. Without it the only way to reach standby is the screen the
        // application happens to open on.
        AppButton {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.actionButtonHeight

            text: rail.ventilating ? qsTr("Stop") : qsTr("Start")
            buttonVariant: rail.ventilating ? AppButton.Danger : AppButton.Success
            fontFamily: Typography.family
            fontSize: Typography.label
            onClicked: rail.ventilationToggled(!rail.ventilating)
        }

        AppButton {
            Layout.preferredWidth: Metrics.px(89)
            Layout.preferredHeight: Metrics.px(32)
            Layout.alignment: Qt.AlignLeft

            text: qsTr("Freeze")
            buttonVariant: AppButton.Primary
            checkable: true
            checked: rail.frozen
            fontFamily: Typography.family
            fontSize: Typography.label
            onToggled: rail.freezeToggled(checked)
        }

        Repeater {
            model: rail.dials

            delegate: DialControl {
                required property var modelData

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter

                label: modelData.label
                value: modelData.value
                from: modelData.from
                to: modelData.to
                stepSize: modelData.step !== undefined ? modelData.step : 1
                unit: modelData.unit !== undefined ? modelData.unit : ""
                decimals: modelData.decimals !== undefined ? modelData.decimals : 0

                onStepRequested: function (proposedValue) {
                    rail.settingRequested(modelData.key, proposedValue)
                }
                onValueClicked: rail.settingActivated(modelData.key)
            }
        }

        Item { Layout.fillHeight: true }
    }
}
