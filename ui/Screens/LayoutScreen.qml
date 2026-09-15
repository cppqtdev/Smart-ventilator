// -----------------------------------------------------------------------
// File: LayoutScreen.qml
// Description: Layout screen - the arrangement picker and the content chooser
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// reference/layout-picker.png shows the five arrangements under the Layout
// tab. reference/trends-selection.png and its siblings show the chooser for
// what goes in a cell, with no bottom tab lit, so it is reached from here.
//
import QtQuick
import QtQuick.Layouts
import "Home"
import "Panes"
import "../Components"
import "../Controls"
import "../Theme"

ScreenShell {
    id: screen

    property var settingsData
    property bool choosingContent: false

    destination: "layout"

    ColumnLayout {
        anchors.fill: parent
        spacing: Metrics.gutter

        LayoutPicker {
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            visible: !screen.choosingContent

            currentId: screen.settingsData ? screen.settingsData.monitoringLayout
                                           : HomeLayouts.defaultId

            onLayoutSelected: function (layoutId) {
                if (screen.settingsData)
                    screen.settingsData.monitoringLayout = layoutId
            }
        }

        ContentSelectionPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: screen.choosingContent
            onConfirmed: screen.choosingContent = false
        }

        AppButton {
            Layout.alignment: Qt.AlignLeft
            Layout.preferredHeight: Metrics.px(30)
            Layout.preferredWidth: Math.max(Metrics.px(170), implicitWidth)
            text: screen.choosingContent ? qsTr("Back to layouts")
                                         : qsTr("Choose cell content")
            buttonVariant: AppButton.Success
            onClicked: screen.choosingContent = !screen.choosingContent
        }

        Item {
            Layout.fillHeight: true
            visible: !screen.choosingContent
        }
    }
}
