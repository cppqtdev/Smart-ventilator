// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/domain/ModeCatalog.h>
#include <sv/domain/PatientCategory.h>

#include <QString>
#include <QVector>

#include <optional>

namespace sv::services {

/**
 * @brief Unit convention for this whole file. Read it before changing anything.
 *
 *   pressure  cmH2O
 *   volume    mL
 *   flow      L/min at the interface, converted to L/s internally
 *   time      seconds
 *   compliance mL/cmH2O
 *   resistance cmH2O/(L/s)
 *
 * The single most common defect in this domain is a flow-unit mismatch in the
 * resistance calculation. L/min where L/s is required yields a plausible
 * number that is wrong by a factor of 60 - large enough to matter clinically,
 * small enough to pass a casual review. Every conversion here is explicit and
 * every function documents which unit it expects.
 */
namespace units {
constexpr double kLitresPerMinuteToLitresPerSecond = 1.0 / 60.0;
constexpr double kMillilitresToLitres = 1.0 / 1000.0;
/// 1 cmH2O.L = 0.0981 J. This is the constant behind the 0.098 in every
/// published mechanical power formula.
constexpr double kCmH2OLitreToJoule = 0.0981;
} // namespace units

/**
 * @brief Why a derived value is not currently trustworthy.
 *
 * A measurement that is invalid in the present mode must not simply be
 * displayed with a caveat buried in the manual. Resistance computed from a
 * decelerating flow, or a stress index computed while the patient is making
 * effort, is not an approximation - it is a wrong number. The UI greys those
 * tiles out and names the reason.
 */
enum class ValidityReason {
    Valid,
    NoData,                  ///< not enough samples yet
    RequiresSquareFlow,      ///< only meaningful under constant inspiratory flow
    RequiresPassivePatient,  ///< spontaneous effort invalidates it
    RequiresHoldManoeuvre,   ///< needs an inspiratory or expiratory hold
    RequiresUnsupported,     ///< must be measured off support
    NotApplicableInMode,     ///< the quantity has no meaning in this mode
    OutOfRange               ///< computed but physiologically implausible
};

/// A derived quantity together with whether it can be believed.
struct Measurement {
    double value = 0.0;
    ValidityReason validity = ValidityReason::NoData;
    /// When the value came from a manoeuvre rather than a continuous estimate.
    bool manoeuvreDerived = false;
    /// Seconds since the manoeuvre, for the "measured at" stamp on the tile.
    double ageSeconds = -1.0;

    bool isValid() const { return validity == ValidityReason::Valid; }
};

/// One breath's worth of measured inputs.
struct BreathSample {
    double peakPressure = 0.0;        ///< PIP, cmH2O
    double plateauPressure = 0.0;     ///< Pplat, cmH2O - hold-derived
    double meanPressure = 0.0;        ///< Pmean, cmH2O
    double setPeep = 0.0;             ///< extrinsic PEEP, cmH2O
    double totalPeep = 0.0;           ///< PEEPtot, cmH2O - hold-derived
    double inspiredVolume = 0.0;      ///< VTi, mL
    double expiredVolume = 0.0;       ///< VTe, mL
    double peakInspiratoryFlow = 0.0; ///< L/min
    double peakExpiratoryFlow = 0.0;  ///< L/min, magnitude
    double respiratoryRate = 0.0;     ///< total f, 1/min
    double spontaneousRate = 0.0;     ///< patient-triggered f, 1/min
    double inspiratoryTime = 0.0;     ///< s
    double expiratoryTime = 0.0;      ///< s
    bool squareFlow = false;          ///< constant inspiratory flow
    bool passive = true;              ///< no detected patient effort
    bool plateauValid = false;        ///< a hold actually produced Pplat
    bool totalPeepValid = false;      ///< a hold actually produced PEEPtot
};

/**
 * @brief Computes respiratory mechanics from measured breath data.
 *
 * Stateless apart from the manoeuvre timestamps. Every function takes what it
 * needs and returns a Measurement rather than a bare double, so a caller
 * cannot display an invalid result by omission.
 */
class RespiratoryMechanics
{
public:
    RespiratoryMechanics();

    // -- Compliance ---------------------------------------------------------

    /**
     * @brief Dynamic compliance, Cdyn = VTe / (PIP - PEEPtot).
     *
     * Needs no hold, so it is available breath by breath. Includes the
     * resistive component, so it reads lower than static compliance.
     */
    Measurement dynamicCompliance(const BreathSample &breath) const;

