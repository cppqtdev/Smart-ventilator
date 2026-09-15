pragma ComponentBehavior: Bound
// -----------------------------------------------------------------------
// File: main.qml
// Description: Root ApplicationWindow with screen routing and controller bindings
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Basic
import QtMultimedia

import "ui/Theme"
import "ui/Components"
import "ui/Controls"
import "ui/Screens"

ApplicationWindow {
    id: root
    width: 1920
    height: 1080
    minimumWidth: 1366
    minimumHeight: 768
    visible: true
    color: Colors.background
    title: qsTr("ICU Smart Ventilator Demo")

    property var patientModel: patientController
    property var ventilatorModel: ventilatorController
    property var alarmModel: alarmController
    property var calibrationModel: calibrationService

    // Which System sub-tab to open on. Standby points it at the pre-use check.
    property int systemPage: 0
    property var eventModel: eventController
    property string currentScreen: "standby"
    // These ten share one ScreenShell, built once below. The window chrome
    // steps aside for them because the shell draws its own.
    readonly property var shellScreens: ["standby", "patient", "monitoring", "controls",
                                         "system", "layout", "events",
                                         "alarms", "tools", "modes"]
    readonly property bool homeActive: root.shellScreens.indexOf(root.currentScreen) >= 0

    // Standby and patient setup are reached from the monitoring and controls
    // tabs, so those are the tabs that stay lit while the operator is on them.
    readonly property string activeTab:
          root.currentScreen === "standby" ? "monitoring"
        : root.currentScreen === "patient" ? "controls"
        : root.currentScreen

    // Controls fills the width with its own dials and has no room for the rail.
    readonly property bool railVisible: root.currentScreen !== "controls"

    // Going to monitoring means starting ventilation. The rule lives here so
    // the tab bar, the bottom bar and the rail's own start button cannot
    // drift apart.
    function goTo(destination) {
        if (destination === "monitoring" && !root.ventilatorModel.running) {
            if (!root.ventilatorModel.requestStartVentilation())
                return
        }
        root.currentScreen = destination
    }
    property bool ventilationActive: ventilatorModel.running
    property bool splashActive: true
    property string clinicalAutomationStatus: ""

    // Normalised highest active alarm priority - "high" | "medium" | "low" |
    // "none". Everything that annunciates reads this one value so the two
    // IEC 60601-1-8 indicators can never disagree with each other.
    readonly property string alarmPriority: alarmModel
        ? (alarmModel.highestPriority || "none") : "none"

    // Metrics is a plain QtObject singleton and cannot read the Screen
    // attached property itself, so the panel geometry is pushed in here.
    // Touch targets are computed in millimetres from this - see Metrics.qml.
    function syncMetrics() {
        Metrics.windowWidth = root.width
        Metrics.windowHeight = root.height
        if (Screen.pixelDensity > 0)
            Metrics.pixelsPerMm = Screen.pixelDensity
    }

    onWidthChanged: root.syncMetrics()
    onHeightChanged: root.syncMetrics()

    function updateColorMode() {
        var mode = appSettings.dayNightMode
        var hour = new Date().getHours()
        var nightStart = appSettings.nightStartHour
        var dayStart = appSettings.dayStartHour
        var scheduledNight = nightStart > dayStart
            ? (hour >= nightStart || hour < dayStart)
            : (hour >= nightStart && hour < dayStart)
        Colors.nightMode = mode === "Night"
            || (mode === "Automatic" && scheduledNight)
    }

    function navigateToScreen() {
        if (root.currentScreen === "standby")
            return standbyScreen
        if (root.currentScreen === "patient")
            return patientScreen
        if (root.currentScreen === "modes")
            return modeScreen
        if (root.currentScreen === "controls")
            return controlsScreen
        if (root.currentScreen === "alarms")
            return alarmScreen
        if (root.currentScreen === "system")
            return systemScreen
        if (root.currentScreen === "events")
            return eventsScreen
        if (root.currentScreen === "tools")
            return toolsScreen
        if (root.currentScreen === "layout")
            return layoutScreen
        if (root.currentScreen === "shutdown")
            return shutdownScreen
        if (root.currentScreen === "emergency")
            return emergencyScreen
        return homeScreen
    }

    header: AppHeader {
        id: header
        visible: !root.splashActive && !root.homeActive
        width: parent.width

        alarmData: root.alarmModel
        clockData: clockController
        batteryData: root.ventilatorModel
        patientData: root.patientModel

        alarmPriority: root.alarmPriority
        mode: root.ventilatorModel.mode
        modeDescription: root.ventilatorModel.modeDescription !== undefined
            ? root.ventilatorModel.modeDescription : ""
        nonInvasive: root.ventilatorModel.nonInvasive === true
        patientCategory: root.patientModel.category
        automationStatus: root.clinicalAutomationStatus
        ventilating: root.ventilationActive
        elapsedText: root.ventilatorModel.ventilationTime
        screenLocked: screenLock.locked

        onAlarmCentreRequested: root.currentScreen = "alarms"
        onAudioPauseRequested: {
            if (root.alarmModel.audioPaused)
                root.alarmModel.resumeAudio()
            else
                root.alarmModel.pauseAudio(root.alarmModel.audioPauseMaxSeconds)
        }
        onAlarmResetRequested: root.alarmModel.resetLatched()
        onLockRequested: screenLock.lockNow()
        onModeRequested: root.currentScreen = "modes"
        onPatientRequested: root.currentScreen = "patient"
        onEmergencyRequested: root.currentScreen = "emergency"
        onShutdownRequested: root.currentScreen = "shutdown"
    }

    Control {
        id: contentArea
        anchors.fill: parent
        leftPadding: root.homeActive ? 0 : Spacing.screenMargin
        rightPadding: root.homeActive ? 0 : Spacing.screenMargin
        topPadding: root.homeActive ? 0 : Spacing.panelGap
        bottomPadding: root.homeActive ? 0 : Spacing.panelGap

        contentItem: Item {
            // The shared chrome is built once and stays put. Only the centre
            // is reloaded when the operator changes tab, so the header,
            // sidebar, rail and tab bar keep their state and their bindings
            // instead of being torn down and rebuilt on every press - which
            // is what made the whole window blink.
            ScreenShell {
                id: shell
                anchors.fill: parent
                visible: root.homeActive && !root.splashActive

                presenter: monitoringPresenter
                destination: root.activeTab
                showRail: root.railVisible

                onNavigate: function (destination) { root.goTo(destination) }
                onModeRequested: root.currentScreen = "modes"
                onPatientRequested: root.currentScreen = "patient"
                onAlarmRequested: root.currentScreen = "alarms"
                onSettingActivated: root.currentScreen = "controls"
                onLockRequested: screenLock.lockNow()

                Loader {
                    anchors.fill: parent
                    active: root.homeActive && !root.splashActive
                    // Synchronous: the pane on its own is cheap, and an
                    // asynchronous load shows an empty centre while it builds.
                    asynchronous: false
                    sourceComponent: root.navigateToScreen()
                }
            }

            Loader {
                id: screenLoader
                anchors.fill: parent
                active: !root.homeActive && !root.splashActive
                asynchronous: false
                sourceComponent: root.navigateToScreen()
            }
        }
    }

    // IEC 60601-1-8 clause 6.3.2.2 requires two distinct visual alarm
    // indicators. This is the one that has to be perceivable at 4 m: a
    // full-width field of the highest active priority, above everything
    // else on the screen including the header. The one legible at 1 m,
    // which names the specific condition, lives inside AppHeader.
    AlarmStrip {
        id: alarmStrip
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        z: 1000
        visible: !root.splashActive
        priority: root.alarmPriority
        audioPaused: root.alarmModel ? root.alarmModel.audioPaused : false
    }

    CommandToast {
        id: commandToast
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: Metrics.screenPadding
        anchors.rightMargin: Metrics.screenPadding
        anchors.bottomMargin: Metrics.screenPadding
                              + (root.homeActive ? Metrics.navHeight + Metrics.gutter : 0)
        z: 950
    }

    Connections {
        target: root.ventilatorModel
        function onCommandRejected(message) {
            commandToast.show(message, true)
        }
    }

    footer: BottomNavigation {
        id: bottomNav
        visible: !root.splashActive && !root.homeActive
        currentScreen: root.currentScreen
        ventilating: root.ventilationActive
        activeAlarmCount: root.alarmModel ? root.alarmModel.activeCount : 0
        alarmPriority: root.alarmPriority
        onNavigate: function (screen) { root.goTo(screen) }
    }

    SplashScreen {
        anchors.fill: parent
        visible: root.splashActive
        softwareVersion: appSettings.softwareVersion
        operatingHours: appSettings.operatingHours

        // Live conditions, not a script. Each one is something that is either
        // true or not by the time the screen hands over.
        storageReady: databaseManager.ready && !databaseManager.readOnly
        operatorsReady: userController.lockTimeoutSeconds > 0
        alarmsReady: root.alarmModel !== undefined && root.alarmModel !== null
        deviceReady: typeof telemetryBridge !== "undefined" && telemetryBridge !== null
                     ? telemetryBridge.linkUp
                     : !root.ventilatorModel.degradedMode
        sessionReady: root.ventilatorModel.patientAccepted

        onFinished: root.splashActive = false
    }

    Component.onCompleted: {
        root.syncMetrics()
        root.updateColorMode()
    }

    Connections {
        target: appSettings
        function onDayNightModeChanged() {
            root.updateColorMode()
        }
        function onDayNightScheduleChanged() {
            root.updateColorMode()
        }
    }

    Timer {
        interval: 60000
        running: appSettings.dayNightMode === "Automatic"
        repeat: true
        onTriggered: root.updateColorMode()
    }

    // IEC 60601-1-8 alarm audio: always alive in the root window so the
    // alarm tone plays regardless of which screen is currently loaded.
    MediaPlayer {
        id: alarmAudioPlayer
        source: "qrc:/ui/Assets/alarm_tone.wav"
        loops: MediaPlayer.Infinite
        audioOutput: AudioOutput {
            volume: root.alarmModel.audioActive ? appSettings.audioVolume / 100.0 : 0.0
        }
    }

    Connections {
        target: root.alarmModel
        function onAudioChanged() {
            if (root.alarmModel.audioActive)
                alarmAudioPlayer.play()
            else
                alarmAudioPlayer.stop()
        }
    }

    // Global interaction detector to reset the screen lock timer.
    MouseArea {
        anchors.fill: parent
        z: 999
        propagateComposedEvents: true
        onPressed: function(mouse) {
            screenLock.resetTimer()
            mouse.accepted = false
        }
    }

    ScreenLockOverlay {
        id: screenLock
        anchors.fill: parent
        timeoutSeconds: userController.lockTimeoutSeconds
        users: userController
        userName: userController.currentUser.length > 0
                  ? userController.currentUser : "clinician"
        onUnlocked: root.ventilatorModel.operatorId = userController.currentUser
    }

    Component {
        id: standbyScreen
        StandbyScreen {
            presenter: monitoringPresenter
            patientData: patientModel
            ventilatorData: ventilatorModel
            onStartRequested: {
                if (root.ventilatorModel.requestStartVentilation())
                    root.currentScreen = "monitoring"
            }
            // The pre-use check lives on the System screen. Standby sends the
            // operator straight to it rather than to a legacy routine that
            // ran nothing and left them on the patient page.
            onSetupRequested: {
                root.systemPage = 1
                root.currentScreen = "system"
            }
        }
    }

    Component {
        id: patientScreen
        PatientSetupScreen {
            presenter: monitoringPresenter
            patientData: patientModel
            catalog: ventilationCatalog
            onContinueRequested: root.currentScreen = "modes"
        }
    }

    Component {
        id: modeScreen
        ModeSelectionScreen {
            presenter: monitoringPresenter
            ventilatorData: ventilatorModel
            catalog: ventilationCatalog
            onModeConfirmed: function (mode) {
                if (root.ventilatorModel.requestModeChange(mode))
                    root.currentScreen = "monitoring"
            }
            onCancelled: root.currentScreen = "monitoring"
        }
    }

    Component {
        id: homeScreen
        HomeScreen {
            presenter: monitoringPresenter
            settingsData: appSettings
        }
    }

    Component {
        id: controlsScreen
        ControlsScreen {
            presenter: monitoringPresenter
            patientData: root.patientModel
            ventilatorData: ventilatorModel
            onSettingRequested: function (key, value) {
                root.ventilatorModel.requestParameterChange(key, Math.round(value))
            }
        }
    }

    Component {
        id: alarmScreen
        AlarmCenterScreen {
            presenter: monitoringPresenter
            alarmData: alarmModel
            ventilatorData: ventilatorModel
            onLimitRequested: function (key, value) {
                root.ventilatorModel.requestAlarmLimitChange(key, Math.round(value))
            }
        }
    }

    Component {
        id: systemScreen
        SystemScreen {
            presenter: monitoringPresenter
            settingsData: appSettings
            ventilatorData: ventilatorModel
            calibrationService: root.calibrationModel
            clockData: clockController
            initialPage: root.systemPage

            // The pane captures initialPage in its own onCompleted, which runs
            // first, so clearing it here means the next visit opens on Info.
            Component.onCompleted: root.systemPage = 0
        }
    }

    Component {
        id: eventsScreen
        EventsScreen {
            presenter: monitoringPresenter
            eventData: eventModel
            logData: typeof logBuffer !== "undefined" ? logBuffer : null
        }
    }

    Component {
        id: toolsScreen
        ToolsScreen {
            presenter: monitoringPresenter
            ventilatorData: ventilatorModel
            alarmData: root.alarmModel
            onInspiratoryHoldRequested: root.ventilatorModel.performInspiratoryHold()
            onExpiratoryHoldRequested: root.ventilatorModel.performExpiratoryHold()
            onSettingsRequested: root.currentScreen = "alarms"
        }
    }

    Component {
        id: layoutScreen
        LayoutScreen {
            presenter: monitoringPresenter
            settingsData: appSettings
        }
    }

    Component {
        id: shutdownScreen
        ShutdownScreen {
            ventilatorData: ventilatorModel
            alarmData: alarmModel
            onShutdownConfirmed: root.currentScreen = "standby"
            onShutdownCancelled: root.currentScreen = "monitoring"
        }
    }

    Component {
        id: emergencyScreen
        EmergencyScreen {
            ventilatorData: ventilatorModel
            alarmData: alarmModel
            onExitEmergency: root.currentScreen = "monitoring"
        }
    }

}
