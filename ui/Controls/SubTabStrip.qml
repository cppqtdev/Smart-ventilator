// -----------------------------------------------------------------------
// File: SubTabStrip.qml
// Description: The green sub-tab row the reference uses inside a screen
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "../Theme"

AppTabBar {
    id: strip

    useSuccessPalette: true
    spacing: Metrics.navGap
    implicitHeight: Metrics.navHeight
}
