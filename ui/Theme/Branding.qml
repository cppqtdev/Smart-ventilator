// -----------------------------------------------------------------------
// File: Branding.qml
// Description: Product identity and start-up configuration, in one place
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Everything a customer changes when the device is badged for them lives
// here: the names, the mark, and how long the start-up screen is allowed to
// hold the display. A rebadge should touch this file and nothing else.
//
pragma Singleton

import QtQuick

QtObject {
    id: branding

    readonly property string company: "Alsons Technology"
    readonly property string companyLine1: "ALSONS"
    readonly property string companyLine2: "TECHNOLOGY"
    readonly property string product: "Smart Ventilator"
    readonly property string model: "SV-1"

    readonly property string regulatoryLine:
        qsTr("Not for clinical use until the pre-use check has passed")

    // Start-up timing. The lower bound stops the screen flashing past on a
    // fast boot; the upper bound is the safety one - a start-up screen must
    // never be what keeps a ventilator off the patient, so it gives up
    // waiting and hands over whatever the checks reported.
    readonly property int splashMinimumMs: 1800
    readonly property int splashMaximumMs: 8000
    readonly property int splashHandoverMs: 420

    // Each stage names a real readiness condition. The splash screen binds
    // the condition and shows the stage as pending, ready or failed; it does
    // not run a timer and pretend.
    readonly property var bootStages: [
        { key: "storage",   label: qsTr("Storage and audit trail") },
        { key: "operators", label: qsTr("Operator accounts") },
        { key: "alarms",    label: qsTr("Alarm system") },
        { key: "device",    label: qsTr("Device link") },
        { key: "session",   label: qsTr("Bedside session") },
        { key: "interface", label: qsTr("Interface") }
    ]
}
