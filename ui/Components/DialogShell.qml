// -----------------------------------------------------------------------
// File: DialogShell.qml
// Description: The chrome every dialog wears - scrim, glass, titled header
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The dialogs each drew their own plain rectangle with a proportional-font
// heading centred in it, which is not the language the rest of the panel
// speaks: every other surface is a Panel, every other label is monospaced,
// and Panel already documents its frosted treatment as the one dialog
// shells use. They shared a comment saying they wore the same shell and no
// code that made it true.
//
// This is that shell. A dialog gives it a title, an optional line of
// explanation, its own content and its actions; it gets the glass surface,
// the accent edge that marks a modal apart from the screen behind it, and
// the header rule, all in one place so two dialogs cannot drift again.
//
import QtQuick
import QtQuick.Layouts
import "../Theme"

// Panel.glass is not used here. Its frosted treatment was removed from
// GlossSurface when the flat reference look landed, and what is left fills
// the whole surface with the accent blue - a dialog drawn that way is a
// solid blue block. A dialog is the same flat surface as every other panel,
// one tone lighter so it reads as sitting above the screen behind it.
Rectangle {
    id: shell

    property string titleText: ""
    property string messageText: ""

    /** The dialog's own content, between the message and the actions. */
    default property alias body: bodyHolder.data
    /** True while the body holds something worth reserving room for. */
    property bool hasBody: false

    /** The buttons, right aligned under a rule. */
    property alias actions: actionHolder.data

    radius: Radius.large
    color: Colors.surfaceRaised
    border.color: Colors.lineStrong
    border.width: Metrics.borderWidth

    implicitWidth: Metrics.px(420)
    implicitHeight: layout.implicitHeight + Spacing.xl * 2

    // A modal is the only surface allowed to sit over another, so it says so
    // with an accent edge rather than by being a different shade of the
    // same navy.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: Radius.large
        anchors.rightMargin: Radius.large
        anchors.topMargin: 0
        height: Metrics.px(3)
        color: Colors.accent
    }

    ColumnLayout {
        id: layout

        anchors.fill: parent
        anchors.margins: Spacing.xl
        anchors.topMargin: Spacing.xl + Metrics.px(3)
        spacing: Spacing.md

        Text {
            Layout.fillWidth: true
            text: shell.titleText
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.subtitle
            font.weight: Typography.bold
            elide: Text.ElideRight
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.hairline
            color: Colors.lineStrong
        }

        Text {
            Layout.fillWidth: true
            visible: shell.messageText.length > 0
            text: shell.messageText
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.small
            lineHeight: Typography.lineNormal
            wrapMode: Text.WordWrap
        }

        Item {
            id: bodyHolder
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            visible: shell.hasBody
        }

        RowLayout {
            id: actionHolder
            Layout.fillWidth: true
            Layout.topMargin: Spacing.sm
            layoutDirection: Qt.RightToLeft
            spacing: Spacing.md
        }
    }
}
