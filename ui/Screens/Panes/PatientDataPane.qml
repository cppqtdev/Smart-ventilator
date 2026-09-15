// -----------------------------------------------------------------------
// File: PatientDataPane.qml
// Description: Controls > Patient - gender and height on a raised panel
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Ventilation time and its reset live in the left column of ControlsPane,
// outside this panel, the way the reference draws them.
//
import QtQuick
import QtQuick.Layouts
import "../../Components"
import "../../Controls"
import "../../Theme"

Rectangle {
    id: pane

    property var patientData
    property var ventilatorData

    signal settingRequested(string key, real value)

    radius: Radius.medium
    color: Colors.surfaceOverlay

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - Spacing.xxl * 2, Metrics.px(320))
        spacing: Spacing.sm

        DialControl {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Metrics.px(183)
            label: ""
            value: pane.patientData ? pane.patientData.height : 170
            from: 30
            to: 220
            stepSize: 1
            unit: "cm"
            onStepRequested: function (proposed) {
                if (pane.patientData)
                    pane.patientData.height = proposed
                pane.settingRequested("height", proposed)
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Metrics.px(40)
            spacing: Spacing.md

            AppIcon {
                Layout.alignment: Qt.AlignVCenter
                name: "gender-male"
                size: Metrics.px(34)
                color: Colors.textPrimary
            }

            AppButton {
                Layout.preferredWidth: Metrics.actionButtonWidth
                Layout.preferredHeight: Metrics.actionButtonHeight
                text: qsTr("Male")
                buttonVariant: AppButton.Primary
                checkable: true
                checked: pane.patientData
                         && String(pane.patientData.gender).toLowerCase() === "male"
                onClicked: {
                    if (pane.patientData)
                        pane.patientData.gender = "Male"
                }
            }

            AppButton {
                Layout.preferredWidth: Metrics.actionButtonWidth
                Layout.preferredHeight: Metrics.actionButtonHeight
                text: qsTr("Female")
                buttonVariant: AppButton.Primary
                checkable: true
                checked: pane.patientData
                         && String(pane.patientData.gender).toLowerCase() === "female"
                onClicked: {
                    if (pane.patientData)
                        pane.patientData.gender = "Female"
                }
            }

            AppIcon {
                Layout.alignment: Qt.AlignVCenter
                name: "gender-female"
                size: Metrics.px(34)
                color: Colors.neutral
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: Metrics.px(14)
            text: qsTr("Pat. height")
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.bodyLarge
            font.weight: Typography.bold
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true
            text: pane.patientData ? pane.patientData.gender : ""
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.body
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true
            text: pane.patientData ? qsTr("IBW: %1 kg").arg(pane.patientData.ibw) : ""
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.body
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
