// -----------------------------------------------------------------------
// File: AlarmCenterScreen.qml
// Description: Alarms screen
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
    property var alarmData
    property var ventilatorData

    signal limitRequested(string key, real value)

    AlarmsPane {
        anchors.fill: parent
        ventilatorData: screen.ventilatorData
        alarmData: screen.alarmData
        onLimitRequested: function (key, value) {
            screen.limitRequested(key, value)
        }
    }
}
