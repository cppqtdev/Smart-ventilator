pragma ComponentBehavior: Bound
// -----------------------------------------------------------------------
// File: BottomNavigation.qml
// Description: Persistent destination bar
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Thirteen destinations is a lot for one row, so they are grouped by what the
// clinician is doing rather than listed arbitrarily:
//
//   observe   monitor, trends, loops
//   act       controls, modes, targets
//   review    alarms, events, clinical
//   maintain  tools, system, layout, settings
//
// A divider between groups costs four pixels and saves a scan of the whole
// row. Labels drop on panels below 1400 px; the icons and the 12 mm touch
// target do not.
//
// The bar keeps real space beneath the buttons. Sitting them flush against the
// bottom edge makes the row read as cut off, and on a panel with a bezel it
// puts the touch target right on the rim where a thumb cannot land squarely.
//
// Note for future edits: this deliberately does not use an inline `component`
// for the button. An inline component does not share scope with the file it is
// declared in, so it cannot see `nav` - the previous version did exactly that
// and its `checked` binding silently never resolved.
//
import QtQuick
import QtQuick.Layouts
import "../Theme"

Item {
    id: nav

    property string currentScreen: "monitoring"
    property bool ventilating: false
    property int activeAlarmCount: 0
    property string alarmPriority: "none"

    signal navigate(string screen)

    readonly property var destinations: [
        { screen: "monitoring", label: qsTr("Monitor"),  icon: "nav-monitoring", group: 0 },
        { screen: "trends",     label: qsTr("Trends"),   icon: "nav-trends",     group: 0 },
        { screen: "loops",      label: qsTr("Loops"),    icon: "nav-loops",      group: 0 },
        { screen: "controls",   label: qsTr("Controls"), icon: "nav-controls",   group: 1 },
        { screen: "modes",      label: qsTr("Modes"),    icon: "nav-modes",      group: 1 },
        { screen: "target",     label: qsTr("Targets"),  icon: "nav-target",     group: 1 },
        { screen: "alarms",     label: qsTr("Alarms"),   icon: "nav-alarms",     group: 2 },
        { screen: "events",     label: qsTr("Events"),   icon: "nav-events",     group: 2 },
        { screen: "clinical",   label: qsTr("Clinical"), icon: "nav-clinical",   group: 2 },
        { screen: "tools",      label: qsTr("Tools"),    icon: "nav-tools",      group: 3 },
        { screen: "system",     label: qsTr("System"),   icon: "nav-system",     group: 3 },
        { screen: "layout",     label: qsTr("Layout"),   icon: "nav-layout",     group: 3 },
        { screen: "settings",   label: qsTr("Settings"), icon: "nav-settings",   group: 3 }
    ]

    implicitHeight: Metrics.navHeight + Spacing.sm

    Rectangle {
        anchors.fill: parent
        color: Colors.surface

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: Metrics.borderWidth
            color: Colors.line
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Spacing.screenMargin
        anchors.rightMargin: Spacing.screenMargin
        anchors.topMargin: Spacing.xs
        anchors.bottomMargin: Spacing.sm
        spacing: Metrics.touchSpacing

        Repeater {
            model: nav.destinations

            delegate: RowLayout {
                id: entry
                required property var modelData
                required property int index

                spacing: Metrics.touchSpacing
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Divider before the first entry of each new group.
                Rectangle {
                    visible: entry.index > 0
                             && nav.destinations[entry.index - 1].group !== entry.modelData.group
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: Spacing.xxs
                    Layout.rightMargin: Spacing.xxs
                    width: Metrics.borderWidth
                    height: Math.round(34 * Metrics.scale)
                    color: Colors.line
                }

                NavigationButton {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: entry.modelData.label
                    iconName: entry.modelData.icon
                    screen: entry.modelData.screen
                    selected: nav.currentScreen === entry.modelData.screen
                    showLabel: !Metrics.compact
                    badgeCount: entry.modelData.screen === "alarms" ? nav.activeAlarmCount : 0
                    badgePriority: entry.modelData.screen === "alarms" ? nav.alarmPriority : "none"
                    onClicked: nav.navigate(entry.modelData.screen)
                }
            }
        }
    }
}
