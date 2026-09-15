// -----------------------------------------------------------------------
// File: NumericReadout.qml
// Description: Label, value and unit stacked in the reference proportions
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "../Theme"

Column {
    id: readout

    property string label: ""
    property string value: "---"
    property string unit: ""
    property color valueColor: Colors.textPrimary
    property int alignment: Text.AlignLeft

    spacing: Metrics.px(2)

    Text {
        width: readout.width
        text: readout.label
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.readoutLabel
        horizontalAlignment: readout.alignment
        elide: Text.ElideRight
    }

    Text {
        width: readout.width
        text: readout.value
        color: readout.valueColor
        font.family: Typography.monoFamily
        font.pixelSize: Typography.readoutValue
        font.weight: Typography.bold
        horizontalAlignment: readout.alignment
    }

    Text {
        width: readout.width
        text: readout.unit
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.readoutUnit
        horizontalAlignment: readout.alignment
        visible: readout.unit.length > 0
    }
}
