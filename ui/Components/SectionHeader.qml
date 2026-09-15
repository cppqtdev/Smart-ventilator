// -----------------------------------------------------------------------
// File: SectionHeader.qml
// Description: Icon + title + optional trailing content row for panels
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../Theme"
import "../Controls"

Item {
    id: header

    property string iconName: ""
    property string title: ""
    property string subtitle: ""
    property color accentColor: Colors.textSecondary
    property alias trailing: trailingSlot.data

    implicitHeight: Math.max(Math.round(32 * Metrics.scale), row.implicitHeight)

    RowLayout {
        id: row
        anchors.fill: parent
        spacing: Spacing.inlineGap

        AppIcon {
            visible: header.iconName.length > 0
            name: header.iconName
            size: Math.round(20 * Metrics.scale)
            color: header.accentColor
            Layout.alignment: Qt.AlignVCenter
        }

        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true

            Text {
                text: header.title
                color: Colors.textPrimary
                font.family: Typography.family
                font.pixelSize: Typography.label
                font.weight: Typography.semibold
                font.letterSpacing: Typography.trackCaps
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                visible: header.subtitle.length > 0
                text: header.subtitle
                color: Colors.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.caption
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        Item {
            id: trailingSlot
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
        }
    }
}
