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

    readonly property var target: panel.presenter ? panel.presenter.asvTarget : ({})

    // The rows were matched against the readout strip by label, and the
    // strip carries PEEPtot, fSpont and Cstat rather than Pinsp, Plateau
    // and fControl. Nothing matched, so every row read as dashes. The
    // presenter supplies the pairs directly now.
    readonly property var rows: {
        if (!panel.target)
            return []
        var key = panel.showSettings ? "settingRows" : "rows"
        return panel.target[key] !== undefined ? panel.target[key] : []
    }

    function show(value) {
        if (value === undefined || value === null || isNaN(value))
            return "---"
        var number = Number(value)
        return Number.isInteger(number) ? String(number) : number.toFixed(1)
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
            Layout.preferredWidth: Metrics.px(126)
            spacing: Spacing.xs

            RowLayout {
                Layout.fillWidth: true
                spacing: Spacing.sm

                Text {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.round(Typography.micro * 1.3)
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("Target")
                    color: Colors.textSecondary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.micro
                }

                Text {
                    Layout.preferredHeight: Math.round(Typography.micro * 1.3)
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("Current")
                    color: Colors.textSecondary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.micro
                }
            }

            // Five setting rows at three stacked lines each overran the
            // panel. The unit shares the label line now, so a row costs two
            // lines, and each line is measured off its glyph size rather
            // than the line box Text reserves for itself.
            Repeater {
                model: panel.rows

                delegate: ColumnLayout {
                    id: row

                    required property var modelData

                    readonly property int captionHeight: Math.round(Typography.micro * 1.3)
                    readonly property int valueHeight: Math.round(Typography.readoutLabel * 1.25)

                    Layout.fillWidth: true
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Spacing.xs

                        Text {
                            Layout.fillWidth: true
                            Layout.preferredHeight: row.captionHeight
                            verticalAlignment: Text.AlignVCenter
                            text: row.modelData.label
                            color: Colors.textSecondary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.micro
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.preferredHeight: row.captionHeight
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                            text: row.modelData.unit
                            color: Colors.textSecondary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.micro
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Spacing.sm

                        Text {
                            Layout.fillWidth: true
                            Layout.preferredHeight: row.valueHeight
                            verticalAlignment: Text.AlignVCenter
                            text: panel.show(row.modelData.target)
                            color: Colors.textSecondary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutLabel
                        }

                        Text {
                            Layout.preferredHeight: row.valueHeight
                            verticalAlignment: Text.AlignVCenter
                            text: panel.show(row.modelData.current)
                            color: Colors.textPrimary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutLabel
                            font.weight: Typography.bold
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
