pragma Singleton
// -----------------------------------------------------------------------
// File: Colors.qml
// Description: Colour tokens sampled from reference/home-monitoring.png
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick

QtObject {
    id: palette

    property bool nightMode: false

    // NOTE: alarm colours are fixed by IEC 60601-1-8 Table 2 and must not
    // follow nightMode or any theme switch.
    property string gasColourStandard: "iso"

    readonly property color transparent: "transparent"

    readonly property color background:     nightMode ? "#140D06" : "#212736"
    readonly property color surfaceSunken:  nightMode ? "#1A1108" : "#1B2130"
    readonly property color surface:        nightMode ? "#241A0D" : "#242C3F"
    readonly property color surfaceRaised:  nightMode ? "#3A2A14" : "#2C3547"
    readonly property color surfaceOverlay: nightMode ? "#4A3A24" : "#515666"
    readonly property color scrim:          "#B3000000"

    readonly property color line:        nightMode ? "#4A3A20" : "#364352"
    readonly property color lineStrong:   nightMode ? "#6A5330" : "#47536A"
    readonly property color lineSubtle:   nightMode ? "#33250F" : "#2C3547"
    readonly property color border:       line
    readonly property color focusRing:    nightMode ? "#F0A63C" : "#8FBBFF"

    readonly property color brand:          nightMode ? "#E0A54B" : "#458CF2"
    readonly property color accent:         brand
    readonly property color accentHover:    nightMode ? "#F2BA65" : "#5A9BF5"
    readonly property color accentPressed:  nightMode ? "#C48A32" : "#2F6FD0"
    readonly property color accentSubtle:   nightMode ? "#4A3418" : "#3161A8"
    readonly property color accentOn:       "#FFFFFF"

    readonly property color success:        nightMode ? "#C8A24A" : "#5BC68A"
    readonly property color successSubtle:  nightMode ? "#3A2E12" : "#439567"
    readonly property color warning:        "#E9B24A"
    readonly property color warningSubtle:  nightMode ? "#4A3410" : "#6E5522"
    readonly property color danger:         "#9E3D40"
    readonly property color dangerSubtle:   nightMode ? "#3E1A1B" : "#5A2426"
    readonly property color info:           accent
    readonly property color infoSubtle:     accentSubtle
    readonly property color neutral:        nightMode ? "#B9A88C" : "#A6ACB4"

    readonly property color alarmHigh:          "#9E3D40"
    readonly property color alarmHighSurface:   "#5A2426"
    readonly property color alarmHighOn:        "#FFFFFF"
    readonly property color alarmMedium:        "#E9B24A"
    readonly property color alarmMediumSurface: "#6E5522"
    readonly property color alarmMediumOn:      "#1B2130"
    readonly property color alarmLow:           "#4FC3E8"
    readonly property color alarmLowSurface:    "#1E4A59"
    readonly property color alarmLowOn:         "#04141A"
    readonly property color alarmNone:          nightMode ? "#3A2A14" : "#2C3547"
    readonly property color alarmSilenced:      neutral

    readonly property color wavePressure: "#65D39F"
    readonly property color waveFlow:     "#CF7ED1"
    readonly property color waveVolume:   "#E0B24A"
    readonly property color waveCo2:      "#4F9BE8"
    readonly property color waveSpo2:     "#65D39F"
    readonly property color wavePes:      "#A6ACB4"

    readonly property color wavePlotBackground: surface
    readonly property color waveGrid:           "#364352"
    readonly property color waveGridMajor:      "#47536A"
    readonly property color waveBaseline:       "#47536A"
    readonly property color waveLimitHigh:      alarmHigh
    readonly property color waveLimitLow:       alarmLow
    readonly property color waveCursor:         "#FFFFFF"

    // ISO 32572 / ISO 5359 assign white to oxygen; the United States uses
    // green. gasColourStandard picks between them.
    readonly property color oxygen: palette.gasColourStandard === "us" ? "#5BC68A" : "#E8EDF5"

    readonly property color textPrimary:   "#FFFFFF"
    readonly property color textSecondary: nightMode ? "#C6AE86" : "#99A0B0"
    readonly property color textMuted:     nightMode ? "#8E7A57" : "#6B7488"
    readonly property color textDisabled:  nightMode ? "#6A5A40" : "#525A6C"
    readonly property color textInverse:   "#1B2130"
    readonly property color textValue:     textPrimary
    readonly property color textLabel:     textPrimary
    readonly property color textUnit:      textSecondary

    readonly property color controlSurface:        nightMode ? "#3A2A14" : "#3161A8"
    readonly property color controlSurfaceHover:   nightMode ? "#4A3720" : "#3A72C4"
    readonly property color controlSurfacePressed: nightMode ? "#2A1E0E" : "#2F6FD0"
    readonly property color controlDisabled:       nightMode ? "#4A4438" : "#898F9B"
    readonly property color controlSelected:       accent
    readonly property color controlTrack:          nightMode ? "#3A3228" : "#3C4557"
    readonly property color controlThumb:          "#FFFFFF"

    readonly property color pendingValue:   warning
    readonly property color pendingSurface: warningSubtle

    readonly property color glassTopEdge:    nightMode ? "#4A3418" : "#2B4E85"
    readonly property color glassTop:        nightMode ? "#33230F" : "#1B3568"
    readonly property color glassMid:        nightMode ? "#291C0C" : "#16295E"
    readonly property color glassGloss:      nightMode ? "#3D2A12" : "#24406A"
    readonly property color glassBottom:     nightMode ? "#1E1408" : "#111F4A"
    readonly property color glassRim:        nightMode ? "#E0A54B" : "#4FA8F0"
    readonly property color glassRimBright:  nightMode ? "#FFD79A" : "#9FDCFF"
    readonly property color glassGlow:       nightMode ? "#D98B24" : "#3D8BFD"
    readonly property color glassTextOn:     nightMode ? "#FFE7BF" : "#EAF4FF"
    readonly property color glassTint:       nightMode ? "#FF9A3D" : "#4A7BFF"
    readonly property color glassHairline:   nightMode ? "#F0C489" : "#8FB4E8"
    readonly property color glassStreak:     "#FFFFFF"
    readonly property color glassStreakWide: nightMode ? "#FFB65C" : "#5FA7F9"

    readonly property color accentBlue:         accent
    readonly property color accentBlueDark:     accentPressed
    readonly property color accentBlueMedium:   accentSubtle
    readonly property color accentBlueSelected: accent
    readonly property color accentCyan:         alarmLow
    readonly property color critical:           alarmHigh
    readonly property color criticalBackground: alarmHighSurface
    readonly property color warningBackground:  alarmMediumSurface
    readonly property color disabled:           controlDisabled
    readonly property color track:              controlTrack
    readonly property color progressBlue:       accent
    readonly property color progressTrack:      controlTrack
    readonly property color buttonMuted:        neutral
    readonly property color buttonInactive:     controlDisabled
    readonly property color buttonTest:         accent
    readonly property color successBright:      success
    readonly property color successDark:        successSubtle
    readonly property color successMuted:       successSubtle
    readonly property color cyan:               alarmLow
    readonly property color magenta:            waveFlow
    readonly property color textLight:          textSecondary
    readonly property color textSubtle:         textMuted
    readonly property color textBackground:     surfaceSunken
}
