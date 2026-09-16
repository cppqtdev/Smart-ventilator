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
    padding: 0
    width: Math.min(parent ? parent.width * 0.38 : Metrics.px(420), Metrics.px(420))
    height: shell.implicitHeight
    anchors.centerIn: parent

    // The screen behind a modal is not available, and has to look it.
    Overlay.modal: Rectangle { color: Colors.scrim }

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

    background: null

    contentItem: DialogShell {
        id: shell

        width: root.width
        titleText: root.titleText
        messageText: root.messageText
        hasBody: true

        ColumnLayout {
            id: pinContent
            width: parent.width
            spacing: Spacing.lg

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
                rowSpacing: Spacing.sm
                columnSpacing: Spacing.sm

                Repeater {
                    model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "C", "0", "OK"]

                    AppButton {
                        required property string modelData

                        Layout.preferredWidth: Metrics.px(96)
                        Layout.preferredHeight: Metrics.buttonHeightPrimary
                        text: modelData
                        fontSize: Typography.bodyLarge
                        enabled: modelData !== "OK"
                                 || root.enteredPin.length === root.pinLength
                        buttonVariant: {
                            if (modelData === "OK")
                                return AppButton.Success
                            if (modelData === "C")
                                return AppButton.Danger
                            return AppButton.Secondary
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

        }

        actions: [
            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(132), implicitWidth)
                Layout.preferredHeight: Metrics.buttonHeightPrimary
                text: qsTr("Cancel")
                buttonVariant: AppButton.Ghost
                onClicked: {
                    root.clearPin()
                    root.close()
                }
            },
            Item { Layout.fillWidth: true }
        ]
    }
}
