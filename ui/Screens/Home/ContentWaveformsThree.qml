// -----------------------------------------------------------------------
// File: ContentWaveformsThree.qml
// Description: Home layout 4 - pressure, flow and volume
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "../../Components"
import "../../Theme"

WaveformPanel {
    id: content


    signal patientClicked()

    channels: [
        {
            key: "paw",
            label: qsTr("Paw"),
            color: Colors.wavePressure,
            minimum: 0, maximum: 40, baseline: 0,
            ticks: [40, 20, 0]
        },
        {
            key: "flow",
            label: qsTr("Flow"),
            color: Colors.waveFlow,
            minimum: -75, maximum: 75, baseline: 0,
            ticks: [75, 0, -75]
        },
        {
            key: "volume",
            label: qsTr("V"),
            color: Colors.waveVolume,
            minimum: 0, maximum: 40, baseline: 0,
            ticks: [40, 20, 0]
        }
    ]
}
