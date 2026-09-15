// -----------------------------------------------------------------------
// File: ModeSelectionScreen.qml
// Description: Modes screen
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
    property var catalog

    signal modeConfirmed(string mode)
    signal cancelled()

    ModesPane {
        anchors.fill: parent
        ventilatorData: screen.ventilatorData
        catalog: screen.catalog
        onModeConfirmed: function (mode) { screen.modeConfirmed(mode) }
        onCancelled: screen.cancelled()
    }
}
