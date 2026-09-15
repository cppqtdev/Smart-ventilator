// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/services/RespiratoryMechanics.h>

#include <QCoreApplication>
#include <QDateTime>

#include <algorithm>
#include <cmath>

namespace sv::services {
namespace {

Measurement invalid(ValidityReason reason)
{
    Measurement m;
    m.validity = reason;
    return m;
}

Measurement valid(double value)
{
    Measurement m;
    m.value = value;
    m.validity = ValidityReason::Valid;
    return m;
}

/// Guards a divisor. Anything smaller is treated as zero rather than producing
/// an enormous quotient that would be displayed as a real measurement.
constexpr double kEpsilon = 1e-6;

bool plausible(double value, double low, double high)
{
    return std::isfinite(value) && value >= low && value <= high;
}

double nowMs()
{
    return double(QDateTime::currentMSecsSinceEpoch());
}

} // namespace

RespiratoryMechanics::RespiratoryMechanics() = default;

// ---------------------------------------------------------------------------
//  Compliance
// ---------------------------------------------------------------------------

Measurement RespiratoryMechanics::dynamicCompliance(const BreathSample &breath) const
{
    if (breath.expiredVolume <= 0.0 || breath.peakPressure <= 0.0)
        return invalid(ValidityReason::NoData);

    // Total PEEP where a hold has supplied it, set PEEP otherwise. Falling
    // back silently would overstate compliance whenever gas is trapped.
    const double peep = breath.totalPeepValid ? breath.totalPeep : breath.setPeep;
    const double gradient = breath.peakPressure - peep;
    if (gradient <= kEpsilon)
        return invalid(ValidityReason::OutOfRange);

    const double cdyn = breath.expiredVolume / gradient;
    if (!plausible(cdyn, 0.2, 400.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(cdyn);
}

Measurement RespiratoryMechanics::staticCompliance(const BreathSample &breath) const
{
    if (!breath.plateauValid)
        return invalid(ValidityReason::RequiresHoldManoeuvre);
    if (breath.expiredVolume <= 0.0)
        return invalid(ValidityReason::NoData);
    if (!breath.passive)
        return invalid(ValidityReason::RequiresPassivePatient);

    const double peep = breath.totalPeepValid ? breath.totalPeep : breath.setPeep;
    const double drivingP = breath.plateauPressure - peep;
    if (drivingP <= kEpsilon)
        return invalid(ValidityReason::OutOfRange);

    const double cstat = breath.expiredVolume / drivingP;
    if (!plausible(cstat, 0.2, 400.0))
        return invalid(ValidityReason::OutOfRange);

    Measurement m = valid(cstat);
    m.manoeuvreDerived = true;
    m.ageSeconds = inspiratoryHoldAgeSeconds();
    if (m.ageSeconds >= 0.0 && m.ageSeconds > kManoeuvreValiditySeconds)
        m.validity = ValidityReason::RequiresHoldManoeuvre;
    return m;
}

Measurement RespiratoryMechanics::elastance(const BreathSample &breath) const
{
    const Measurement c = staticCompliance(breath).isValid()
        ? staticCompliance(breath)
        : dynamicCompliance(breath);
    if (!c.isValid() || c.value <= kEpsilon)
        return invalid(c.validity);
    // mL/cmH2O -> cmH2O/L
    return valid(1000.0 / c.value);
}

// ---------------------------------------------------------------------------
//  Resistance
// ---------------------------------------------------------------------------

Measurement RespiratoryMechanics::inspiratoryResistance(const BreathSample &breath) const
{
    // The formula assumes the resistive pressure drop is being measured at a
    // steady flow. Decelerating flow breaks that assumption entirely, so this
    // is refused rather than approximated.
    if (!breath.squareFlow)
        return invalid(ValidityReason::RequiresSquareFlow);
    if (!breath.plateauValid)
        return invalid(ValidityReason::RequiresHoldManoeuvre);
    if (!breath.passive)
        return invalid(ValidityReason::RequiresPassivePatient);

    const double flowLps = breath.peakInspiratoryFlow
                         * units::kLitresPerMinuteToLitresPerSecond;
    if (flowLps <= kEpsilon)
        return invalid(ValidityReason::NoData);

    const double transairway = breath.peakPressure - breath.plateauPressure;
    if (transairway < 0.0)
        return invalid(ValidityReason::OutOfRange);

    const double raw = transairway / flowLps;
    if (!plausible(raw, 0.0, 300.0))
        return invalid(ValidityReason::OutOfRange);

    Measurement m = valid(raw);
    m.manoeuvreDerived = true;
    m.ageSeconds = inspiratoryHoldAgeSeconds();
    return m;
}

Measurement RespiratoryMechanics::expiratoryResistance(const BreathSample &breath) const
{
    if (!breath.plateauValid)
        return invalid(ValidityReason::RequiresHoldManoeuvre);

    const double flowLps = std::abs(breath.peakExpiratoryFlow)
                         * units::kLitresPerMinuteToLitresPerSecond;
    if (flowLps <= kEpsilon)
        return invalid(ValidityReason::NoData);

    const double peep = breath.totalPeepValid ? breath.totalPeep : breath.setPeep;
    const double gradient = breath.plateauPressure - peep;
    if (gradient <= 0.0)
        return invalid(ValidityReason::OutOfRange);

    const double rexp = gradient / flowLps;
    if (!plausible(rexp, 0.0, 300.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(rexp);
}

Measurement RespiratoryMechanics::timeConstant(const BreathSample &breath) const
{
    const Measurement c = dynamicCompliance(breath);
    const Measurement r = expiratoryResistance(breath);
    if (!c.isValid())
        return invalid(c.validity);
    if (!r.isValid())
        return invalid(r.validity);

    // tau = C * R with compliance in L/cmH2O, so the mL compliance is
    // converted first. Skipping this is the other classic factor-of-1000 bug
    // in this file's subject matter.
    const double complianceLitres = c.value * units::kMillilitresToLitres;
    const double tau = complianceLitres * r.value;
    if (!plausible(tau, 0.0, 10.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(tau);
}

// ---------------------------------------------------------------------------
//  Gas trapping
// ---------------------------------------------------------------------------

Measurement RespiratoryMechanics::autoPeep(const BreathSample &breath) const
{
    if (!breath.totalPeepValid)
        return invalid(ValidityReason::RequiresHoldManoeuvre);
    if (!breath.passive)
        return invalid(ValidityReason::RequiresPassivePatient);

    const double intrinsic = breath.totalPeep - breath.setPeep;
    if (!plausible(intrinsic, -2.0, 40.0))
        return invalid(ValidityReason::OutOfRange);

    Measurement m = valid(std::max(0.0, intrinsic));
    m.manoeuvreDerived = true;
    m.ageSeconds = expiratoryHoldAgeSeconds();
    if (m.ageSeconds >= 0.0 && m.ageSeconds > kManoeuvreValiditySeconds)
        m.validity = ValidityReason::RequiresHoldManoeuvre;
    return m;
}

// ---------------------------------------------------------------------------
//  Leak
// ---------------------------------------------------------------------------

Measurement RespiratoryMechanics::leakPercent(const BreathSample &breath) const
{
    if (breath.inspiredVolume <= kEpsilon)
        return invalid(ValidityReason::NoData);

    const double leak = (breath.inspiredVolume - breath.expiredVolume)
                      / breath.inspiredVolume * 100.0;
    // A negative figure means expired exceeded inspired, which is a sensor or
    // calibration problem rather than a negative leak.
    if (!plausible(leak, -20.0, 100.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(std::clamp(leak, 0.0, 100.0));
}

Measurement RespiratoryMechanics::leakFlow(const BreathSample &breath) const
{
    if (breath.respiratoryRate <= kEpsilon)
        return invalid(ValidityReason::NoData);

    const double perBreathMl = breath.inspiredVolume - breath.expiredVolume;
    const double lpm = perBreathMl * breath.respiratoryRate * units::kMillilitresToLitres;
    if (!plausible(lpm, -5.0, 200.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(std::max(0.0, lpm));
}

Measurement RespiratoryMechanics::compensatedTidalVolume(const BreathSample &breath,
                                                         bool nonInvasive) const
{
    if (breath.expiredVolume <= 0.0 && breath.inspiredVolume <= 0.0)
        return invalid(ValidityReason::NoData);

    if (!nonInvasive) {
        // Invasively, expired volume is already the best estimate of what
        // left the lung.
        return valid(breath.expiredVolume);
    }

    const Measurement leak = leakPercent(breath);
    if (!leak.isValid())
        return valid(breath.expiredVolume);

    // The leak is distributed across both phases, so neither VTi nor VTe is
    // the lung volume. Splitting the deficit evenly is the simple form of what
    // leak-compensating ventilators do, and it is what the volume alarms
    // should be evaluated against in NIV.
    const double deficit = breath.inspiredVolume - breath.expiredVolume;
    const double compensated = breath.expiredVolume + deficit * 0.5;
    if (!plausible(compensated, 0.0, 3000.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(compensated);
}

// ---------------------------------------------------------------------------
//  Lung protection
// ---------------------------------------------------------------------------

Measurement RespiratoryMechanics::drivingPressure(const BreathSample &breath) const
{
    if (!breath.plateauValid)
        return invalid(ValidityReason::RequiresHoldManoeuvre);
    if (!breath.passive)
        return invalid(ValidityReason::RequiresPassivePatient);

    const double peep = breath.totalPeepValid ? breath.totalPeep : breath.setPeep;
    const double dp = breath.plateauPressure - peep;
    if (!plausible(dp, 0.0, 80.0))
        return invalid(ValidityReason::OutOfRange);

    Measurement m = valid(dp);
    m.manoeuvreDerived = true;
    m.ageSeconds = inspiratoryHoldAgeSeconds();
    return m;
}

Measurement RespiratoryMechanics::mechanicalPower(const BreathSample &breath,
                                                  domain::ControlVariable controlVariable) const
{
    using domain::ControlVariable;

    if (breath.respiratoryRate <= kEpsilon)
        return invalid(ValidityReason::NoData);

    const double peep = breath.totalPeepValid ? breath.totalPeep : breath.setPeep;
    double power = 0.0;

    switch (controlVariable) {
    case ControlVariable::Volume: {
        // Giosa et al. volume-control surrogate. Minute ventilation in L/min,
        // inspiratory flow in L/min, pressures in cmH2O.
        const double minuteVentilation = breath.expiredVolume
                                       * breath.respiratoryRate
                                       * units::kMillilitresToLitres;
        if (minuteVentilation <= kEpsilon || breath.peakInspiratoryFlow <= kEpsilon)
            return invalid(ValidityReason::NoData);
        power = minuteVentilation
              * (breath.peakPressure + peep + breath.peakInspiratoryFlow / 6.0)
              / 20.0;
        break;
    }
    case ControlVariable::Pressure:
    case ControlVariable::DualAdaptive: {
        // Becher et al. pressure-control form. Needs a real plateau.
        if (!breath.plateauValid)
            return invalid(ValidityReason::RequiresHoldManoeuvre);
        const double vtLitres = breath.inspiredVolume * units::kMillilitresToLitres;
        if (vtLitres <= kEpsilon)
            return invalid(ValidityReason::NoData);
        power = units::kCmH2OLitreToJoule
              * breath.respiratoryRate
              * vtLitres
              * breath.plateauPressure;
        break;
    }
    case ControlVariable::Spontaneous:
        // The patient is doing an unmeasured share of the work, so ventilator
        // mechanical power is not the quantity of interest.
        return invalid(ValidityReason::NotApplicableInMode);
    case ControlVariable::Oscillatory:
    case ControlVariable::Flow:
        return invalid(ValidityReason::NotApplicableInMode);
    }

    if (!plausible(power, 0.0, 200.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(power);
}

Measurement RespiratoryMechanics::stressIndex(const QVector<double> &inspiratoryPressures,
                                              double sampleIntervalSeconds,
                                              const BreathSample &breath) const
{
    if (!breath.squareFlow)
        return invalid(ValidityReason::RequiresSquareFlow);
    if (!breath.passive)
        return invalid(ValidityReason::RequiresPassivePatient);
    if (inspiratoryPressures.size() < 8 || sampleIntervalSeconds <= 0.0)
        return invalid(ValidityReason::NoData);

    // Fit Paw(t) = a*t^b + c. With c taken as the pressure at the start of
    // inspiration, the remainder linearises under logarithms:
    //   log(Paw - c) = log(a) + b*log(t)
    // and b falls out of an ordinary least-squares slope.
    const double c = inspiratoryPressures.first();

    double sumX = 0.0, sumY = 0.0, sumXX = 0.0, sumXY = 0.0;
    int n = 0;

    for (int i = 1; i < inspiratoryPressures.size(); ++i) {
        const double t = i * sampleIntervalSeconds;
        const double dp = inspiratoryPressures.at(i) - c;
        if (dp <= kEpsilon || t <= kEpsilon)
            continue;
        const double x = std::log(t);
        const double y = std::log(dp);
        sumX += x;
        sumY += y;
        sumXX += x * x;
        sumXY += x * y;
        ++n;
    }

    if (n < 5)
        return invalid(ValidityReason::NoData);

    const double denominator = n * sumXX - sumX * sumX;
    if (std::abs(denominator) < kEpsilon)
        return invalid(ValidityReason::OutOfRange);

    const double b = (n * sumXY - sumX * sumY) / denominator;
    if (!plausible(b, 0.1, 4.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(b);
}

// ---------------------------------------------------------------------------
//  Drive and weaning
// ---------------------------------------------------------------------------

Measurement RespiratoryMechanics::rapidShallowBreathingIndex(const BreathSample &breath,
                                                             double pressureSupport) const
{
    // Computed on a supported breath the index is falsely reassuring, which is
    // the opposite of what a weaning decision needs. Gate it.
    if (pressureSupport > 5.0)
        return invalid(ValidityReason::RequiresUnsupported);
    if (breath.spontaneousRate <= kEpsilon)
        return invalid(ValidityReason::RequiresUnsupported);

    const double vtLitres = breath.expiredVolume * units::kMillilitresToLitres;
    if (vtLitres <= kEpsilon)
        return invalid(ValidityReason::NoData);

    const double rsbi = breath.spontaneousRate / vtLitres;
    if (!plausible(rsbi, 1.0, 1000.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(rsbi);
}

Measurement RespiratoryMechanics::p01(double pressureDrop) const
{
    const double magnitude = std::abs(pressureDrop);
    if (!plausible(magnitude, 0.0, 30.0))
        return invalid(ValidityReason::OutOfRange);

    Measurement m = valid(magnitude);
    m.manoeuvreDerived = true;
    return m;
}

Measurement RespiratoryMechanics::ventilatorWorkOfBreathing(const BreathSample &breath) const
{
    if (breath.expiredVolume <= kEpsilon)
        return invalid(ValidityReason::NoData);

    const double peep = breath.totalPeepValid ? breath.totalPeep : breath.setPeep;
    // Work per litre delivered, from the mean pressure applied across the
    // breath. This is ventilator work, not patient work - patient work needs
    // an oesophageal pressure signal, which this device does not have.
    const double meanApplied = breath.meanPressure - peep;
    if (meanApplied <= 0.0)
        return invalid(ValidityReason::OutOfRange);

    const double wob = meanApplied * units::kCmH2OLitreToJoule;
    if (!plausible(wob, 0.0, 10.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(wob);
}

// ---------------------------------------------------------------------------
//  Dosing
// ---------------------------------------------------------------------------

Measurement RespiratoryMechanics::tidalVolumePerKg(const BreathSample &breath,
                                                   double bodyWeightKg) const
{
    if (bodyWeightKg <= kEpsilon)
        return invalid(ValidityReason::NoData);
    if (breath.expiredVolume <= 0.0)
        return invalid(ValidityReason::NoData);

    const double perKg = breath.expiredVolume / bodyWeightKg;
    if (!plausible(perKg, 0.0, 40.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(perKg);
}

Measurement RespiratoryMechanics::expiredMinuteVolume(const BreathSample &breath) const
{
    if (breath.respiratoryRate <= kEpsilon || breath.expiredVolume <= 0.0)
        return invalid(ValidityReason::NoData);
    const double mv = breath.expiredVolume * breath.respiratoryRate
                    * units::kMillilitresToLitres;
    if (!plausible(mv, 0.0, 80.0))
        return invalid(ValidityReason::OutOfRange);
    return valid(mv);
}

Measurement RespiratoryMechanics::spontaneousFraction(const BreathSample &breath) const
{
    if (breath.respiratoryRate <= kEpsilon)
        return invalid(ValidityReason::NoData);
    const double fraction = breath.spontaneousRate / breath.respiratoryRate;
    return valid(std::clamp(fraction, 0.0, 1.0));
}

// ---------------------------------------------------------------------------
//  Manoeuvre bookkeeping
// ---------------------------------------------------------------------------

void RespiratoryMechanics::markInspiratoryHold() { m_lastInspiratoryHoldMs = nowMs(); }
void RespiratoryMechanics::markExpiratoryHold() { m_lastExpiratoryHoldMs = nowMs(); }

double RespiratoryMechanics::inspiratoryHoldAgeSeconds() const
{
    if (m_lastInspiratoryHoldMs < 0.0)
        return -1.0;
    return (nowMs() - m_lastInspiratoryHoldMs) / 1000.0;
}

double RespiratoryMechanics::expiratoryHoldAgeSeconds() const
{
    if (m_lastExpiratoryHoldMs < 0.0)
        return -1.0;
    return (nowMs() - m_lastExpiratoryHoldMs) / 1000.0;
}

// ---------------------------------------------------------------------------

QString describeValidity(ValidityReason reason)
{
    switch (reason) {
    case ValidityReason::Valid:
        return {};
    case ValidityReason::NoData:
        return QCoreApplication::translate("RespiratoryMechanics",
            "Waiting for measured data");
    case ValidityReason::RequiresSquareFlow:
        return QCoreApplication::translate("RespiratoryMechanics",
            "Needs constant inspiratory flow - not valid with a decelerating "
            "flow pattern");
    case ValidityReason::RequiresPassivePatient:
        return QCoreApplication::translate("RespiratoryMechanics",
            "Not valid while the patient is making respiratory effort");
    case ValidityReason::RequiresHoldManoeuvre:
        return QCoreApplication::translate("RespiratoryMechanics",
            "Perform an inspiratory or expiratory hold to measure");
    case ValidityReason::RequiresUnsupported:
        return QCoreApplication::translate("RespiratoryMechanics",
            "Measure during unsupported spontaneous breathing");
    case ValidityReason::NotApplicableInMode:
        return QCoreApplication::translate("RespiratoryMechanics",
            "Not applicable in the active ventilation mode");
    case ValidityReason::OutOfRange:
        return QCoreApplication::translate("RespiratoryMechanics",
            "Measured value is outside the plausible range - check the sensors");
    }
    return {};
}

QString describeValidityShort(ValidityReason reason)
{
    switch (reason) {
    case ValidityReason::Valid:
        return {};
    case ValidityReason::NoData:
        return QCoreApplication::translate("RespiratoryMechanics", "waiting");
    case ValidityReason::RequiresSquareFlow:
        return QCoreApplication::translate("RespiratoryMechanics", "needs square flow");
    case ValidityReason::RequiresPassivePatient:
        return QCoreApplication::translate("RespiratoryMechanics", "patient effort");
    case ValidityReason::RequiresHoldManoeuvre:
        return QCoreApplication::translate("RespiratoryMechanics", "hold to measure");
    case ValidityReason::RequiresUnsupported:
        return QCoreApplication::translate("RespiratoryMechanics", "measure off support");
    case ValidityReason::NotApplicableInMode:
        return QCoreApplication::translate("RespiratoryMechanics", "n/a this mode");
    case ValidityReason::OutOfRange:
        return QCoreApplication::translate("RespiratoryMechanics", "check sensors");
    }
    return {};
}

} // namespace sv::services
