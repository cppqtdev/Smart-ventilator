// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/presentation/MonitoringPresenter.h>

#include <sv/common/LogBuffer.h>
#include <sv/common/Units.h>

#include <QVector>

#include "../../controllers/AlarmController.h"
#include "../../controllers/PatientController.h"
#include "../../controllers/VentilatorController.h"

#include <cmath>

namespace sv::presentation {
namespace {

QString formatted(double value, int decimals)
{
    if (!std::isfinite(value))
        return QStringLiteral("---");
    return QString::number(value, 'f', decimals);
}

} // namespace

MonitoringPresenter::MonitoringPresenter(QObject *parent)
    : QObject(parent)
{
}

MonitoringPresenter::~MonitoringPresenter() = default;

void MonitoringPresenter::attach(VentilatorController *ventilator,
                                 PatientController *patient,
                                 AlarmController *alarms)
{
    if (m_ventilator != nullptr)
        m_ventilator->disconnect(this);
    if (m_patient != nullptr)
        m_patient->disconnect(this);
    if (m_alarms != nullptr)
        m_alarms->disconnect(this);

    m_ventilator = ventilator;
    m_patient = patient;
    m_alarms = alarms;

    if (m_ventilator != nullptr) {
        connect(m_ventilator, &VentilatorController::waveformChanged,
                this, &MonitoringPresenter::publishLatestSamples);
        connect(m_ventilator, &VentilatorController::measurementsChanged,
                this, &MonitoringPresenter::measurementsChanged);
        connect(m_ventilator, &VentilatorController::settingsChanged,
                this, &MonitoringPresenter::settingsChanged);
        connect(m_ventilator, &VentilatorController::frozenChanged,
                this, &MonitoringPresenter::settingsChanged);
        connect(m_ventilator, &VentilatorController::runningChanged,
                this, &MonitoringPresenter::settingsChanged);
        connect(m_ventilator, &VentilatorController::readinessChanged,
                this, &MonitoringPresenter::settingsChanged);
    }
    if (m_patient != nullptr) {
        connect(m_patient, &PatientController::patientChanged,
                this, &MonitoringPresenter::patientChanged);
    }
    if (m_alarms != nullptr) {
        connect(m_alarms, &AlarmController::conditionsChanged,
                this, &MonitoringPresenter::bannerChanged);
    }

    emit measurementsChanged();
    emit settingsChanged();
    emit bannerChanged();
    emit patientChanged();
}

QVariantMap MonitoringPresenter::tile(const QString &key, const QString &label,
                                      double value, int decimals, const QString &unit,
                                      const QString &upper, const QString &lower) const
{
    return QVariantMap{
        {QStringLiteral("key"), key},
        {QStringLiteral("label"), label},
        {QStringLiteral("value"), formatted(value, decimals)},
        {QStringLiteral("unit"), unit},
        {QStringLiteral("upper"), upper},
        {QStringLiteral("lower"), lower}
    };
}

QVariantMap MonitoringPresenter::readout(const QString &key, const QString &label,
                                         double value, int decimals,
                                         const QString &unit, bool accent) const
{
    return QVariantMap{
        {QStringLiteral("key"), key},
        {QStringLiteral("label"), label},
        {QStringLiteral("value"), formatted(value, decimals)},
        {QStringLiteral("unit"), unit},
        {QStringLiteral("accent"), accent}
    };
}

QVariantMap MonitoringPresenter::dial(const QString &key, const QString &label,
                                      double value, double from, double to,
                                      double step, const QString &unit,
                                      int decimals) const
{
    return QVariantMap{
        {QStringLiteral("key"), key},
        {QStringLiteral("label"), label},
        {QStringLiteral("value"), value},
        {QStringLiteral("from"), from},
        {QStringLiteral("to"), to},
        {QStringLiteral("step"), step},
        {QStringLiteral("unit"), unit},
        {QStringLiteral("decimals"), decimals}
    };
}

QVariantList MonitoringPresenter::sidebarTiles() const
{
    if (m_ventilator == nullptr)
        return {};

    const QString pressure = QString::fromUtf8(units::Symbol::pressure);
    return QVariantList{
        tile(QStringLiteral("ppeak"), tr("Ppeak"), m_ventilator->ppeak(), 0, pressure,
             QString::number(m_ventilator->alarmHighPressure()),
             QString::number(m_ventilator->alarmLowPressure())),
        tile(QStringLiteral("vte"), tr("VTE"), m_ventilator->vte(), 0,
             QString::fromUtf8(units::Symbol::volume),
             QString(), QString::number(m_ventilator->alarmLowVt())),
        tile(QStringLiteral("mv"), tr("MV"), m_ventilator->expMinVol(), 1,
             QString::fromUtf8(units::Symbol::minuteVolume),
             QString::number(m_ventilator->alarmHighMv()), QString()),
        tile(QStringLiteral("ftotal"), tr("fTotal"), m_ventilator->ftotal(), 0,
             QString::fromUtf8(units::Symbol::rate), QString(), QString()),
        tile(QStringLiteral("pmean"), tr("Pmean"), m_ventilator->pmean(), 0, pressure,
             QString(), QString())
    };
}

QVariantList MonitoringPresenter::readouts() const
{
    if (m_ventilator == nullptr)
        return {};

    return QVariantList{
        readout(QStringLiteral("totalPeep"), tr("PEEPtot"), m_ventilator->totalPeep(), 0,
                QString::fromUtf8(units::Symbol::pressure)),
        readout(QStringLiteral("fspont"), tr("fSpont"), m_ventilator->spontaneousRate(), 0,
                QStringLiteral("1/min")),
        readout(QStringLiteral("petco2"), tr("PetCO2"), m_ventilator->etco2(), 0,
                QString::fromUtf8(units::Symbol::partialPressure), true),
        readout(QStringLiteral("cstat"), tr("Cstat"), m_ventilator->compliance(), 1,
                QString::fromUtf8(units::Symbol::compliance)),
        readout(QStringLiteral("rinsp"), tr("Rinsp"), m_ventilator->resistance(), 0,
                QString::fromUtf8(units::Symbol::resistance)),
        readout(QStringLiteral("spo2"), tr("SpO2"), m_ventilator->spo2(), 0,
                QString::fromUtf8(units::Symbol::percent))
    };
}

QVariantList MonitoringPresenter::dials() const
{
    if (m_ventilator == nullptr)
        return {};

    return QVariantList{
        dial(QStringLiteral("fio2"), tr("Oxygen"), m_ventilator->fio2(),
             units::roomAirFio2Percent, units::pureOxygenFio2Percent, 1,
             QString::fromUtf8(units::Symbol::percent)),
        dial(QStringLiteral("peep"), tr("PEEP C/PAP"), m_ventilator->peep(),
             0, 35, 1, QString::fromUtf8(units::Symbol::pressure)),
        dial(QStringLiteral("minuteVolume"), tr("%MinVol"), m_ventilator->minuteVolume(),
             25, 350, 5, QString::fromUtf8(units::Symbol::percent))
    };
}

QVariantList MonitoringPresenter::banner() const
{
    return m_alarms != nullptr ? m_alarms->banner() : QVariantList{};
}

QVariantMap MonitoringPresenter::patient() const
{
    if (m_patient == nullptr)
        return {};

    return QVariantMap{
        {QStringLiteral("category"), m_patient->category()},
        {QStringLiteral("gender"), m_patient->gender()},
        {QStringLiteral("height"), m_patient->height()},
        {QStringLiteral("weight"), m_patient->weight()},
        {QStringLiteral("ibw"), m_patient->ibw()}
    };
}

QString MonitoringPresenter::mode() const
{
    return m_ventilator != nullptr ? m_ventilator->mode() : QStringLiteral("---");
}

bool MonitoringPresenter::ventilating() const
{
    return m_ventilator != nullptr && m_ventilator->running();
}

int MonitoringPresenter::measuredRate() const
{
    if (m_ventilator == nullptr)
        return 0;
    return int(qRound(m_ventilator->ftotal()));
}

QVariantMap MonitoringPresenter::asvTarget() const
{
    if (m_ventilator == nullptr)
        return {};

    const double rate = m_ventilator->ftotal();
    const double tidalVolume = m_ventilator->vte();
    const double minuteVolume = m_ventilator->expMinVol();
    const double ibw = qMax(1, m_ventilator->patientIbwKg());

    // The window an adaptive mode is allowed to pick a rate and a volume
    // inside: fast enough to clear carbon dioxide, slow enough to let the
    // lung empty, and a volume that stays off both the dead space at the
    // bottom and the overdistension limit at the top.
    const double minVolume = 4.4 * ibw;
    const double maxVolume = 12.0 * ibw;

    // What the mode is asking for against what the patient is giving back.
    // A row with no target is a measurement the device does not aim at, and
    // one with no current has not been measured yet; both read as dashes.
    const auto row = [](const QString &label, const QString &unit,
                        const QVariant &target, const QVariant &current) {
        return QVariantMap{
            {QStringLiteral("label"), label},
            {QStringLiteral("unit"), unit},
            {QStringLiteral("target"), target},
            {QStringLiteral("current"), current}
        };
    };

    const auto measured = [](double value) {
        return value > 0.0 ? QVariant(value) : QVariant();
    };

    const double setPinsp = m_ventilator->peep() + m_ventilator->pressureSupport();

    QVariantList rows{
        row(QStringLiteral("Pinsp"), QStringLiteral("cmH2O"),
            setPinsp, measured(m_ventilator->ppeak())),
        row(QStringLiteral("fControl"), QStringLiteral("b/min"),
            m_ventilator->respiratoryRate(), measured(rate)),
        row(QStringLiteral("SpO2"), QStringLiteral("%"),
            QVariant(), measured(m_ventilator->spo2()))
    };

    QVariantList settingRows{
        row(QStringLiteral("Pinsp"), QStringLiteral("cmH2O"),
            setPinsp, measured(m_ventilator->ppeak())),
        row(QStringLiteral("Plateau"), QStringLiteral("cmH2O"),
            QVariant(), measured(m_ventilator->pplat())),
        row(QStringLiteral("Pmean"), QStringLiteral("cmH2O"),
            QVariant(), measured(m_ventilator->pmean())),
        row(QStringLiteral("PEEP/CPAP"), QStringLiteral("cmH2O"),
            m_ventilator->peep(), measured(m_ventilator->totalPeep())),
        row(QStringLiteral("Vt/IBW"), QStringLiteral("mL/kg"),
            QVariant(), measured(tidalVolume / ibw))
    };

    return QVariantMap{
        {QStringLiteral("minuteVolume"), minuteVolume},
        {QStringLiteral("rate"), rate},
        {QStringLiteral("tidalVolume"), tidalVolume},
        {QStringLiteral("minRate"), 15.0},
        {QStringLiteral("maxRate"), 60.0},
        {QStringLiteral("minVolume"), minVolume},
        {QStringLiteral("maxVolume"), maxVolume},
        {QStringLiteral("rows"), rows},
        {QStringLiteral("settingRows"), settingRows}
    };
}

namespace {

// A scale is snapped onto a ladder rather than taking whatever number the
// arithmetic produced, so the lane keeps a round full-scale value the
// operator can read a trace against, and so the scale does not creep every
// time a setting moves by one.
double snapUp(double wanted, const QVector<double> &ladder)
{
    for (double step : ladder) {
        if (wanted <= step)
            return step;
    }
    return ladder.isEmpty() ? wanted : ladder.last();
}

const QVector<double> kFlowLadder{5, 10, 15, 20, 30, 40, 60, 75, 100, 120, 150, 180};
const QVector<double> kVolumeLadder{20, 50, 100, 200, 300, 500, 800, 1200, 1600, 2000};
const QVector<double> kPressureLadder{20, 30, 40, 50, 60, 80, 100};
const QVector<double> kCo2Ladder{40, 60, 80, 100};

} // namespace

QVariantMap MonitoringPresenter::waveformScales() const
{
    if (m_ventilator == nullptr) {
        return QVariantMap{
            {QStringLiteral("pressure"), 40.0},
            {QStringLiteral("flow"), 75.0},
            {QStringLiteral("volume"), 800.0},
            {QStringLiteral("co2"), 60.0}
        };
    }

    // Pressure is read against the limit that would alarm, so the lane is
    // sized from that limit rather than from the patient, and with no
    // headroom over it: a trace at the top of the lane is a trace at the
    // limit, which is what the reference screens draw.
    const double pressure = snapUp(m_ventilator->alarmHighPressure(), kPressureLadder);

    // Peak inspiratory flow runs about one and a half litres a minute per
    // kilogram of ideal body weight across the categories this device covers,
    // with headroom over it so a spontaneous effort is not clipped.
    const double ibw = qMax(1, m_ventilator->patientIbwKg());
    const double flow = snapUp(qMax(5.0, ibw * 1.5), kFlowLadder);

    // Volume is read against the breath that is set, with the same headroom.
    const double volume = snapUp(qMax(20.0, m_ventilator->tidalVolume() * 1.6), kVolumeLadder);

    const double co2 = snapUp(m_ventilator->alarmHighEtco2() * 1.3, kCo2Ladder);

    return QVariantMap{
        {QStringLiteral("pressure"), pressure},
        {QStringLiteral("flow"), flow},
        {QStringLiteral("volume"), volume},
        {QStringLiteral("co2"), co2}
    };
}

QString MonitoringPresenter::readinessReason() const
{
    return m_ventilator != nullptr ? m_ventilator->readinessReason() : QString();
}

bool MonitoringPresenter::frozen() const
{
    return m_ventilator != nullptr && m_ventilator->frozen();
}

bool MonitoringPresenter::nonInvasive() const
{
    return m_ventilator != nullptr && m_ventilator->nonInvasive();
}

bool MonitoringPresenter::spontaneous() const
{
    return m_ventilator != nullptr && m_ventilator->spontaneousRate() > 0.0;
}

bool MonitoringPresenter::requestSetting(const QString &key, double value)
{
    if (m_ventilator == nullptr)
        return false;

    const int target = int(std::lround(value));
    const QVariant before = m_ventilator->property(key.toUtf8().constData());
    const bool accepted = m_ventilator->requestParameterChange(key, target);

    if (auto *log = sv::common::LogBuffer::instance()) {
        log->noteControl(key, before, target, accepted,
                         accepted ? QString() : m_ventilator->lastCommandMessage());
    }
    return accepted;
}

QVariantList MonitoringPresenter::waveform(const QString &channelKey) const
{
    if (m_ventilator == nullptr)
        return {};
    if (channelKey == QLatin1String("paw"))
        return m_ventilator->pressureWaveform();
    if (channelKey == QLatin1String("flow"))
        return m_ventilator->flowWaveform();
    if (channelKey == QLatin1String("volume"))
        return m_ventilator->volumeWaveform();
    if (channelKey == QLatin1String("co2"))
        return m_ventilator->co2Waveform();
    return {};
}

void MonitoringPresenter::publishLatestSamples()
{
    if (m_ventilator == nullptr)
        return;

    const struct { const char *key; QVariantList buffer; } channels[] = {
        {"paw", m_ventilator->pressureWaveform()},
        {"flow", m_ventilator->flowWaveform()},
        {"volume", m_ventilator->volumeWaveform()},
        {"co2", m_ventilator->co2Waveform()}
    };

    for (const auto &channel : channels) {
        if (channel.buffer.isEmpty())
            continue;
        emit sampleAppended(QString::fromLatin1(channel.key),
                            channel.buffer.constLast().toDouble());
    }
}

bool MonitoringPresenter::requestStart()
{
    if (m_ventilator == nullptr)
        return false;
    const bool started = m_ventilator->requestStartVentilation();
    if (auto *log = sv::common::LogBuffer::instance()) {
        log->noteControl(QStringLiteral("ventilation"), QStringLiteral("standby"),
                         QStringLiteral("running"), started,
                         started ? QString() : m_ventilator->lastCommandMessage());
    }
    return started;
}

void MonitoringPresenter::requestStop()
{
    if (m_ventilator == nullptr)
        return;
    const bool wasRunning = m_ventilator->running();
    m_ventilator->stopVentilation();
    if (auto *log = sv::common::LogBuffer::instance()) {
        log->noteControl(QStringLiteral("ventilation"),
                         wasRunning ? QStringLiteral("running") : QStringLiteral("standby"),
                         QStringLiteral("standby"), true);
    }
}

void MonitoringPresenter::toggleFreeze()
{
    if (m_ventilator == nullptr)
        return;
    const bool before = m_ventilator->frozen();
    m_ventilator->toggleFreeze();
    if (auto *log = sv::common::LogBuffer::instance())
        log->noteControl(QStringLiteral("freeze"), before, !before, true);
}

} // namespace sv::presentation
