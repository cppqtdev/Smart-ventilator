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
    property var channels: []
    property var presenter
    property bool frozen: false
    property int sweepSeconds: 15
    property int tickSeconds: 2

    // The controller appends one sample per channel every 45 ms, so a buffer
    // of sweepSeconds worth of samples is exactly one sweep across the panel.
    property int sampleRateHz: 22

    readonly property int axisHeight: Metrics.px(26)

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
        anchors.margins: Spacing.lg
        spacing: Spacing.lg

        Repeater {
            id: channelRepeater
            model: panel.channels

            delegate: WaveformChannel {
                required property var modelData

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: Metrics.waveformHeightMin

                channelKey: modelData.key
                presenter: panel.presenter
                capacity: Math.max(60, panel.sweepSeconds * panel.sampleRateHz)
                label: modelData.label
                traceColor: modelData.color
                minimumValue: modelData.minimum
                maximumValue: modelData.maximum
                baselineValue: modelData.baseline !== undefined ? modelData.baseline : 0
                ticks: modelData.ticks
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
