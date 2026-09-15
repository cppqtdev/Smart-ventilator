// -----------------------------------------------------------------------
// File: SystemScreen.qml
// Description: System screen
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
    property var settingsData
    property var ventilatorData
    property var calibrationService
    property var clockData

    /** Which sub-tab to open on. 1 is Tests and Calibration. */
    property int initialPage: 0

    SystemPane {
        id: pane
        anchors.fill: parent

        settingsData: screen.settingsData
        ventilatorData: screen.ventilatorData
        calibrationService: screen.calibrationService
        clockData: screen.clockData

        // Set once rather than bound, so the operator's own tab choice sticks
        // while they are on this screen.
        Component.onCompleted: pane.pageIndex = screen.initialPage
    }
}
