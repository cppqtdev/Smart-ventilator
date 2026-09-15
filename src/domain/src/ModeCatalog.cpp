// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/domain/ModeCatalog.h>

#include <QHash>

namespace sv::domain {
namespace {

using P = ParameterId;

// Parameters every mode exposes. Oxygen and PEEP are set in essentially all
// ventilation; hoisting them keeps each mode entry to what actually
// distinguishes it.
const QVector<P> kUniversal { P::Fio2, P::Peep };

QVector<P> with(const QVector<P> &head, const QVector<P> &tail)
{
    QVector<P> out = head;
    out.reserve(head.size() + tail.size());
    for (P id : tail)
        out.append(id);
    return out;
}

QVector<ModeDefinition> buildCatalog()
{
    QVector<ModeDefinition> modes;

    // -- Volume control -----------------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("VCV");
        m.label = QStringLiteral("VC-CMV");
        m.shortLabel = QStringLiteral("VCV");
        m.description = QStringLiteral("Volume control, mandatory");
        m.controlVariable = ControlVariable::Volume;
        m.sequence = BreathSequence::ContinuousMandatory;
        m.settable = with(kUniversal, { P::TidalVolume, P::RespiratoryRate,
                                        P::IeRatio, P::InspiratoryTime,
                                        P::FlowPattern, P::FlowTrigger,
                                        P::PressureLimit, P::SighEnabled });
        // Pressure is the dependent variable here, so it is the early warning
        // that compliance is deteriorating.
        m.derived = { P::PInsp };
        m.primaryMetrics = { QStringLiteral("ppeak"), QStringLiteral("pplat"),
                             QStringLiteral("pmean"), QStringLiteral("compliance"),
                             QStringLiteral("resistance"), QStringLiteral("drivingPressure") };
        m.primaryAlarms = { QStringLiteral("paw.high"), QStringLiteral("paw.low"),
                            QStringLiteral("mv.high"), QStringLiteral("mv.low"),
                            QStringLiteral("obstruction") };
        m.clinicalNote = QStringLiteral(
            "Tidal volume is guaranteed; airway pressure varies with the "
            "patient's compliance and resistance. Square flow makes the "
            "inspiratory resistance and stress index measurements valid.");
        modes.append(m);
    }

    // -- Pressure control ---------------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("PCV");
        m.label = QStringLiteral("PC-CMV");
        m.shortLabel = QStringLiteral("PCV");
        m.description = QStringLiteral("Pressure control, mandatory");
        m.controlVariable = ControlVariable::Pressure;
        m.sequence = BreathSequence::ContinuousMandatory;
        m.settable = with(kUniversal, { P::PInsp, P::RespiratoryRate, P::IeRatio,
                                        P::InspiratoryTime, P::RiseTime,
                                        P::FlowTrigger, P::SighEnabled });
        // Volume is the dependent variable: a falling VTe at a fixed pressure
        // is the signal that something has changed.
        m.derived = { P::TidalVolume };
        m.primaryMetrics = { QStringLiteral("vte"), QStringLiteral("mve"),
                             QStringLiteral("pmean"), QStringLiteral("compliance"),
                             QStringLiteral("drivingPressure") };
        m.primaryAlarms = { QStringLiteral("vte.low"), QStringLiteral("vte.high"),
                            QStringLiteral("mv.low"), QStringLiteral("paw.low"),
                            QStringLiteral("obstruction") };
        m.clinicalNote = QStringLiteral(
            "Airway pressure is guaranteed; tidal volume varies with the "
            "patient's mechanics. Watch expired tidal volume, not pressure, "
            "for deterioration.");
        modes.append(m);
    }

