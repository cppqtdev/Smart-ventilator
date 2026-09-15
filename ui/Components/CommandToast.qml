// -----------------------------------------------------------------------
// File: CommandToast.qml
// Description: Transient banner for an accepted or refused command
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// A refused command used to leave the screen unchanged, which reads as a dead
// button. Every rejection now says what it was and why.
//
// The bar sizes itself to the sentence it carries and centres over the
// content. A full-width slab reads as an alarm, which this is not.
//
import QtQuick
import QtQuick.Layouts
import "../Controls"
import "../Theme"

Item {
    id: toast

    property int visibleMs: 5000

    property string message: ""
    property bool rejection: false

    readonly property int barHeight: Metrics.px(44)

    function show(text, refused) {
        if (!text || String(text).length === 0)
            return
        toast.message = String(text)
        toast.rejection = refused === true
        body.opacity = 1
        hideTimer.restart()
    }

    implicitHeight: toast.barHeight
    visible: body.opacity > 0

    Rectangle {
        id: body

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom

        width: Math.min(row.implicitWidth + Spacing.xl + Spacing.sm, toast.width)
        height: toast.barHeight
        radius: Radius.large
        opacity: 0
        color: toast.rejection ? Colors.alarmHigh : Colors.surfaceRaised
        border.width: Metrics.borderWidth
        border.color: toast.rejection ? Colors.danger : Colors.line

        Behavior on opacity {
            NumberAnimation { duration: Metrics.durationNormal }
        }

        RowLayout {
            id: row

            anchors.fill: parent
            anchors.leftMargin: Spacing.lg
            anchors.rightMargin: Spacing.sm
            spacing: Spacing.sm

            AppIcon {
                Layout.alignment: Qt.AlignVCenter
                name: toast.rejection ? "alarm-high" : "info"
                size: Metrics.px(18)
                color: Colors.textPrimary
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: toast.message
                color: Colors.textPrimary
                font.family: Typography.family
                font.pixelSize: Typography.label
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                id: dismiss

                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: Metrics.px(30)
                Layout.preferredHeight: Metrics.px(30)
                Layout.fillWidth: false
                Layout.fillHeight: false
                radius: Radius.small
                color: dismissArea.pressed ? Colors.line
                     : (dismissArea.containsMouse ? Colors.surfaceOverlay : Colors.transparent)

                AppIcon {
                    anchors.centerIn: parent
                    name: "close"
                    size: Metrics.px(14)
                    color: Colors.textPrimary
                }

                MouseArea {
                    id: dismissArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        hideTimer.stop()
                        body.opacity = 0
                    }
                }

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Dismiss")
                Accessible.onPressAction: {
                    hideTimer.stop()
                    body.opacity = 0
                }
            }
        }
    }

    Timer {
        id: hideTimer
        interval: toast.visibleMs
        onTriggered: body.opacity = 0
    }
}
