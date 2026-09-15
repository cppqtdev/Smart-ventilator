// -----------------------------------------------------------------------
// File: ToolsPane.qml
// Description: Tools screen - P/V Tool, configuration, hold and utilities
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: pane

    property var ventilatorData
    property var alarmData
    property int pageIndex: 0
    property string cursor: ""

    signal inspiratoryHoldRequested()
    signal expiratoryHoldRequested()

    readonly property var pages: [
        { key: "pv",       label: qsTr("P/V Tools") },
        { key: "config",   label: qsTr("Configuration") },
        { key: "hold",     label: qsTr("Hold") },
        { key: "utility",  label: qsTr("Utilities") }
    ]

    // Readings arrive as doubles, so they are rounded here. Printing one
    // raw puts 15.200000000000001 on a clinical screen.
    function reading(key, decimals) {
        if (!pane.ventilatorData)
            return "---"
        var value = pane.ventilatorData[key]
        if (value === undefined || value === null)
            return "---"
        var places = decimals === undefined ? 1 : decimals
        return isNaN(value) ? String(value) : Number(value).toFixed(places)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Metrics.gutter

        SubTabStrip {
            Layout.fillWidth: true
            model: pane.pages
            currentIndex: pane.pageIndex
            onActivated: function (index) { pane.pageIndex = index }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Metrics.gutter
            visible: pane.pageIndex === 0

            ColumnLayout {
                id: toolColumn

                // Chips default to their label width as a minimum, which is
                // what lets a two chip row push past the column and draw
                // over the loop chart. Inside this column they are allowed
                // to shrink and elide instead.
                readonly property int chipMinimum: Metrics.px(72)

                Layout.fillWidth: false
                Layout.preferredWidth: Metrics.px(232)
                Layout.maximumWidth: Metrics.px(232)
                Layout.alignment: Qt.AlignTop
                spacing: Spacing.md

                ChipButton {
                    Layout.fillWidth: true
                    Layout.minimumWidth: toolColumn.chipMinimum
                    text: qsTr("Reference")
                    checkable: true
                }

                DialControl {
                    Layout.fillWidth: true
                    label: ""
                    unit: "cmH2O"
                    from: 0
                    to: 60
                    value: 0
                }

                Text {
                    text: qsTr("Compliance")
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.readoutLabel
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.sm

                    ChipButton {
                        Layout.fillWidth: true
                        Layout.minimumWidth: toolColumn.chipMinimum
                        labelPadding: Spacing.xs
                        text: qsTr("Cursor 1")
                        checkable: true
                        checked: pane.cursor === "1"
                        onClicked: pane.cursor = "1"
                    }

                    ChipButton {
                        Layout.fillWidth: true
                        Layout.minimumWidth: toolColumn.chipMinimum
                        labelPadding: Spacing.xs
                        text: qsTr("Cursor 2")
                        checkable: true
                        checked: pane.cursor === "2"
                        onClicked: pane.cursor = "2"
                    }
                }

                Text {
                    text: qsTr("Current Settings")
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.readoutLabel
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.lg

                    ColumnLayout {
                        spacing: 0
                        Text {
                            text: qsTr("Ptop")
                            color: Colors.textPrimary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutLabel
                        }
                        Text {
                            text: pane.reading("ppeak")
                            color: Colors.textPrimary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutValue
                            font.weight: Typography.bold
                        }
                        Text {
                            text: "cmH2O"
                            color: Colors.textPrimary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutUnit
                        }
                    }

                    ColumnLayout {
                        spacing: 0
                        Text {
                            text: qsTr("Pcuff")
                            color: Colors.textPrimary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutLabel
                        }
                        Text {
                            text: pane.reading("totalPeep")
                            color: Colors.textPrimary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutValue
                            font.weight: Typography.bold
                        }
                        Text {
                            text: "cmH2O"
                            color: Colors.textPrimary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutUnit
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.sm

                    ChipButton {
                        Layout.fillWidth: true
                        Layout.minimumWidth: toolColumn.chipMinimum
                        labelPadding: Spacing.xs
                        text: qsTr("Start/Stop")
                    }

                    ChipButton {
                        Layout.fillWidth: true
                        Layout.minimumWidth: toolColumn.chipMinimum
                        labelPadding: Spacing.xs
                        text: qsTr("Settings")
                    }
                }

                Item { Layout.fillHeight: true }
            }

            LoopChart {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: Metrics.px(240)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Spacing.lg
            visible: pane.pageIndex === 2

            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(170), implicitWidth)
                Layout.preferredHeight: Metrics.px(30)
                text: qsTr("Inspiratory hold")
                buttonVariant: AppButton.Primary
                enabled: pane.ventilatorData
                         && pane.ventilatorData.running
                         && !pane.ventilatorData.holdInProgress
                onClicked: pane.inspiratoryHoldRequested()
            }

            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(170), implicitWidth)
                Layout.preferredHeight: Metrics.px(30)
                text: qsTr("Expiratory hold")
                buttonVariant: AppButton.Primary
                enabled: pane.ventilatorData
                         && pane.ventilatorData.running
                         && !pane.ventilatorData.holdInProgress
                onClicked: pane.expiratoryHoldRequested()
            }

            Item { Layout.fillHeight: true }
        }

        UtilitiesPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: pane.pageIndex === 3
            ventilatorData: pane.ventilatorData
            alarmData: pane.alarmData
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: pane.pageIndex === 1

            Text {
                anchors.centerIn: parent
                text: qsTr("No entries.")
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.readoutLabel
            }
        }
    }
}
