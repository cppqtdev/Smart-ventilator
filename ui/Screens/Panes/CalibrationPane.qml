// -----------------------------------------------------------------------
// File: CalibrationPane.qml
// Description: System > Tests and Calibration - runs the device procedures
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: pane

    property var service

    readonly property bool busy: pane.service ? pane.service.busy : false

    // CalibrationService::State. The service is a context property, so the
    // enum is not a QML type and the values are matched by number.
    readonly property int stateRunning: 1
    readonly property int statePassed: 2
    readonly property int stateFailed: 3
    readonly property int stateAborted: 4

    function colourForState(state) {
        switch (state) {
        case pane.statePassed:  return Colors.success
        case pane.stateFailed:  return Colors.alarmHigh
        case pane.stateAborted: return Colors.warning
        case pane.stateRunning: return Colors.accent
        default:                return Colors.textSecondary
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Spacing.md

        Repeater {
            model: pane.service

            delegate: RowLayout {
                required property string key
                required property string name
                required property int state
                required property string stateName
                required property string stamp
                required property string message
                required property bool requiresStandby

                Layout.fillWidth: true
                spacing: Spacing.lg

                ChipButton {
                    Layout.preferredWidth: Math.max(Metrics.px(150), implicitWidth)
                    text: name
                    enabled: !pane.busy
                    onClicked: {
                        if (pane.service)
                            pane.service.start(key)
                    }
                }

                AppCheckBox {
                    Layout.alignment: Qt.AlignVCenter
                    checked: state === pane.statePassed
                    enabled: false
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Text {
                        Layout.fillWidth: true
                        text: stamp.length > 0 ? stamp : qsTr("Not run")
                        color: Colors.textPrimary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.caption
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: message
                        color: pane.colourForState(state)
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.micro
                        elide: Text.ElideRight
                        visible: message.length > 0
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: stateName
                    color: pane.colourForState(state)
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.caption
                }

                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: qsTr("standby only")
                    color: Colors.textSecondary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.micro
                    visible: requiresStandby
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Spacing.md
            spacing: Spacing.md

            AppButton {
                Layout.preferredHeight: Metrics.actionButtonHeight
                text: qsTr("Run all")
                buttonVariant: AppButton.Success
                enabled: !pane.busy
                onClicked: {
                    if (pane.service)
                        pane.service.startAll()
                }
            }

            AppButton {
                Layout.preferredHeight: Metrics.actionButtonHeight
                text: qsTr("Stop")
                buttonVariant: AppButton.Danger
                enabled: pane.busy
                onClicked: {
                    if (pane.service)
                        pane.service.cancel()
                }
            }

            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.px(56)
            radius: Radius.small
            color: Colors.surfaceRaised
            visible: pane.busy

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Spacing.md
                spacing: Spacing.sm

                Text {
                    Layout.fillWidth: true
                    text: pane.service
                          ? qsTr("%1 - %2").arg(pane.service.activeName).arg(pane.service.statusText)
                          : ""
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.caption
                    elide: Text.ElideRight
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Metrics.px(8)
                    radius: Radius.xs
                    color: Colors.controlTrack

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: parent.width * (pane.service ? pane.service.progress / 100 : 0)
                        radius: parent.radius
                        color: Colors.accent
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: pane.service ? pane.service.lastRejection : ""
            color: Colors.warning
            font.family: Typography.monoFamily
            font.pixelSize: Typography.caption
            wrapMode: Text.WordWrap
            visible: !pane.busy && text.length > 0
        }

        Item { Layout.fillHeight: true }
    }
}
