// -----------------------------------------------------------------------
// File: AlarmsPane.qml
// Description: Alarms screen - limit dials with their current-value bars
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: pane

    property var ventilatorData
    property var alarmData
    property int pageIndex: 0

    signal limitRequested(string key, real value)

    readonly property var pages: [
        { key: "page1", label: qsTr("Page 1") },
        { key: "page2", label: qsTr("Page 2") },
        { key: "log",   label: qsTr("Alarm Log") }
    ]

    // The rings take as much of the pane as the column count allows, up to
    // the size the design draws them at.
    readonly property int gaugeSpacing: Metrics.px(14)

    readonly property int columnCount:
        pane.pageIndex === 0 ? pane.pageOne.length : pane.pageTwo.length

    readonly property int gaugeSize:
        Math.max(Metrics.px(70),
                 Math.min(Metrics.px(126),
                          Math.floor((pane.width - pane.gaugeSpacing * (pane.columnCount - 1))
                                     / Math.max(1, pane.columnCount))))

    function limit(key, fallback) {
        if (!pane.ventilatorData)
            return fallback
        var value = pane.ventilatorData[key]
        return value === undefined ? fallback : value
    }

    function priorityColour(priority) {
        var name = String(priority).toLowerCase()
        if (name === "critical" || name === "high")
            return Colors.alarmHigh
        if (name === "warning" || name === "medium")
            return Colors.alarmMedium
        if (name === "advisory" || name === "low")
            return Colors.alarmLow
        return Colors.textSecondary
    }

    function reading(key) {
        if (!pane.ventilatorData)
            return 0
        var value = pane.ventilatorData[key]
        return value === undefined ? 0 : Math.round(value)
    }

    // Keys match VentilatorController::requestAlarmLimitChange. A key it does
    // not know is refused, which is what made every adjustment here fail.
    readonly property var pageOne: [
        { key: "highPressure", label: qsTr("Paw"),       unit: "cmH2O",
          high: pane.limit("alarmHighPressure", 30), low: pane.limit("alarmLowPressure", 5),
          current: pane.reading("ppeak"), from: 0, to: 80 },
        { key: "highMv",       label: qsTr("ExpMinVol"), unit: "l/min",
          high: pane.limit("alarmHighMv", 12), low: 4,
          current: pane.reading("expMinVol"), from: 0, to: 30 },
        { key: "apneaTime",    label: qsTr("Apnea"),     unit: "s",
          high: pane.limit("alarmApneaTime", 20), low: 5,
          current: pane.reading("ftotal"), from: 0, to: 60 },
        { key: "lowVt",        label: qsTr("VT"),        unit: "ml",
          high: pane.limit("tidalVolume", 500), low: pane.limit("alarmLowVt", 270),
          current: pane.reading("vte"), from: 0, to: 2000 }
    ]

    readonly property var pageTwo: [
        { key: "lowSpo2",      label: qsTr("SpO2"),      unit: "%",
          high: 100, low: pane.limit("alarmLowSpo2", 90),
          current: pane.reading("spo2"), from: 0, to: 100 },
        { key: "lowPressure",  label: qsTr("Paw low"),   unit: "cmH2O",
          high: pane.limit("alarmHighPressure", 30), low: pane.limit("alarmLowPressure", 5),
          current: pane.reading("pmean"), from: 0, to: 80 }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: Metrics.gutter

        SubTabStrip {
            Layout.fillWidth: true
            model: pane.pages
            currentIndex: pane.pageIndex
            onActivated: function (index) { pane.pageIndex = index }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: pane.gaugeSpacing
            visible: pane.pageIndex < 2

            Repeater {
                model: pane.pageIndex === 0 ? pane.pageOne : pane.pageTwo

                delegate: AlarmLimitColumn {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    entry: modelData
                    gaugeSize: pane.gaugeSize
                    onLimitRequested: function (key, value) {
                        pane.limitRequested(key, value)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: pane.pageIndex === 2
            radius: Radius.medium
            color: Colors.surface

            ListView {
                id: alarmLog
                anchors.fill: parent
                anchors.margins: Spacing.md
                clip: true
                spacing: Spacing.xs
                model: pane.alarmData

                delegate: Rectangle {
                    required property string time
                    required property string priority
                    required property string source
                    required property string description
                    required property string status

                    width: alarmLog.width
                    height: Metrics.px(30)
                    radius: Radius.xs
                    color: Colors.surfaceRaised

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Spacing.md
                        anchors.rightMargin: Spacing.md
                        spacing: Spacing.md

                        Text {
                            Layout.preferredWidth: Metrics.px(84)
                            text: time
                            color: Colors.textSecondary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.caption
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.preferredWidth: Metrics.px(72)
                            text: priority
                            color: pane.priorityColour(priority)
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.caption
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.preferredWidth: Metrics.px(88)
                            text: source
                            color: Colors.textSecondary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.caption
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: description
                            color: Colors.textPrimary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.caption
                            elide: Text.ElideRight
                        }

                        Text {
                            text: status
                            color: Colors.textSecondary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.caption
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                text: qsTr("No alarm has been logged in this session.")
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.readoutLabel
                visible: alarmLog.count === 0
            }
        }
    }
}
