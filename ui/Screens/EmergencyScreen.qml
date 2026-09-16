// -----------------------------------------------------------------------
// File: EmergencyScreen.qml
// Description: Streamlined emergency mode display with focused vital data
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import "../Theme"
import "../Components"
import "../Controls"

Control {
    id: root

    property var ventilatorData
    property var alarmData

    signal exitEmergency()

    // HARDWARE: Emergency mode should be triggered automatically by:
    //   - Patient disconnect detection (impedance/flow drop)
    //   - Multiple simultaneous critical alarms
    //   - Power failure switchover to battery
    //   - Operator emergency button (physical hardware button)
    // The screen should also activate backup ventilation parameters
    // and send emergency notification to nurse call system.

    contentItem: ColumnLayout {
        spacing: Spacing.screenMargin_8

        // Emergency banner
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.px(64)
            radius: Radius.small
            color: Colors.critical

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Spacing.xxl
                anchors.rightMargin: Spacing.xxl

                Text {
                    Layout.fillWidth: true
                    text: "EMERGENCY MODE"
                    color: Colors.textPrimary
                    font.pixelSize: Typography.title
                    font.weight: Font.DemiBold
                }

                PrimaryButton {
                    Layout.preferredWidth: Metrics.px(180)
                    Layout.preferredHeight: Metrics.buttonHeightStandard
                    text: "Exit Emergency"
                    buttonColor: Colors.warningBackground
                    onClicked: root.exitEmergency()
                }
            }
        }

        // Main content: large waveforms and critical vitals side by side
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Spacing.screenMargin_8

            // Waveforms (left, dominant area)
            Panel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Spacing.screenMargin_10
                    spacing: Spacing.screenMargin_6

                    WaveformChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: "Paw"
                        traceColor: Colors.wavePressure
                        samples: root.ventilatorData
                            ? root.ventilatorData.pressureWaveform
                            : []
                        frozen: root.ventilatorData ? root.ventilatorData.frozen : false
                        minimumValue: 0
                        maximumValue: 60
                        unit: "cmH2O"
                        tickValues: [60, 30, 0]
                    }

                    WaveformChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: "Flow"
                        traceColor: Colors.waveFlow
                        samples: root.ventilatorData
                            ? root.ventilatorData.flowWaveform
                            : []
                        frozen: root.ventilatorData ? root.ventilatorData.frozen : false
                        minimumValue: -90
                        maximumValue: 90
                        unit: "L/min"
                    }

                    WaveformChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: "Volume"
                        traceColor: Colors.waveVolume
                        samples: root.ventilatorData
                            ? root.ventilatorData.volumeWaveform
                            : []
                        frozen: root.ventilatorData ? root.ventilatorData.frozen : false
                        minimumValue: 0
                        maximumValue: 900
                        unit: "mL"
                    }
                }
            }

            // Critical vitals (right, narrow column)
            ColumnLayout {
                Layout.fillWidth: false
                Layout.preferredWidth: Metrics.px(260)
                Layout.fillHeight: true
                spacing: Spacing.screenMargin_8

                MetricTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: "Ppeak"
                    value: root.ventilatorData
                        ? root.ventilatorData.ppeak : 0
                    unit: "cmH2O"
                    state: root.ventilatorData
                        && root.ventilatorData.ppeak > root.ventilatorData.alarmHighPressure
                        ? "critical" : "normal"
                }

                MetricTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: "PEEP"
                    value: root.ventilatorData
                        ? root.ventilatorData.peep : 0
                    unit: "cmH2O"
                }

                MetricTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: "SpO2"
                    value: root.ventilatorData
                        ? root.ventilatorData.spo2 : 0
                    unit: "%"
                    state: root.ventilatorData
                        && root.ventilatorData.spo2 > 0
                        && root.ventilatorData.spo2
                            < root.ventilatorData.alarmLowSpo2
                        ? "critical" : "normal"
                }

                MetricTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: "EtCO2"
                    value: root.ventilatorData
                        ? root.ventilatorData.etco2 : 0
                    unit: "mmHg"
                    state: root.ventilatorData
                        && root.ventilatorData.etco2 > 50
                        ? "warning" : "normal"
                }

                MetricTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: "FiO2"
                    value: root.ventilatorData
                        ? root.ventilatorData.fio2 : 0
                    unit: "%"
                }

                MetricTile {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    label: "Minute Vol"
                    value: root.ventilatorData
                        ? root.ventilatorData.minuteVolume : 0
                    unit: "%"
                    state: root.ventilatorData
                        && root.ventilatorData.minuteVolume
                            > root.ventilatorData.alarmHighMv * 10
                        ? "critical" : "normal"
                }
            }
        }
    }
}
