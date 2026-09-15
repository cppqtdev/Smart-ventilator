// -----------------------------------------------------------------------
// File: LungPanel.qml
// Description: The breathing lung over the patient block and its readouts
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The lung breathes with the delivered breath rather than on a decorative
// loop, and rests when the ventilator is stopped, because a lung that
// breathes in standby tells the operator something that is not true.
//
import QtQuick
import QtQuick.Layouts
import "../../Components"
import "../../Theme"

Rectangle {
    id: panel

    property var presenter
    property bool frozen: false

    readonly property var patient: panel.presenter ? panel.presenter.patient : ({})
    readonly property var readouts: panel.presenter ? panel.presenter.readouts : []

    signal patientClicked()

    radius: Radius.medium
    color: Colors.surface

    // The readouts used to stack under the picture, which left the picture
    // a few pixels of a shared half-width panel. The reference sets them in
    // a column either side instead, so the picture keeps the middle.
    readonly property var leftReadouts: {
        var all = panel.readouts ? panel.readouts : []
        return all.slice(0, Math.ceil(all.length / 2))
    }
    readonly property var rightReadouts: {
        var all = panel.readouts ? panel.readouts : []
        return all.slice(Math.ceil(all.length / 2))
    }

    component ReadoutColumn: ColumnLayout {
        id: readoutColumn

        property var entries: []
        property int textAlignment: Text.AlignLeft

        Layout.fillHeight: true
        Layout.fillWidth: false
        Layout.preferredWidth: Metrics.px(62)
        spacing: Spacing.sm

        Repeater {
            model: readoutColumn.entries

            delegate: NumericReadout {
                required property var modelData

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                alignment: readoutColumn.textAlignment
                labelSize: Typography.readoutLabelCompact
                valueSize: Typography.readoutValueCompact
                unitSize: Typography.readoutUnitCompact

                label: modelData.label
                value: modelData.value
                unit: modelData.unit
                valueColor: modelData.accent === true ? Colors.accent : Colors.textPrimary
            }
        }

        Item { Layout.fillHeight: true }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.md
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

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Spacing.xs

            ReadoutColumn {
                entries: panel.leftReadouts
                textAlignment: Text.AlignLeft
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: Metrics.px(80)

                BreathingLung {
                    anchors.fill: parent
                    presenter: panel.presenter
                    frozen: panel.frozen
                }
            }

            ReadoutColumn {
                entries: panel.rightReadouts
                textAlignment: Text.AlignRight
            }
        }
    }
}