    // -- Adaptive pressure / PRVC ------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("PRVC");
        m.label = QStringLiteral("PRVC");
        m.shortLabel = QStringLiteral("PRVC");
        m.description = QStringLiteral("Pressure-regulated volume control");
        m.controlVariable = ControlVariable::DualAdaptive;
        m.sequence = BreathSequence::ContinuousMandatory;
        m.settable = with(kUniversal, { P::TidalVolume, P::RespiratoryRate,
                                        P::IeRatio, P::RiseTime, P::FlowTrigger,
                                        P::PressureLimit, P::SighEnabled });
        // The controller moves Pinsp breath to breath to hit the target VT, so
        // the delivered pressure is the diagnostic value and must be displayed
        // prominently rather than hidden.
        m.derived = { P::PInsp };
        m.primaryMetrics = { QStringLiteral("pinspDelivered"), QStringLiteral("ppeak"),
                             QStringLiteral("compliance"), QStringLiteral("drivingPressure") };
        m.primaryAlarms = { QStringLiteral("pressure.limit.reached"),
                            QStringLiteral("vte.low"), QStringLiteral("paw.high"),
                            QStringLiteral("obstruction") };
        m.clinicalNote = QStringLiteral(
            "The ventilator adjusts inspiratory pressure breath to breath to "
            "reach the target volume. A rising delivered Pinsp is the earliest "
            "sign of worsening compliance.");
        modes.append(m);
    }

    // -- SIMV ---------------------------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("SIMV");
        m.label = QStringLiteral("SIMV (VC) + PS");
        m.shortLabel = QStringLiteral("SIMV");
        m.description = QStringLiteral("Synchronised intermittent mandatory");
        m.controlVariable = ControlVariable::Volume;
        m.sequence = BreathSequence::IntermittentMandatory;
        m.settable = with(kUniversal, { P::TidalVolume, P::RespiratoryRate,
                                        P::PSupport, P::IeRatio, P::InspiratoryTime,
                                        P::FlowPattern, P::FlowTrigger, P::Ets,
                                        P::RiseTime, P::TiMax, P::SighEnabled });
        m.derived = { P::PInsp };
        m.primaryMetrics = { QStringLiteral("fspont"), QStringLiteral("ftotal"),
                             QStringLiteral("vte"), QStringLiteral("mve"),
                             QStringLiteral("rsbi") };
        m.primaryAlarms = { QStringLiteral("apnea"), QStringLiteral("mv.low"),
                            QStringLiteral("rate.high"), QStringLiteral("paw.high") };
        m.clinicalNote = QStringLiteral(
            "Mandatory breaths are synchronised to patient effort; breaths "
            "between them are pressure supported. Watch the spontaneous "
            "fraction of the total rate.");
        modes.append(m);
    }

    // -- Pressure support / CPAP -------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("PSV");
        m.label = QStringLiteral("PSV / SPONT");
        m.shortLabel = QStringLiteral("PSV");
        m.description = QStringLiteral("Pressure support, spontaneous");
        m.controlVariable = ControlVariable::Spontaneous;
        m.sequence = BreathSequence::ContinuousSpontaneous;
        m.requiresPatientEffort = true;
        m.settable = with(kUniversal, { P::PSupport, P::RiseTime, P::FlowTrigger,
                                        P::Ets, P::TiMax, P::ApneaTime,
                                        P::BackupRate, P::BackupTidalVolume });
        // Everything about the breath is patient-determined here.
        m.derived = { P::TidalVolume, P::RespiratoryRate, P::InspiratoryTime };
        m.primaryMetrics = { QStringLiteral("vte"), QStringLiteral("ftotal"),
                             QStringLiteral("rsbi"), QStringLiteral("p01"),
                             QStringLiteral("mve") };
        // Apnoea is the critical alarm in a fully spontaneous mode: there is
        // no mandatory rate underneath the patient.
        m.primaryAlarms = { QStringLiteral("apnea"), QStringLiteral("mv.low"),
                            QStringLiteral("vte.low"), QStringLiteral("rate.high") };
        m.clinicalNote = QStringLiteral(
            "Every breath is patient-initiated and patient-terminated. Apnoea "
            "backup must be configured - there is no mandatory rate beneath "
            "the patient.");
        modes.append(m);
    }

    {
        ModeDefinition m;
        m.key = QStringLiteral("CPAP");
        m.label = QStringLiteral("CPAP");
        m.shortLabel = QStringLiteral("CPAP");
        m.description = QStringLiteral("Continuous positive airway pressure");
        m.controlVariable = ControlVariable::Spontaneous;
        m.sequence = BreathSequence::ContinuousSpontaneous;
        m.requiresPatientEffort = true;
        m.settable = with(kUniversal, { P::FlowTrigger, P::ApneaTime,
                                        P::BackupRate, P::BackupTidalVolume });
        m.derived = { P::TidalVolume, P::RespiratoryRate, P::InspiratoryTime };
        m.primaryMetrics = { QStringLiteral("vte"), QStringLiteral("ftotal"),
                             QStringLiteral("mve"), QStringLiteral("rsbi") };
        m.primaryAlarms = { QStringLiteral("apnea"), QStringLiteral("mv.low") };
        m.clinicalNote = QStringLiteral(
            "Constant airway pressure with no inspiratory assistance. "
            "Functionally pressure support set to zero.");
        modes.append(m);
    }

    // -- BiPAP / DuoPAP -----------------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("BiPAP");
        m.label = QStringLiteral("DuoPAP / BiPAP");
        m.shortLabel = QStringLiteral("BiPAP");
        m.description = QStringLiteral("Two pressure levels, spontaneous permitted");
        m.controlVariable = ControlVariable::Pressure;
        m.sequence = BreathSequence::IntermittentMandatory;
        m.settable = with(kUniversal, { P::PHigh, P::THigh, P::RespiratoryRate,
                                        P::PSupport, P::RiseTime, P::FlowTrigger,
                                        P::Ets });
        m.derived = { P::TidalVolume };
        m.primaryMetrics = { QStringLiteral("vte"), QStringLiteral("mve"),
                             QStringLiteral("fspont"), QStringLiteral("pmean") };
        m.primaryAlarms = { QStringLiteral("vte.low"), QStringLiteral("mv.low"),
                            QStringLiteral("paw.high") };
        m.clinicalNote = QStringLiteral(
            "The patient may breathe spontaneously at either pressure level.");
        modes.append(m);
    }

    // -- APRV ---------------------------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("APRV");
        m.label = QStringLiteral("APRV");
        m.shortLabel = QStringLiteral("APRV");
        m.description = QStringLiteral("Airway pressure release ventilation");
        m.controlVariable = ControlVariable::Pressure;
        m.sequence = BreathSequence::IntermittentMandatory;
        m.settable = { P::Fio2, P::PHigh, P::THigh, P::PLow, P::TLow,
                       P::RiseTime, P::FlowTrigger };
        m.derived = { P::TidalVolume, P::RespiratoryRate };
        m.primaryMetrics = { QStringLiteral("vteSpont"), QStringLiteral("releaseVolume"),
                             QStringLiteral("fspont"), QStringLiteral("pmean"),
                             QStringLiteral("autoPeep") };
        m.primaryAlarms = { QStringLiteral("mv.low"), QStringLiteral("paw.high"),
                            QStringLiteral("apnea") };
        m.clinicalNote = QStringLiteral(
            "PEEP is not set directly - the low pressure level and the release "
            "time set it together. T Low is set from the expiratory time "
            "constant; too long permits derecruitment.");
        modes.append(m);
    }

    // -- Non-invasive -------------------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("NIV");
        m.label = QStringLiteral("NIV");
        m.shortLabel = QStringLiteral("NIV");
        m.description = QStringLiteral("Non-invasive pressure support");
        m.controlVariable = ControlVariable::Spontaneous;
        m.sequence = BreathSequence::ContinuousSpontaneous;
        m.nonInvasive = true;
        m.requiresPatientEffort = true;
        m.settable = with(kUniversal, { P::PSupport, P::RiseTime, P::FlowTrigger,
                                        P::Ets, P::TiMax, P::ApneaTime });
        m.derived = { P::TidalVolume, P::RespiratoryRate, P::InspiratoryTime };
        m.primaryMetrics = { QStringLiteral("leakPercent"), QStringLiteral("leakLpm"),
                             QStringLiteral("vte"), QStringLiteral("ftotal"),
                             QStringLiteral("mve") };
        // With a vented mask the intentional leak is large, so leak is the
        // primary alarm and the volume alarms retarget onto the leak-compensated
        // tidal volume rather than raw VTe.
        m.primaryAlarms = { QStringLiteral("leak.high"), QStringLiteral("apnea"),
                            QStringLiteral("mv.low") };
        m.relaxedAlarms = { QStringLiteral("vte.low"), QStringLiteral("paw.low"),
                            QStringLiteral("circuit.disconnect") };
        m.clinicalNote = QStringLiteral(
            "Leak is expected and is reported in L/min rather than as a "
            "percentage. Volume alarms follow the leak-compensated tidal "
            "volume; disconnection detection is retuned for a vented mask.");
        modes.append(m);
    }

    {
        ModeDefinition m;
        m.key = QStringLiteral("NIV-ST");
        m.label = QStringLiteral("NIV-ST");
        m.shortLabel = QStringLiteral("NIV-ST");
        m.description = QStringLiteral("Non-invasive spontaneous / timed");
        m.controlVariable = ControlVariable::Pressure;
        m.sequence = BreathSequence::IntermittentMandatory;
        m.nonInvasive = true;
        m.settable = with(kUniversal, { P::PInsp, P::RespiratoryRate,
                                        P::InspiratoryTime, P::RiseTime,
                                        P::FlowTrigger, P::Ets, P::TiMax });
        m.derived = { P::TidalVolume };
        m.primaryMetrics = { QStringLiteral("leakLpm"), QStringLiteral("vte"),
                             QStringLiteral("fspont"), QStringLiteral("mve") };
        m.primaryAlarms = { QStringLiteral("leak.high"), QStringLiteral("mv.low") };
        m.relaxedAlarms = { QStringLiteral("vte.low"), QStringLiteral("paw.low"),
                            QStringLiteral("circuit.disconnect") };
        m.clinicalNote = QStringLiteral(
            "Backup mandatory breaths are delivered if the patient's rate "
            "falls below the set rate.");
        modes.append(m);
    }

    // -- Adaptive support ---------------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("ASV");
        m.label = QStringLiteral("ASV");
        m.shortLabel = QStringLiteral("ASV");
        m.description = QStringLiteral("Adaptive support ventilation");
        m.controlVariable = ControlVariable::DualAdaptive;
        m.sequence = BreathSequence::IntermittentMandatory;
        m.settable = with(kUniversal, { P::MinVolumePercent, P::PasvLimit,
                                        P::RiseTime, P::FlowTrigger, P::Ets,
                                        P::SighEnabled });
        // Everything except the minute-volume target is computed from the
        // measured compliance and expiratory time constant.
        m.derived = { P::TidalVolume, P::RespiratoryRate, P::PInsp, P::IeRatio };
        m.primaryMetrics = { QStringLiteral("targetVt"), QStringLiteral("targetRate"),
                             QStringLiteral("pinspDelivered"), QStringLiteral("rcexp"),
                             QStringLiteral("compliance"), QStringLiteral("mve") };
        m.primaryAlarms = { QStringLiteral("mv.low"), QStringLiteral("mv.high"),
                            QStringLiteral("pressure.limit.reached"),
                            QStringLiteral("apnea") };
        m.clinicalNote = QStringLiteral(
            "Only the minute volume target is set. Tidal volume, rate, "
            "inspiratory pressure and I:E are derived from the measured "
            "mechanics using a least-work-of-breathing calculation.");
        modes.append(m);
    }

    // -- HFOV ---------------------------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("HFOV");
        m.label = QStringLiteral("HFOV");
        m.shortLabel = QStringLiteral("HFOV");
        m.description = QStringLiteral("High-frequency oscillatory");
        m.controlVariable = ControlVariable::Oscillatory;
        m.sequence = BreathSequence::ContinuousMandatory;
        m.supportsApneaBackup = false;
        m.settable = { P::Fio2, P::HfovMeanPaw, P::HfovAmplitude, P::HfovFrequency };
        m.derived = {};
        m.primaryMetrics = { QStringLiteral("dco2"), QStringLiteral("pmean"),
                             QStringLiteral("amplitudeDelivered") };
        m.primaryAlarms = { QStringLiteral("pmean.high"), QStringLiteral("pmean.low"),
                            QStringLiteral("circuit.disconnect") };
        m.relaxedAlarms = { QStringLiteral("vte.low"), QStringLiteral("vte.high"),
                            QStringLiteral("apnea"), QStringLiteral("mv.low") };
        m.clinicalNote = QStringLiteral(
            "Tidal volumes are below dead space, so volume and apnoea alarms "
            "do not apply. Gas exchange is tracked with DCO2 (frequency times "
            "tidal volume squared).");
        modes.append(m);
    }

    // -- High-flow oxygen ---------------------------------------------------
    {
        ModeDefinition m;
        m.key = QStringLiteral("HFO2");
        m.label = QStringLiteral("High Flow O2");
        m.shortLabel = QStringLiteral("HF O2");
        m.description = QStringLiteral("High-flow oxygen therapy");
        m.controlVariable = ControlVariable::Flow;
        m.sequence = BreathSequence::ContinuousSpontaneous;
        m.nonInvasive = true;
        m.supportsApneaBackup = false;
        m.settable = { P::Fio2, P::HighFlowRate };
        m.derived = {};
        m.primaryMetrics = { QStringLiteral("spo2"), QStringLiteral("ftotal") };
        m.primaryAlarms = { QStringLiteral("fio2.low"), QStringLiteral("fio2.high"),
                            QStringLiteral("flow.low") };
        m.relaxedAlarms = { QStringLiteral("vte.low"), QStringLiteral("vte.high"),
                            QStringLiteral("mv.low"), QStringLiteral("apnea"),
                            QStringLiteral("paw.high"), QStringLiteral("paw.low") };
        m.clinicalNote = QStringLiteral(
            "Oxygen therapy, not ventilation. The ventilation alarms are not "
            "applicable and are disabled.");
        modes.append(m);
    }

    return modes;
}

