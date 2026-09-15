// -----------------------------------------------------------------------
// File: ControlsPane.qml
// Description: Controls screen - Basic settings and Patient data
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The reference draws this screen as one card holding a three by three grid.
// The page switch sits in the first cell, so the dials start in the same
// column the switch occupies. Every cell is placed by row and column rather
// than by flow order, because the two pages fill different cells and an
// auto-flowing grid would reshuffle when one of them is hidden.
//
import QtQuick
import QtQuick.Layouts
import "../../Components"
import "../../Controls"
import "../../Theme"
import "../../Dialogs"

Item {
    id: pane

    property var presenter
    property var patientData
    property var ventilatorData

    property string page: "basic"

    // Flow or pressure triggering. The reference lights F and leaves P
    // dark; a pair that cannot be switched is decoration, so it is a real
    // choice here even though the controller takes a flow threshold either
    // way today.
    property bool triggerByFlow: true

    signal settingRequested(string key, real value)

    readonly property bool basicPage: pane.page === "basic"

    // Keys match VentilatorController::requestParameterChange, and the ranges
    // match the bounds it clamps to. A key or a range it does not share is
    // refused on every step.
    readonly property var basicDials: [
        { key: "inspiratoryTime", label: qsTr("Insp. time"),  value: pane.settingValue("inspiratoryTime", 1), from: 1,  to: 5,   step: 1, unit: "s" },
        { key: "fio2",            label: qsTr("Oxygen"),      value: pane.settingValue("fio2", 60),           from: 21, to: 100, step: 1, unit: "%" },
        { key: "peep",            label: qsTr("PEEP C/PAP"),  value: pane.settingValue("peep", 15),           from: 0,  to: 30,  step: 1, unit: "cmH2O" },
        { key: "pressureSupport", label: qsTr("Psupport"),    value: pane.settingValue("pressureSupport", 12), from: 0, to: 40,  step: 1, unit: "cmH2O" },
        { key: "trigger",         label: qsTr("Trigger"),     value: pane.settingValue("trigger", 5),          from: 1, to: 10,  step: 1, unit: "l/min" },
        { key: "minuteVolume",    label: qsTr("%MinVol"),     value: pane.settingValue("minuteVolume", 110),   from: 20, to: 400, step: 5, unit: "%" },
        { key: "respiratoryRate", label: qsTr("Rate"),        value: pane.settingValue("respiratoryRate", 14), from: 4, to: 60,  step: 1, unit: "1/min" }
    ]

    function settingValue(key, fallback) {
        if (!pane.ventilatorData)
            return fallback
        var value = pane.ventilatorData[key]
        return value === undefined ? fallback : value
    }

    // The page switch appears on both pages. On Basic it is the first cell of
    // the grid, which is what puts the dials in the same column as it; on
    // Patient the grid is not used at all, because a three column grid whose
    // other two columns hold nothing gives them no width and squashes
    // anything spanning them.
    component PageSwitch: ColumnLayout {
        id: switcher

        property bool basicChecked: true

        signal chose(string page)

        spacing: Metrics.px(22)

        AppButton {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.actionButtonHeight
            text: qsTr("Basic")
            buttonVariant: AppButton.Success
            selected: switcher.basicChecked
            onClicked: switcher.chose("basic")
        }

        AppButton {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.actionButtonHeight
            text: qsTr("Patient")
            buttonVariant: AppButton.Success
            selected: !switcher.basicChecked
            onClicked: switcher.chose("patient")
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Radius.medium
        color: Colors.surface
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: Metrics.px(20)
        columns: 3
        rowSpacing: Metrics.px(82)
        columnSpacing: Metrics.px(78)
        visible: pane.basicPage

        PageSwitch {
            Layout.row: 0
            Layout.column: 0
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Layout.fillHeight: false
            basicChecked: pane.basicPage
            onChose: function (page) { pane.page = page }
        }

        Repeater {
            model: pane.basicDials

            delegate: DialControl {
                required property var modelData
                required property int index

                Layout.row: Math.floor((index + 1) / 3)
                Layout.column: (index + 1) % 3
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.fillHeight: false

                label: modelData.label
                value: modelData.value
                from: modelData.from
                to: modelData.to
                stepSize: modelData.step
                unit: modelData.unit

                onStepRequested: function (proposed) {
                    pane.settingRequested(modelData.key, proposed)
                }
            }
        }

        RowLayout {
            Layout.row: 2
            Layout.column: 2
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Layout.fillHeight: false
            spacing: Spacing.md

            ChipButton {
                text: qsTr("F")
                selected: pane.triggerByFlow
                onClicked: pane.triggerByFlow = true

            }

            ChipButton {
                text: qsTr("P")
                selected: !pane.triggerByFlow
                onClicked: pane.triggerByFlow = false

            }

            Item { Layout.fillWidth: true }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Metrics.px(20)
        spacing: Metrics.px(20)
        visible: !pane.basicPage

        ColumnLayout {
            Layout.fillWidth: false
            Layout.preferredWidth: Metrics.pageTabWidth
            Layout.alignment: Qt.AlignTop
            spacing: Spacing.xl

            PageSwitch {
                Layout.fillWidth: true
                basicChecked: pane.basicPage
                onChose: function (page) { pane.page = page }
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: Metrics.px(40)
                text: qsTr("Ventilation\nTime")
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.bodyLarge
                font.weight: Typography.bold
                horizontalAlignment: Text.AlignHCenter
                lineHeight: Typography.lineTight
            }

            Text {
                Layout.fillWidth: true
                text: pane.ventilatorData ? pane.ventilatorData.ventilationTime : "0:0:0"
                color: Colors.textPrimary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.title
                font.weight: Typography.bold
                horizontalAlignment: Text.AlignHCenter
            }

            AppButton {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Metrics.actionButtonWidth
                Layout.preferredHeight: Metrics.actionButtonHeight
                text: qsTr("Reset")
                buttonVariant: AppButton.Success
                enabled: pane.ventilatorData
                onClicked: resetTimeConfirm.open()
            }

            Item { Layout.fillHeight: true }
        }

        PatientDataPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: Metrics.px(432)
            Layout.alignment: Qt.AlignTop

            patientData: pane.patientData
            ventilatorData: pane.ventilatorData
            onSettingRequested: function (key, value) {
                pane.settingRequested(key, value)
            }
        }
    }

    ConfirmDialog {
        id: resetTimeConfirm

        titleText: qsTr("Reset ventilation time")
        messageText: qsTr("Clear the elapsed ventilation time for this patient? Ventilation is not affected.")
        confirmText: qsTr("Reset")

        onConfirmed: {
            if (pane.ventilatorData)
                pane.ventilatorData.resetVentilationTime()
        }
    }
}
