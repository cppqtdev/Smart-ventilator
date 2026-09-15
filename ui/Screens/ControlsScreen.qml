// -----------------------------------------------------------------------
// File: ControlsScreen.qml
// Description: Controls screen
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "Panes"

// The shared chrome - header, status banner, sidebar tiles, control rail and
// tab bar - lives once in main.qml. This file is only what changes when the
// operator moves between tabs.
Item {
    id: screen

    property var presenter
    property var patientData
    property var ventilatorData

    signal settingRequested(string key, real value)

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
