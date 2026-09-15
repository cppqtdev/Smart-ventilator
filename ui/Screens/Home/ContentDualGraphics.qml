// -----------------------------------------------------------------------
// File: ContentDualGraphics.qml
// Description: Home layout 6 - waveforms over the target graph and the lung
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The reference pairs the two waveforms with two pictures of the same
// breath: the adaptive target, which says what the mode is aiming at, and
// the lung, which says what the patient is doing with it.
//
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

    spacing: Metrics.gutter

    WaveformPanel {
        Layout.fillWidth: true
        Layout.preferredHeight: Metrics.px(258)
        frozen: content.frozen
        presenter: content.presenter
        channels: [
            {
                key: "paw",
                label: qsTr("Paw"),
                color: Colors.wavePressure,
                scaleKey: "pressure",
                baseline: 0
            },
            {
                key: "flow",
                label: qsTr("Flow"),
                color: Colors.waveFlow,
                scaleKey: "flow",
                signedScale: true,
                baseline: 0
            }
        ]
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Metrics.gutter

        AsvTargetPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            presenter: content.presenter
        }

        LungPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            presenter: content.presenter
            frozen: content.frozen
            onPatientClicked: content.patientClicked()
        }
    }
}
