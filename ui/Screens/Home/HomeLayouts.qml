pragma Singleton
// -----------------------------------------------------------------------
// File: HomeLayouts.qml
// Description: The selectable home layouts and their content sources
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick

QtObject {
    id: layouts

    readonly property var entries: [
        {
            id: 1,
            label: qsTr("Layout 1"),
            source: "ContentMonitoring.qml",
            cells: [{ x: 0, y: 0, w: 2, h: 1 }, { x: 0, y: 1, w: 2, h: 1 }]
        },
        {
            id: 2,
            label: qsTr("Layout 2"),
            source: "ContentWaveformNumerics.qml",
            cells: [{ x: 0, y: 0, w: 1, h: 2 }, { x: 1, y: 0, w: 1, h: 1 }, { x: 1, y: 1, w: 1, h: 1 }]
        },
        {
            id: 3,
            label: qsTr("Layout 3"),
            source: "ContentWaveformsFour.qml",
            cells: [{ x: 0, y: 0, w: 1, h: 1 }, { x: 1, y: 0, w: 1, h: 1 },
                    { x: 0, y: 1, w: 1, h: 1 }, { x: 1, y: 1, w: 1, h: 1 }]
        },
        {
            id: 4,
            label: qsTr("Layout 4"),
            source: "ContentWaveformsThree.qml",
            cells: [{ x: 0, y: 0, w: 2, h: 1 }, { x: 0, y: 1, w: 2, h: 1 }, { x: 0, y: 2, w: 2, h: 1 }]
        },
        {
            id: 5,
            label: qsTr("Layout 5"),
            source: "ContentDynamicLung.qml",
            cells: [{ x: 0, y: 0, w: 2, h: 2 }]
        },
        {
            id: 6,
            label: qsTr("Layout 6"),
            source: "ContentDualGraphics.qml",
            cells: [{ x: 0, y: 0, w: 2, h: 1 },
                    { x: 0, y: 1, w: 1, h: 1 }, { x: 1, y: 1, w: 1, h: 1 }]
        },
        {
            id: 7,
            label: qsTr("Layout 7"),
            source: "ContentAsvPair.qml",
            cells: [{ x: 0, y: 0, w: 2, h: 1 },
                    { x: 0, y: 1, w: 1, h: 1 }, { x: 1, y: 1, w: 1, h: 1 }]
        }
    ]

    readonly property int defaultId: 1

    function entryFor(layoutId) {
        for (var i = 0; i < layouts.entries.length; ++i) {
            if (layouts.entries[i].id === layoutId)
                return layouts.entries[i]
        }
        return layouts.entries[0]
    }

    readonly property string base: "qrc:/ui/Screens/Home/"

    function sourceFor(layoutId) {
        return layouts.base + layouts.entryFor(layoutId).source
    }
}
