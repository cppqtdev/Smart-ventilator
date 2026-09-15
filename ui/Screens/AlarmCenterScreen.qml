// -----------------------------------------------------------------------
// File: AlarmCenterScreen.qml
// Description: Alarms screen
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "Panes"
import "../Components"

ScreenShell {
    id: screen

    property var alarmData
    property var ventilatorData

    signal limitRequested(string key, real value)

    destination: "alarms"

    AlarmsPane {
        anchors.fill: parent
        ventilatorData: screen.ventilatorData
        alarmData: screen.alarmData
        onLimitRequested: function (key, value) {
            screen.limitRequested(key, value)
        }
    }
}
