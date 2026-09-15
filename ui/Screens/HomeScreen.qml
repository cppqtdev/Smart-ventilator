// -----------------------------------------------------------------------
// File: HomeScreen.qml
// Description: Monitoring screen with a selectable, persisted content layout
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "Home"
import "../Theme"

// The shared chrome - header, status banner, sidebar tiles, control rail and
// tab bar - lives once in main.qml. This file is only what changes when the
// operator moves between tabs.
Item {
    id: home

    property var presenter
    property var settingsData

    property int layoutId: home.settingsData
                           ? home.settingsData.monitoringLayout
                           : HomeLayouts.defaultId

    // Writing the setting is enough: layoutId is bound to it, and the store
    // clamps the value. Assigning layoutId here as well would break that
    // binding and leave the screen showing something the store never held.
    function selectLayout(id) {
        if (home.settingsData)
            home.settingsData.monitoringLayout = id
        else
            home.layoutId = id
    }

    Loader {
        anchors.fill: parent
        source: HomeLayouts.sourceFor(home.layoutId)

        onLoaded: {
            item.presenter = Qt.binding(function () { return home.presenter })
            if (item.frozen !== undefined)
                item.frozen = Qt.binding(function () { return home.frozen })
            item.patientClicked.connect(home.patientRequested)
        }
    }
}