    /**
     * @brief Static compliance, Cstat = VTe / (Pplat - PEEPtot).
     *
     * Requires an inspiratory hold long enough for the plateau to settle.
     * Uses TOTAL PEEP, not set PEEP - using set PEEP overestimates compliance
     * whenever gas trapping is present, which is precisely the situation where
     * the number is being consulted.
     *
     * Uses EXPIRED volume; inspired volume overestimates whenever there is a
     * leak.
     *
     * Normal in an intubated adult is roughly 70-100 mL/cmH2O.
     */
    Measurement staticCompliance(const BreathSample &breath) const;

    /// Elastance, the reciprocal of compliance, in cmH2O/L.
    Measurement elastance(const BreathSample &breath) const;

    // -- Resistance ---------------------------------------------------------

    /**
     * @brief Inspiratory airway resistance, (PIP - Pplat) / inspiratory flow.
     *
     * Valid ONLY under constant (square) inspiratory flow. Under the
     * decelerating flow of every pressure-control mode the formula does not
     * hold, and the function reports RequiresSquareFlow rather than returning
     * a number that looks reasonable.
     *
     * Intubated adult reference is 5-10 cmH2O/(L/s) - the tube dominates.
     * Intubated neonate is 50-150. Above 15-20 is abnormal for an adult.
     */
    Measurement inspiratoryResistance(const BreathSample &breath) const;

    /// Expiratory resistance, (Pplat - PEEPtot) / peak expiratory flow.
    Measurement expiratoryResistance(const BreathSample &breath) const;

    /**
     * @brief Expiratory time constant, tau = C * R, in seconds.
     *
     * Roughly three time constants are needed to exhale 95% of the breath, so
     * this is what an expiratory time should be checked against when auto-PEEP
     * is suspected.
     */
    Measurement timeConstant(const BreathSample &breath) const;

    // -- Trapping -----------------------------------------------------------

    /**
     * @brief Intrinsic PEEP, PEEPtot - PEEPset.
     *
     * Requires an end-expiratory hold held until flow reaches zero. Normal is
     * zero; above about 5 cmH2O is clinically significant. The measurement is
     * invalid if the patient makes an inspiratory effort during the hold, and
     * the caller is expected to reject it in that case.
     */
    Measurement autoPeep(const BreathSample &breath) const;

    // -- Leak ---------------------------------------------------------------

    /**
     * @brief Leak as a percentage, (VTi - VTe) / VTi * 100.
     *
     * The right presentation for invasive ventilation. Uninformative in NIV,
     * where a vented mask leaks deliberately and continuously.
     */
    Measurement leakPercent(const BreathSample &breath) const;

    /**
     * @brief Leak as a flow, (VTi - VTe) * f / 1000, in L/min.
     *
     * The right presentation for non-invasive ventilation, where the
     * intentional leak makes a percentage meaningless.
     */
    Measurement leakFlow(const BreathSample &breath) const;

    /**
     * @brief Leak-compensated tidal volume in mL.
     *
     * A third volume, distinct from both VTi and VTe, estimating what actually
     * reached the lung. Volume alarms should be evaluated against THIS rather
     * than raw VTe whenever leak compensation is active - otherwise a high-leak
     * NIV patient generates continuous nuisance low-volume alarms. All three
     * volumes stay visible to the operator.
     */
    Measurement compensatedTidalVolume(const BreathSample &breath, bool nonInvasive) const;

    // -- Lung protection ----------------------------------------------------

    /**
     * @brief Driving pressure, Pplat - PEEPtot, equivalently VT / Crs.
     *
     * Target below 14 cmH2O in moderate-to-severe ARDS. Valid only in a
     * passive patient - spontaneous effort makes the plateau meaningless.
     */
    Measurement drivingPressure(const BreathSample &breath) const;

    /**
     * @brief Mechanical power in J/min.
     *
     * Dispatches on the mode's control variable, because the published volume
     * control and pressure control equations are NOT interchangeable and
     * applying one to the other gives a materially wrong number:
     *
     *   volume control   MP = [ MV * (Ppeak + PEEP + Flow/6) ] / 20
     *                    (Giosa et al., ICM Exp 2019;7(1):61)
     *   pressure control MP = 0.098 * f * VTi(L) * Pplat
     *                    (Becher et al., Intensive Care Med 2019;45:1321-3)
     *
     * Above roughly 17 J/min is associated with higher mortality.
     */
    Measurement mechanicalPower(const BreathSample &breath,
                                domain::ControlVariable controlVariable) const;

