// -----------------------------------------------------------------------
// File: ScreenShell.qml
// Description: The chrome every reference screen shares
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Every screen in reference/ carries the same four fixed regions and differs
// only in the centre. Building that once means a change to the header or the
// rail lands on all of them at the same time.
//
import QtQuick
import QtQuick.Layouts
import "../Controls"
import "../Theme"

Item {
    id: shell

    property var presenter
    property string destination: "monitoring"
    property bool showRail: true

    default property alias content: centre.data

    signal navigate(string destination)
    signal patientRequested()
    signal modeRequested()
    signal alarmRequested(int index)
    signal settingActivated(string key)
    signal lockRequested()

    readonly property bool frozen: shell.presenter ? shell.presenter.frozen : false

    readonly property var navigationTabs: [
        { key: "monitoring", label: qsTr("Monitoring") },
        { key: "controls",   label: qsTr("Controls") },
        { key: "system",     label: qsTr("System") },
        { key: "layout",     label: qsTr("Layout") },
        { key: "events",     label: qsTr("Events") },
        { key: "alarms",     label: qsTr("Alarms") },
        { key: "tools",      label: qsTr("Tools") },
        { key: "modes",      label: qsTr("Modes") }
    ]

    function tabIndexFor(key) {
        for (var i = 0; i < shell.navigationTabs.length; ++i) {
            if (shell.navigationTabs[i].key === key)
                return i
        }
        return 0
    }

    Rectangle {
        anchors.fill: parent
        color: Colors.background
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Metrics.screenPadding
        spacing: Metrics.gutter

        HomeHeader {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.headerHeight

            mode: shell.presenter ? shell.presenter.mode : "---"
            patientCategory: shell.presenter ? shell.presenter.patient.category : "adult"
            nonInvasive: shell.presenter ? shell.presenter.nonInvasive : false
            spontaneous: shell.presenter ? shell.presenter.spontaneous : false
            alarms: shell.presenter ? shell.presenter.banner : []

            onModeClicked: shell.modeRequested()
            onPatientClicked: shell.patientRequested()
            onLockRequested: shell.lockRequested()
            onAlarmActivated: function (index) { shell.alarmRequested(index) }
        }

        // A technical fault takes a row of its own. As an overlay it sat on
        // top of the header and hid the clock and the ventilation mode, which
        // is the last thing to hide when something has gone wrong.
        SystemStatusBanner {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.px(36)
            databaseData: databaseManager
            ventilatorData: ventilatorController
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Metrics.gutter

            ColumnLayout {
                Layout.fillWidth: false
                Layout.preferredWidth: Metrics.sidebarWidth
                Layout.fillHeight: true
                spacing: Metrics.tileGap

                Repeater {
                    model: shell.presenter ? shell.presenter.sidebarTiles : []

                    delegate: MetricTile {
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        label: modelData.label
                        value: modelData.value
                        unit: modelData.unit
                        upperLimit: modelData.upper
                        lowerLimit: modelData.lower
                    }
                }
            }

            Item {
                id: centre
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            ControlRail {
                Layout.preferredWidth: Metrics.railWidth
                Layout.fillHeight: true
                visible: shell.showRail

                dials: shell.presenter ? shell.presenter.dials : []
                frozen: shell.frozen
                ventilating: shell.presenter ? shell.presenter.ventilating : false

                onVentilationToggled: function (start) {
                    if (!shell.presenter)
                        return
                    if (start) {
                        if (shell.presenter.requestStart())
                            shell.navigate("monitoring")
                    } else {
                        shell.presenter.requestStop()
                        shell.navigate("standby")
                    }
                }

                onFreezeToggled: {
                    if (shell.presenter)
                        shell.presenter.toggleFreeze()
                }
                onSettingRequested: function (key, value) {
                    if (shell.presenter)
                        shell.presenter.requestSetting(key, value)
                }
                onSettingActivated: function (key) { shell.settingActivated(key) }
            }
        }

        AppTabBar {
            Layout.fillWidth: true
            Layout.preferredHeight: Metrics.navHeight
            model: shell.navigationTabs
            currentIndex: shell.tabIndexFor(shell.destination)
            onActivated: function (index, key) { shell.navigate(key) }
        }
    }
}
