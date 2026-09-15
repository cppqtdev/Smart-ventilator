// -----------------------------------------------------------------------
// File: SystemScreen.qml
// Description: System screen
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "Panes"
import "../Components"

ScreenShell {
    id: screen

    property var settingsData
    property var ventilatorData
    property var calibrationService
    property var clockData

    destination: "system"

    SystemPane {
        anchors.fill: parent
        settingsData: screen.settingsData
        ventilatorData: screen.ventilatorData
        calibrationService: screen.calibrationService
        clockData: screen.clockData
    }
}