    /**
     * @brief Stress index exponent b from Paw(t) = a*t^b + c.
     *
     *   b < 1  downward concavity, tidal recruitment, consider raising PEEP
     *   b ~ 1  linear, acceptable
     *   b > 1  upward concavity, overdistension, consider lowering VT or PEEP
     *
     * Valid only under constant inspiratory flow in a passive patient. Both
     * conditions are checked and reported rather than assumed.
     *
     * @param inspiratoryPressures Airway pressure samples across inspiration.
     * @param sampleIntervalSeconds Spacing of those samples.
     */
    Measurement stressIndex(const QVector<double> &inspiratoryPressures,
                            double sampleIntervalSeconds,
                            const BreathSample &breath) const;

    // -- Drive and weaning --------------------------------------------------

    /**
     * @brief Rapid shallow breathing index, f / VT(L).
     *
     * Below 105 predicts weaning success (Yang & Tobin, NEJM 1991). Measured
     * during UNSUPPORTED spontaneous breathing; computed on a supported breath
     * it gives a falsely reassuring number, so the support level gates it.
     *
     * @param pressureSupport Current support in cmH2O. Above a few cmH2O the
     *        result is reported as RequiresUnsupported.
     */
    Measurement rapidShallowBreathingIndex(const BreathSample &breath,
                                           double pressureSupport) const;

    /**
     * @brief P0.1 - airway pressure 100 ms into an occluded effort, cmH2O.
     *
     * A measure of central respiratory drive, reported as a positive
     * magnitude. Normal spontaneous breathing is around 0.5-1.5 cmH2O; above
     * roughly 3.5-4 suggests high drive and excessive load.
     *
     * @param pressureDrop Magnitude of the deflection below baseline.
     */
    Measurement p01(double pressureDrop) const;

    /**
     * @brief Work of breathing performed by the VENTILATOR, in J/L.
     *
     * Deliberately named for what it measures. True patient work of breathing
     * needs an oesophageal balloon; without one, only ventilator work can be
     * computed, and presenting it as patient work is a clinical
     * misinterpretation risk. The UI labels it WOBvent.
     */
    Measurement ventilatorWorkOfBreathing(const BreathSample &breath) const;

    // -- Dosing -------------------------------------------------------------

    /// Tidal volume per kilogram of the supplied body weight, mL/kg.
    Measurement tidalVolumePerKg(const BreathSample &breath, double bodyWeightKg) const;

    /// Expired minute volume in L/min.
    Measurement expiredMinuteVolume(const BreathSample &breath) const;

    /// Spontaneous fraction of the total rate, 0-1.
    Measurement spontaneousFraction(const BreathSample &breath) const;

    // -- Manoeuvre bookkeeping ----------------------------------------------

    /// Records that an inspiratory hold produced a plateau just now.
    void markInspiratoryHold();
    /// Records that an expiratory hold produced a total PEEP just now.
    void markExpiratoryHold();

    /// Seconds since the last inspiratory hold, or -1 if there has been none.
    double inspiratoryHoldAgeSeconds() const;
    /// Seconds since the last expiratory hold, or -1 if there has been none.
    double expiratoryHoldAgeSeconds() const;

    /**
     * @brief How long a hold-derived value stays displayable, in seconds.
     *
     * After this the value is stale: the patient's mechanics have had time to
     * change and a plateau from ten minutes ago should not be presented beside
     * live values as though it were one of them.
     */
    static constexpr double kManoeuvreValiditySeconds = 300.0;

private:
    double m_lastInspiratoryHoldMs = -1.0;
    double m_lastExpiratoryHoldMs = -1.0;
};

/// Human-readable reason, for the tooltip on a greyed-out tile.
QString describeValidity(ValidityReason reason);

/**
 * @brief Two or three words for the same reason, to sit inside a tile.
 *
 * A metric tile is a few characters wide. Putting the full sentence in one
 * overflows it and hides the value, so the long form belongs in a tooltip and
 * this belongs on the tile. Both are provided rather than having each screen
 * invent its own abbreviation.
 */
QString describeValidityShort(ValidityReason reason);

} // namespace sv::services
