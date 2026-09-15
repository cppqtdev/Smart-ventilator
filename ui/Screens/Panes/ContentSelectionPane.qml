// -----------------------------------------------------------------------
// File: ContentSelectionPane.qml
// Description: Chooses what a layout cell shows - trend, loop, graphic or wave
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: pane

    property int pageIndex: 0
    property string trendWindow: "1 Hour"
    property string trendPair: "Ppeak/PEEP"
    property string selection: ""
    property bool pairMenuOpen: false

    signal confirmed(string page, string selection)

    readonly property var pages: [
        { key: "trends",    label: qsTr("Trends") },
        { key: "loops",     label: qsTr("Loops") },
        { key: "graphics",  label: qsTr("Graphics") },
        { key: "waveforms", label: qsTr("Waveforms") }
    ]

    readonly property var trendWindows: ["1 Hour", "6 Hour", "12 Hour", "24 Hour", "72 Hour"]

    readonly property var trendPairs: [
        "Ppeak/PEEP", "ExpMinVol/MVSpont", "fTotal/fControl", "VDaw/VTE",
        "VTE/Vtalv", "SpO2/Oxygenand", "SpO2/FiO2"
    ]

    readonly property var loopPairs: [
        "Pressure / Volume", "Volume / PCO2",
        "Pressure / Flow",   "Volume / FCO2",
        "Volume / Flow",     "Pes / Volume",
        "Monitoring",        "Ptranspulm / Volume"
    ]

    readonly property var graphics: [
        "Dynamic Lung", "Vent Status", "ASV Graph", "Monitoring"
    ]

    readonly property var waveforms: [
        "Pressure", "PCO2", "Pes", "Flow", "FCO2", "Ptranspulm", "Volume"
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: Metrics.gutter

        SubTabStrip {
            Layout.fillWidth: true
            model: pane.pages
            currentIndex: pane.pageIndex
            onActivated: function (index) {
                pane.pageIndex = index
                pane.selection = ""
                pane.pairMenuOpen = false
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            spacing: Metrics.px(28)
            visible: pane.pageIndex === 0

            ColumnLayout {
                Layout.fillWidth: false
                Layout.preferredWidth: Math.max(Metrics.px(110), implicitWidth)
                Layout.alignment: Qt.AlignTop
                spacing: Spacing.md

                Repeater {
                    model: pane.trendWindows

                    delegate: ChipButton {
                        required property var modelData
                        Layout.fillWidth: true
                        text: modelData
                        checkable: true
                        checked: pane.trendWindow === modelData
                        onClicked: pane.trendWindow = modelData
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: false
                Layout.preferredWidth: Math.max(Metrics.px(170), implicitWidth)
                Layout.alignment: Qt.AlignTop
                spacing: Spacing.sm

                ChipButton {
                    Layout.fillWidth: true
                    text: pane.trendPair
                    onClicked: pane.pairMenuOpen = !pane.pairMenuOpen

                    AppIcon {
                        anchors.right: parent.right
                        anchors.rightMargin: Spacing.sm
                        anchors.verticalCenter: parent.verticalCenter
                        name: "chevron-down"
                        size: Metrics.px(16)
                        color: Colors.textInverse
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: menuColumn.implicitHeight + Spacing.sm * 2
                    visible: pane.pairMenuOpen
                    radius: Radius.xs
                    color: Colors.surfaceOverlay

                    ColumnLayout {
                        id: menuColumn
                        anchors.fill: parent
                        anchors.margins: Spacing.sm
                        spacing: Spacing.xs

                        Repeater {
                            model: pane.trendPairs

                            delegate: Text {
                                required property var modelData
                                Layout.fillWidth: true
                                text: modelData
                                color: pane.trendPair === modelData
                                       ? Colors.textPrimary : Colors.textSecondary
                                font.family: Typography.monoFamily
                                font.pixelSize: Typography.caption
                                horizontalAlignment: Text.AlignHCenter

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        pane.trendPair = modelData
                                        pane.pairMenuOpen = false
                                    }
                                }
                            }
                        }
                    }
                }

                AppButton {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Math.max(Metrics.px(78), implicitWidth)
                    Layout.preferredHeight: Metrics.px(28)
                    text: qsTr("Confirm")
                    buttonVariant: AppButton.Success
                    onClicked: pane.confirmed("trends",
                                              pane.trendWindow + " " + pane.trendPair)
                }
            }

            Item { Layout.fillWidth: true }
        }

        OptionGrid {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            visible: pane.pageIndex === 1
            columns: 2
            options: pane.loopPairs
            selection: pane.selection
            onChosen: function (option) {
                pane.selection = option
                pane.confirmed("loops", option)
            }
        }

        OptionGrid {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            visible: pane.pageIndex === 2
            columns: 1
            options: pane.graphics
            selection: pane.selection
            onChosen: function (option) {
                pane.selection = option
                pane.confirmed("graphics", option)
            }
        }

        OptionGrid {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            visible: pane.pageIndex === 3
            columns: 3
            options: pane.waveforms
            selection: pane.selection
            onChosen: function (option) {
                pane.selection = option
                pane.confirmed("waveforms", option)
            }
        }

        Item { Layout.fillHeight: true }
    }

    component OptionGrid: GridLayout {
        id: grid

        property var options: []
        property string selection: ""

        signal chosen(string option)

        rowSpacing: Spacing.md
        columnSpacing: Spacing.md

        Repeater {
            model: grid.options

            delegate: ChipButton {
                required property var modelData

                Layout.preferredWidth: Math.max(Metrics.px(130), implicitWidth)
                text: modelData
                checkable: true
                checked: grid.selection === modelData
                onClicked: grid.chosen(modelData)
            }
        }
    }
}
