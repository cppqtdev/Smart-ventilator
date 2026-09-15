// -----------------------------------------------------------------------
// File: AppTabBar.qml
// Description: Equal-width tab strip with a single checked entry
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../Theme"

Item {
    id: bar

    property var model: []
    property int currentIndex: 0
    property bool useSuccessPalette: false
    property int spacing: Metrics.navGap

    signal activated(int index, string key)

    implicitHeight: Metrics.navHeight

    function keyAt(index) {
        var entry = bar.model[index]
        if (entry === undefined)
            return ""
        return entry.key !== undefined ? entry.key : String(entry)
    }

    function labelAt(index) {
        var entry = bar.model[index]
        if (entry === undefined)
            return ""
        return entry.label !== undefined ? entry.label : String(entry)
    }

    RowLayout {
        anchors.fill: parent
        spacing: bar.spacing

        Repeater {
            model: bar.model

            delegate: AppTabButton {
                required property int index

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 0
                Layout.minimumWidth: 0

                text: bar.labelAt(index)
                checked: bar.currentIndex === index
                useSuccessPalette: bar.useSuccessPalette

                onClicked: {
                    bar.currentIndex = index
                    bar.activated(index, bar.keyAt(index))
                }
            }
        }
    }
}
