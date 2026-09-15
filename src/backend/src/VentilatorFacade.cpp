// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/backend/VentilatorFacade.h>

#include <sv/infrastructure/IVentilatorHardware.h>
#include <sv/infrastructure/DatabaseManager.h>
#include <sv/services/SettingsValidator.h>
#include <sv/services/WaveformEngine.h>
#include <sv/services/BreathSimulator.h>

#include "src/controllers/AlarmController.h"

#include <QtMath>

namespace sv::backend {

VentilatorFacade::VentilatorFacade(sv::infrastructure::DatabaseManager *database,
                                   AlarmController *alarmController,
                                   sv::infrastructure::IVentilatorHardware *hardware,
                                   sv::services::SettingsValidator *validator,
                                   sv::services::WaveformEngine *waveforms,
                                   sv::services::BreathSimulator *breathSim,
                                   QObject *parent)
    : QObject(parent)
    , m_database(database)
    , m_alarmController(alarmController)
    , m_hardware(hardware)
    , m_validator(validator)
    , m_waveforms(waveforms)
    , m_breathSim(breathSim)
{
    m_sampleTimer.setInterval(45);
    connect(&m_sampleTimer, &QTimer::timeout, this, &VentilatorFacade::updateSimulation);

    m_ventilationTimer.setInterval(1000);
    connect(&m_ventilationTimer, &QTimer::timeout, this, [this]() {
        ++m_ventilationSeconds;
        emit measurementsChanged();
    });

    m_lastHardwareHeartbeatUtc = QDateTime::currentDateTimeUtc();
    m_backendWatchdogTimer.setInterval(1000);
    connect(&m_backendWatchdogTimer, &QTimer::timeout,
            this, &VentilatorFacade::checkBackendHeartbeat);
    m_backendWatchdogTimer.start();
}

// -- Property accessors -------------------------------------------------------

bool VentilatorFacade::running() const { return m_running; }
bool VentilatorFacade::frozen() const { return m_frozen; }
QString VentilatorFacade::mode() const { return m_mode; }
int VentilatorFacade::fio2() const { return m_fio2; }
int VentilatorFacade::peep() const { return m_peep; }
int VentilatorFacade::pressureSupport() const { return m_pressureSupport; }
int VentilatorFacade::inspiratoryTime() const { return m_inspiratoryTime; }
int VentilatorFacade::respiratoryRate() const { return m_respiratoryRate; }
int VentilatorFacade::trigger() const { return m_trigger; }
int VentilatorFacade::minuteVolume() const { return m_minuteVolume; }
int VentilatorFacade::tidalVolume() const { return m_tidalVolume; }
double VentilatorFacade::ppeak() const { return m_metrics.ppeak; }
double VentilatorFacade::pplat() const { return m_metrics.pplat; }
double VentilatorFacade::pmean() const { return m_metrics.pmean; }
double VentilatorFacade::spo2() const { return m_metrics.spo2; }
double VentilatorFacade::etco2() const { return m_metrics.etco2; }
double VentilatorFacade::compliance() const { return m_metrics.compliance; }
double VentilatorFacade::resistance() const { return m_metrics.resistance; }
double VentilatorFacade::vte() const { return m_metrics.vte; }
double VentilatorFacade::ftotal() const { return m_metrics.ftotal; }
double VentilatorFacade::rcexp() const { return m_metrics.rcexp; }
double VentilatorFacade::expMinVol() const { return m_metrics.expMinVol; }
double VentilatorFacade::drivingPressure() const { return m_metrics.drivingPressure; }
QString VentilatorFacade::ieRatio() const { return m_metrics.ieRatio; }

QString VentilatorFacade::ventilationTime() const
{
    int h = m_ventilationSeconds / 3600;
    int m = (m_ventilationSeconds % 3600) / 60;
    int s = m_ventilationSeconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}

QString VentilatorFacade::lastCommandMessage() const { return m_lastCommandMessage; }
QString VentilatorFacade::operatorId() const { return m_operatorId; }
QString VentilatorFacade::patientCategory() const { return m_patientCategory; }
int VentilatorFacade::patientIbwKg() const { return m_patientIbwKg; }
bool VentilatorFacade::backendConnected() const { return m_backendConnected; }
bool VentilatorFacade::degradedMode() const { return m_degradedMode; }
QString VentilatorFacade::backendState() const { return m_backendState; }

double VentilatorFacade::workOfBreathing() const { return m_metrics.workOfBreathing; }
double VentilatorFacade::stressIndex() const { return m_metrics.stressIndex; }
double VentilatorFacade::deadSpaceFraction() const { return m_metrics.deadSpaceFraction; }
int VentilatorFacade::highFio2Minutes() const { return m_metrics.highFio2Minutes; }
bool VentilatorFacade::patientDisconnected() const { return m_metrics.patientDisconnected; }
bool VentilatorFacade::circuitOcclusion() const { return m_metrics.circuitOcclusion; }

int VentilatorFacade::ventilationSeconds() const { return m_ventilationSeconds; }
int VentilatorFacade::alarmHighPressure() const { return m_alarmHighPressure; }
int VentilatorFacade::alarmLowPressure() const { return m_alarmLowPressure; }
int VentilatorFacade::alarmApneaTime() const { return m_alarmApneaTime; }
int VentilatorFacade::alarmLowVt() const { return m_alarmLowVt; }
int VentilatorFacade::alarmHighMv() const { return m_alarmHighMv; }
int VentilatorFacade::alarmLowSpo2() const { return m_alarmLowSpo2; }
bool VentilatorFacade::apneaBackupEnabled() const { return m_apneaBackupEnabled; }

QVariantList VentilatorFacade::pressureWaveform() const
{
    return m_waveforms ? m_waveforms->pressureWaveform() : QVariantList{};
}

QVariantList VentilatorFacade::flowWaveform() const
{
    return m_waveforms ? m_waveforms->flowWaveform() : QVariantList{};
}

QVariantList VentilatorFacade::volumeWaveform() const
{
    return m_waveforms ? m_waveforms->volumeWaveform() : QVariantList{};
}

QVariantList VentilatorFacade::co2Waveform() const
{
    return m_waveforms ? m_waveforms->co2Waveform() : QVariantList{};
}

// -- Domain helpers -----------------------------------------------------------

sv::domain::SetpointSet VentilatorFacade::currentSetpoints() const
{
    sv::domain::SetpointSet s;
    s.fio2 = m_fio2;
    s.peep = m_peep;
    s.pressureSupport = m_pressureSupport;
    s.inspiratoryTime = m_inspiratoryTime;
    s.respiratoryRate = m_respiratoryRate;
    s.trigger = m_trigger;
    s.minuteVolume = m_minuteVolume;
    s.tidalVolume = m_tidalVolume;
    return s;
}

sv::domain::AlarmLimits VentilatorFacade::currentAlarmLimits() const
{
    sv::domain::AlarmLimits a;
    a.highPressure = m_alarmHighPressure;
    a.lowPressure = m_alarmLowPressure;
    a.apneaTime = m_alarmApneaTime;
    a.lowVt = m_alarmLowVt;
    a.highMv = m_alarmHighMv;
    a.lowSpo2 = m_alarmLowSpo2;
    return a;
}

sv::domain::Patient VentilatorFacade::currentPatient() const
{
    sv::domain::Patient p;
    p.category = m_patientCategory;
    p.weight = m_patientIbwKg;
    return p;
}

// -- Commands -----------------------------------------------------------------

void VentilatorFacade::startVentilation()
{
    QString reason;
    if (m_validator) {
        if (!m_validator->validateStart(currentSetpoints(), currentAlarmLimits(),
                                        m_degradedMode, currentPatient(), &reason)) {
            setCommandMessage(reason);
            emit commandRejected(reason);
            return;
        }
    }
    if (m_running)
        return;
    if (m_degradedMode) {
        const QString message = QStringLiteral("Cannot start: backend is in degraded mode");
        setCommandMessage(message);
        emit commandRejected(message);
        return;
    }
    m_running = true;
    m_ventilationSeconds = 0;
    m_sampleTimer.start();
    m_ventilationTimer.start();
    if (m_database)
        m_database->logEvent(QStringLiteral("Ventilation"),
                             QStringLiteral("Ventilation started"),
                             QStringLiteral("Active"));
    emit runningChanged();
}

bool VentilatorFacade::requestStartVentilation()
{
    QString reason;
    if (m_validator) {
        if (!m_validator->validateStart(currentSetpoints(), currentAlarmLimits(),
                                        m_degradedMode, currentPatient(), &reason)) {
            setCommandMessage(reason);
            emit commandRejected(reason);
            return false;
        }
    }
    startVentilation();
    setCommandMessage(QStringLiteral("Ventilation started"));
    return true;
}

void VentilatorFacade::stopVentilation()
{
    if (!m_running)
        return;
    m_running = false;
    m_sampleTimer.stop();
    m_ventilationTimer.stop();
    if (m_waveforms)
        m_waveforms->clear();
    m_metrics = sv::domain::ClinicalMetrics{};
    if (m_database)
        m_database->logEvent(QStringLiteral("Ventilation"),
                             QStringLiteral("Ventilation stopped"),
                             QStringLiteral("Standby"));
    emit runningChanged();
    emit measurementsChanged();
    emit waveformChanged();
}

void VentilatorFacade::toggleFreeze()
{
    m_frozen = !m_frozen;
    if (m_waveforms)
        m_waveforms->setFrozen(m_frozen);
    if (m_database)
        m_database->logEvent(QStringLiteral("Monitoring"),
                             m_frozen ? QStringLiteral("Waveforms frozen")
                                      : QStringLiteral("Waveforms resumed"));
    emit frozenChanged();
}

void VentilatorFacade::runCalibration()
{
    if (m_database)
        m_database->logEvent(QStringLiteral("Calibration"),
                             QStringLiteral("Pressure, flow and oxygen checks completed"),
                             QStringLiteral("Passed"));
}

bool VentilatorFacade::requestModeChange(const QString &mode)
{
    if (m_validator) {
        QString reason;
        if (!m_validator->validateMode(mode, &reason)) {
            setCommandMessage(reason);
            emit commandRejected(reason);
            return false;
        }
    }
    if (m_mode == mode)
        return true;
    const QString oldMode = m_mode;
    m_mode = mode;
    logSettingChange(QStringLiteral("Mode"), oldMode, mode);
    setCommandMessage(QStringLiteral("Mode changed to %1").arg(mode));
    emit settingsChanged();
    return true;
}

bool VentilatorFacade::requestApneaBackupChange(bool enabled)
{
    if (m_apneaBackupEnabled == enabled)
        return true;
    const bool oldValue = m_apneaBackupEnabled;
    m_apneaBackupEnabled = enabled;
    logSettingChange(QStringLiteral("Apnea backup"), oldValue, enabled);
    setCommandMessage(enabled ? QStringLiteral("Apnea backup enabled")
                              : QStringLiteral("Apnea backup disabled"));
    emit settingsChanged();
    return true;
}

bool VentilatorFacade::requestParameterChange(const QString &parameter, int value)
{
    return applyParameterChange(parameter, value, true);
}

bool VentilatorFacade::requestAlarmLimitChange(const QString &limit, int value)
{
    return applyAlarmLimitChange(limit, value, true);
}

void VentilatorFacade::recordHardwareHeartbeat()
{
    m_lastHardwareHeartbeatUtc = QDateTime::currentDateTimeUtc();
    if (!m_backendConnected || m_degradedMode)
        setDegradedMode(false, QStringLiteral("Simulator connected"));
}

void VentilatorFacade::setBackendConnected(bool connected)
{
    m_backendConnected = connected;
    if (connected) {
        m_lastHardwareHeartbeatUtc = QDateTime::currentDateTimeUtc();
        setDegradedMode(false, QStringLiteral("Simulator connected"));
    } else {
        setDegradedMode(true, QStringLiteral("Backend disconnected"));
    }
    emit backendStateChanged();
}

// -- Setters ------------------------------------------------------------------

void VentilatorFacade::setMode(const QString &value) { requestModeChange(value); }
void VentilatorFacade::setFio2(int value) { applyParameterChange(QStringLiteral("fio2"), value, false); }
void VentilatorFacade::setPeep(int value) { applyParameterChange(QStringLiteral("peep"), value, false); }
void VentilatorFacade::setPressureSupport(int value) { applyParameterChange(QStringLiteral("pressureSupport"), value, false); }
void VentilatorFacade::setInspiratoryTime(int value) { applyParameterChange(QStringLiteral("inspiratoryTime"), value, false); }
void VentilatorFacade::setRespiratoryRate(int value) { applyParameterChange(QStringLiteral("respiratoryRate"), value, false); }
void VentilatorFacade::setTrigger(int value) { applyParameterChange(QStringLiteral("trigger"), value, false); }
void VentilatorFacade::setMinuteVolume(int value) { applyParameterChange(QStringLiteral("minuteVolume"), value, false); }
void VentilatorFacade::setTidalVolume(int value) { applyParameterChange(QStringLiteral("tidalVolume"), value, false); }
void VentilatorFacade::setAlarmHighPressure(int value) { applyAlarmLimitChange(QStringLiteral("highPressure"), value, false); }
void VentilatorFacade::setAlarmLowPressure(int value) { applyAlarmLimitChange(QStringLiteral("lowPressure"), value, false); }
void VentilatorFacade::setAlarmApneaTime(int value) { applyAlarmLimitChange(QStringLiteral("apneaTime"), value, false); }
void VentilatorFacade::setAlarmLowVt(int value) { applyAlarmLimitChange(QStringLiteral("lowVt"), value, false); }
void VentilatorFacade::setAlarmHighMv(int value) { applyAlarmLimitChange(QStringLiteral("highMv"), value, false); }
void VentilatorFacade::setAlarmLowSpo2(int value) { applyAlarmLimitChange(QStringLiteral("lowSpo2"), value, false); }
void VentilatorFacade::setApneaBackupEnabled(bool value) { requestApneaBackupChange(value); }

void VentilatorFacade::setOperatorId(const QString &operatorId)
{
    const QString normalized = operatorId.trimmed().isEmpty()
        ? QStringLiteral("unauthenticated")
        : operatorId.trimmed();
    if (m_operatorId == normalized)
        return;
    m_operatorId = normalized;
    emit operatorChanged();
}

void VentilatorFacade::setPatientContext(const QString &category)
{
    const QString normalized = category.trimmed().isEmpty()
        ? QStringLiteral("Adult")
        : category.trimmed();
    if (m_patientCategory == normalized)
        return;
    m_patientCategory = normalized;
    setCommandMessage(QStringLiteral("Patient category set to %1").arg(normalized));
    emit patientContextChanged();
}

void VentilatorFacade::setPatientIbwKg(int ibwKg)
{
    ibwKg = qBound(1, ibwKg, 180);
    if (m_patientIbwKg == ibwKg)
        return;
    m_patientIbwKg = ibwKg;
    emit patientContextChanged();
}

// -- Parameter validation and application -------------------------------------

bool VentilatorFacade::applyParameterChange(const QString &parameter, int value, bool audited)
{
    int *target = nullptr;
    int low = 0;
    int high = 0;
    QString label;
    QString unit;

    const auto patient = currentPatient();

    if (parameter == QStringLiteral("fio2")) {
        target = &m_fio2; low = 21; high = 100;
        label = QStringLiteral("FiO2"); unit = QStringLiteral("%");
    } else if (parameter == QStringLiteral("peep")) {
        target = &m_peep; low = 0; high = qMin(30, m_alarmHighPressure - 5);
        label = QStringLiteral("PEEP"); unit = QStringLiteral("cmH2O");
    } else if (parameter == QStringLiteral("pressureSupport")) {
        target = &m_pressureSupport; low = 0; high = 40;
        label = QStringLiteral("Pressure support"); unit = QStringLiteral("cmH2O");
    } else if (parameter == QStringLiteral("inspiratoryTime")) {
        target = &m_inspiratoryTime; low = 1; high = 5;
        label = QStringLiteral("Inspiratory time"); unit = QStringLiteral("s");
    } else if (parameter == QStringLiteral("respiratoryRate")) {
        target = &m_respiratoryRate;
        low = sv::domain::categoryMinRr(patient);
        high = sv::domain::categoryMaxRr(patient);
        label = QStringLiteral("Respiratory rate"); unit = QStringLiteral("1/min");
    } else if (parameter == QStringLiteral("trigger")) {
        target = &m_trigger; low = 1; high = 10;
        label = QStringLiteral("Trigger"); unit = QStringLiteral("L/min");
    } else if (parameter == QStringLiteral("minuteVolume")) {
        target = &m_minuteVolume; low = 20; high = 400;
        label = QStringLiteral("%MinVol"); unit = QStringLiteral("%");
    } else if (parameter == QStringLiteral("tidalVolume")) {
        target = &m_tidalVolume;
        low = sv::domain::categoryMinVt(patient);
        high = sv::domain::categoryMaxVt(patient);
        label = QStringLiteral("Tidal volume"); unit = QStringLiteral("mL");
    }

    if (!target) {
        const QString message = QStringLiteral("Unknown parameter: %1").arg(parameter);
        setCommandMessage(message);
        emit commandRejected(message);
        return false;
    }

    const int requested = value;
    value = qBound(qMin(low, high), value, qMax(low, high));
    if (requested != value) {
        const QString message = QStringLiteral("%1 limited to %2 %3").arg(label).arg(value).arg(unit);
        setCommandMessage(message);
        emit commandRejected(message);
        return false;
    }

    if (m_validator) {
        QString envelopeReason;
        if (!m_validator->validateSettingEnvelope(parameter, value, currentSetpoints(),
                                                  currentAlarmLimits(), currentPatient(),
                                                  &envelopeReason)) {
            setCommandMessage(envelopeReason);
            emit commandRejected(envelopeReason);
            return false;
        }
    }

    if (*target == value)
        return true;
    const int oldValue = *target;
    *target = value;
    if (audited)
        logSettingChange(label, oldValue, QStringLiteral("%1 %2").arg(value).arg(unit));
    setCommandMessage(QStringLiteral("%1 set to %2 %3").arg(label).arg(value).arg(unit));
    emit settingsChanged();
    evaluateAlarms();
    return true;
}

bool VentilatorFacade::applyAlarmLimitChange(const QString &limit, int value, bool audited)
{
    int *target = nullptr;
    int low = 0;
    int high = 0;
    QString label;
    QString unit;

    if (limit == QStringLiteral("highPressure")) {
        target = &m_alarmHighPressure; low = qMax(10, m_alarmLowPressure + 5); high = 80;
        label = QStringLiteral("High pressure alarm"); unit = QStringLiteral("cmH2O");
    } else if (limit == QStringLiteral("lowPressure")) {
        target = &m_alarmLowPressure; low = 0; high = qMin(40, m_alarmHighPressure - 5);
        label = QStringLiteral("Low pressure alarm"); unit = QStringLiteral("cmH2O");
    } else if (limit == QStringLiteral("apneaTime")) {
        target = &m_alarmApneaTime; low = 5; high = 60;
        label = QStringLiteral("Apnea time alarm"); unit = QStringLiteral("s");
    } else if (limit == QStringLiteral("lowVt")) {
        target = &m_alarmLowVt; low = 20; high = qMax(20, m_tidalVolume - 20);
        label = QStringLiteral("Low VT alarm"); unit = QStringLiteral("mL");
    } else if (limit == QStringLiteral("highMv")) {
        target = &m_alarmHighMv; low = 1; high = 30;
        label = QStringLiteral("High MV alarm"); unit = QStringLiteral("L/min");
    } else if (limit == QStringLiteral("lowSpo2")) {
        target = &m_alarmLowSpo2; low = 70; high = 100;
        label = QStringLiteral("Low SpO2 alarm"); unit = QStringLiteral("%");
    }

    if (!target) {
        const QString message = QStringLiteral("Unknown alarm limit: %1").arg(limit);
        setCommandMessage(message);
        emit commandRejected(message);
        return false;
    }

    const int requested = value;
    value = qBound(qMin(low, high), value, qMax(low, high));
    if (requested != value) {
        const QString message = QStringLiteral("%1 limited to %2 %3").arg(label).arg(value).arg(unit);
        setCommandMessage(message);
        emit commandRejected(message);
        return false;
    }

    if (*target == value)
        return true;
    const int oldValue = *target;
    *target = value;
    if (audited)
        logSettingChange(label, oldValue, QStringLiteral("%1 %2").arg(value).arg(unit));
    setCommandMessage(QStringLiteral("%1 set to %2 %3").arg(label).arg(value).arg(unit));
    emit settingsChanged();
    evaluateAlarms();
    return true;
}

// -- Simulation tick ----------------------------------------------------------

void VentilatorFacade::updateSimulation()
{
    if (!m_running)
        return;

    recordHardwareHeartbeat();
    ++m_sampleIndex;

    const double dt = m_sampleTimer.interval() / 1000.0;
    const auto setpoints = currentSetpoints();

    sv::services::BreathPhase bp;
    if (m_breathSim)
        bp = m_breathSim->advancePhase(m_phase, dt, m_respiratoryRate, m_inspiratoryTime);
    else
        bp.phase = std::fmod(m_phase + dt * m_respiratoryRate / 60.0, 1.0);
    m_phase = bp.phase;

    if (m_hardware) {
        auto sample = m_hardware->tick(dt, m_phase, bp.inspiration, bp.normalized,
                                       setpoints, m_mode);
        if (m_waveforms && !m_frozen)
            m_waveforms->appendSample(sample);

        m_metrics = m_hardware->computeMetrics(setpoints, m_sampleIndex);
    }

    evaluateAlarms();
    saveSnapshotIfDue();
    emit measurementsChanged();
    if (!m_frozen)
        emit waveformChanged();
}

// -- Alarm evaluation ---------------------------------------------------------

void VentilatorFacade::evaluateAlarms()
{
    if (!m_alarmController)
        return;

    const bool wasPreviouslyActive = m_alarmController->active();

    if (m_metrics.patientDisconnected) {
        if (!wasPreviouslyActive || m_alarmController->headline() != QStringLiteral("Patient Disconnect")) {
            m_alarmController->addAlarm(
                QStringLiteral("Critical"), QStringLiteral("Circuit"),
                QStringLiteral("No airway pressure detected -- check patient connection"),
                QStringLiteral("Active"));
        }
        m_alarmController->setActive(true);
        m_alarmController->setPriority(QStringLiteral("Critical"));
        m_alarmController->setHeadline(QStringLiteral("Patient Disconnect"));
        m_alarmController->setDetail(QStringLiteral("Check circuit and patient"));
        return;
    }

    if (m_metrics.circuitOcclusion) {
        if (!wasPreviouslyActive || m_alarmController->headline() != QStringLiteral("Circuit Occlusion")) {
            m_alarmController->addAlarm(
                QStringLiteral("Critical"), QStringLiteral("Circuit"),
                QStringLiteral("High pressure with no flow -- check for obstruction"),
                QStringLiteral("Active"));
        }
        m_alarmController->setActive(true);
        m_alarmController->setPriority(QStringLiteral("Critical"));
        m_alarmController->setHeadline(QStringLiteral("Circuit Occlusion"));
        m_alarmController->setDetail(QStringLiteral("Check tubing and filters"));
        return;
    }

    if (m_metrics.ppeak > m_alarmHighPressure) {
        if (!wasPreviouslyActive || m_alarmController->headline() != QStringLiteral("High Pressure")) {
            m_alarmController->addAlarm(
                QStringLiteral("Critical"), QStringLiteral("Pressure"),
                QStringLiteral("Paw above limit — Ppeak %1 cmH2O").arg(qRound(m_metrics.ppeak)),
                QStringLiteral("Active"));
        }
        m_alarmController->setActive(true);
        m_alarmController->setPriority(QStringLiteral("Critical"));
        m_alarmController->setHeadline(QStringLiteral("High Pressure"));
        m_alarmController->setDetail(QStringLiteral("Paw above limit"));
        return;
    }

    if (m_minuteVolume > m_alarmHighMv * 10) {
        if (!wasPreviouslyActive || m_alarmController->headline() != QStringLiteral("High Minute Volume")) {
            m_alarmController->addAlarm(
                QStringLiteral("Critical"), QStringLiteral("Volume"),
                QStringLiteral("Minute volume %1% exceeds limit").arg(m_minuteVolume),
                QStringLiteral("Active"));
        }
        m_alarmController->setActive(true);
        m_alarmController->setPriority(QStringLiteral("Critical"));
        m_alarmController->setHeadline(QStringLiteral("High Minute Volume"));
        m_alarmController->setDetail(QStringLiteral("CT Low"));
        return;
    }

    if (m_metrics.spo2 < m_alarmLowSpo2 && m_metrics.spo2 > 0) {
        if (!wasPreviouslyActive || m_alarmController->headline() != QStringLiteral("Low SpO2")) {
            m_alarmController->addAlarm(
                QStringLiteral("Warning"), QStringLiteral("Oximetry"),
                QStringLiteral("SpO2 %1% below threshold").arg(qRound(m_metrics.spo2)),
                QStringLiteral("Active"));
        }
        m_alarmController->setActive(true);
        m_alarmController->setPriority(QStringLiteral("Warning"));
        m_alarmController->setHeadline(QStringLiteral("Low SpO2"));
        m_alarmController->setDetail(QStringLiteral("Oxygen saturation below 90%"));
        return;
    }

    if (m_metrics.etco2 > 50) {
        if (!wasPreviouslyActive || m_alarmController->headline() != QStringLiteral("High EtCO2")) {
            m_alarmController->addAlarm(
                QStringLiteral("Warning"), QStringLiteral("Capnography"),
                QStringLiteral("EtCO2 %1 mmHg above limit").arg(qRound(m_metrics.etco2)),
                QStringLiteral("Active"));
        }
        m_alarmController->setActive(true);
        m_alarmController->setPriority(QStringLiteral("Warning"));
        m_alarmController->setHeadline(QStringLiteral("High EtCO2"));
        m_alarmController->setDetail(QStringLiteral("End-tidal CO2 elevated"));
        return;
    }

    if (m_metrics.highFio2Minutes > 120 && m_fio2 > 60) {
        if (!wasPreviouslyActive || m_alarmController->headline() != QStringLiteral("O2 Toxicity Risk")) {
            m_alarmController->addAlarm(
                QStringLiteral("Warning"), QStringLiteral("Oxygen"),
                QStringLiteral("FiO2 >60% for %1 min -- consider weaning").arg(m_metrics.highFio2Minutes),
                QStringLiteral("Active"));
        }
        m_alarmController->setActive(true);
        m_alarmController->setPriority(QStringLiteral("Warning"));
        m_alarmController->setHeadline(QStringLiteral("O2 Toxicity Risk"));
        m_alarmController->setDetail(QStringLiteral("Prolonged high FiO2 exposure"));
        return;
    }

    if (m_metrics.drivingPressure > 15.0) {
        if (!wasPreviouslyActive || m_alarmController->headline() != QStringLiteral("High Driving Pressure")) {
            m_alarmController->addAlarm(
                QStringLiteral("Warning"), QStringLiteral("Pressure"),
                QStringLiteral("Driving pressure %1 cmH2O -- target <15").arg(qRound(m_metrics.drivingPressure)),
                QStringLiteral("Active"));
        }
        m_alarmController->setActive(true);
        m_alarmController->setPriority(QStringLiteral("Warning"));
        m_alarmController->setHeadline(QStringLiteral("High Driving Pressure"));
        m_alarmController->setDetail(QStringLiteral("Lung injury risk -- reduce Vt or increase PEEP"));
        return;
    }

    if (wasPreviouslyActive) {
        m_alarmController->addAlarm(
            QStringLiteral("Info"), QStringLiteral("System"),
            QStringLiteral("All parameters within normal limits"),
            QStringLiteral("Resolved"));
    }
    m_alarmController->setActive(false);
    m_alarmController->setPriority(QStringLiteral("Normal"));
    m_alarmController->setHeadline(QStringLiteral("No Active Alarms"));
    m_alarmController->setDetail(QStringLiteral("System normal"));
}

// -- Persistence --------------------------------------------------------------

QVariantMap VentilatorFacade::snapshot() const
{
    return {
        {QStringLiteral("mode"), m_mode},
        {QStringLiteral("fio2"), m_fio2},
        {QStringLiteral("peep"), m_peep},
        {QStringLiteral("pressureSupport"), m_pressureSupport},
        {QStringLiteral("respiratoryRate"), m_respiratoryRate},
        {QStringLiteral("minuteVolume"), m_minuteVolume},
        {QStringLiteral("tidalVolume"), m_tidalVolume},
        {QStringLiteral("ppeak"), m_metrics.ppeak},
        {QStringLiteral("pplat"), m_metrics.pplat},
        {QStringLiteral("pmean"), m_metrics.pmean},
        {QStringLiteral("spo2"), m_metrics.spo2},
        {QStringLiteral("etco2"), m_metrics.etco2},
        {QStringLiteral("compliance"), m_metrics.compliance},
        {QStringLiteral("resistance"), m_metrics.resistance}
    };
}

void VentilatorFacade::saveSnapshotIfDue()
{
    if (!m_database)
        return;
    ++m_snapshotCounter;
    if (m_snapshotCounter < 25)
        return;
    m_snapshotCounter = 0;
    m_database->saveParameterSnapshot(snapshot());
}

// -- Backend watchdog ---------------------------------------------------------

void VentilatorFacade::checkBackendHeartbeat()
{
    if (!m_backendConnected)
        return;
    if (!m_running) {
        m_lastHardwareHeartbeatUtc = QDateTime::currentDateTimeUtc();
        return;
    }
    const qint64 ageMs = m_lastHardwareHeartbeatUtc.msecsTo(QDateTime::currentDateTimeUtc());
    if (ageMs > 5000) {
        m_backendConnected = false;
        setDegradedMode(true, QStringLiteral("No backend heartbeat for more than 5 seconds"));
        emit backendStateChanged();
    }
}

// -- Internal helpers ---------------------------------------------------------

void VentilatorFacade::setCommandMessage(const QString &message)
{
    if (m_lastCommandMessage == message)
        return;
    m_lastCommandMessage = message;
    emit commandMessageChanged();
}

void VentilatorFacade::setDegradedMode(bool degraded, const QString &state)
{
    const bool changed = m_degradedMode != degraded || m_backendState != state;
    m_degradedMode = degraded;
    m_backendState = state;
    if (degraded && m_running)
        stopVentilation();
    if (m_alarmController && degraded) {
        m_alarmController->raiseAlarm(QStringLiteral("Critical"),
                                      QStringLiteral("Backend"),
                                      QStringLiteral("Backend Disconnected"),
                                      state);
    }
    if (changed)
        emit backendStateChanged();
}

void VentilatorFacade::logSettingChange(const QString &parameter,
                                        const QVariant &oldValue,
                                        const QVariant &newValue)
{
    if (!m_database)
        return;
    m_database->logEvent(QStringLiteral("Setting"),
                         QStringLiteral("%1 changed from %2 to %3 by %4")
                             .arg(parameter, oldValue.toString(),
                                  newValue.toString(), m_operatorId),
                         QStringLiteral("Applied"));
}

} // namespace sv::backend
