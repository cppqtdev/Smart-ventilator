#include "VentilationCatalog.h"

#include <sv/domain/ModeCatalog.h>
#include <sv/domain/PatientCategory.h>
#include <sv/domain/VentilationParameter.h>

#include <QCoreApplication>

using namespace sv::domain;

namespace {

PatientCategory categoryOf(const QString &text)
{
    return patientCategoryFromString(text);
}

const ParameterSpec *specOf(const QString &key)
{
    return ParameterCatalog::find(key);
}

} // namespace

VentilationCatalog::VentilationCatalog(QObject *parent)
    : QObject(parent)
{
}

QString VentilationCatalog::patientCategory() const { return m_patientCategory; }

void VentilationCatalog::setPatientCategory(const QString &category)
{
    const QString normalised = toString(categoryOf(category));
    if (m_patientCategory == normalised)
        return;
    m_patientCategory = normalised;
    emit patientCategoryChanged();
    // Every limit in the UI is a function of the category, so the whole
    // catalogue view is invalidated, not just the category string.
    emit catalogChanged();
}

QString VentilationCatalog::mode() const { return m_mode; }

void VentilationCatalog::setMode(const QString &modeKey)
{
    if (m_mode == modeKey)
        return;
    if (!ModeCatalog::find(modeKey))
        return;
    m_mode = modeKey;
    emit modeChanged();
    emit catalogChanged();
}

QVariantList VentilationCatalog::modes() const
{
    QVariantList out;
    const auto &all = ModeCatalog::all();
    out.reserve(all.size());
    for (const ModeDefinition &m : all)
        out.append(ModeCatalog::describe(m.key));
    return out;
}

QVariantMap VentilationCatalog::activeMode() const
{
    return ModeCatalog::describe(m_mode);
}

QVariantList VentilationCatalog::settableParameters() const
{
    return ModeCatalog::settableParameters(m_mode, categoryOf(m_patientCategory));
}

QVariantList VentilationCatalog::derivedParameters() const
{
    return ModeCatalog::derivedParameters(m_mode, categoryOf(m_patientCategory));
}

bool VentilationCatalog::nonInvasive() const
{
    return ModeCatalog::findOrDefault(m_mode).nonInvasive;
}

QString VentilationCatalog::clinicalNote() const
{
    return ModeCatalog::findOrDefault(m_mode).clinicalNote;
}

bool VentilationCatalog::usesDirectWeight() const
{
    return sv::domain::usesDirectWeight(categoryOf(m_patientCategory));
}

QVariantMap VentilationCatalog::modeInfo(const QString &modeKey) const
{
    return ModeCatalog::describe(modeKey);
}

QVariantList VentilationCatalog::settableParametersFor(const QString &modeKey,
                                                       const QString &category) const
{
    return ModeCatalog::settableParameters(modeKey, categoryOf(category));
}

QVariantList VentilationCatalog::derivedParametersFor(const QString &modeKey,
                                                      const QString &category) const
{
    return ModeCatalog::derivedParameters(modeKey, categoryOf(category));
}

QVariantMap VentilationCatalog::parameterInfo(const QString &parameterKey) const
{
    return parameterInfoFor(parameterKey, m_patientCategory);
}

QVariantMap VentilationCatalog::parameterInfoFor(const QString &parameterKey,
                                                 const QString &category) const
{
    const ParameterSpec *s = specOf(parameterKey);
    if (!s)
        return {};
    QVariantMap info = ParameterCatalog::describe(s->id, categoryOf(category));
    info.insert(QStringLiteral("settable"), ModeCatalog::isSettable(m_mode, s->id));
    return info;
}

bool VentilationCatalog::isSettable(const QString &parameterKey) const
{
    const ParameterSpec *s = specOf(parameterKey);
    return s && ModeCatalog::isSettable(m_mode, s->id);
}

bool VentilationCatalog::isDerived(const QString &parameterKey) const
{
    const ParameterSpec *s = specOf(parameterKey);
    if (!s)
        return false;
    return ModeCatalog::findOrDefault(m_mode).derived.contains(s->id);
}

