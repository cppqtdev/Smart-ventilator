// -----------------------------------------------------------------------
// File: ConfirmDialog.qml
// Description: Touch confirmation for an action that cannot be undone
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The panel has no pointer and no keyboard, so a single tap is the whole
// interaction - there is no hover to warn the clinician what a control is
// about to do and no Escape to back out of it. Anything that discards
// state therefore asks first, in the same shell TouchPinDialog wears.
//
// Confirm sits on the right and carries the danger tint by default, so the
// destructive answer is never the one a thumb finds by habit.
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import "../Theme"
import "../Controls"
import "../Components"

Popup {
    id: root

    property string titleText: qsTr("Confirm")
    property string messageText: ""
    property string confirmText: qsTr("Confirm")
    property string cancelText: qsTr("Cancel")

    /** Confirm reads as the dangerous answer unless the caller says otherwise. */
    property string confirmVariant: "danger"

    signal confirmed()

    modal: true
    closePolicy: Popup.NoAutoClose
    padding: 0
    width: Math.min(parent ? parent.width * 0.38 : Metrics.px(420), Metrics.px(420))
    height: shell.implicitHeight
    anchors.centerIn: parent

    // The screen behind a modal is not available, and has to look it.
    Overlay.modal: Rectangle { color: Colors.scrim }

    background: null

    contentItem: DialogShell {
        id: shell

        width: root.width
        titleText: root.titleText
        messageText: root.messageText

        actions: [
            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(132), implicitWidth)
                Layout.preferredHeight: Metrics.touchPrimary
                text: root.confirmText
                buttonVariant: root.confirmVariant === "danger"
                               ? AppButton.Danger : AppButton.Primary
                onClicked: {
                    root.close()
                    root.confirmed()
                }
            },
            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(132), implicitWidth)
                Layout.preferredHeight: Metrics.touchPrimary
                text: root.cancelText
                buttonVariant: AppButton.Ghost
                onClicked: root.close()
            },
            Item { Layout.fillWidth: true }
        ]
    }
}