const QVector<ModeDefinition> &catalog()
{
    static const QVector<ModeDefinition> modes = buildCatalog();
    return modes;
}

const QHash<QString, int> &keyIndex()
{
    static const QHash<QString, int> index = []() {
        QHash<QString, int> map;
        const auto &modes = catalog();
        for (int i = 0; i < modes.size(); ++i)
            map.insert(modes.at(i).key, i);
        return map;
    }();
    return index;
}

QString controlVariableName(ControlVariable variable)
{
    switch (variable) {
    case ControlVariable::Volume:       return QStringLiteral("volume");
    case ControlVariable::Pressure:     return QStringLiteral("pressure");
    case ControlVariable::DualAdaptive: return QStringLiteral("adaptive");
    case ControlVariable::Spontaneous:  return QStringLiteral("spontaneous");
    case ControlVariable::Oscillatory:  return QStringLiteral("oscillatory");
    case ControlVariable::Flow:         return QStringLiteral("flow");
    }
    return QStringLiteral("pressure");
}

QString breathSequenceName(BreathSequence sequence)
{
    switch (sequence) {
    case BreathSequence::ContinuousMandatory:   return QStringLiteral("CMV");
    case BreathSequence::IntermittentMandatory: return QStringLiteral("IMV");
    case BreathSequence::ContinuousSpontaneous: return QStringLiteral("CSV");
    }
    return QStringLiteral("CMV");
}

} // namespace

