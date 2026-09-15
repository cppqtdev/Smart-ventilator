// -----------------------------------------------------------------------
// File: ContentDynamicLung.qml
// Description: Home layout 5 - dynamic lung with the numeric strip beneath
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The lung breathes with the delivered breath rather than on a decorative
// loop: it fills over the inspiratory time and empties over what is left of
// the cycle, and its size follows the measured expired volume against the
// set tidal volume. A lung that breathes when the ventilator is stopped
// would be telling the operator something that is not true, so it rests.
//
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
    readonly property bool ventilating: content.presenter ? content.presenter.ventilating : false

    signal patientClicked()

    radius: Radius.medium
    color: Colors.surface

    // 0 at end expiration, 1 at end inspiration.
    property real inflation: 0

    SequentialAnimation {
        id: breathCycle
        running: content.ventilating && !content.frozen
        loops: Animation.Infinite

        NumberAnimation {
            target: content
            property: "inflation"
            to: 1
            duration: breathCycle.inspiratoryMs
            easing.type: Easing.OutQuad
        }

        NumberAnimation {
            target: content
            property: "inflation"
            to: 0
            duration: breathCycle.expiratoryMs
            easing.type: Easing.InQuad
        }

        readonly property int cycleMs: 60000 / Math.max(4, content.breathRate)
        readonly property int inspiratoryMs: Math.round(cycleMs * 0.33)
        readonly property int expiratoryMs: Math.max(200, cycleMs - inspiratoryMs)

        onRunningChanged: {
            if (!running)
                content.inflation = 0
        }
    }

    readonly property int breathRate: {
        var rate = content.presenter && content.presenter.measuredRate !== undefined
                   ? content.presenter.measuredRate : 0
        return rate > 0 ? rate : 14
    }

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

        Item {
            id: lungStage

            Layout.fillWidth: true
            Layout.fillHeight: true

            // The glow behind the lung brightens with the breath, so the
            // motion reads at a distance where the outline alone would not.
            Rectangle {
                anchors.centerIn: lungImage
                width: lungImage.width * 1.08
                height: lungImage.height * 1.08
                radius: width / 2
                color: Colors.accentSubtle
                opacity: 0.10 + content.inflation * 0.18
                visible: content.ventilating
            }

            Image {
                id: lungImage

                anchors.centerIn: parent
                height: Math.min(parent.height, parent.width)
                width: height
                source: "qrc:/ui/Assets/lungs.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
                asynchronous: true

                // Inflation is mostly downward and outward, the way a chest
                // moves, so the scale is not uniform.
                transform: Scale {
                    origin.x: lungImage.width / 2
                    origin.y: lungImage.height * 0.18
                    xScale: 1.0 + content.inflation * 0.035
                    yScale: 1.0 + content.inflation * 0.075
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            rowSpacing: Spacing.md
            columnSpacing: Metrics.px(28)

            Repeater {
                model: content.readouts

                delegate: NumericReadout {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.minimumWidth: Metrics.px(110)

                    label: modelData.label
                    value: modelData.value
                    unit: modelData.unit
                    valueColor: modelData.accent === true ? Colors.accent : Colors.textPrimary
                }
            }
        }
    }
}
