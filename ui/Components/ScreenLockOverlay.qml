// -----------------------------------------------------------------------
// File: ScreenLockOverlay.qml
// Description: Inactivity-triggered screen lock overlay for clinical security
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Controls.Basic

import "../Theme"
import "../Controls"

Rectangle {
    id: root

    property int timeoutSeconds: 300
    property bool locked: false

    /** UserController. Without one the overlay falls back to a plain button. */
    property var users
    property string userName: "clinician"

    property string entry: ""
    property string message: ""

    function release() {
        root.entry = ""
        root.message = ""
        root.locked = false
        inactivityTimer.restart()
        root.unlocked()
    }

    // A device with no operator account must not lock anyone out of a running
    // ventilator. When none is configured the overlay says so and opens on a
    // press, and the missing account is what gets fixed, not the lock.
    readonly property bool credentialsConfigured:
        root.users !== undefined && root.users !== null && root.users.accountCount > 0

    function press(symbol) {
        if (symbol === "C") {
            root.entry = ""
            root.message = ""
            return
        }
        if (symbol === "OK") {
            if (!root.users)
                return
            if (root.users.login(root.userName, root.entry)) {
                root.release()
            } else {
                root.entry = ""
                root.message = root.users.lastLoginError.length > 0
                               ? root.users.lastLoginError
                               : qsTr("Wrong number")
            }
            return
        }
        if (root.entry.length < 8)
            root.entry += symbol
        root.message = ""
    }

    onLockedChanged: {
        if (root.locked) {
            root.entry = ""
            root.message = ""
        }
    }

    signal unlocked()

    color: "#CC000000"
    visible: root.locked
    z: 1000

    // Inactivity timer that triggers the lock.
    Timer {
        id: inactivityTimer
        interval: root.timeoutSeconds * 1000
        running: !root.locked
        repeat: false
        onTriggered: root.locked = true
    }

    function resetTimer() {
        if (!root.locked)
            inactivityTimer.restart()
    }

    /** @brief Locks now, without waiting for the inactivity timer. */
    function lockNow() {
        inactivityTimer.stop()
        root.locked = true
    }

    // Blocks every button and the wheel while locked, with no composed event
    // propagation, so nothing reaches the controls underneath.
    MouseArea {
        anchors.fill: parent
        enabled: root.locked
        acceptedButtons: Qt.AllButtons
        preventStealing: true
        onClicked: function(mouse) { mouse.accepted = true }
        onPressed: function(mouse) { mouse.accepted = true }
        onReleased: function(mouse) { mouse.accepted = true }
        onWheel: function(wheel) { wheel.accepted = true }
    }

    Column {
        anchors.centerIn: parent
        spacing: Spacing.xxl

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Screen Locked")
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.headline
            font.weight: Typography.semibold
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.credentialsConfigured
                  ? qsTr("Enter the personal identification number for %1").arg(root.userName)
                  : qsTr("No operator account is configured on this device")
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.body
            horizontalAlignment: Text.AlignHCenter
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Spacing.md
            visible: root.credentialsConfigured

            Repeater {
                model: 4

                delegate: Rectangle {
                    required property int index

                    width: Metrics.px(22)
                    height: width
                    radius: width / 2
                    color: index < root.entry.length ? Colors.accent : Colors.transparent
                    border.width: Metrics.borderWidth
                    border.color: Colors.line
                }
            }
        }

        Grid {
            anchors.horizontalCenter: parent.horizontalCenter
            columns: 3
            spacing: Spacing.md
            visible: root.credentialsConfigured

            Repeater {
                model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "C", "0", "OK"]

                delegate: AppButton {
                    required property var modelData

                    implicitWidth: Metrics.px(84)
                    implicitHeight: Metrics.px(56)
                    fontSize: Typography.bodyLarge
                    text: modelData
                    buttonVariant: modelData === "OK" ? AppButton.Success
                                 : modelData === "C" ? AppButton.Neutral
                                 : AppButton.Secondary
                    onClicked: root.press(modelData)
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.message
            color: Colors.alarmHigh
            font.family: Typography.monoFamily
            font.pixelSize: Typography.caption
            horizontalAlignment: Text.AlignHCenter
            visible: root.message.length > 0
        }

        AppButton {
            anchors.horizontalCenter: parent.horizontalCenter
            implicitWidth: Metrics.px(240)
            implicitHeight: Metrics.px(64)
            text: qsTr("Unlock")
            buttonVariant: AppButton.Primary
            visible: !root.credentialsConfigured
            onClicked: root.release()
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Locks after %1 seconds without use").arg(root.timeoutSeconds)
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.caption
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
