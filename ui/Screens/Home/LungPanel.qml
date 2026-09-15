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
    readonly property bool ventilating: panel.presenter ? panel.presenter.ventilating : false

    signal patientClicked()

    radius: Radius.medium
    color: Colors.surface

    // 0 at end expiration, 1 at end inspiration.
    property real inflation: 0

    readonly property int breathRate: {
        var rate = panel.presenter && panel.presenter.measuredRate !== undefined
                   ? panel.presenter.measuredRate : 0
        return rate > 0 ? rate : 14
    }

    SequentialAnimation {
        id: breathCycle
        running: panel.ventilating && !panel.frozen
        loops: Animation.Infinite

        NumberAnimation {
            target: panel; property: "inflation"; to: 1
            duration: breathCycle.inspiratoryMs; easing.type: Easing.OutQuad
        }

        NumberAnimation {
            target: panel; property: "inflation"; to: 0
            duration: breathCycle.expiratoryMs; easing.type: Easing.InQuad
        }

        readonly property int cycleMs: 60000 / Math.max(4, panel.breathRate)
        readonly property int inspiratoryMs: Math.round(cycleMs * 0.33)
        readonly property int expiratoryMs: Math.max(200, cycleMs - inspiratoryMs)

        onRunningChanged: { if (!running) panel.inflation = 0 }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.md
        spacing: Spacing.sm

        PatientSummary {
            Layout.alignment: Qt.AlignTop
            gender: panel.patient.gender !== undefined ? panel.patient.gender : ""
            heightText: panel.patient.height !== undefined
                        ? qsTr("%1 cm").arg(panel.patient.height) : ""
            weightText: panel.patient.ibw !== undefined
                        ? qsTr("IBW: %1 kg").arg(panel.patient.ibw) : ""
            onClicked: panel.patientClicked()
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Image {
                id: lungImage
                anchors.centerIn: parent
                height: Math.min(parent.height, parent.width)
                width: height
                source: "qrc:/ui/Assets/lungs.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
                asynchronous: true

                transform: Scale {
                    origin.x: lungImage.width / 2
                    origin.y: lungImage.height * 0.18
                    xScale: 1.0 + panel.inflation * 0.035
                    yScale: 1.0 + panel.inflation * 0.075
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            rowSpacing: Spacing.sm
            columnSpacing: Spacing.md

            Repeater {
                model: panel.readouts

                delegate: NumericReadout {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.minimumWidth: Metrics.px(78)

                    label: modelData.label
                    value: modelData.value
                    unit: modelData.unit
                    valueColor: modelData.accent === true ? Colors.accent : Colors.textPrimary
                }
            }
        }
    }
}