namespace ModeCatalog {

const QVector<ModeDefinition> &all() { return catalog(); }

const ModeDefinition *find(const QString &key)
{
    const auto it = keyIndex().constFind(key);
    if (it == keyIndex().constEnd())
        return nullptr;
    return &catalog().at(*it);
}

const ModeDefinition &findOrDefault(const QString &key)
{
    if (const ModeDefinition *mode = find(key))
        return *mode;
    // PCV is the fallback rather than the first entry by accident: it is the
    // mode whose control set is closest to universally applicable, so an
    // unrecognised key never leaves the operator without a usable screen.
    static const ModeDefinition &fallback = *find(QStringLiteral("PCV"));
    return fallback;
}

bool isSettable(const QString &modeKey, ParameterId id)
{
    const ModeDefinition *mode = find(modeKey);
    return mode && mode->settable.contains(id);
}

QStringList keys()
{
    QStringList out;
    out.reserve(catalog().size());
    for (const ModeDefinition &m : catalog())
        out.append(m.key);
    return out;
}

QVariantMap describe(const QString &modeKey)
{
    const ModeDefinition &m = findOrDefault(modeKey);
    return QVariantMap{
        {QStringLiteral("key"), m.key},
        {QStringLiteral("label"), m.label},
        {QStringLiteral("shortLabel"), m.shortLabel},
        {QStringLiteral("description"), m.description},
        {QStringLiteral("controlVariable"), controlVariableName(m.controlVariable)},
        {QStringLiteral("breathSequence"), breathSequenceName(m.sequence)},
        {QStringLiteral("nonInvasive"), m.nonInvasive},
        {QStringLiteral("supportsApneaBackup"), m.supportsApneaBackup},
        {QStringLiteral("requiresPatientEffort"), m.requiresPatientEffort},
        {QStringLiteral("primaryMetrics"), m.primaryMetrics},
        {QStringLiteral("primaryAlarms"), m.primaryAlarms},
        {QStringLiteral("relaxedAlarms"), m.relaxedAlarms},
        {QStringLiteral("clinicalNote"), m.clinicalNote}
    };
}

QVariantList settableParameters(const QString &modeKey, PatientCategory category)
{
    const ModeDefinition &m = findOrDefault(modeKey);
    QVariantList out;
    out.reserve(m.settable.size());
    for (ParameterId id : m.settable) {
        QVariantMap entry = ParameterCatalog::describe(id, category);
        entry.insert(QStringLiteral("settable"), true);
        out.append(entry);
    }
    return out;
}

QVariantList derivedParameters(const QString &modeKey, PatientCategory category)
{
    const ModeDefinition &m = findOrDefault(modeKey);
    QVariantList out;
    out.reserve(m.derived.size());
    for (ParameterId id : m.derived) {
        QVariantMap entry = ParameterCatalog::describe(id, category);
        entry.insert(QStringLiteral("settable"), false);
        out.append(entry);
    }
    return out;
}

} // namespace ModeCatalog
} // namespace sv::domain