QVariantMap VentilationCatalog::validate(const QString &parameterKey, double value) const
{
    QVariantMap result{
        {QStringLiteral("accepted"), false},
        {QStringLiteral("clamped"), false},
        {QStringLiteral("value"), value},
        {QStringLiteral("severity"), QStringLiteral("rejected")},
        {QStringLiteral("message"), QString()}
    };

    const ParameterSpec *s = specOf(parameterKey);
    if (!s) {
        result[QStringLiteral("message")] =
            QCoreApplication::translate("VentilationCatalog",
                                        "Unknown parameter '%1'").arg(parameterKey);
        return result;
    }

    const PatientCategory category = categoryOf(m_patientCategory);

    if (!ModeCatalog::isSettable(m_mode, s->id)) {
        result[QStringLiteral("message")] =
            QCoreApplication::translate("VentilationCatalog",
                                        "%1 is not set in %2 - the ventilator derives it")
                .arg(s->label, ModeCatalog::findOrDefault(m_mode).label);
        return result;
    }

    const ParameterRange range = ParameterCatalog::effectiveRange(s->id, category);

    // Tier 1 and 2: outside the effective range is a hard refusal. This is the
    // boundary that stops an adult tidal volume reaching a neonate.
    if (!range.contains(value)) {
        result[QStringLiteral("value")] = range.clamp(value);
        result[QStringLiteral("clamped")] = true;
        result[QStringLiteral("message")] =
            QCoreApplication::translate("VentilationCatalog",
                                        "%1 must be between %2 and %3 %4 for a %5 patient")
                .arg(s->label)
                .arg(range.minimum, 0, 'f', s->decimals)
                .arg(range.maximum, 0, 'f', s->decimals)
                .arg(s->unit, m_patientCategory.toLower());
        return result;
    }

    result[QStringLiteral("accepted")] = true;

    // Tier 3: inside the range but outside usual practice. Permitted, but the
    // UI is expected to ask for an explicit confirmation.
    const ParameterAdvisory &a = ParameterCatalog::advisory(s->id, category);
    if (a.isSet() && !a.within(value)) {
        result[QStringLiteral("severity")] = QStringLiteral("advisory");
        const QString band = QStringLiteral("%1-%2 %3")
            .arg(a.low, 0, 'f', s->decimals)
            .arg(a.high, 0, 'f', s->decimals)
            .arg(s->unit);
        result[QStringLiteral("message")] = a.rationale.isEmpty()
            ? QCoreApplication::translate("VentilationCatalog",
                                          "Outside the usual %1 range of %2")
                  .arg(m_patientCategory.toLower(), band)
            : QCoreApplication::translate("VentilationCatalog",
                                          "Outside the usual range of %1. %2")
                  .arg(band, a.rationale);
        return result;
    }

    result[QStringLiteral("severity")] = QStringLiteral("ok");
    return result;
}

double VentilationCatalog::startupValue(const QString &parameterKey) const
{
    const ParameterSpec *s = specOf(parameterKey);
    if (!s)
        return 0.0;
    return ParameterCatalog::startupValue(s->id, categoryOf(m_patientCategory));
}

double VentilationCatalog::predictedBodyWeight(double heightCm, bool female) const
{
    return predictedBodyWeightKg(heightCm, female);
}

double VentilationCatalog::tidalVolumeTarget(double weightKg, bool lungProtective) const
{
    const PatientCategory category = categoryOf(m_patientCategory);
    if (lungProtective)
        return weightKg * lungProtectiveTidalVolumePerKg(category);

    double low = 0.0;
    double high = 0.0;
    tidalVolumePerKgRange(category, &low, &high);
    return weightKg * (low + high) * 0.5;
}

QStringList VentilationCatalog::primaryAlarms() const
{
    return ModeCatalog::findOrDefault(m_mode).primaryAlarms;
}

QStringList VentilationCatalog::relaxedAlarms() const
{
    return ModeCatalog::findOrDefault(m_mode).relaxedAlarms;
}

bool VentilationCatalog::isAlarmRelaxed(const QString &conditionId) const
{
    return ModeCatalog::findOrDefault(m_mode).relaxedAlarms.contains(conditionId);
}
