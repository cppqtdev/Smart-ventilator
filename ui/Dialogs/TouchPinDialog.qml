// -----------------------------------------------------------------------
// File: TouchPinDialog.qml
// Description: Touch-first numeric PIN dialog for protected actions
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import "../Theme"
import "../Controls"
import "../Components"

Popup {
    id: root

    property string titleText: "Authorization Required"
    property string messageText: "Enter supervisor PIN to continue"
    property string enteredPin: ""
    property int pinLength: 4

    signal accepted(string pin)

    modal: true
    closePolicy: Popup.NoAutoClose
    width: Math.min(parent ? parent.width * 0.38 : 560, 560)
    height: pinPanel.implicitHeight
    anchors.centerIn: parent

    function appendDigit(digit) {
        if (root.enteredPin.length < root.pinLength)
            root.enteredPin += digit
    }

    function clearPin() {
        root.enteredPin = ""
    }

    function submit() {
        if (root.enteredPin.length === root.pinLength) {
            var pin = root.enteredPin
            root.clearPin()
            root.close()
            root.accepted(pin)
        }
    }

    onOpened: root.clearPin()

    background: Rectangle {
        radius: Radius.medium
        color: Colors.surface
        border.color: Colors.line
        border.width: 1
    }

    contentItem: Panel {
        id: pinPanel
        implicitHeight: pinContent.implicitHeight + 48

        ColumnLayout {
            id: pinContent
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 24
            spacing: 18

            Text {
                Layout.fillWidth: true
                text: root.titleText
                color: Colors.textPrimary
                font.pixelSize: Typography.subtitleLarge
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text: root.messageText
                color: Colors.textSecondary
                font.pixelSize: Typography.body
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            // Drawn rather than typeset: a glyph-based dot row inherits the
            // font's advance width and drifts out of alignment across the
            // fallback chain, and the filled/hollow pair is not reliably
            // present in every fallback face.
            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: Spacing.sm

                Repeater {
                    model: root.pinLength

                    Rectangle {
                        required property int index
                        width: Math.round(18 * Metrics.scale)
                        height: width
                        radius: width / 2
                        color: index < root.enteredPin.length
                            ? Colors.textPrimary : Colors.transparent
                        border.width: Metrics.borderWidth
                        border.color: index < root.enteredPin.length
                            ? Colors.textPrimary : Colors.border

                        Behavior on color {
                            ColorAnimation { duration: Metrics.durationFast }
                        }
                    }
                }
            }

            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                columns: 3
                rowSpacing: 10
                columnSpacing: 10

                Repeater {
                    model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "C", "0", "OK"]

                    PrimaryButton {
                        required property string modelData

                        Layout.preferredWidth: 104
                        Layout.preferredHeight: 64
                        text: modelData
                        buttonColor: {
                            if (modelData === "OK")
                                return root.enteredPin.length === root.pinLength ? Colors.success : Colors.disabled
                            if (modelData === "C")
                                return Colors.critical
                            return Colors.surfaceRaised
                        }
                        onClicked: {
                            if (modelData === "OK")
                                root.submit()
                            else if (modelData === "C")
                                root.clearPin()
                            else
                                root.appendDigit(modelData)
                        }
                    }
                }
            }

            PrimaryButton {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 220
                Layout.preferredHeight: 52
                text: "Cancel"
                buttonColor: Colors.disabled
                onClicked: {
                    root.clearPin()
                    root.close()
                }
            }
        }
    }
}
