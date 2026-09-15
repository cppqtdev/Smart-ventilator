// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <cstdint>

namespace sv::units {

// Canonical internal units. Everything that crosses a layer boundary is in
// these; conversion happens at the edge, never in the middle.
//
//   pressure          cmH2O
//   volume            mL
//   flow              L/min
//   time              ms
//   rate              breaths/min
//   compliance        mL/cmH2O
//   resistance        cmH2O/(L/s)
//   fraction          percent 0..100
//   temperature       degrees Celsius
//   mass              grams
//   length            millimetres

// UTF-8 byte strings. Convert with QString::fromUtf8, never fromLatin1:
// the degree sign is two bytes.
struct Symbol
{
    static constexpr const char *pressure = "cmH2O";
    static constexpr const char *volume = "mL";
    static constexpr const char *flow = "L/min";
    static constexpr const char *minuteVolume = "L/min";
    static constexpr const char *rate = "b/min";
    static constexpr const char *compliance = "mL/cmH2O";
    static constexpr const char *resistance = "cmH2O/(L/s)";
    static constexpr const char *percent = "%";
    static constexpr const char *seconds = "s";
    static constexpr const char *milliseconds = "ms";
    static constexpr const char *partialPressure = "mmHg";
    static constexpr const char *temperature = "°C";
    static constexpr const char *mass = "kg";
    static constexpr const char *length = "cm";
    static constexpr const char *power = "J/min";
    static constexpr const char *ratio = "";
};

// Conversion factors. Named in the direction they read: kPaPerCmH2O
// multiplies a cmH2O value to get kPa.
inline constexpr double kPaPerCmH2O = 0.0980665;
inline constexpr double cmH2OPerKPa = 1.0 / kPaPerCmH2O;
inline constexpr double mbarPerCmH2O = 0.980665;
inline constexpr double cmH2OPerMbar = 1.0 / mbarPerCmH2O;
inline constexpr double mmHgPerCmH2O = 0.735559;
inline constexpr double cmH2OPerMmHg = 1.0 / mmHgPerCmH2O;

inline constexpr double litresPerMillilitre = 0.001;
inline constexpr double millilitresPerLitre = 1000.0;

inline constexpr double litresPerMinutePerMillilitrePerSecond = 0.06;
inline constexpr double millilitresPerSecondPerLitrePerMinute = 1.0 / 0.06;

inline constexpr double secondsPerMinute = 60.0;
inline constexpr double millisecondsPerSecond = 1000.0;
inline constexpr double millisecondsPerMinute = 60000.0;

inline constexpr double kilogramsPerGram = 0.001;
inline constexpr double centimetresPerInch = 2.54;

// Physiological and regulatory constants.
inline constexpr double atmosphericPressureMmHg = 760.0;
inline constexpr double waterVapourPressureMmHg = 47.0;
inline constexpr double ambientFio2Percent = 20.9;
inline constexpr double roomAirFio2Percent = 21.0;
inline constexpr double pureOxygenFio2Percent = 100.0;

// ARDSNet predicted body weight, in kilograms, from height in centimetres.
inline constexpr double pbwMaleIntercept = 50.0;
inline constexpr double pbwFemaleIntercept = 45.5;
inline constexpr double pbwSlopePerInchOverFiveFeet = 2.3;
inline constexpr double pbwBaselineHeightCm = 152.4;

// ISO 80601-2-12 and IEC 60601-1-8 timing limits, in the units named.
inline constexpr int audioPauseMaximumSeconds = 120;
inline constexpr int alarmHighFlashPeriodMs = 500;
inline constexpr int alarmMediumFlashPeriodMs = 1666;
inline constexpr int apneaDetectionDefaultSeconds = 20;

// Physical ranges the device itself can reach, before any patient-category
// or clinical-advisory narrowing.
inline constexpr double deviceMinimumPressureCmH2O = 0.0;
inline constexpr double deviceMaximumPressureCmH2O = 100.0;
inline constexpr double deviceMinimumVolumeMl = 2.0;
inline constexpr double deviceMaximumVolumeMl = 2500.0;
inline constexpr double deviceMinimumFlowLpm = 0.0;
inline constexpr double deviceMaximumFlowLpm = 260.0;
inline constexpr double deviceMinimumRateBpm = 1.0;
inline constexpr double deviceMaximumRateBpm = 150.0;

inline constexpr double toKPa(double cmH2O) { return cmH2O * kPaPerCmH2O; }
inline constexpr double fromKPa(double kPa) { return kPa * cmH2OPerKPa; }
inline constexpr double toMbar(double cmH2O) { return cmH2O * mbarPerCmH2O; }
inline constexpr double fromMbar(double mbar) { return mbar * cmH2OPerMbar; }
inline constexpr double toMmHg(double cmH2O) { return cmH2O * mmHgPerCmH2O; }
inline constexpr double fromMmHg(double mmHg) { return mmHg * cmH2OPerMmHg; }

inline constexpr double toLitres(double millilitres) { return millilitres * litresPerMillilitre; }
inline constexpr double toMillilitres(double litres) { return litres * millilitresPerLitre; }

inline constexpr double flowToMlPerSecond(double litresPerMinute)
{
    return litresPerMinute * millilitresPerSecondPerLitrePerMinute;
}

inline constexpr double flowToLitresPerMinute(double millilitresPerSecond)
{
    return millilitresPerSecond * litresPerMinutePerMillilitrePerSecond;
}

inline constexpr double breathPeriodMs(double breathsPerMinute)
{
    return breathsPerMinute > 0.0 ? millisecondsPerMinute / breathsPerMinute : 0.0;
}

} // namespace sv::units
