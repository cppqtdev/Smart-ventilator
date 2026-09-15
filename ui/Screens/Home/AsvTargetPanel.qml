// -----------------------------------------------------------------------
// File: AsvTargetPanel.qml
// Description: The adaptive target graph beside the numbers it was set from
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Rectangle {
    id: panel

    property var presenter
    property bool showSettings: false

    readonly property var readouts: panel.presenter ? panel.presenter.readouts : []
    readonly property var target: panel.presenter ? panel.presenter.asvTarget : ({})

    function pick(label) {
        for (var i = 0; i < panel.readouts.length; ++i) {
            if (panel.readouts[i].label === label)
                return panel.readouts[i]
        }
        return null
    }

    radius: Radius.medium
    color: Colors.surface

    RowLayout {
        anchors.fill: parent
        anchors.margins: Spacing.md
        spacing: Spacing.md

        AsvTargetChart {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: Metrics.px(150)
            target: panel.target
            color: Colors.transparent
        }

        ColumnLayout {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.preferredWidth: Metrics.px(104)
            spacing: Spacing.sm

            RowLayout {
                Layout.fillWidth: true
                spacing: Spacing.sm

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Target")
                    color: Colors.textSecondary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.micro
                }

                Text {
                    text: qsTr("Current")
                    color: Colors.textSecondary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.micro
                }
            }

            Repeater {
                model: panel.showSettings
                       ? ["Pinsp", "Plateau", "Pmean", "PEEP/CPAP"]
                       : ["Pinsp", "fControl", "SpO2"]

                delegate: ColumnLayout {
                    id: row

                    required property var modelData
                    readonly property var entry: panel.pick(row.modelData)

                    Layout.fillWidth: true
                    spacing: 0

                    Text {
                        text: row.modelData
                        color: Colors.textSecondary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.micro
                    }

                    Text {
                        text: row.entry ? row.entry.value : "---"
                        color: Colors.textPrimary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.readoutLabel
                        font.weight: Typography.bold
                    }

                    Text {
                        text: row.entry && row.entry.unit !== undefined
                              ? row.entry.unit : ""
                        color: Colors.textSecondary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.micro
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
