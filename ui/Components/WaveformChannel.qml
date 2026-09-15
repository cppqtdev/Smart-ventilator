// -----------------------------------------------------------------------
// File: WaveformChannel.qml
// Description: One labelled trace with its own y-axis ticks
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import SmartVentilator.Render 1.0
import "../Theme"

Item {
    id: channel

    property string channelKey: ""
    property var presenter
    property string label: ""
    property color traceColor: Colors.wavePressure
    property real minimumValue: 0
    property real maximumValue: 40
    property real baselineValue: 0
    property var ticks: []
    property int capacity: 600
    property bool frozen: false
    property alias view: trace

    readonly property int axisWidth: Metrics.px(44)

    function append(value) {
        if (!channel.frozen)
            trace.appendSample(value)
    }

    function clear() {
        trace.clear()
    }

    // The buffer already holds history when a channel first appears, so it is
    // primed once and then fed one sample at a time.
    function prime() {
        if (!channel.presenter || channel.channelKey.length === 0)
            return
        trace.clear()
        var history = channel.presenter.waveform(channel.channelKey)
        if (history !== undefined && history.length > 0)
            trace.appendSamples(history)
    }

    function receiveSample(key, value) {
        if (key === channel.channelKey)
            channel.append(value)
    }

    // A Connections block binds its target when the channel is built, and at
    // that moment the presenter is still being threaded down from the screen.
    // Connecting by hand every time the presenter changes is what makes the
    // trace start on the first tick instead of never.
    function rebind() {
        if (channel.boundPresenter === channel.presenter)
            return
        if (channel.boundPresenter)
            channel.boundPresenter.sampleAppended.disconnect(channel.receiveSample)
        channel.boundPresenter = channel.presenter ? channel.presenter : null
        if (channel.boundPresenter)
            channel.boundPresenter.sampleAppended.connect(channel.receiveSample)
        channel.prime()
    }

    property var boundPresenter: null

    onPresenterChanged: channel.rebind()
    onChannelKeyChanged: channel.prime()
    Component.onCompleted: channel.rebind()
    Component.onDestruction: {
        if (channel.boundPresenter)
            channel.boundPresenter.sampleAppended.disconnect(channel.receiveSample)
    }

    Text {
        id: title
        anchors.left: parent.left
        anchors.top: parent.top
        text: channel.label
        color: Colors.textPrimary
        font.family: Typography.monoFamily
        font.pixelSize: Typography.channelLabel
        font.weight: Typography.bold
    }

    Item {
        id: plotArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: title.bottom
        anchors.topMargin: Spacing.xs
        anchors.bottom: parent.bottom

        Repeater {
            model: channel.ticks

            delegate: Item {
                required property var modelData

                readonly property real fraction:
                    (channel.maximumValue - modelData) /
                    Math.max(0.000001, channel.maximumValue - channel.minimumValue)

                anchors.left: parent.left
                anchors.right: parent.right
                height: Typography.axisTick
                y: Math.round(fraction * (plotArea.height - height))

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: channel.axisWidth - Spacing.sm
                    text: String(modelData)
                    color: Colors.textSecondary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.axisTick
                }
            }
        }

        WaveformView {
            id: trace
            anchors.left: parent.left
            anchors.leftMargin: channel.axisWidth
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            sweep: true
            ageOpacity: 0.32
            capacity: channel.capacity
            frozen: channel.frozen

            minimumValue: channel.minimumValue
            maximumValue: channel.maximumValue
            baselineValue: channel.baselineValue

            lineColor: channel.traceColor
            fillColor: channel.traceColor
            fillOpacity: 0.26
            lineWidth: Metrics.waveformStroke
            backgroundColor: Colors.transparent
            gridColor: Colors.waveGrid
            gridColorMajor: Colors.waveGrid
            baselineColor: Colors.waveBaseline
            showGrid: true
            showFill: true
            showBaseline: channel.minimumValue < 0
            gridRows: Math.max(1, channel.ticks.length - 1)
            gridColumns: 7
            gridMinorDivisions: 1
        }
    }
}
