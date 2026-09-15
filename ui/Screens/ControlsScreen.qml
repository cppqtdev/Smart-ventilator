// -----------------------------------------------------------------------
// File: ControlsScreen.qml
// Description: Controls screen
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "Panes"
import "../Components"

ScreenShell {
    id: screen

    property var patientData
    property var ventilatorData

    signal settingRequested(string key, real value)

    destination: "controls"
    showRail: false

    ControlsPane {
        anchors.fill: parent
        presenter: screen.presenter
        patientData: screen.patientData
        ventilatorData: screen.ventilatorData
        onSettingRequested: function (key, value) {
            screen.settingRequested(key, value)
        }
    }
}
