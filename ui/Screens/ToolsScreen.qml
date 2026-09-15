// -----------------------------------------------------------------------
// File: ToolsScreen.qml
// Description: Tools screen
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "Panes"
import "../Components"

ScreenShell {
    id: screen

    property var ventilatorData
    property var alarmData

    signal inspiratoryHoldRequested()
    signal expiratoryHoldRequested()
    signal settingsRequested()

    destination: "tools"

    ToolsPane {
        anchors.fill: parent
        ventilatorData: screen.ventilatorData
        alarmData: screen.alarmData
        onInspiratoryHoldRequested: screen.inspiratoryHoldRequested()
        onExpiratoryHoldRequested: screen.expiratoryHoldRequested()
        onSettingsRequested: screen.settingsRequested()
    }
}
