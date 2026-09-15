pragma Singleton
// -----------------------------------------------------------------------
// File: Icons.qml
// Description: Named icon registry and priority/level icon resolvers
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Every icon in the application is addressed through this singleton, never
// by raw path. Two reasons:
//   * A typo becomes an empty string at the call site instead of a silently
//     broken Image, and the set is greppable.
//   * The resolvers at the bottom - alarm priority, battery level, patient
//     category - keep the mapping from state to glyph in one place, so the
//     shape coding stays consistent everywhere it is drawn.
//
// Shape carries meaning alongside colour: triangle = high priority,
// circle = medium, square = low. IEC 60601-1-8 does not allow colour to be
// the only channel, and roughly 8 % of male clinicians have a colour
// vision deficiency.
//
import QtQuick

QtObject {
    id: registry

    readonly property string base: "qrc:/ui/Assets/icons/"

    /** Resolves a registry name to a URL. Unknown names return "". */
    function get(name) {
        if (!name)
            return ""
        return base + name + ".svg"
    }

    // -- Navigation -------------------------------------------------------
    readonly property url monitoring: get("nav-monitoring")
    readonly property url controls:   get("nav-controls")
    readonly property url trends:     get("nav-trends")
    readonly property url loops:      get("nav-loops")
    readonly property url clinical:   get("nav-clinical")
    readonly property url system:     get("nav-system")
    readonly property url layout:     get("nav-layout")
    readonly property url target:     get("nav-target")
    readonly property url alarms:     get("nav-alarms")
    readonly property url tools:      get("nav-tools")
    readonly property url modes:      get("nav-modes")
    readonly property url settings:   get("nav-settings")
    readonly property url events:     get("nav-events")
    readonly property url standby:    get("nav-standby")

    // -- Power and connectivity -------------------------------------------
    readonly property url powerAc:      get("power-ac")
    readonly property url powerDc:      get("power-dc")
    readonly property url powerPlug:    get("power-plug")
    readonly property url powerOff:     get("power-off")
    readonly property url network:      get("network")
    readonly property url networkOff:   get("network-off")
    readonly property url lock:         get("lock")
    readonly property url unlock:       get("unlock")

    // -- Alarms -----------------------------------------------------------
    readonly property url bell:            get("alarm-bell")
    readonly property url audioPaused:     get("alarm-audio-paused")
    readonly property url audioOff:        get("alarm-audio-off")
    readonly property url alarmHigh:       get("alarm-high")
    readonly property url alarmMedium:     get("alarm-medium")
    readonly property url alarmLow:        get("alarm-low")
    readonly property url alarmResolved:   get("alarm-resolved")
    readonly property url alarmReset:      get("alarm-reset")
    readonly property url emergency:       get("emergency")

    // -- Clinical ---------------------------------------------------------
    readonly property url modeSpont:      get("mode-spont")
    readonly property url ventilatorMask: get("ventilator-mask")
    readonly property url lungs:       get("lungs")
    readonly property url patient:     get("patient")
    readonly property url heartPulse:  get("heart-pulse")
    readonly property url spo2:        get("spo2")
    readonly property url co2:         get("co2")
    readonly property url oxygen:      get("o2-cylinder")
    readonly property url circuit:     get("circuit")
    readonly property url leak:        get("leak")
    readonly property url compliance:  get("compliance")
    readonly property url resistance:  get("resistance")
    readonly property url gauge:       get("gauge")
    readonly property url waveform:    get("waveform")
    readonly property url genderMale:   get("gender-male")
    readonly property url genderFemale: get("gender-female")

    // -- Manoeuvres -------------------------------------------------------
    readonly property url holdInspiratory: get("hold-inspiratory")
    readonly property url holdExpiratory:  get("hold-expiratory")
    readonly property url manualBreath:    get("manual-breath")
    readonly property url o2Flush:         get("o2-flush")
    readonly property url nebulizer:       get("nebulizer")
    readonly property url nebulizerTimer:  get("nebulizer-timer")
    readonly property url suction:         get("suction")
    readonly property url recruitment:     get("recruitment")
    readonly property url freeze:          get("freeze")
    readonly property url calibrate:       get("calibrate")

    // -- Actions ----------------------------------------------------------
    readonly property url plus:         get("plus")
    readonly property url minus:        get("minus")
    readonly property url check:        get("check")
    readonly property url close:        get("close")
    readonly property url chevronUp:    get("chevron-up")
    readonly property url chevronDown:  get("chevron-down")
    readonly property url chevronLeft:  get("chevron-left")
    readonly property url chevronRight: get("chevron-right")
    readonly property url arrowUp:      get("arrow-up")
    readonly property url arrowDown:    get("arrow-down")
    readonly property url arrowLeft:    get("arrow-left")
    readonly property url arrowRight:   get("arrow-right")
    readonly property url play:         get("play")
    readonly property url pause:        get("pause")
    readonly property url stop:         get("stop")
    readonly property url refresh:      get("refresh")
    readonly property url save:         get("save")
    readonly property url exportIcon:   get("export")
    readonly property url importIcon:   get("import")
    // "print" is on QML's illegal-property-name list (it collides with the
    // JS global), so the registry name carries the Icon suffix like the
    // export and import entries above.
    readonly property url printIcon:    get("print")
    readonly property url trash:        get("trash")
    readonly property url edit:         get("edit")
    readonly property url search:       get("search")
    readonly property url filter:       get("filter")
    readonly property url eye:          get("eye")
    readonly property url eyeOff:       get("eye-off")
    readonly property url info:         get("info")
    readonly property url help:         get("help")
    readonly property url clock:        get("clock")
    readonly property url keypad:       get("keypad")
    readonly property url expand:       get("expand")
    readonly property url collapse:     get("collapse")
    readonly property url more:         get("more")
    readonly property url dot:          get("dot")
    readonly property url dotOutline:   get("dot-outline")
    readonly property url shieldCheck:  get("shield-check")
    readonly property url grid:         get("grid")

    // =====================================================================
    //  Resolvers
    // =====================================================================

    /**
     * Priority glyph for an alarm. Shape, not just colour, encodes rank.
     * @param priority "high" | "medium" | "low" | anything else
     */
    function alarmPriority(priority) {
        switch (String(priority).toLowerCase()) {
        case "high":
        case "critical":
            return alarmHigh
        case "medium":
        case "warning":
            return alarmMedium
        case "low":
        case "advisory":
            return alarmLow
        default:
            return alarmResolved
        }
    }

    /**
     * Battery glyph for a charge level.
     * @param percent 0-100
     * @param charging true when mains is supplying the pack
     */
    function battery(percent, charging) {
        if (charging)
            return get("battery-charging")
        if (percent >= 90) return get("battery-full")
        if (percent >= 65) return get("battery-high")
        if (percent >= 40) return get("battery-medium")
        if (percent >= 15) return get("battery-low")
        if (percent > 0)   return get("battery-critical")
        return get("battery-empty")
    }

    /**
     * Patient silhouette for a category.
     * @param category "Adult" | "Pediatric" | "Neonatal"
     */
    function patientCategory(category) {
        switch (String(category).toLowerCase()) {
        case "pediatric":
        case "paediatric":
        case "ped":
            return get("patient-pediatric")
        case "neonatal":
        case "neonate":
        case "neo":
            return get("patient-neonatal")
        default:
            return get("patient-adult")
        }
    }

    /**
     * Trend arrow for a direction.
     * @param direction "rising" | "falling" | anything else
     */
    function trend(direction) {
        switch (String(direction).toLowerCase()) {
        case "rising":
        case "up":
            return get("trend-up")
        case "falling":
        case "down":
            return get("trend-down")
        default:
            return get("trend-flat")
        }
    }

    /** Waveform channel glyph by signal name. */
    function channel(signalName) {
        switch (String(signalName).toLowerCase()) {
        case "co2":
        case "etco2":
            return co2
        case "spo2":
            return spo2
        case "o2":
        case "fio2":
            return oxygen
        default:
            return waveform
        }
    }
}
