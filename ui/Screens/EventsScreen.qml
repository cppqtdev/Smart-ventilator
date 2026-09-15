// -----------------------------------------------------------------------
// File: EventsScreen.qml
// Description: Events screen
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "Panes"
import "../Components"

ScreenShell {
    id: screen

    property var eventData
    property var logData

    destination: "events"

    EventsPane {
        anchors.fill: parent
        eventData: screen.eventData
        logData: screen.logData
    }
}
