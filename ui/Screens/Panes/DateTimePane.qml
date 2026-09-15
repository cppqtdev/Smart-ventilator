// -----------------------------------------------------------------------
// File: DateTimePane.qml
// Description: System > Settings > Day and Time - sets the device clock
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The rings hold a pending date that is only committed on Apply, so a half
// entered date never becomes the device clock. Every event and alarm carries
// this clock, which is why the change is written to the log.
//
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: pane

    property var clock

    property int pendingYear: 2026
    property int pendingMonth: 1
    property int pendingDay: 1
    property int pendingHour: 0
    property int pendingMinute: 0

    property bool edited: false
    property string feedback: ""

    // Three spinners and two gaps across, two rows and the summary down.
    // The pane is given far more room than the reference screen has, so the
    // ring grows into it rather than leaving the block in a corner.
    readonly property int spinnerGap: Metrics.px(34)

    // Each spin box is the ring plus a step button and its gap either side.
    readonly property real spinnerRatio: 1.842

    readonly property int ringFromWidth:
        Math.floor((pane.width - pane.spinnerGap * 2) / 3 / pane.spinnerRatio)

    readonly property int ringFromHeight:
        Math.floor((pane.height - Metrics.px(150)) / 2) - Metrics.px(34)

    readonly property int ringSize:
        Math.max(Metrics.px(84),
                 Math.min(Metrics.px(140),
                          Math.min(pane.ringFromWidth, pane.ringFromHeight)))

    readonly property int daysThisMonth: pane.clock
        ? pane.clock.daysInMonth(pane.pendingYear, pane.pendingMonth) : 31

    function two(number) {
        return number < 10 ? "0" + number : String(number)
    }

    readonly property string pendingDate:
        "%1/%2/%3".arg(pane.two(pane.pendingDay)).arg(pane.two(pane.pendingMonth)).arg(pane.pendingYear)

    readonly property string pendingTime:
        "%1:%2".arg(pane.two(pane.pendingHour)).arg(pane.two(pane.pendingMinute))

    function loadFromClock() {
        if (!pane.clock)
            return
        pane.pendingYear = pane.clock.year
        pane.pendingMonth = pane.clock.month
        pane.pendingDay = pane.clock.day
        pane.pendingHour = pane.clock.hour
        pane.pendingMinute = pane.clock.minute
        pane.edited = false
        pane.feedback = ""
    }

    function touched() {
        pane.edited = true
        pane.feedback = ""
        if (pane.pendingDay > pane.daysThisMonth)
            pane.pendingDay = pane.daysThisMonth
    }

    Component.onCompleted: pane.loadFromClock()
    onClockChanged: pane.loadFromClock()

    ColumnLayout {
        anchors.fill: parent
        spacing: Metrics.px(30)

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: pane.spinnerGap

            RingSpinner {
                ringSize: pane.ringSize
                label: qsTr("Hour")
                from: 0
                to: 23
                value: pane.pendingHour
                onValueSet: function (newValue) {
                    pane.pendingHour = newValue
                    pane.touched()
                }
            }

            RingSpinner {
                ringSize: pane.ringSize
                label: qsTr("Minute")
                from: 0
                to: 59
                value: pane.pendingMinute
                onValueSet: function (newValue) {
                    pane.pendingMinute = newValue
                    pane.touched()
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: pane.spinnerGap

            RingSpinner {
                ringSize: pane.ringSize
                label: qsTr("Day")
                from: 1
                to: pane.daysThisMonth
                value: pane.pendingDay
                onValueSet: function (newValue) {
                    pane.pendingDay = newValue
                    pane.touched()
                }
            }

            RingSpinner {
                ringSize: pane.ringSize
                label: qsTr("Month")
                from: 1
                to: 12
                value: pane.pendingMonth
                onValueSet: function (newValue) {
                    pane.pendingMonth = newValue
                    pane.touched()
                }
            }

            RingSpinner {
                ringSize: pane.ringSize
                label: qsTr("Year")
                from: 2020
                to: 2099
                wrap: false
                value: pane.pendingYear
                onValueSet: function (newValue) {
                    pane.pendingYear = newValue
                    pane.touched()
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Metrics.px(28)

            ColumnLayout {
                Layout.fillWidth: false
                spacing: Spacing.xs

                Text {
                    text: qsTr("Date: %1").arg(pane.pendingDate)
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.readoutLabel
                }

                Text {
                    text: qsTr("Time: %1").arg(pane.pendingTime)
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.readoutLabel
                }
            }

            AppButton {
                Layout.preferredWidth: Metrics.actionButtonWidth
                Layout.preferredHeight: Metrics.actionButtonHeight
                text: qsTr("Apply")
                buttonVariant: AppButton.Success
                enabled: pane.edited
                onClicked: {
                    if (!pane.clock)
                        return
                    var ok = pane.clock.applyDateTime(pane.pendingYear, pane.pendingMonth,
                                                      pane.pendingDay, pane.pendingHour,
                                                      pane.pendingMinute)
                    pane.feedback = ok ? qsTr("Device clock set")
                                       : qsTr("That date does not exist")
                    if (ok)
                        pane.edited = false
                }
            }

            AppButton {
                Layout.preferredWidth: Metrics.actionButtonWidth
                Layout.preferredHeight: Metrics.actionButtonHeight
                text: qsTr("Revert")
                buttonVariant: AppButton.Secondary
                enabled: pane.edited
                onClicked: pane.loadFromClock()
            }

            Text {
                Layout.preferredWidth: Metrics.px(170)
                text: pane.feedback
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.caption
                wrapMode: Text.WordWrap
            }
        }

        Item { Layout.fillHeight: true }
    }
}
