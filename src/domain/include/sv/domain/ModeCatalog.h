// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include "PatientCategory.h"
#include "VentilationParameter.h"

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

namespace sv::domain {

/// What the ventilator holds constant, and therefore what the patient's
/// mechanics turn into a dependent variable. This is not cosmetic: it decides
/// which mechanical-power equation applies and whether a resistance or stress
/// index measurement is valid at all.
enum class ControlVariable {
    Volume,        ///< VT set, pressure results
    Pressure,      ///< pressure set, VT results
    DualAdaptive,  ///< pressure adjusted breath to breath to hit a target VT
    Spontaneous,   ///< patient determines VT, rate and Ti
    Oscillatory,   ///< HFOV
    Flow           ///< high-flow oxygen, no ventilation
};

/// Who initiates and who terminates each breath.
enum class BreathSequence {
    ContinuousMandatory,     ///< CMV - every breath mandatory
    IntermittentMandatory,   ///< IMV / SIMV - mandatory plus spontaneous
    ContinuousSpontaneous    ///< CSV - every breath spontaneous
};

/**
 * @brief Everything the UI and the alarm layer need to know about one mode.
 *
 * The central idea, borrowed from the Dräger therapy-bar architecture: the
 * control set, the monitored set and the alarm profile are all functions of
 * the active mode. Driving them from this one declarative table rather than
 * from branching code in each screen means a new mode is a table entry, the
 * mode-to-control mapping is reviewable as a single artefact during usability
 * engineering, and no screen can drift out of step with another.
 */
struct ModeDefinition {
    QString key;              ///< stable id, e.g. "PCV"
    QString label;            ///< operator-facing name, e.g. "PC-CMV"
    QString shortLabel;       ///< header badge text
    QString description;      ///< one line, shown under the badge
    ControlVariable controlVariable = ControlVariable::Pressure;
    BreathSequence sequence = BreathSequence::ContinuousMandatory;

    bool nonInvasive = false;
    bool supportsApneaBackup = true;
    bool requiresPatientEffort = false;   ///< apnoea is a critical alarm here

    /// Parameters the operator sets in this mode, in display order.
    QVector<ParameterId> settable;

    /// Values the ventilator determines. Shown as monitored, never editable.
    QVector<ParameterId> derived;

    /// Monitored quantities worth promoting to the tile grid in this mode.
    QStringList primaryMetrics;

    /// Alarm condition ids that are clinically primary here.
    QStringList primaryAlarms;

    /// Alarm condition ids that are relaxed or disabled in this mode.
    QStringList relaxedAlarms;

    /// Free-text note surfaced on the mode selection card.
    QString clinicalNote;
};

namespace ModeCatalog {

/// Every supported mode, in the order the selection screen shows them.
const QVector<ModeDefinition> &all();

/// Definition by key. Returns nullptr for an unknown key.
const ModeDefinition *find(const QString &key);

/// Definition by key, falling back to a safe default rather than nullptr.
const ModeDefinition &findOrDefault(const QString &key);

/// True when the mode exposes this parameter as settable.
bool isSettable(const QString &modeKey, ParameterId id);

/// QML-facing description of a mode.
QVariantMap describe(const QString &modeKey);

/// QML-facing settable-parameter list for a mode and patient category. Each
/// entry is a ParameterCatalog::describe() map with the mode's own ordering.
QVariantList settableParameters(const QString &modeKey, PatientCategory category);

/// QML-facing derived-parameter list for a mode.
QVariantList derivedParameters(const QString &modeKey, PatientCategory category);

/// All mode keys.
QStringList keys();

} // namespace ModeCatalog
} // namespace sv::domain
