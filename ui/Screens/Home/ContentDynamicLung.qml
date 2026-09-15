// -----------------------------------------------------------------------
// File: ContentDynamicLung.qml
// Description: Home layout 5 - dynamic lung with the numeric strip beneath
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Components"
import "../../Theme"

Rectangle {
    id: content

    property var presenter
    property bool frozen: false

    readonly property var readouts: content.presenter ? content.presenter.readouts : []
    readonly property var patient: content.presenter ? content.presenter.patient : ({})

    signal patientClicked()

    radius: Radius.medium
    color: Colors.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.lg
        spacing: Spacing.lg

        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.lg

            PatientSummary {
                Layout.alignment: Qt.AlignTop
                gender: content.patient.gender !== undefined ? content.patient.gender : ""
                heightText: content.patient.height !== undefined
                        ? qsTr("%1 cm").arg(content.patient.height) : ""
                weightText: content.patient.ibw !== undefined
                        ? qsTr("IBW: %1 kg").arg(content.patient.ibw) : ""
                onClicked: content.patientClicked()
            }

            Item { Layout.fillWidth: true }

            Text {
                Layout.alignment: Qt.AlignTop
                text: qsTr("PVI")
                color: Colors.textSecondary
                font.family: Typography.family
                font.pixelSize: Typography.label
            }
        }

        Image {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/ui/Assets/lungs.png"
            fillMode: Image.PreserveAspectFit
            smooth: true
            asynchronous: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.xxl

            Repeater {
                model: content.readouts

                delegate: NumericReadout {
                    required property var modelData

                    Layout.fillWidth: true
                    label: modelData.label
                    value: modelData.value
                    unit: modelData.unit
                    valueColor: modelData.accent === true ? Colors.accent : Colors.textPrimary
                }
            }
        }
    }
}
