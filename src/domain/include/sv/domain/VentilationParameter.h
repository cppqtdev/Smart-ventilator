// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include "PatientCategory.h"

#include <QString>
#include <QVariantMap>
#include <QVector>

#include <array>

namespace sv::domain {

/**
 * @brief Every operator-settable or ventilator-derived ventilation parameter.
 *
 * The enum is the internal identity; ParameterSpec::key is the stable string
 * used by QML, the settings store, the audit log and the CAN signal names.
 * Keys never change once shipped - a recorded setting change that cannot be
 * resolved back to a parameter is a hole in the device history.
 */
enum class ParameterId {
    Fio2,
    Peep,
    PInsp,
    PSupport,
    TidalVolume,
    RespiratoryRate,
    InspiratoryTime,
    IeRatio,
    FlowTrigger,
    PressureTrigger,
    RiseTime,
    Ets,
    FlowPattern,
    PressureLimit,
    PHigh,
    PLow,
    THigh,
    TLow,
    MinVolumePercent,
    PasvLimit,
    TiMax,
    SighEnabled,
    BackupRate,
    BackupTidalVolume,
    ApneaTime,
    HfovFrequency,
    HfovAmplitude,
    HfovMeanPaw,
    HighFlowRate,

    Count
};

/// How the operator manipulates the value, which decides the widget.
enum class ParameterKind {
    Continuous,   ///< numeric dial / slider
    Discrete,     ///< a short list of named choices
    Toggle        ///< on / off
};

/// A closed numeric interval plus its adjustment granularity.
struct ParameterRange {
    double minimum = 0.0;
    double maximum = 0.0;
    double step = 1.0;

    constexpr bool contains(double value) const
    {
        return value >= minimum && value <= maximum;
    }
    constexpr double clamp(double value) const
    {
        return value < minimum ? minimum : (value > maximum ? maximum : value);
    }
    constexpr bool isValid() const { return maximum > minimum; }
};

/**
 * @brief Soft clinical guidance for a parameter.
 *
 * The third limit tier. Values outside this band are permitted but warned
 * about; values outside the category range are refused. Conflating the two is
 * the usual structural mistake and it makes the paediatric and neonatal safety
 * argument very hard to present to a reviewer.
 */
struct ParameterAdvisory {
    double low = 0.0;
    double high = 0.0;
    QString rationale;

    bool isSet() const { return high > low; }
    bool within(double value) const { return !isSet() || (value >= low && value <= high); }
};

/**
 * @brief The full specification of one parameter across all patient categories.
 *
 * Three independent limit tiers, deliberately not merged:
 *
 *   1. @c device   - what the hardware can physically deliver, and what the
 *                    ISO 80601-2-12 accuracy claim is written against. A
 *                    regulatory statement, not a clinical opinion.
 *   2. @c category - what is safe for this patient size. Narrower than the
 *                    device range, and the tier that actually protects a
 *                    neonate from an adult tidal volume.
 *   3. @c advisory - what is usually appropriate. Warns, permits override
 *                    with confirmation, never blocks.
 */
struct ParameterSpec {
    ParameterId id = ParameterId::Count;
    QString key;
    QString label;
    QString shortLabel;
    QString unit;
    int decimals = 0;
    ParameterKind kind = ParameterKind::Continuous;

    ParameterRange device;
    std::array<ParameterRange, patientCategoryCount> category {};
    std::array<ParameterAdvisory, patientCategoryCount> advisory {};
    std::array<double, patientCategoryCount> startup {};

    /// Named choices for ParameterKind::Discrete, empty otherwise.
    QStringList choices;

    /// True when a change needs an explicit confirm step beyond the usual one.
    bool hazardous = false;

    /// One line explaining what the parameter does, for the UI help affordance.
    QString help;
};

/**
 * @brief The parameter table.
 *
 * Deliberately a flat, declarative table rather than branching code. It is the
 * artefact a 62366 reviewer reads, the thing a unit test iterates over, and the
 * single place a limit changes.
 */
namespace ParameterCatalog {

/// Spec for one parameter. Returns a default-constructed spec if unknown.
const ParameterSpec &spec(ParameterId id);

/// Spec by stable key, e.g. "peep". Returns nullptr if unknown.
const ParameterSpec *find(const QString &key);

/// Every spec, in enum order.
const QVector<ParameterSpec> &all();

/// Effective adjustable range: the category range intersected with the device
/// range. This is the interval the UI must not let the operator leave.
ParameterRange effectiveRange(ParameterId id, PatientCategory category);

/// Startup value for a category, clamped into the effective range.
double startupValue(ParameterId id, PatientCategory category);

/// Advisory band for a category.
const ParameterAdvisory &advisory(ParameterId id, PatientCategory category);

/// QML-facing description of a parameter for a given category.
QVariantMap describe(ParameterId id, PatientCategory category);

} // namespace ParameterCatalog

} // namespace sv::domain
