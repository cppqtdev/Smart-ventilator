// -----------------------------------------------------------------------
// File: ContentDynamicLung.qml
// Description: Home layout 5 - dynamic lung with the numeric strip beneath
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The lung breathes with the delivered breath rather than on a decorative
// loop: it fills over the inspiratory time and empties over what is left of
// the cycle, and its size follows the measured expired volume against the
// set tidal volume. A lung that breathes when the ventilator is stopped
// would be telling the operator something that is not true, so it rests.
//
import QtQuick
import QtQuick.Layouts
import "../../Components"
import "../../Theme"

Rectangle {
    id: content

    property var presenter
    property bool frozen: false

    readonly property var readouts: content.presenter ? content.presenter.readouts : []
    readonly property var patient: content.presenter ? content.presenter.patient : ({})

    // Plethysmographic variability is not measured without a plethysmograph,
    // so it reads as dashes rather than inventing a number.
    readonly property string pleth: "-------"

    signal patientClicked()

    radius: Radius.medium
    color: Colors.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.lg
        spacing: Spacing.lg

        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.lg

            PatientSummary {
                Layout.alignment: Qt.AlignTop
                gender: content.patient.gender !== undefined ? content.patient.gender : ""
                heightText: content.patient.height !== undefined
                        ? qsTr("%1 cm").arg(content.patient.height) : ""
                weightText: content.patient.ibw !== undefined
                        ? qsTr("IBW: %1 kg").arg(content.patient.ibw) : ""
                onClicked: content.patientClicked()
            }

            Item { Layout.fillWidth: true }

            NumericReadout {
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: Metrics.px(90)
                alignment: Text.AlignRight
                label: qsTr("PVI")
                value: content.pleth
                unit: "%"
            }
        }

        Item {
            id: lungStage

            Layout.fillWidth: true
            Layout.fillHeight: true

            BreathingLung {
                anchors.fill: parent
                presenter: content.presenter
                frozen: content.frozen
            }
        }

        // The reference puts all six across the foot of the screen. That
        // only fits where the pane is wide enough for them, so the row count
        // follows the width rather than being fixed at three.
        GridLayout {
            Layout.fillWidth: true
            columns: Math.max(2, Math.min(content.readouts.length,
                                          Math.floor(width / Metrics.px(118))))
            rowSpacing: Spacing.md
            columnSpacing: Metrics.px(20)

            Repeater {
                model: content.readouts

                delegate: NumericReadout {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.minimumWidth: Metrics.px(96)

                    label: modelData.label
                    value: modelData.value
                    unit: modelData.unit
                    valueColor: modelData.accent === true ? Colors.accent : Colors.textPrimary
                }
            }
        }
    }
}
