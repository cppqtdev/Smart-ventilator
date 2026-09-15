// -----------------------------------------------------------------------
// File: ContentWaveformNumerics.qml
// Description: Home layout 2 - two waveforms over the numeric summary panel
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Components"
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
        Layout.fillHeight: true
        Layout.minimumHeight: Metrics.px(220)
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

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: Metrics.px(210)
        radius: Radius.medium
        color: Colors.surface

        GridLayout {
            anchors.fill: parent
            anchors.margins: Spacing.lg
            columns: 4
            rowSpacing: Spacing.xl
            columnSpacing: Spacing.xxl

            Repeater {
                model: content.readouts

                delegate: NumericReadout {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop

                    label: modelData.label
                    value: modelData.value
                    unit: modelData.unit
                    valueColor: modelData.accent === true ? Colors.accent : Colors.textPrimary
                }
            }
        }
    }
}
