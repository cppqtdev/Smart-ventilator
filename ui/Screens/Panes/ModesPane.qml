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

    // The reference names the modes the way the device is badged, which is
    // not what the controller calls them. Each chip therefore carries the
    // controller mode it stands for; a chip with no mode is one this device
    // is not fitted with and is drawn greyed, exactly as the reference draws
    // the noninvasive row.
    readonly property var groups: [
        {
            title: qsTr("Volume controlled (Adaptive)"),
            modes: [
                { label: "APVcmv",  key: "PRVC" },
                { label: "APVsimv", key: "" },
                { label: "(S)CMV",  key: "VCV" },
                { label: "SIMV",    key: "SIMV" }
            ]
        },
        {
            title: qsTr("Pressure controlled (Biphasic)"),
            modes: [
                { label: "PCV+",    key: "PCV" },
                { label: "PSIMV+",  key: "" },
                { label: "SPONT",   key: "PSV" },
                { label: "CPAP",    key: "CPAP" },
                { label: "DuoPAP",  key: "BiPAP" },
                { label: "APRV",    key: "" }
            ]
        },
        {
            title: qsTr("Intelligent Ventilation"),
            modes: [
                { label: "ASV",             key: "ASV" },
                { label: "INTELLiVENT-ASV", key: "" }
            ]
        },
        {
            title: qsTr("Noninvasive"),
            modes: [
                { label: "NIV",      key: "" },
                { label: "NIV-ST",   key: "" },
                { label: "HiFlowO2", key: "" }
            ]
        }
    ]

    // A mode the controller will refuse is shown greyed rather than hidden,
    // so the operator can see the device has the row but not the option.
    function isAvailable(key) {
        if (!key || !pane.ventilatorData)
            return false
        return pane.ventilatorData.isModeSupported(key)
    }

    function describe(key) {
        if (!key || !pane.catalog)
            return ""
        var info = pane.catalog.modeInfo(key)
        if (info === undefined || info === null || info.label === undefined)
            return ""
        return info.label
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

                            width: Math.max(Metrics.px(96), implicitWidth)
                            labelPadding: Spacing.md
                            text: modelData.label
                            enabled: pane.isAvailable(modelData.key)
                            selected: modelData.key.length > 0
                                     && pane.pendingMode === modelData.key
                            onClicked: pane.pendingMode = modelData.key

                            Accessible.description: pane.describe(modelData.key)
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
