// -----------------------------------------------------------------------
// File: PatientSetupScreen.qml
// Description: Patient category, gender and height before ventilation
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
    property var catalog

    signal continueRequested()

    readonly property var categories: [
        { key: "Adult",     label: qsTr("Adult"),     icon: "patient-adult" },
        { key: "Pediatric", label: qsTr("Pediatric"), icon: "patient-pediatric" },
        { key: "Neonatal",  label: qsTr("Neonatal"),  icon: "patient-neonatal" }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: Metrics.gutter

        Text {
            text: qsTr("PATIENT SELECTION")
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.caption
            font.weight: Typography.bold
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Metrics.gutter

            Repeater {
                model: screen.categories

                delegate: AppButton {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: Metrics.px(34)

                    text: modelData.label
                    buttonVariant: AppButton.Primary
                    selected: screen.patientData
                             && screen.patientData.category === modelData.key
                    onClicked: {
                        if (screen.patientData)
                            screen.patientData.category = modelData.key
                    }
                }
            }
        }

        PatientDataPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            patientData: screen.patientData
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Metrics.gutter

            Item { Layout.fillWidth: true }

            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(110), implicitWidth)
                Layout.preferredHeight: Metrics.px(34)
                text: qsTr("Confirm")
                buttonVariant: AppButton.Success
                onClicked: screen.continueRequested()
            }
        }
    }
}
