// -----------------------------------------------------------------------
// File: ContentWaveformsFour.qml
// Description: Home layout 3 - pressure, flow, volume and carbon dioxide
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
        },
        {
            key: "volume",
            label: qsTr("V"),
            color: Colors.waveVolume,
            scaleKey: "volume",
            baseline: 0
        },
        {
            key: "co2",
            label: qsTr("PCO2"),
            color: Colors.waveCo2,
            scaleKey: "co2",
            baseline: 0
        }
    ]
}
