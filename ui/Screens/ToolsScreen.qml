// -----------------------------------------------------------------------
// File: ToolsScreen.qml
// Description: Tools screen
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
    property var ventilatorData
    property var alarmData

    signal inspiratoryHoldRequested()
    signal expiratoryHoldRequested()
    signal settingsRequested()

    ToolsPane {
        anchors.fill: parent
        ventilatorData: screen.ventilatorData
        alarmData: screen.alarmData
        onInspiratoryHoldRequested: screen.inspiratoryHoldRequested()
        onExpiratoryHoldRequested: screen.expiratoryHoldRequested()
        onSettingsRequested: screen.settingsRequested()
    }
}
