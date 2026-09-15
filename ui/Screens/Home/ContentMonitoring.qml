// -----------------------------------------------------------------------
// File: ContentMonitoring.qml
// Description: Home layout 1 - two waveforms over the numeric summary
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Components"
import "../../Controls"
import "../../Theme"

ColumnLayout {
    id: content

    property var presenter
    property bool frozen: false

    readonly property var readouts: content.presenter ? content.presenter.readouts : []
    readonly property var patient: content.presenter ? content.presenter.patient : ({})

    signal patientClicked()

    spacing: Metrics.px(20)

    WaveformPanel {
        id: waves
        Layout.fillWidth: true
        Layout.preferredHeight: Metrics.px(274)
        frozen: content.frozen
        presenter: content.presenter
        channels: [
            {
                key: "paw",
                label: qsTr("Paw"),
                color: Colors.wavePressure,
                minimum: 0,
                maximum: 40,
                baseline: 0,
                ticks: [40, 20, 0]
            },
            {
                key: "flow",
                label: qsTr("Flow"),
                color: Colors.waveFlow,
                minimum: -75,
                maximum: 75,
                baseline: 0,
                ticks: [75, 0, -75]
            }
        ]
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 0

        GridLayout {
            Layout.fillWidth: false
            Layout.preferredWidth: Metrics.px(227)
            Layout.fillHeight: true
            columns: 2
            rowSpacing: Spacing.lg
            columnSpacing: Metrics.px(7)

            Repeater {
                model: content.readouts

                delegate: NumericReadout {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignTop

                    label: modelData.label
                    value: modelData.value
                    unit: modelData.unit
                    valueColor: modelData.accent === true ? Colors.accent : Colors.textPrimary
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Image {
                anchors.centerIn: parent
                width: Math.min(parent.width, Metrics.px(216))
                height: Math.min(parent.height, Metrics.px(236))
                source: "qrc:/ui/Assets/lungs.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
                asynchronous: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: false
            Layout.preferredWidth: Metrics.px(85)
            Layout.fillHeight: true
            spacing: 0

            PatientSummary {
                Layout.fillWidth: true
                gender: content.patient.gender !== undefined ? content.patient.gender : ""
                heightText: content.patient.height !== undefined
                        ? qsTr("%1 cm").arg(content.patient.height) : ""
                weightText: content.patient.ibw !== undefined
                        ? qsTr("IBW: %1 kg").arg(content.patient.ibw) : ""
                onClicked: content.patientClicked()
            }

            Item { Layout.fillHeight: true }

            NumericReadout {
                Layout.fillWidth: true
                alignment: Text.AlignRight
                label: qsTr("PVI")
                value: "-------"
                unit: "%"
                valueColor: Colors.textSecondary
            }
        }
    }
}
