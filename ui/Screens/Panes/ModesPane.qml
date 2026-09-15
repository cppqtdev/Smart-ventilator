// -----------------------------------------------------------------------
// File: ModesPane.qml
// Description: Modes screen - the mode grid grouped by control variable
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: pane

    property var ventilatorData
    property var catalog
    property string pendingMode: pane.ventilatorData ? pane.ventilatorData.mode : ""

    signal modeConfirmed(string mode)
    signal cancelled()

    readonly property var groups: [
        {
            title: qsTr("Volume controlled"),
            modes: ["VCV", "PRVC", "SIMV"]
        },
        {
            title: qsTr("Pressure controlled"),
            modes: ["PCV", "PSV", "BiPAP", "APRV"]
        },
        {
            title: qsTr("Spontaneous"),
            modes: ["CPAP"]
        },
        {
            title: qsTr("Intelligent ventilation"),
            modes: ["ASV"]
        },
        {
            title: qsTr("Noninvasive"),
            modes: ["NIV", "NIV-ST"]
        },
        {
            title: qsTr("High frequency"),
            modes: ["HFOV", "HFO2"]
        }
    ]

    function labelFor(mode) {
        if (!pane.catalog)
            return mode
        var info = pane.catalog.modeInfo(mode)
        if (info === undefined || info === null || info.label === undefined)
            return mode
        return info.label
    }

    // A mode the controller will refuse is shown greyed rather than hidden,
    // so the operator can see the device has the row but not the option.
    function isAvailable(mode) {
        if (!pane.ventilatorData)
            return false
        return pane.ventilatorData.isModeSupported(mode)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Spacing.lg

        Repeater {
            model: pane.groups

            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: Spacing.sm

                Text {
                    text: modelData.title
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.readoutLabel
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: Spacing.md

                    Repeater {
                        model: modelData.modes

                        delegate: ChipButton {
                            required property var modelData

                            width: Math.max(Metrics.px(78), implicitWidth)
                            labelPadding: Spacing.md
                            text: pane.labelFor(modelData)
                            enabled: pane.isAvailable(modelData)
                            checkable: true
                            checked: pane.pendingMode === modelData
                            onClicked: pane.pendingMode = modelData
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Spacing.md

            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(78), implicitWidth)
                Layout.preferredHeight: Metrics.actionButtonHeight
                text: qsTr("Cancel")
                buttonVariant: AppButton.Success
                onClicked: pane.cancelled()
            }

            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(78), implicitWidth)
                Layout.preferredHeight: Metrics.actionButtonHeight
                text: qsTr("Confirm")
                buttonVariant: AppButton.Success
                enabled: pane.pendingMode.length > 0
                onClicked: pane.modeConfirmed(pane.pendingMode)
            }
        }
    }
}
