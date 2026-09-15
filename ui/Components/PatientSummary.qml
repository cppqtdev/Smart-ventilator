// -----------------------------------------------------------------------
// File: PatientSummary.qml
// Description: Gender, height and ideal body weight, left aligned
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// An Item wrapping a ColumnLayout, not a bare Column: a Column positions its
// children itself and refuses any child that anchors to it, so the MouseArea
// that makes the block tappable would disable the whole column.
//
import QtQuick
import QtQuick.Layouts
import "../Theme"

Item {
    id: summary

    property string gender: ""
    property string heightText: ""
    property string weightText: ""
    property int fontSize: Typography.label
    property int alignment: Text.AlignLeft

    signal clicked()

    implicitWidth: column.implicitWidth
    implicitHeight: column.implicitHeight

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Metrics.px(4)

        Text {
            Layout.fillWidth: true
            text: summary.gender
            color: Colors.textPrimary
            font.family: Typography.family
            font.pixelSize: summary.fontSize
            font.weight: Typography.bold
            horizontalAlignment: summary.alignment
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: summary.heightText
            color: Colors.textSecondary
            font.family: Typography.family
            font.pixelSize: summary.fontSize
            horizontalAlignment: summary.alignment
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: summary.weightText
            color: Colors.textSecondary
            font.family: Typography.family
            font.pixelSize: summary.fontSize
            horizontalAlignment: summary.alignment
            elide: Text.ElideRight
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: summary.clicked()
    }
}
