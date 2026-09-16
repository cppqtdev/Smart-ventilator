// -----------------------------------------------------------------------
// File: StandbyScreen.qml
// Description: Standby - patient selection before ventilation starts
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "Panes"
import "../Controls"
import "../Theme"

// The shared chrome - header, status banner, sidebar tiles, control rail and
// tab bar - lives once in main.qml. This file is only what changes when the
// operator moves between tabs.
Item {
    id: screen

    property var presenter
    property var patientData
    property var ventilatorData
    property int standbySeconds: 0

    signal startRequested()
    signal setupRequested()

    readonly property var categories: [
        { key: "Neonatal",  label: qsTr("Neonatal") },
        { key: "Pediatric", label: qsTr("Adult/ped.") },
        { key: "Adult",     label: qsTr("Last patient") }
    ]

    function elapsedText() {
        var total = Math.max(0, screen.standbySeconds)
        var hours = Math.floor(total / 3600)
        var minutes = Math.floor((total % 3600) / 60)
        var seconds = total % 60
        return Qt.formatTime(new Date(0, 0, 0, hours, minutes, seconds), "hh:mm:ss")
    }

    // Admitting the patient is what arms the safe ranges and clears the
    // previous patient's pre-use check, so the category is not just a label.
    function selectCategory(key) {
        if (screen.patientData)
            screen.patientData.category = key
        if (screen.ventilatorData) {
            screen.ventilatorData.acceptPatient(
                key, screen.patientData ? screen.patientData.ibw : 70)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Metrics.gutter

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.px(120)
            radius: Radius.medium
            color: Colors.accentSubtle

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Spacing.sm

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: screen.elapsedText()
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.title
                    font.weight: Typography.bold
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("No ventilation delivered to the patient.")
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.readoutLabel
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Deactivate humidifier during standby.")
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.readoutLabel
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Metrics.gutter

            Repeater {
                model: screen.categories

                delegate: AppButton {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: Metrics.px(30)

                    text: modelData.label
                    buttonVariant: AppButton.Primary
                    selected: screen.patientData
                             && screen.patientData.category === modelData.key
                    onClicked: screen.selectCategory(modelData.key)
                }
            }
        }

        PatientDataPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            patientData: screen.patientData
            ventilatorData: screen.ventilatorData
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Metrics.gutter

            AppButton {
                Layout.fillWidth: true
                Layout.preferredHeight: Metrics.buttonHeightSmall
                text: qsTr("Test & Calib")
                buttonVariant: AppButton.Primary
                onClicked: screen.setupRequested()
            }

            AppButton {
                Layout.fillWidth: true
                Layout.preferredHeight: Metrics.buttonHeightSmall
                text: qsTr("Start Ventilation")
                buttonVariant: AppButton.Success
                enabled: screen.ventilatorData
                         && screen.ventilatorData.readyToVentilate
                onClicked: screen.startRequested()
            }
        }

        Text {
            Layout.fillWidth: true
            text: screen.ventilatorData ? screen.ventilatorData.readinessReason : ""
            color: Colors.warning
            font.family: Typography.monoFamily
            font.pixelSize: Typography.caption
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: text.length > 0
        }
    }
}
