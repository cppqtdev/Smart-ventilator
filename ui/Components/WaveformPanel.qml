// -----------------------------------------------------------------------
// File: WaveformPanel.qml
// Description: Stacked waveform channels over a shared time axis
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../Theme"

Rectangle {
    id: panel

    // Each entry: { key, label, color, minimum, maximum, baseline, ticks }
    //
    // A channel may instead give { scaleKey, signedScale } and let the panel
    // take its full scale from the presenter. The fixed scales drew an eight
    // kilo patient's flow as a flat line inside a plus and minus 75 litre
    // lane, and pegged an adult's volume against a 40 millilitre ceiling.
    property var channels: []
    property var presenter
    property bool frozen: false
    property int sweepSeconds: 15
    property int tickSeconds: 2

    // The controller appends one sample per channel every 45 ms, so a buffer
    // of sweepSeconds worth of samples is exactly one sweep across the panel.
    property int sampleRateHz: 22

    readonly property int axisHeight: Metrics.px(22)

    readonly property var scales: panel.presenter && panel.presenter.waveformScales
                                  ? panel.presenter.waveformScales : ({})

    /** Full scale for a channel: its own number, or the patient's. */
    function fullScale(entry) {
        if (entry.scaleKey !== undefined && panel.scales[entry.scaleKey] !== undefined)
            return panel.scales[entry.scaleKey]
        return entry.maximum !== undefined ? entry.maximum : 1
    }

    function lowerBound(entry) {
        if (entry.scaleKey === undefined)
            return entry.minimum !== undefined ? entry.minimum : 0
        return entry.signedScale === true ? -panel.fullScale(entry) : 0
    }

    function ticksFor(entry) {
        if (entry.ticks !== undefined)
            return entry.ticks
        var top = panel.fullScale(entry)
        if (entry.signedScale === true)
            return [top, 0, -top]
        return [top, Math.round(top / 2), 0]
    }

    radius: Radius.medium
    color: Colors.surface

    function channelAt(index) {
        return channelRepeater.itemAt(index)
    }

    function appendTo(index, value) {
        var item = panel.channelAt(index)
        if (item !== null)
            item.append(value)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.md
        spacing: Spacing.md

        Repeater {
            id: channelRepeater
            model: panel.channels

            delegate: WaveformChannel {
                required property var modelData

                Layout.fillWidth: true
                Layout.fillHeight: true
                // A lane asked for 100 as its floor, so two lanes plus the
                // time axis wanted more than the shorter panels are given
                // and the axis was pushed out of the panel. 100 is what a
                // lane wants; 64 is what it can still be read at.
                Layout.preferredHeight: Metrics.waveformHeightMin
                Layout.minimumHeight: Metrics.px(64)

                channelKey: modelData.key
                presenter: panel.presenter
                capacity: Math.max(60, panel.sweepSeconds * panel.sampleRateHz)
                label: modelData.label
                traceColor: modelData.color
                minimumValue: panel.lowerBound(modelData)
                maximumValue: panel.fullScale(modelData)
                baselineValue: modelData.baseline !== undefined ? modelData.baseline : 0
                ticks: panel.ticksFor(modelData)
                frozen: panel.frozen
            }
        }

        Item {
            id: timeAxis
            Layout.fillWidth: true
            Layout.preferredHeight: panel.axisHeight

            readonly property int tickCount:
                Math.max(1, Math.floor(panel.sweepSeconds / panel.tickSeconds))

            Row {
                anchors.left: parent.left
                anchors.leftMargin: Metrics.px(44)
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0

                Repeater {
                    model: timeAxis.tickCount

                    delegate: Item {
                        required property int index

                        width: (timeAxis.width - Metrics.px(44)) / timeAxis.tickCount
                        height: Typography.axisTick

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: tickLabel.left
                            anchors.rightMargin: Spacing.sm
                            anchors.verticalCenter: parent.verticalCenter
                            height: Metrics.hairline
                            color: Colors.waveGrid
                        }

                        Text {
                            id: tickLabel
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            text: String((index + 1) * panel.tickSeconds)
                            color: Colors.textSecondary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.axisTick
                        }

                        Rectangle {
                            anchors.left: tickLabel.right
                            anchors.leftMargin: Spacing.sm
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            height: Metrics.hairline
                            color: Colors.waveGrid
                        }
                    }
                }
            }
        }
    }
}
