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
    property int labelSize: Typography.readoutLabel
    property int valueSize: Typography.readoutValue
    property int unitSize: Typography.readoutUnit

    spacing: Metrics.px(2)

    Text {
        width: readout.width
        text: readout.label
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: readout.labelSize
        horizontalAlignment: readout.alignment
        elide: Text.ElideRight
    }

    Text {
        width: readout.width
        text: readout.value
        color: readout.valueColor
        font.family: Typography.monoFamily
        font.pixelSize: readout.valueSize
        font.weight: Typography.bold
        horizontalAlignment: readout.alignment
    }

    Text {
        width: readout.width
        text: readout.unit
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: readout.unitSize
        horizontalAlignment: readout.alignment
        elide: Text.ElideRight
        visible: readout.unit.length > 0
    }
}
