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

    readonly property int daysThisMonth: pane.clock
        ? pane.clock.daysInMonth(pane.pendingYear, pane.pendingMonth) : 31

    function two(number) {
        return number < 10 ? "0" + number : String(number)
    }

    readonly property string pendingDate:
        "%1-%2-%3".arg(pane.pendingYear).arg(pane.two(pane.pendingMonth)).arg(pane.two(pane.pendingDay))

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
        spacing: Spacing.xl

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Metrics.px(48)

            RingSpinner {
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
            spacing: Metrics.px(48)

            RingSpinner {
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
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
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
                Layout.fillWidth: true
                text: pane.feedback
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.caption
                elide: Text.ElideRight
            }
        }

        Item { Layout.fillHeight: true }
    }
}
