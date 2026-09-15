// -----------------------------------------------------------------------
// File: EventsScreen.qml
// Description: Events screen
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
    property var eventData
    property var logData

    EventsPane {
        anchors.fill: parent
        eventData: screen.eventData
        logData: screen.logData
    }
}
