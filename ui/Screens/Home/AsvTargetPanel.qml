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

    readonly property var target: panel.presenter ? panel.presenter.asvTarget : ({})

    // The rows were matched against the readout strip by label, and the
    // strip carries PEEPtot, fSpont and Cstat rather than Pinsp, Plateau
    // and fControl. Nothing matched, so every row read as dashes. The
    // presenter supplies the pairs directly now.
    readonly property var rows: {
        if (!panel.target)
            return []
        return panel.target.rows !== undefined ? panel.target.rows : []
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
            Layout.minimumWidth: Metrics.px(120)
            target: panel.target
            color: Colors.transparent
        }

        // reference/layout-dual-graphics.png draws this column at roughly a
        // third of the width it had, which is what left the graph beside it
        // too narrow to read. Label, the pair, then the unit, the way the
        // reference stacks them.
        ColumnLayout {
            Layout.fillWidth: false
            Layout.fillHeight: true
            Layout.preferredWidth: Metrics.px(70)
            spacing: Spacing.xs

            RowLayout {
                Layout.fillWidth: true
                spacing: Spacing.xs

                Text {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.round(Typography.readoutUnitCompact * 1.4)
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("Target")
                    color: Colors.textSecondary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.readoutUnitCompact
                }

                Text {
                    Layout.preferredHeight: Math.round(Typography.readoutUnitCompact * 1.4)
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("Current")
                    color: Colors.textSecondary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.readoutUnitCompact
                }
            }

            Repeater {
                model: panel.rows

                delegate: ColumnLayout {
                    id: row

                    required property var modelData

                    readonly property int labelHeight: Math.round(Typography.readoutLabelCompact * 1.4)
                    readonly property int valueHeight: Math.round(Typography.readoutValueCompact * 1.25)
                    readonly property int unitHeight: Math.round(Typography.readoutUnitCompact * 1.4)

                    Layout.fillWidth: true
                    spacing: 0

                    Text {
                        Layout.fillWidth: true
                        Layout.preferredHeight: row.labelHeight
                        verticalAlignment: Text.AlignVCenter
                        text: row.modelData.label
                        color: Colors.textPrimary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.readoutLabelCompact
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Spacing.xs

                        Text {
                            Layout.fillWidth: true
                            Layout.preferredHeight: row.valueHeight
                            verticalAlignment: Text.AlignVCenter
                            text: panel.show(row.modelData.target)
                            color: Colors.textSecondary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutValueCompact
                        }

                        Text {
                            Layout.preferredHeight: row.valueHeight
                            verticalAlignment: Text.AlignVCenter
                            text: panel.show(row.modelData.current)
                            color: Colors.textPrimary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutValueCompact
                            font.weight: Typography.bold
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.preferredHeight: row.unitHeight
                        verticalAlignment: Text.AlignVCenter
                        text: row.modelData.unit
                        color: Colors.textSecondary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.readoutUnitCompact
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
