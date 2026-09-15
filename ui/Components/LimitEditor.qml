// -----------------------------------------------------------------------
// File: LimitEditor.qml
// Description: Modal spin box for one alarm limit, with cancel and apply
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The reference draws the alarm limits as bare rings with nothing to press.
// A ring that cannot be changed is not a limit control, so pressing one
// opens this: the same spin box the date and time page uses, over a scrim,
// with the change only reaching the device on Apply.
//
import QtQuick
import QtQuick.Layouts
import "../Controls"
import "../Theme"

Item {
    id: editor

    property string limitKey: ""
    property string title: ""
    property string unit: ""
    property int minimumValue: 0
    property int maximumValue: 100
    property int pendingValue: 0

    signal accepted(string key, int value)

    visible: false

    function open(key, range) {
        if (!key || !range || range.minimum === undefined)
            return
        editor.limitKey = key
        editor.title = range.label !== undefined ? range.label : key
        editor.unit = range.unit !== undefined ? range.unit : ""
        editor.minimumValue = range.minimum
        editor.maximumValue = range.maximum
        editor.pendingValue = range.value !== undefined ? range.value : range.minimum
        editor.visible = true
    }

    function close() {
        editor.visible = false
        editor.limitKey = ""
    }

    // The scrim takes the press, so nothing behind the panel reacts while
    // the limit is being set.
    Rectangle {
        anchors.fill: parent
        color: Colors.scrim

        MouseArea {
            anchors.fill: parent
            onClicked: editor.close()
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - Metrics.screenPadding * 2, Metrics.px(360))
        height: panel.implicitHeight + Metrics.px(40)
        radius: Radius.medium
        color: Colors.surfaceRaised
        border.width: Metrics.borderWidth
        border.color: Colors.line

        MouseArea { anchors.fill: parent }

        ColumnLayout {
            id: panel
            anchors.centerIn: parent
            width: parent.width - Metrics.px(40)
            spacing: Spacing.lg

            Text {
                Layout.fillWidth: true
                text: editor.title
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.readoutLabel
                font.weight: Typography.bold
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }

            RingSpinner {
                Layout.alignment: Qt.AlignHCenter
                ringSize: Metrics.px(110)
                label: editor.unit
                wrap: false
                from: editor.minimumValue
                to: editor.maximumValue
                value: editor.pendingValue
                onValueSet: function (newValue) { editor.pendingValue = newValue }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Allowed %1 to %2 %3")
                          .arg(editor.minimumValue).arg(editor.maximumValue).arg(editor.unit)
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.caption
                horizontalAlignment: Text.AlignHCenter
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Spacing.md

                AppButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Metrics.actionButtonHeight
                    text: qsTr("Cancel")
                    buttonVariant: AppButton.Secondary
                    onClicked: editor.close()
                }

                AppButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Metrics.actionButtonHeight
                    text: qsTr("Apply")
                    buttonVariant: AppButton.Success
                    onClicked: {
                        editor.accepted(editor.limitKey, editor.pendingValue)
                        editor.close()
                    }
                }
            }
        }
    }
}
