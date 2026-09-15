// -----------------------------------------------------------------------
// File: SystemPane.qml
// Description: System screen - Info, Tests & Calib, Sensors, Settings
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: pane

    property var settingsData
    property var ventilatorData
    property var calibrationService
    property var clockData
    property var alarmAudio

    property int pageIndex: 0
    property int infoPage: 0
    property string settingsSelection: "loudness"

    readonly property var pages: [
        { key: "info",     label: qsTr("Info") },
        { key: "tests",    label: qsTr("Tests & Calib") },
        { key: "sensors",  label: qsTr("Sensors") },
        { key: "settings", label: qsTr("Settings") }
    ]

    // The three Info chips all showed the same list, and the software
    // version lived only on the splash screen where a service engineer never
    // sees it. Each chip is its own page now, and the first one is the block
    // that identifies this device.
    readonly property var identityRows: [
        { name: qsTr("Software"),       value: pane.settingsData ? pane.settingsData.softwareVersion : "---" },
        { name: qsTr("Build"),          value: pane.settingsData ? pane.settingsData.buildId : "---" },
        { name: qsTr("Serial"),         value: pane.settingsData ? pane.settingsData.serialNumber : "---" },
        { name: qsTr("Operating hrs"),  value: pane.settingsData
                                               ? pane.settingsData.operatingHours.toFixed(2) : "---" },
        { name: qsTr("Mode"),           value: pane.ventilatorData ? pane.ventilatorData.mode : "---" },
        { name: qsTr("Language"),       value: pane.settingsData ? pane.settingsData.language : "---" },
        { name: qsTr("Time zone"),      value: pane.settingsData ? pane.settingsData.timeZoneId : "---" }
    ]

    readonly property var linkRows: [
        { name: qsTr("Alarm sound"),    value: pane.alarmAudio
                                               ? (pane.alarmAudio.available ? qsTr("Ready") : qsTr("UNAVAILABLE"))
                                               : "---" },
        { name: qsTr("Loudness"),       value: pane.settingsData
                                               ? (pane.settingsData.audioVolume + " %") : "---" },
        { name: qsTr("Data link"),      value: pane.ventilatorData
                                               ? (pane.ventilatorData.backendConnected
                                                  ? qsTr("Connected") : qsTr("Disconnected"))
                                               : "---" },
        { name: qsTr("Mains"),          value: pane.ventilatorData
                                               ? (pane.ventilatorData.mainsConnected
                                                  ? qsTr("Connected") : qsTr("On battery"))
                                               : "---" },
        { name: qsTr("Battery"),        value: pane.ventilatorData
                                               ? (pane.ventilatorData.batteryRuntimeMinutes >= 0
                                                  ? pane.ventilatorData.batteryRuntimeMinutes + qsTr(" min")
                                                  : qsTr("Unknown"))
                                               : "---" }
    ]

    readonly property var infoRows: {
        if (pane.infoPage === 0)
            return pane.identityRows
        if (pane.infoPage === 2)
            return pane.linkRows
        return pane.deviceOptions
    }

    readonly property var deviceOptions: [
        { name: qsTr("Options:"),        value: "---" },
        { name: qsTr("Adult/ped."),      value: qsTr("Neonatal") },
        { name: qsTr("nCPAP"),           value: qsTr("TRC") },
        { name: qsTr("DuoPAP/APRV"),     value: qsTr("Trends/Loops") },
        { name: qsTr("NIV/NIV-ST"),      value: qsTr("P/V Tool Pro") },
        { name: qsTr("Masimo Rainbow"),  value: "---" },
        { name: qsTr("Hi Flow O2"),      value: "---" }
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

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: pane.pageIndex

            RowLayout {
                spacing: Metrics.gutter

                ColumnLayout {
                    Layout.fillWidth: false
                    Layout.preferredWidth: Math.max(Metrics.px(110), implicitWidth)
                    Layout.alignment: Qt.AlignTop
                    spacing: Spacing.md

                    Repeater {
                        model: 3

                        delegate: ChipButton {
                            required property int index
                            Layout.fillWidth: true
                            text: [qsTr("Device"), qsTr("Options"), qsTr("Status")][index]
                            selected: pane.infoPage === index
                            onClicked: pane.infoPage = index
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignTop
                    Layout.maximumHeight: Metrics.px(190)
                    radius: Radius.small
                    color: Colors.surfaceOverlay

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Spacing.md
                        spacing: Spacing.xs

                        Repeater {
                            model: pane.infoRows

                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: Spacing.lg

                                Text {
                                    Layout.preferredWidth: Metrics.px(130)
                                    text: modelData.name
                                    color: Colors.textPrimary
                                    font.family: Typography.monoFamily
                                    font.pixelSize: Typography.caption
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.value
                                    color: Colors.textPrimary
                                    font.family: Typography.monoFamily
                                    font.pixelSize: Typography.caption
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                Item { Layout.fillWidth: true }
            }

            CalibrationPane {
                service: pane.calibrationService
            }

            RowLayout {
                spacing: Metrics.gutter

                ChipButton {
                    Layout.preferredWidth: Math.max(Metrics.px(110), implicitWidth)
                    Layout.alignment: Qt.AlignTop
                    text: qsTr("On/Off")
                }

                AppCheckBox {
                    Layout.alignment: Qt.AlignTop
                    checked: true
                    text: qsTr("O2 Cell")
                }

                Item { Layout.fillWidth: true }
            }

            RowLayout {
                spacing: Metrics.gutter

                ColumnLayout {
                    Layout.fillWidth: false
                    Layout.preferredWidth: Math.max(Metrics.px(110), implicitWidth)
                    Layout.alignment: Qt.AlignTop
                    spacing: Spacing.md

                    ChipButton {
                        Layout.fillWidth: true
                        text: qsTr("Loudness")
                        selected: pane.settingsSelection === "loudness"
                        onClicked: pane.settingsSelection = "loudness"
                    }

                    ChipButton {
                        Layout.fillWidth: true
                        text: qsTr("Day & Night")
                        selected: pane.settingsSelection === "brightness"
                        onClicked: pane.settingsSelection = "brightness"
                    }

                    ChipButton {
                        Layout.fillWidth: true
                        text: qsTr("Day & Time")
                        selected: pane.settingsSelection === "datetime"
                        onClicked: pane.settingsSelection = "datetime"
                    }
                }

                DateTimePane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: pane.settingsSelection === "datetime"
                    clock: pane.clockData
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredHeight: settingsColumn.implicitHeight + Spacing.xxl * 2
                    radius: Radius.small
                    color: Colors.surfaceOverlay
                    visible: pane.settingsSelection !== "datetime"

                    ColumnLayout {
                        id: settingsColumn
                        anchors.centerIn: parent
                        spacing: Spacing.md

                        DialControl {
                            Layout.alignment: Qt.AlignHCenter
                            label: ""
                            unit: "%"
                            from: 0
                            to: 100
                            stepSize: 5
                            value: pane.settingsSelection === "brightness"
                                   ? (pane.settingsData ? pane.settingsData.brightness : 60)
                                   : (pane.settingsData ? pane.settingsData.audioVolume : 60)
                            onStepRequested: function (proposed) {
                                if (!pane.settingsData)
                                    return
                                if (pane.settingsSelection === "brightness")
                                    pane.settingsData.brightness = proposed
                                else
                                    pane.settingsData.audioVolume = proposed
                            }
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: pane.settingsSelection === "brightness"
                                  ? qsTr("Brightness") : qsTr("Loudness")
                            color: Colors.textPrimary
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.readoutLabel
                        }

                        // IEC 60601-1-8 expects the auditory alarm signal to
                        // be verifiable. This is how the operator hears that
                        // the speaker works at the loudness they just set.
                        AppButton {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: Math.max(Metrics.px(110), implicitWidth)
                            Layout.preferredHeight: Metrics.px(32)
                            visible: pane.settingsSelection !== "brightness"
                            text: qsTr("Test sound")
                            buttonVariant: AppButton.Primary
                            enabled: pane.alarmAudio !== null
                            onClicked: {
                                if (pane.alarmAudio)
                                    pane.alarmAudio.playTestTone()
                            }
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.maximumWidth: Metrics.px(220)
                            visible: pane.settingsSelection !== "brightness"
                                     && pane.alarmAudio !== null
                                     && !pane.alarmAudio.available
                            text: qsTr("No sound output. The ventilator cannot sound an alarm.")
                            color: Colors.critical
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                            font.family: Typography.monoFamily
                            font.pixelSize: Typography.micro
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: Spacing.md
                            visible: pane.settingsSelection === "brightness"

                            AppButton {
                                Layout.preferredWidth: Math.max(Metrics.px(62), implicitWidth)
                                Layout.preferredHeight: Metrics.px(28)
                                text: qsTr("Day")
                                buttonVariant: AppButton.Primary
                                selected: pane.settingsData
                                         && pane.settingsData.dayNightMode === "Day"
                                onClicked: {
                                    if (pane.settingsData)
                                        pane.settingsData.dayNightMode = "Day"
                                }
                            }

                            AppButton {
                                Layout.preferredWidth: Math.max(Metrics.px(62), implicitWidth)
                                Layout.preferredHeight: Metrics.px(28)
                                text: qsTr("Night")
                                buttonVariant: AppButton.Primary
                                selected: pane.settingsData
                                         && pane.settingsData.dayNightMode === "Night"
                                onClicked: {
                                    if (pane.settingsData)
                                        pane.settingsData.dayNightMode = "Night"
                                }
                            }
                        }

                        AppButton {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: Math.max(Metrics.px(96), implicitWidth)
                            Layout.preferredHeight: Metrics.px(28)
                            text: pane.settingsSelection === "brightness"
                                  ? qsTr("Automatic") : qsTr("Test")
                            buttonVariant: AppButton.Primary
                            onClicked: {
                                if (pane.settingsSelection === "brightness"
                                        && pane.settingsData)
                                    pane.settingsData.dayNightMode = "Automatic"
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }
    }
}
