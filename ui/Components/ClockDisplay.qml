// -----------------------------------------------------------------------
// File: ClockDisplay.qml
// Description: Wall clock, date, and ventilation elapsed time
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Elapsed ventilation time sits beside the wall clock because every entry
// in the event log and the trend timeline is read against it.
//
import QtQuick
import QtQuick.Layouts
import "../Theme"
import "../Controls"

Item {
    id: clock

    property string timeText: "--:--"
    property string dateText: ""
    property string elapsedText: ""
    property bool ventilating: false

    implicitWidth: column.implicitWidth
    implicitHeight: Math.round(48 * Metrics.scale)

    ColumnLayout {
        id: column
        anchors.centerIn: parent
        spacing: 0

        Text {
            Layout.alignment: Qt.AlignRight
            text: clock.timeText
            color: Colors.textPrimary
            font.family: Typography.numberFamily
            font.features: Typography.numericFeatures
            font.pixelSize: Typography.bodyLarge
            font.weight: Typography.semibold
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Spacing.xs

            Text {
                visible: clock.dateText.length > 0
                text: clock.dateText
                color: Colors.textMuted
                font.family: Typography.family
                font.pixelSize: Typography.micro
            }

            AppIcon {
                visible: clock.ventilating && clock.elapsedText.length > 0
                name: "clock"
                size: Math.round(12 * Metrics.scale)
                color: Colors.success
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                visible: clock.elapsedText.length > 0
                text: clock.elapsedText
                color: clock.ventilating ? Colors.success : Colors.textMuted
                font.family: Typography.numberFamily
                font.features: Typography.numericFeatures
                font.pixelSize: Typography.micro
                font.weight: Typography.semibold
            }
        }
    }
}
