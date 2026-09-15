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
    width: Math.min(parent ? parent.width * 0.38 : 560, 560)
    height: confirmPanel.implicitHeight
    anchors.centerIn: parent

    background: Rectangle {
        radius: Radius.medium
        color: Colors.surface
        border.color: Colors.line
        border.width: Metrics.borderWidth
    }

    contentItem: Panel {
        id: confirmPanel

        implicitHeight: confirmContent.implicitHeight + Metrics.px(48)

        ColumnLayout {
            id: confirmContent

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Metrics.px(24)
            spacing: Spacing.lg

            Text {
                Layout.fillWidth: true
                text: root.titleText
                color: Colors.textPrimary
                font.family: Typography.family
                font.pixelSize: Typography.subtitleLarge
                font.weight: Typography.semibold
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text: root.messageText
                color: Colors.textSecondary
                font.family: Typography.family
                font.pixelSize: Typography.body
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Spacing.sm
                spacing: Spacing.lg

                PrimaryButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Metrics.touchPrimary
                    text: root.cancelText
                    variant: "ghost"
                    onClicked: root.close()
                }

                PrimaryButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Metrics.touchPrimary
                    text: root.confirmText
                    variant: root.confirmVariant
                    onClicked: {
                        root.close()
                        root.confirmed()
                    }
                }
            }
        }
    }
}
