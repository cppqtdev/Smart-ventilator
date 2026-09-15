// -----------------------------------------------------------------------
// File: ModeSelectionScreen.qml
// Description: Modes screen
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "Panes"
import "../Components"

ScreenShell {
    id: screen

    property var ventilatorData
    property var catalog

    signal modeConfirmed(string mode)
    signal cancelled()

    destination: "modes"

    ModesPane {
        anchors.fill: parent
        ventilatorData: screen.ventilatorData
        catalog: screen.catalog
        onModeConfirmed: function (mode) { screen.modeConfirmed(mode) }
        onCancelled: screen.cancelled()
    }
}
