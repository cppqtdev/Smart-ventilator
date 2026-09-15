// -----------------------------------------------------------------------
// File: AsvNumericsPanel.qml
// Description: The patient block over the settings the target was read from
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// reference/layout-dual-graphics.png puts this beside the target graph
// rather than inside it: the patient at the top, then the derived pressures
// in two columns with the number on the left and its name beside it.
//
import QtQuick
import QtQuick.Layouts
import "../../Components"
import "../../Theme"

Rectangle {
    id: panel

    property var presenter

    readonly property var patient: panel.presenter ? panel.presenter.patient : ({})
    readonly property var target: panel.presenter ? panel.presenter.asvTarget : ({})

    readonly property var rows: {
        if (!panel.target)
            return []
        return panel.target.settingRows !== undefined ? panel.target.settingRows : []
    }

    // Plethysmographic variability is not measured without a plethysmograph.
    readonly property string pleth: "-------"

    signal patientClicked()

    function show(value) {
        if (value === undefined || value === null || isNaN(value))
            return "---"
        var number = Number(value)
        return Number.isInteger(number) ? String(number) : number.toFixed(1)
    }

    radius: Radius.medium
    color: Colors.surface

    component SettingCell: RowLayout {
        id: cell

        property string valueText: "---"
        property string labelText: ""
        property string unitText: ""
        property color valueColor: Colors.textPrimary

        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Spacing.sm

        Text {
            Layout.preferredWidth: Metrics.px(44)
            Layout.alignment: Qt.AlignVCenter
            text: cell.valueText
            color: cell.valueColor
            font.family: Typography.monoFamily
            font.pixelSize: Typography.readoutValueCompact
            font.weight: Typography.bold
            horizontalAlignment: Text.AlignLeft
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 0

            Text {
                Layout.fillWidth: true
                text: cell.labelText
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.readoutLabelCompact
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: cell.unitText
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.readoutUnitCompact
                elide: Text.ElideRight
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.md
        spacing: Spacing.md

        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.sm

            PatientSummary {
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                fontSize: Typography.readoutLabelCompact
                gender: panel.patient.gender !== undefined ? panel.patient.gender : ""
                heightText: panel.patient.height !== undefined
                            ? qsTr("%1 cm").arg(panel.patient.height) : ""
                weightText: panel.patient.ibw !== undefined
                            ? qsTr("IBW: %1 kg").arg(panel.patient.ibw) : ""
                onClicked: panel.patientClicked()
            }

            Item { Layout.fillWidth: true }

            NumericReadout {
                Layout.alignment: Qt.AlignTop
                alignment: Text.AlignRight
                labelSize: Typography.readoutLabelCompact
                valueSize: Typography.readoutValueCompact
                unitSize: Typography.readoutUnitCompact
                label: qsTr("PVI")
                value: panel.pleth
                unit: "%"
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 2
            rowSpacing: Spacing.md
            columnSpacing: Spacing.md

            Repeater {
                model: panel.rows

                delegate: SettingCell {
                    required property var modelData

                    valueText: panel.show(modelData.current)
                    labelText: modelData.label
                    unitText: modelData.unit
                }
            }
        }
    }
}
