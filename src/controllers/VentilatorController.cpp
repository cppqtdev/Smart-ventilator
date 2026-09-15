#include "VentilatorController.h"

#include "AlarmController.h"
#include <sv/domain/ModeCatalog.h>
#include "src/core/DatabaseManager.h"

#include <QtMath>
#include <QSet>

#include <algorithm>
#include <cmath>

namespace {
double clampDouble(double value, double low, double high)
{
    return qMax(low, qMin(high, value));
}
}

VentilatorController::VentilatorController(DatabaseManager *database,
                                           AlarmController *alarmController,
                                           QObject *parent)
    : QObject(parent)
    , m_database(database)
    , m_alarmController(alarmController)
{
    m_sampleTimer.setInterval(45);
    connect(&m_sampleTimer, &QTimer::timeout, this, &VentilatorController::updateSimulation);

    m_ventilationTimer.setInterval(1000);
    connect(&m_ventilationTimer, &QTimer::timeout, this, [this]() {
        ++m_ventilationSeconds;

        // Apnea is the absence of a breath, so it is counted from the
        // measured rate rather than from anything the setpoints say. The
        // alarm limit is in seconds, and this is the number it compares to.
        if (m_ftotal > 0.0)
            m_apneaSeconds = 0;
        else
            ++m_apneaSeconds;

        evaluateAlarms();
        emit measurementsChanged();
    });

    // A hold manoeuvre closes the valves for a fixed interval and then latches
    // the settled pressure. Until one runs, every hold-derived quantity -
    // static compliance, resistance, driving pressure, auto-PEEP - reports
    // itself as unmeasurable rather than showing a stale or invented number.
    m_pvTimer.setInterval(60);
    connect(&m_pvTimer, &QTimer::timeout, this, &VentilatorController::stepPvTool);

    m_holdTimer.setSingleShot(true);
    connect(&m_holdTimer, &QTimer::timeout, this, [this]() {
        if (m_holdIsInspiratory) {
            m_plateauValid = true;
            m_mechanics.markInspiratoryHold();
            setCommandMessage(QStringLiteral("Plateau pressure captured"));
        } else {
            m_totalPeepValid = true;
            m_mechanics.markExpiratoryHold();
            setCommandMessage(QStringLiteral("Total PEEP captured"));
        }
        m_holdInProgress = false;
        emit measurementsChanged();
    });

    // Both timed manoeuvres restore the setting they changed when they end,
    // so a forgotten boost cannot leave the patient on pure oxygen.
    m_oxygenBoostTimer.setSingleShot(true);
    connect(&m_oxygenBoostTimer, &QTimer::timeout, this, [this]() {
        m_fio2 = m_fio2BeforeBoost > 0 ? m_fio2BeforeBoost : m_fio2;
        setCommandMessage(QStringLiteral("Oxygen boost ended"));
        emit settingsChanged();
        emit manoeuvreChanged();
    });

    m_nebuliserTimer.setSingleShot(true);
    connect(&m_nebuliserTimer, &QTimer::timeout, this, [this]() {
        setCommandMessage(QStringLiteral("Nebuliser finished"));
        emit manoeuvreChanged();
    });

    m_lastHardwareHeartbeatUtc = QDateTime::currentDateTimeUtc();
    m_backendWatchdogTimer.setInterval(1000);
    connect(&m_backendWatchdogTimer, &QTimer::timeout,
            this, &VentilatorController::checkBackendHeartbeat);
    m_backendWatchdogTimer.start();
}

bool VentilatorController::running() const { return m_running; }
bool VentilatorController::frozen() const { return m_frozen; }
QString VentilatorController::mode() const { return m_mode; }
int VentilatorController::fio2() const { return m_fio2; }
int VentilatorController::peep() const { return m_peep; }
int VentilatorController::pressureSupport() const { return m_pressureSupport; }
int VentilatorController::inspiratoryTime() const { return m_inspiratoryTime; }
int VentilatorController::respiratoryRate() const { return m_respiratoryRate; }
int VentilatorController::trigger() const { return m_trigger; }
int VentilatorController::minuteVolume() const { return m_minuteVolume; }
int VentilatorController::tidalVolume() const { return m_tidalVolume; }
double VentilatorController::ppeak() const { return m_ppeak; }
double VentilatorController::pplat() const { return m_pplat; }
double VentilatorController::pmean() const { return m_pmean; }
double VentilatorController::spo2() const { return m_spo2; }
double VentilatorController::etco2() const { return m_etco2; }
double VentilatorController::compliance() const { return m_compliance; }
double VentilatorController::resistance() const { return m_resistance; }
double VentilatorController::vte() const { return m_vte; }
double VentilatorController::ftotal() const { return m_ftotal; }
double VentilatorController::rcexp() const { return m_rcexp; }
double VentilatorController::expMinVol() const { return m_expMinVol; }

double VentilatorController::drivingPressure() const
{
    // Driving pressure = Pplat - PEEP. Target < 15 cmH2O for lung protection.
    return qMax(0.0, m_pplat - m_peep);
}

QString VentilatorController::ieRatio() const
{
    double totalCycle = 60.0 / qMax(1, m_respiratoryRate);
    double insp = qMax(0.3, static_cast<double>(m_inspiratoryTime));
    double exp = totalCycle - insp;
    if (exp <= 0) return QStringLiteral("1:0");
    return QStringLiteral("1:%1").arg(exp / insp, 0, 'f', 1);
}

double VentilatorController::workOfBreathing() const { return m_workOfBreathing; }
double VentilatorController::stressIndex() const { return m_stressIndex; }
double VentilatorController::deadSpaceFraction() const { return m_deadSpaceFraction; }
int VentilatorController::highFio2Minutes() const { return m_highFio2Minutes; }
bool VentilatorController::patientDisconnected() const { return m_patientDisconnected; }
bool VentilatorController::circuitOcclusion() const { return m_circuitOcclusion; }

int VentilatorController::ventilationSeconds() const { return m_ventilationSeconds; }
int VentilatorController::alarmHighPressure() const { return m_alarmHighPressure; }
int VentilatorController::alarmLowPressure() const { return m_alarmLowPressure; }
int VentilatorController::alarmApneaTime() const { return m_alarmApneaTime; }
int VentilatorController::apneaSeconds() const { return m_apneaSeconds; }
int VentilatorController::alarmLowVt() const { return m_alarmLowVt; }
int VentilatorController::alarmHighMv() const { return m_alarmHighMv; }
int VentilatorController::alarmLowSpo2() const { return m_alarmLowSpo2; }
int VentilatorController::alarmLowMv() const { return m_alarmLowMv; }
int VentilatorController::alarmHighVt() const { return m_alarmHighVt; }
int VentilatorController::alarmHighRate() const { return m_alarmHighRate; }
int VentilatorController::alarmLowRate() const { return m_alarmLowRate; }
int VentilatorController::alarmHighFio2() const { return m_alarmHighFio2; }
int VentilatorController::alarmLowFio2() const { return m_alarmLowFio2; }
int VentilatorController::alarmHighEtco2() const { return m_alarmHighEtco2; }
int VentilatorController::alarmLowEtco2() const { return m_alarmLowEtco2; }
bool VentilatorController::spo2Monitored() const { return m_spo2Monitored; }
bool VentilatorController::apneaBackupEnabled() const { return m_apneaBackupEnabled; }

QString VentilatorController::ventilationTime() const
{
    int h = m_ventilationSeconds / 3600;
    int m = (m_ventilationSeconds % 3600) / 60;
    int s = m_ventilationSeconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}
QString VentilatorController::lastCommandMessage() const { return m_lastCommandMessage; }
QString VentilatorController::operatorId() const { return m_operatorId; }
QString VentilatorController::patientCategory() const { return m_patientCategory; }
int VentilatorController::patientIbwKg() const { return m_patientIbwKg; }
bool VentilatorController::backendConnected() const { return m_backendConnected; }
bool VentilatorController::degradedMode() const { return m_degradedMode; }
QString VentilatorController::backendState() const { return m_backendState; }
QVariantList VentilatorController::pressureWaveform() const { return m_pressureWaveform; }
QVariantList VentilatorController::flowWaveform() const { return m_flowWaveform; }
QVariantList VentilatorController::volumeWaveform() const { return m_volumeWaveform; }
QVariantList VentilatorController::co2Waveform() const { return m_co2Waveform; }

// ---------------------------------------------------------------------------
//  Mode identity, read from the declarative catalogue rather than duplicated
// ---------------------------------------------------------------------------

QString VentilatorController::modeDescription() const
{
    return sv::domain::ModeCatalog::findOrDefault(m_mode).description;
}

bool VentilatorController::nonInvasive() const
{
    return sv::domain::ModeCatalog::findOrDefault(m_mode).nonInvasive;
}

// ---------------------------------------------------------------------------
//  Measured inputs to the mechanics engine
// ---------------------------------------------------------------------------

double VentilatorController::vti() const { return m_vti; }
double VentilatorController::totalPeep() const { return m_totalPeep; }
double VentilatorController::peakInspiratoryFlow() const { return m_peakInspFlow; }
double VentilatorController::peakExpiratoryFlow() const { return m_peakExpFlow; }
double VentilatorController::spontaneousRate() const { return m_spontaneousRate; }
bool VentilatorController::squareFlow() const { return m_squareFlow; }
bool VentilatorController::passivePatient() const { return m_passivePatient; }
bool VentilatorController::plateauValid() const { return m_plateauValid; }
bool VentilatorController::totalPeepValid() const { return m_totalPeepValid; }

bool VentilatorController::holdInProgress() const { return m_holdInProgress; }

void VentilatorController::performInspiratoryHold(int milliseconds)
{
    // HARDWARE: this must close both the inspiratory and expiratory valves and
    // hold them shut for the requested interval, then sample the settled
    // airway pressure as the plateau.
    if (!m_running || m_holdInProgress)
        return;
    m_holdInProgress = true;
    m_holdIsInspiratory = true;
    setCommandMessage(QStringLiteral("Inspiratory hold in progress"));

    m_holdTimer.setSingleShot(true);
    m_holdTimer.start(qBound(300, milliseconds, 5000));
    emit measurementsChanged();
}

void VentilatorController::performExpiratoryHold(int milliseconds)
{
    // HARDWARE: hold at end-expiration until flow reaches zero, then read the
    // settled pressure as PEEPtot. Abort and discard if the patient triggers.
    if (!m_running || m_holdInProgress)
        return;
    m_holdInProgress = true;
    m_holdIsInspiratory = false;
    setCommandMessage(QStringLiteral("Expiratory hold in progress"));

    m_holdTimer.setSingleShot(true);
    m_holdTimer.start(qBound(300, milliseconds, 5000));
    emit measurementsChanged();
}

bool VentilatorController::oxygenBoostActive() const
{
    return m_oxygenBoostTimer.isActive();
}

int VentilatorController::oxygenBoostRemaining() const
{
    return m_oxygenBoostTimer.isActive()
        ? (m_oxygenBoostTimer.remainingTime() + 999) / 1000 : 0;
}

bool VentilatorController::nebuliserActive() const
{
    return m_nebuliserTimer.isActive();
}

int VentilatorController::nebuliserRemaining() const
{
    return m_nebuliserTimer.isActive()
        ? (m_nebuliserTimer.remainingTime() + 999) / 1000 : 0;
}

bool VentilatorController::deliverManualBreath()
{
    // HARDWARE: this must queue one mandatory breath at the current settings
    // for delivery at the start of the next expiratory phase.
    if (!m_running) {
        const QString message = QStringLiteral("Manual breath needs ventilation running");
        setCommandMessage(message);
        emit commandRejected(message);
        return false;
    }
    if (m_holdInProgress) {
        const QString message = QStringLiteral("Manual breath refused during a hold");
        setCommandMessage(message);
        emit commandRejected(message);
        return false;
    }

    // Restarting the breath cycle at inspiration is what a manual breath is.
    m_phase = 0.0;
    setCommandMessage(QStringLiteral("Manual breath delivered"));
    logSettingChange(QStringLiteral("Manual breath"), QString(), QString());
    return true;
}

bool VentilatorController::startOxygenBoost(int seconds)
{
    if (!m_running) {
        const QString message = QStringLiteral("Oxygen boost needs ventilation running");
        setCommandMessage(message);
        emit commandRejected(message);
        return false;
    }
    if (m_oxygenBoostTimer.isActive())
        return true;

    m_fio2BeforeBoost = m_fio2;
    m_fio2 = 100;

    m_oxygenBoostTimer.setSingleShot(true);
    m_oxygenBoostTimer.start(qBound(30, seconds, 300) * 1000);

    setCommandMessage(QStringLiteral("100 percent oxygen for %1 s").arg(oxygenBoostRemaining()));
    logSettingChange(QStringLiteral("Oxygen boost"),
                     QString::number(m_fio2BeforeBoost), QStringLiteral("100"));
    emit settingsChanged();
    emit manoeuvreChanged();
    return true;
}

void VentilatorController::cancelOxygenBoost()
{
    if (!m_oxygenBoostTimer.isActive())
        return;
    m_oxygenBoostTimer.stop();
    m_fio2 = m_fio2BeforeBoost > 0 ? m_fio2BeforeBoost : m_fio2;
    setCommandMessage(QStringLiteral("Oxygen boost ended"));
    emit settingsChanged();
    emit manoeuvreChanged();
}

bool VentilatorController::startNebuliser(int minutes)
{
    // HARDWARE: drives the nebuliser output. On a device that entrains gas
    // this also has to correct the delivered volume for the added flow.
    if (!m_running) {
        const QString message = QStringLiteral("Nebuliser needs ventilation running");
        setCommandMessage(message);
        emit commandRejected(message);
        return false;
    }
    if (m_nebuliserTimer.isActive())
        return true;

    m_nebuliserTimer.setSingleShot(true);
    m_nebuliserTimer.start(qBound(1, minutes, 60) * 60 * 1000);

    setCommandMessage(QStringLiteral("Nebuliser running"));
    logSettingChange(QStringLiteral("Nebuliser"), QStringLiteral("off"),
                     QStringLiteral("%1 min").arg(qBound(1, minutes, 60)));
    emit manoeuvreChanged();
    return true;
}

void VentilatorController::cancelNebuliser()
{
    if (!m_nebuliserTimer.isActive())
        return;
    m_nebuliserTimer.stop();
    setCommandMessage(QStringLiteral("Nebuliser stopped"));
    emit manoeuvreChanged();
}

QVariantMap VentilatorController::mechanics() const
{
    using namespace sv::services;

    const sv::domain::ModeDefinition &mode =
        sv::domain::ModeCatalog::findOrDefault(m_mode);

    auto pack = [](const Measurement &m, const QString &unit, int decimals) {
        return QVariantMap{
            {QStringLiteral("value"), m.value},
            {QStringLiteral("text"), m.isValid()
                ? QString::number(m.value, 'f', decimals)
                : QStringLiteral("--")},
            {QStringLiteral("valid"), m.isValid()},
            {QStringLiteral("reason"), describeValidity(m.validity)},
            // Short form for the tile itself; "reason" is the tooltip text.
            {QStringLiteral("hint"), describeValidityShort(m.validity)},
            {QStringLiteral("unit"), unit},
            {QStringLiteral("manoeuvre"), m.manoeuvreDerived},
            {QStringLiteral("age"), m.ageSeconds}
        };
    };

    QVariantMap out;
    out.insert(QStringLiteral("cdyn"),
               pack(m_mechanics.dynamicCompliance(m_breath), QStringLiteral("mL/cmH2O"), 1));
    out.insert(QStringLiteral("cstat"),
               pack(m_mechanics.staticCompliance(m_breath), QStringLiteral("mL/cmH2O"), 1));
    out.insert(QStringLiteral("elastance"),
               pack(m_mechanics.elastance(m_breath), QStringLiteral("cmH2O/L"), 1));
    out.insert(QStringLiteral("rinsp"),
               pack(m_mechanics.inspiratoryResistance(m_breath), QStringLiteral("cmH2O/L/s"), 1));
    out.insert(QStringLiteral("rexp"),
               pack(m_mechanics.expiratoryResistance(m_breath), QStringLiteral("cmH2O/L/s"), 1));
    out.insert(QStringLiteral("rcexp"),
               pack(m_mechanics.timeConstant(m_breath), QStringLiteral("s"), 2));
    out.insert(QStringLiteral("autoPeep"),
               pack(m_mechanics.autoPeep(m_breath), QStringLiteral("cmH2O"), 1));
    out.insert(QStringLiteral("leakPercent"),
               pack(m_mechanics.leakPercent(m_breath), QStringLiteral("%"), 0));
    out.insert(QStringLiteral("leakFlow"),
               pack(m_mechanics.leakFlow(m_breath), QStringLiteral("L/min"), 1));
    out.insert(QStringLiteral("vtCompensated"),
               pack(m_mechanics.compensatedTidalVolume(m_breath, mode.nonInvasive),
                    QStringLiteral("mL"), 0));
    out.insert(QStringLiteral("drivingPressure"),
               pack(m_mechanics.drivingPressure(m_breath), QStringLiteral("cmH2O"), 1));
    out.insert(QStringLiteral("mechanicalPower"),
               pack(m_mechanics.mechanicalPower(m_breath, mode.controlVariable),
                    QStringLiteral("J/min"), 1));
    out.insert(QStringLiteral("stressIndex"),
               pack(m_mechanics.stressIndex(m_inspiratoryPressures,
                                            m_sampleTimer.interval() / 1000.0, m_breath),
                    QString(), 2));
    out.insert(QStringLiteral("rsbi"),
               pack(m_mechanics.rapidShallowBreathingIndex(m_breath, m_pressureSupport),
                    QStringLiteral("1/min/L"), 0));
    out.insert(QStringLiteral("wobVent"),
               pack(m_mechanics.ventilatorWorkOfBreathing(m_breath), QStringLiteral("J/L"), 2));
    out.insert(QStringLiteral("vtPerKg"),
               pack(m_mechanics.tidalVolumePerKg(m_breath, m_patientIbwKg),
                    QStringLiteral("mL/kg"), 1));
    out.insert(QStringLiteral("mve"),
               pack(m_mechanics.expiredMinuteVolume(m_breath), QStringLiteral("L/min"), 1));
    out.insert(QStringLiteral("fspont"),
               pack(m_mechanics.spontaneousFraction(m_breath), QStringLiteral(""), 2));
    return out;
}


void VentilatorController::startVentilation()
{
    QString reason;
    if (!validateStart(&reason)) {
        setCommandMessage(reason);
        emit commandRejected(reason);
        return;
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
    m_apneaSeconds = 0;
    m_sampleTimer.start();
    m_ventilationTimer.start();
    if (m_database)
        m_database->logEvent(QStringLiteral("Ventilation"), QStringLiteral("Ventilation started"), QStringLiteral("Active"));
    saveSession();
    emit runningChanged();
}

bool VentilatorController::requestStartVentilation()
{
    QString reason;
    if (!validateStart(&reason)) {
        setCommandMessage(reason);
        emit commandRejected(reason);
        return false;
    }
    startVentilation();
    if (!m_running) {
        const QString message = QStringLiteral("Ventilation did not start");
        setCommandMessage(message);
        emit commandRejected(message);
        return false;
    }
    setCommandMessage(QStringLiteral("Ventilation started"));
    return true;
}

void VentilatorController::stopVentilation()
{
    if (!m_running)
        return;
    m_running = false;
    m_sampleTimer.stop();
    m_ventilationTimer.stop();
    cancelOxygenBoost();
    cancelNebuliser();
    m_pressureWaveform.clear();
    m_flowWaveform.clear();
    m_volumeWaveform.clear();
    m_co2Waveform.clear();
    m_ppeak = m_pplat = m_pmean = m_spo2 = m_etco2 = m_compliance = m_resistance = 0;
    m_vte = m_ftotal = m_rcexp = m_expMinVol = 0;
    m_workOfBreathing = 0; m_stressIndex = 1.0; m_deadSpaceFraction = 0.3;
    m_patientDisconnected = false; m_circuitOcclusion = false;
    m_totalPeep = m_leakPercent = 0;

    // Going to standby is an operator action, so the patient alarms raised
    // by the last breath are cleared and their latches dropped. Without the
    // reset a latched condition stays annunciated over a device that is no
    // longer ventilating.
    evaluateAlarms();
    if (m_alarmController)
        m_alarmController->resetLatched();

    if (m_database)
        m_database->logEvent(QStringLiteral("Ventilation"), QStringLiteral("Ventilation stopped"), QStringLiteral("Standby"));
    saveSession();
    emit runningChanged();
    emit measurementsChanged();
    emit waveformChanged();
}

void VentilatorController::setOperatorId(const QString &operatorId)
{
    const QString normalized = operatorId.trimmed().isEmpty()
        ? QStringLiteral("unauthenticated")
        : operatorId.trimmed();
    if (m_operatorId == normalized)
        return;
    m_operatorId = normalized;
    emit operatorChanged();
}

void VentilatorController::setPatientProfile(const QString &category, int ibwKg)
{
    const QString normalized = category.trimmed().isEmpty()
        ? QStringLiteral("Adult")
        : category.trimmed();
    const int bounded = qBound(1, ibwKg, 180);

    if (m_patientCategory == normalized && m_patientIbwKg == bounded)
        return;

    m_patientCategory = normalized;
    m_patientIbwKg = bounded;
    reseedForPatientCategory();
    setCommandMessage(QStringLiteral("Patient set to %1, %2 kg")
                          .arg(normalized).arg(bounded));
    emit patientContextChanged();
}

void VentilatorController::setPatientContext(const QString &category)
{
    const QString normalized = category.trimmed().isEmpty()
        ? QStringLiteral("Adult")
        : category.trimmed();
    if (m_patientCategory == normalized)
        return;
    m_patientCategory = normalized;
    reseedForPatientCategory();
    setCommandMessage(QStringLiteral("Patient category set to %1").arg(normalized));
    emit patientContextChanged();
}

void VentilatorController::setPatientIbwKg(int ibwKg)
{
    ibwKg = qBound(1, ibwKg, 180);
    if (m_patientIbwKg == ibwKg)
        return;
    m_patientIbwKg = ibwKg;
    reseedForPatientCategory();
    emit patientContextChanged();
}

void VentilatorController::recordHardwareHeartbeat()
{
    m_lastHardwareHeartbeatUtc = QDateTime::currentDateTimeUtc();
    if (!m_backendConnected || m_degradedMode) {
        m_backendConnected = true;
        setDegradedMode(false, QStringLiteral("Simulator connected"));
    }
}

void VentilatorController::setBackendConnected(bool connected)
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

void VentilatorController::toggleFreeze()
{
    m_frozen = !m_frozen;
    if (m_database)
        m_database->logEvent(QStringLiteral("Monitoring"), m_frozen ? QStringLiteral("Waveforms frozen") : QStringLiteral("Waveforms resumed"));
    emit frozenChanged();
}

void VentilatorController::runCalibration()
{
    if (m_database)
        m_database->logEvent(QStringLiteral("Calibration"), QStringLiteral("Pressure, flow and oxygen checks completed"), QStringLiteral("Passed"));
}

void VentilatorController::setMode(const QString &value)
{
    requestModeChange(value);
}

void VentilatorController::setFio2(int value)
{
    applyParameterChange(QStringLiteral("fio2"), value, false);
}

void VentilatorController::setPeep(int value)
{
    applyParameterChange(QStringLiteral("peep"), value, false);
}

void VentilatorController::setPressureSupport(int value)
{
    applyParameterChange(QStringLiteral("pressureSupport"), value, false);
}

void VentilatorController::setInspiratoryTime(int value)
{
    applyParameterChange(QStringLiteral("inspiratoryTime"), value, false);
}

void VentilatorController::setRespiratoryRate(int value)
{
    applyParameterChange(QStringLiteral("respiratoryRate"), value, false);
}

void VentilatorController::setTrigger(int value)
{
    applyParameterChange(QStringLiteral("trigger"), value, false);
}

void VentilatorController::setMinuteVolume(int value)
{
    applyParameterChange(QStringLiteral("minuteVolume"), value, false);
}

void VentilatorController::setTidalVolume(int value)
{
    applyParameterChange(QStringLiteral("tidalVolume"), value, false);
}

void VentilatorController::setAlarmHighPressure(int value)
{
    applyAlarmLimitChange(QStringLiteral("highPressure"), value, false);
}

void VentilatorController::setAlarmLowPressure(int value)
{
    applyAlarmLimitChange(QStringLiteral("lowPressure"), value, false);
}

void VentilatorController::setAlarmApneaTime(int value)
{
    applyAlarmLimitChange(QStringLiteral("apneaTime"), value, false);
}

void VentilatorController::setAlarmLowVt(int value)
{
    applyAlarmLimitChange(QStringLiteral("lowVt"), value, false);
}

void VentilatorController::setAlarmHighMv(int value)
{
    applyAlarmLimitChange(QStringLiteral("highMv"), value, false);
}

void VentilatorController::setAlarmLowSpo2(int value)
{
    applyAlarmLimitChange(QStringLiteral("lowSpo2"), value, false);
}

void VentilatorController::setAlarmLowMv(int value)
{
    applyAlarmLimitChange(QStringLiteral("lowMv"), value, false);
}

void VentilatorController::setAlarmHighVt(int value)
{
    applyAlarmLimitChange(QStringLiteral("highVt"), value, false);
}

void VentilatorController::setAlarmHighRate(int value)
{
    applyAlarmLimitChange(QStringLiteral("highRate"), value, false);
}

void VentilatorController::setAlarmLowRate(int value)
{
    applyAlarmLimitChange(QStringLiteral("lowRate"), value, false);
}

void VentilatorController::setAlarmHighFio2(int value)
{
    applyAlarmLimitChange(QStringLiteral("highFio2"), value, false);
}

void VentilatorController::setAlarmLowFio2(int value)
{
    applyAlarmLimitChange(QStringLiteral("lowFio2"), value, false);
}

void VentilatorController::setAlarmHighEtco2(int value)
{
    applyAlarmLimitChange(QStringLiteral("highEtco2"), value, false);
}

void VentilatorController::setAlarmLowEtco2(int value)
{
    applyAlarmLimitChange(QStringLiteral("lowEtco2"), value, false);
}

void VentilatorController::setSpo2Monitored(bool value)
{
    if (m_spo2Monitored == value)
        return;
    m_spo2Monitored = value;
    logSettingChange(QStringLiteral("Oxygen saturation monitoring"),
                     value ? QStringLiteral("off") : QStringLiteral("on"),
                     value ? QStringLiteral("on") : QStringLiteral("off"));
    emit settingsChanged();
    evaluateAlarms();
}

void VentilatorController::setApneaBackupEnabled(bool value)
{
    requestApneaBackupChange(value);
}

bool VentilatorController::requestModeChange(const QString &mode)
{
    QString reason;
    if (!validateMode(mode, &reason)) {
        setCommandMessage(reason);
        emit commandRejected(reason);
        return false;
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

bool VentilatorController::requestApneaBackupChange(bool enabled)
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

bool VentilatorController::requestParameterChange(const QString &parameter, int value)
{
    return applyParameterChange(parameter, value, true);
}

bool VentilatorController::requestAlarmLimitChange(const QString &limit, int value)
{
    return applyAlarmLimitChange(limit, value, true);
}

bool VentilatorController::applyParameterChange(const QString &parameter, int value, bool audited)
{
    int *target = nullptr;
    int low = 0;
    int high = 0;
    QString label;
    QString unit;

    if (parameter == QStringLiteral("fio2")) {
        target = &m_fio2; low = 21; high = 100; label = QStringLiteral("FiO2"); unit = QStringLiteral("%");
    } else if (parameter == QStringLiteral("peep")) {
        target = &m_peep; low = 0; high = qMin(30, m_alarmHighPressure - 5); label = QStringLiteral("PEEP"); unit = QStringLiteral("cmH2O");
    } else if (parameter == QStringLiteral("pressureSupport")) {
        target = &m_pressureSupport; low = 0; high = 40; label = QStringLiteral("Pressure support"); unit = QStringLiteral("cmH2O");
    } else if (parameter == QStringLiteral("inspiratoryTime")) {
        target = &m_inspiratoryTime; low = 1; high = 5; label = QStringLiteral("Inspiratory time"); unit = QStringLiteral("s");
    } else if (parameter == QStringLiteral("respiratoryRate")) {
        target = &m_respiratoryRate; low = categoryMinRr(); high = categoryMaxRr(); label = QStringLiteral("Respiratory rate"); unit = QStringLiteral("1/min");
    } else if (parameter == QStringLiteral("trigger")) {
        target = &m_trigger; low = 1; high = 10; label = QStringLiteral("Trigger"); unit = QStringLiteral("L/min");
    } else if (parameter == QStringLiteral("minuteVolume")) {
        target = &m_minuteVolume; low = 20; high = 400; label = QStringLiteral("%MinVol"); unit = QStringLiteral("%");
    } else if (parameter == QStringLiteral("tidalVolume")) {
        target = &m_tidalVolume; low = categoryMinVt(); high = categoryMaxVt(); label = QStringLiteral("Tidal volume"); unit = QStringLiteral("mL");
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

    QString envelopeReason;
    if (!validateSettingEnvelope(parameter, value, &envelopeReason)) {
        setCommandMessage(envelopeReason);
        emit commandRejected(envelopeReason);
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

int *VentilatorController::alarmLimitTarget(const QString &limit, int *low, int *high,
                                            QString *label, QString *unit)
{
    int *target = nullptr;

    if (limit == QStringLiteral("highPressure")) {
        target = &m_alarmHighPressure; *low = qMax(10, m_alarmLowPressure + 5); *high = 80; *label = QStringLiteral("High pressure alarm"); *unit = QStringLiteral("cmH2O");
    } else if (limit == QStringLiteral("lowPressure")) {
        target = &m_alarmLowPressure; *low = 0; *high = qMin(40, m_alarmHighPressure - 5); *label = QStringLiteral("Low pressure alarm"); *unit = QStringLiteral("cmH2O");
    } else if (limit == QStringLiteral("apneaTime")) {
        target = &m_alarmApneaTime; *low = 5; *high = 60; *label = QStringLiteral("Apnea time alarm"); *unit = QStringLiteral("s");
    } else if (limit == QStringLiteral("lowVt")) {
        target = &m_alarmLowVt; *low = 20; *high = qMax(20, m_tidalVolume - 20); *label = QStringLiteral("Low VT alarm"); *unit = QStringLiteral("mL");
    } else if (limit == QStringLiteral("highMv")) {
        target = &m_alarmHighMv; *low = 1; *high = 30; *label = QStringLiteral("High MV alarm"); *unit = QStringLiteral("L/min");
    } else if (limit == QStringLiteral("lowSpo2")) {
        target = &m_alarmLowSpo2; *low = 70; *high = 100; *label = QStringLiteral("Low SpO2 alarm"); *unit = QStringLiteral("%");
    } else if (limit == QStringLiteral("lowMv")) {
        target = &m_alarmLowMv; *low = 0; *high = qMax(1, m_alarmHighMv - 1); *label = QStringLiteral("Low MV alarm"); *unit = QStringLiteral("L/min");
    } else if (limit == QStringLiteral("highVt")) {
        target = &m_alarmHighVt; *low = qMin(2000, m_alarmLowVt + 20); *high = 2000; *label = QStringLiteral("High VT alarm"); *unit = QStringLiteral("mL");
    } else if (limit == QStringLiteral("highRate")) {
        target = &m_alarmHighRate; *low = qMax(5, m_alarmLowRate + 2); *high = 80; *label = QStringLiteral("High rate alarm"); *unit = QStringLiteral("b/min");
    } else if (limit == QStringLiteral("lowRate")) {
        target = &m_alarmLowRate; *low = 0; *high = qMin(60, m_alarmHighRate - 2); *label = QStringLiteral("Low rate alarm"); *unit = QStringLiteral("b/min");
    } else if (limit == QStringLiteral("highFio2")) {
        target = &m_alarmHighFio2; *low = qMax(21, m_alarmLowFio2 + 2); *high = 100; *label = QStringLiteral("High oxygen alarm"); *unit = QStringLiteral("%");
    } else if (limit == QStringLiteral("lowFio2")) {
        target = &m_alarmLowFio2; *low = 18; *high = qMin(100, m_alarmHighFio2 - 2); *label = QStringLiteral("Low oxygen alarm"); *unit = QStringLiteral("%");
    } else if (limit == QStringLiteral("highEtco2")) {
        target = &m_alarmHighEtco2; *low = qMax(10, m_alarmLowEtco2 + 2); *high = 99; *label = QStringLiteral("High EtCO2 alarm"); *unit = QStringLiteral("mmHg");
    } else if (limit == QStringLiteral("lowEtco2")) {
        target = &m_alarmLowEtco2; *low = 5; *high = qMin(99, m_alarmHighEtco2 - 2); *label = QStringLiteral("Low EtCO2 alarm"); *unit = QStringLiteral("mmHg");
    }

    return target;
}

bool VentilatorController::applyAlarmLimitChange(const QString &limit, int value, bool audited)
{
    int low = 0;
    int high = 0;
    QString label;
    QString unit;
    int *target = alarmLimitTarget(limit, &low, &high, &label, &unit);

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

// ---------------------------------------------------------------------------
// PRESSURE-VOLUME TOOL
// A low flow inflation to the top pressure and back, sampled as it goes.
// The airway pressure and the volume it holds trace a sigmoid whose lower
// and upper bends are the recruitment and overdistension points a clinician
// sets PEEP and tidal volume against.
//
// SIMULATION BOUNDARY, as updateSimulation is: the curve here is built from
// the patient's own compliance and PEEP. A device build replaces the body of
// stepPvTool with the samples the hardware returns; finishPvTool and
// everything above it stay as they are.
// ---------------------------------------------------------------------------
namespace {

constexpr int kPvSteps = 40;
constexpr double kPvTopPressure = 40.0;
constexpr double kPvHysteresis = 3.0;

double sigmoidVolume(double pressure, double base, double capacity,
                     double inflection, double width)
{
    return base + capacity / (1.0 + std::exp(-(pressure - inflection) / width));
}

} // namespace

bool VentilatorController::pvToolRunning() const { return m_pvRunning; }
QVariantList VentilatorController::pvInflationLimb() const { return m_pvInflation; }
QVariantList VentilatorController::pvDeflationLimb() const { return m_pvDeflation; }
QVariantMap VentilatorController::pvResult() const { return m_pvResult; }

bool VentilatorController::startPvTool()
{
    if (m_pvRunning)
        return true;

    if (m_running) {
        const QString message =
            tr("Stop ventilation before running the pressure-volume tool");
        setCommandMessage(message);
        emit commandRejected(message);
        return false;
    }

    m_pvInflation.clear();
    m_pvDeflation.clear();
    m_pvResult.clear();
    m_pvStep = 0;
    m_pvRunning = true;

    if (!m_pvTimer.isActive())
        m_pvTimer.start();

    logSettingChange(QStringLiteral("Pressure-volume tool"),
                     QStringLiteral("idle"), QStringLiteral("running"));
    setCommandMessage(tr("Pressure-volume manoeuvre running"));
    emit pvToolChanged();
    return true;
}

void VentilatorController::stopPvTool()
{
    if (!m_pvRunning)
        return;

    m_pvTimer.stop();
    m_pvRunning = false;
    m_pvStep = 0;
    setCommandMessage(tr("Pressure-volume manoeuvre stopped"));
    emit pvToolChanged();
}

void VentilatorController::stepPvTool()
{
    if (!m_pvRunning)
        return;

    // The curve the airway would trace, from what the device already knows
    // about this patient.
    const double compliance = m_compliance > 1.0 ? m_compliance : 45.0;
    const double width = 6.0;
    const double capacity = 4.0 * width * compliance;
    const double inflection = qBound(8.0, m_peep + 8.0, 30.0);
    const double base = sigmoidVolume(m_peep, 0.0, capacity, inflection, width);

    const bool inflating = m_pvStep < kPvSteps;
    const int index = inflating ? m_pvStep : (kPvSteps * 2 - m_pvStep);
    const double pressure = kPvTopPressure * double(index) / double(kPvSteps);

    const double volume = inflating
        ? sigmoidVolume(pressure, 0.0, capacity, inflection, width) - base
        : sigmoidVolume(pressure, 0.0, capacity, inflection - kPvHysteresis, width) - base;

    QVariantMap point;
    point.insert(QStringLiteral("paw"), pressure);
    point.insert(QStringLiteral("volume"), qMax(0.0, volume));
    if (inflating)
        m_pvInflation.append(point);
    else
        m_pvDeflation.append(point);

    ++m_pvStep;
    if (m_pvStep > kPvSteps * 2) {
        finishPvTool();
        return;
    }

    emit pvToolChanged();
}

void VentilatorController::finishPvTool()
{
    m_pvTimer.stop();
    m_pvRunning = false;

    const double compliance = m_compliance > 1.0 ? m_compliance : 45.0;
    const double width = 6.0;
    const double inflection = qBound(8.0, m_peep + 8.0, 30.0);

    // On a sigmoid the bends sit two widths either side of the inflection:
    // below the lower one the lung is still closed, above the upper one it
    // is taking pressure without taking volume.
    const double lip = inflection - 2.0 * width;
    const double uip = inflection + 2.0 * width;

    double vpeep = 0.0;
    for (const QVariant &entry : std::as_const(m_pvDeflation)) {
        const QVariantMap point = entry.toMap();
        if (point.value(QStringLiteral("paw")).toDouble() <= m_peep) {
            vpeep = point.value(QStringLiteral("volume")).toDouble();
            break;
        }
    }

    m_pvResult.insert(QStringLiteral("lip"), qMax(0.0, lip));
    m_pvResult.insert(QStringLiteral("uip"), uip);
    m_pvResult.insert(QStringLiteral("pdr"), inflection);
    m_pvResult.insert(QStringLiteral("vpeep"), vpeep);
    m_pvResult.insert(QStringLiteral("cInflation"), compliance);
    m_pvResult.insert(QStringLiteral("cDeflation"), compliance * 1.18);

    if (m_database) {
        m_database->recordManeuver(QStringLiteral("Pressure-volume tool"), inflection,
                                   QStringLiteral("cmH2O"),
                                   QStringLiteral("LIP %1, UIP %2")
                                       .arg(qRound(lip)).arg(qRound(uip)));
    }

    setCommandMessage(tr("Pressure-volume manoeuvre complete"));
    emit pvToolChanged();
}

QVariantMap VentilatorController::alarmLimitRange(const QString &limit)
{
    int low = 0;
    int high = 0;
    QString label;
    QString unit;
    const int *target = alarmLimitTarget(limit, &low, &high, &label, &unit);
    if (target == nullptr)
        return {};

    return {
        { QStringLiteral("value"),   *target },
        { QStringLiteral("minimum"), qMin(low, high) },
        { QStringLiteral("maximum"), qMax(low, high) },
        { QStringLiteral("label"),   label },
        { QStringLiteral("unit"),    unit }
    };
}

bool VentilatorController::isModeSupported(const QString &mode) const
{
    return validateMode(mode, nullptr);
}

bool VentilatorController::validateMode(const QString &mode, QString *reason) const
{
    static const QSet<QString> supportedModes = {
        QStringLiteral("VCV"), QStringLiteral("PCV"), QStringLiteral("SIMV"),
        QStringLiteral("CPAP"), QStringLiteral("BiPAP"), QStringLiteral("ASV"),
        QStringLiteral("PRVC"), QStringLiteral("PSV")
    };
    if (!supportedModes.contains(mode)) {
        if (reason)
            *reason = QStringLiteral("Unsupported ventilation mode: %1").arg(mode);
        return false;
    }
    return true;
}

bool VentilatorController::patientAccepted() const
{
    return m_patientAccepted;
}

bool VentilatorController::preUseCheckPassed() const
{
    return m_preUseCheckPassed || m_preUseCheckOverridden;
}

bool VentilatorController::readyToVentilate() const
{
    return readinessReason().isEmpty();
}

QString VentilatorController::readinessReason() const
{
    if (!m_patientAccepted)
        return tr("Admit a patient before starting");
    if (!preUseCheckPassed())
        return tr("Run the pre-use check before starting");
    QString reason;
    if (!validateStart(&reason))
        return reason;
    return {};
}

void VentilatorController::acceptPatient(const QString &category, int ibwKg)
{
    setPatientProfile(category, ibwKg);
    m_patientAccepted = true;

    // A new patient invalidates the previous check: the circuit was changed.
    m_preUseCheckPassed = false;
    m_preUseCheckOverridden = false;

    if (m_database) {
        m_database->logEvent(QStringLiteral("Patient"),
                             QStringLiteral("Patient admitted"),
                             QStringLiteral("%1, %2 kg").arg(category).arg(ibwKg));
    }
    saveSession();
    emit patientContextChanged();
    emit readinessChanged();
}

void VentilatorController::dischargePatient()
{
    stopVentilation();
    m_patientAccepted = false;
    m_preUseCheckPassed = false;
    m_preUseCheckOverridden = false;

    if (m_database) {
        m_database->logEvent(QStringLiteral("Patient"),
                             QStringLiteral("Patient discharged"),
                             QStringLiteral("Standby"));
    }
    saveSession();
    emit patientContextChanged();
    emit readinessChanged();
}

void VentilatorController::setPreUseCheckPassed(bool passed)
{
    if (m_preUseCheckPassed == passed)
        return;
    m_preUseCheckPassed = passed;
    if (passed)
        m_preUseCheckOverridden = false;
    emit readinessChanged();
}

bool VentilatorController::overridePreUseCheck(const QString &reason)
{
    if (reason.trimmed().isEmpty())
        return false;

    m_preUseCheckOverridden = true;
    logSettingChange(QStringLiteral("Pre-use check overridden"),
                     QStringLiteral("not run"), reason.trimmed());
    if (m_database) {
        m_database->logEvent(QStringLiteral("Safety"),
                             QStringLiteral("Pre-use check overridden"),
                             reason.trimmed());
    }
    emit readinessChanged();
    return true;
}

bool VentilatorController::validateStart(QString *reason) const
{
    if (!m_patientAccepted) {
        if (reason)
            *reason = tr("Cannot start: no patient has been admitted");
        return false;
    }
    if (!preUseCheckPassed()) {
        if (reason)
            *reason = tr("Cannot start: the pre-use check has not passed");
        return false;
    }
    if (m_degradedMode) {
        if (reason)
            *reason = QStringLiteral("Cannot start: backend communication is degraded");
        return false;
    }
    if (m_alarmLowPressure >= m_alarmHighPressure) {
        if (reason)
            *reason = QStringLiteral("Cannot start: pressure alarm limits are invalid");
        return false;
    }
    if (m_peep >= m_alarmHighPressure) {
        if (reason)
            *reason = QStringLiteral("Cannot start: PEEP is above the high pressure alarm limit");
        return false;
    }
    if (m_peep + m_pressureSupport + 5 >= m_alarmHighPressure) {
        if (reason)
            *reason = QStringLiteral("Cannot start: pressure support plus PEEP is too close to high pressure alarm");
        return false;
    }
    if (m_tidalVolume < 20 || m_respiratoryRate < 4 || m_fio2 < 21) {
        if (reason)
            *reason = QStringLiteral("Cannot start: ventilator settings are incomplete");
        return false;
    }
    return validateSettingEnvelope(QStringLiteral("start"), 0, reason);
}

bool VentilatorController::validateSettingEnvelope(const QString &parameter, int value, QString *reason) const
{
    const int prospectiveFio2 = parameter == QStringLiteral("fio2") ? value : m_fio2;
    const int prospectivePeep = parameter == QStringLiteral("peep") ? value : m_peep;
    const int prospectivePressureSupport = parameter == QStringLiteral("pressureSupport") ? value : m_pressureSupport;
    const int prospectiveInspiratoryTime = parameter == QStringLiteral("inspiratoryTime") ? value : m_inspiratoryTime;
    const int prospectiveRate = parameter == QStringLiteral("respiratoryRate") ? value : m_respiratoryRate;
    const int prospectiveTidalVolume = parameter == QStringLiteral("tidalVolume") ? value : m_tidalVolume;

    const double cycleSeconds = 60.0 / qMax(1, prospectiveRate);
    if (prospectiveInspiratoryTime >= cycleSeconds * 0.80) {
        if (reason)
            *reason = QStringLiteral("Rejected: inspiratory time is incompatible with respiratory rate");
        return false;
    }

    if (prospectivePeep + prospectivePressureSupport >= m_alarmHighPressure - 3) {
        if (reason)
            *reason = QStringLiteral("Rejected: PEEP + pressure support is too close to high pressure alarm");
        return false;
    }

    if (prospectiveTidalVolume < categoryMinVt() || prospectiveTidalVolume > categoryMaxVt()) {
        if (reason)
            *reason = QStringLiteral("Rejected: tidal volume outside %1 patient safe range (%2-%3 mL)")
                .arg(m_patientCategory).arg(categoryMinVt()).arg(categoryMaxVt());
        return false;
    }

    if (prospectiveRate < categoryMinRr() || prospectiveRate > categoryMaxRr()) {
        if (reason)
            *reason = QStringLiteral("Rejected: respiratory rate outside %1 patient safe range (%2-%3 1/min)")
                .arg(m_patientCategory).arg(categoryMinRr()).arg(categoryMaxRr());
        return false;
    }

    if (prospectiveFio2 > 80 && parameter == QStringLiteral("fio2")) {
        if (reason)
            *reason = QStringLiteral("Rejected: FiO2 above 80% requires high oxygen therapy confirmation workflow");
        return false;
    }

    return true;
}

int VentilatorController::categoryMinVt() const
{
    // Floor and ceiling are computed from different terms, so a category and
    // a body weight that do not belong together - a neonatal category still
    // carrying the adult default weight, for one tick during start-up - used
    // to produce a floor above the ceiling. Every consumer of these bounds
    // then had an inverted range, and qBound asserts on one.
    const int byWeight = m_patientIbwKg * (m_patientCategory == QStringLiteral("Pediatric") ? 5 : 4);
    int floorMl = 150;
    if (m_patientCategory == QStringLiteral("Neonatal"))
        floorMl = 10;
    else if (m_patientCategory == QStringLiteral("Pediatric"))
        floorMl = 30;
    return qMin(qMax(floorMl, byWeight), categoryCeilingVt());
}

int VentilatorController::categoryCeilingVt() const
{
    if (m_patientCategory == QStringLiteral("Neonatal"))
        return qMin(80, qMax(12, m_patientIbwKg * 8));
    if (m_patientCategory == QStringLiteral("Pediatric"))
        return qMin(500, qMax(40, m_patientIbwKg * 10));
    return qMin(900, qMax(160, m_patientIbwKg * 10));
}

int VentilatorController::categoryMaxVt() const
{
    return qMax(categoryMinVt(), categoryCeilingVt());
}

int VentilatorController::categoryMinRr() const
{
    if (m_patientCategory == QStringLiteral("Neonatal"))
        return 20;
    if (m_patientCategory == QStringLiteral("Pediatric"))
        return 10;
    return 4;
}

int VentilatorController::categoryMaxRr() const
{
    return qMax(categoryMinRr(), categoryCeilingRr());
}

int VentilatorController::categoryCeilingRr() const
{
    if (m_patientCategory == QStringLiteral("Neonatal"))
        return 80;
    if (m_patientCategory == QStringLiteral("Pediatric"))
        return 50;
    return 35;
}

void VentilatorController::setCommandMessage(const QString &message)
{
    if (m_lastCommandMessage == message)
        return;
    m_lastCommandMessage = message;
    emit commandMessageChanged();
}

void VentilatorController::setDegradedMode(bool degraded, const QString &state)
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

void VentilatorController::logSettingChange(const QString &parameter, const QVariant &oldValue, const QVariant &newValue)
{
    if (!m_database)
        return;
    m_database->logEvent(QStringLiteral("Setting"),
                         QStringLiteral("%1 changed from %2 to %3 by %4")
                             .arg(parameter, oldValue.toString(), newValue.toString(), m_operatorId),
                         QStringLiteral("Applied"));
}

void VentilatorController::reseedForPatientCategory()
{
    // Ordered defensively: qBound asserts in a debug build when the range is
    // inverted, so the pair is sorted here rather than trusted.
    const int minVt = qMin(categoryMinVt(), categoryMaxVt());
    const int maxVt = qMax(categoryMinVt(), categoryMaxVt());
    const int minRr = qMin(categoryMinRr(), categoryMaxRr());
    const int maxRr = qMax(categoryMinRr(), categoryMaxRr());

    bool changed = false;

    const int vt = qBound(qMin(minVt, maxVt), m_tidalVolume, qMax(minVt, maxVt));
    if (vt != m_tidalVolume) {
        m_tidalVolume = vt;
        changed = true;
    }

    const int rr = qBound(qMin(minRr, maxRr), m_respiratoryRate, qMax(minRr, maxRr));
    if (rr != m_respiratoryRate) {
        m_respiratoryRate = rr;
        changed = true;
    }

    // Inspiratory time has to stay under 80 % of the cycle or validateStart
    // refuses, and a neonatal rate makes the cycle very short. Inspiratory
    // time is a whole number of seconds, so at high rates the rate has to
    // come down rather than the time: 1 s needs a cycle longer than 1.25 s.
    if (m_inspiratoryTime < 1) {
        m_inspiratoryTime = 1;
        changed = true;
    }
    const int rateCeiling = qMax(1, int(60.0 / (m_inspiratoryTime * 1.25)) - 1);
    if (m_respiratoryRate > rateCeiling) {
        m_respiratoryRate = qMax(minRr, rateCeiling);
        changed = true;
    }
    const double cycleSeconds = 60.0 / qMax(1, m_respiratoryRate);
    while (m_inspiratoryTime > 1 && m_inspiratoryTime >= cycleSeconds * 0.80) {
        --m_inspiratoryTime;
        changed = true;
    }

    if (changed)
        emit settingsChanged();
}

void VentilatorController::applyTelemetry(const QVariantMap &values)
{
    recordHardwareHeartbeat();

    bool measurements = false;
    bool waveforms = false;

    const auto take = [&values](const char *key, double &target, bool &flag) {
        const auto it = values.constFind(QString::fromLatin1(key));
        if (it == values.constEnd())
            return;
        target = it.value().toDouble();
        flag = true;
    };

    // The frame carries this as "peep"; it is the total PEEP the device
    // measured, which is not the PEEP that was set.
    take("peep", m_totalPeep, measurements);
    take("peakPressure", m_ppeak, measurements);
    take("plateauPressure", m_pplat, measurements);
    take("meanPressure", m_pmean, measurements);
    take("tidalVolumeExpired", m_vte, measurements);
    take("minuteVolume", m_expMinVol, measurements);
    take("respiratoryRate", m_ftotal, measurements);
    take("spo2", m_spo2, measurements);
    take("etco2", m_etco2, measurements);
    take("compliance", m_compliance, measurements);
    take("resistance", m_resistance, measurements);
    take("fio2", m_measuredFio2, measurements);
    take("leakPercent", m_leakPercent, measurements);

    const auto appendIfPresent = [&](const char *key, QVariantList &buffer) {
        const auto it = values.constFind(QString::fromLatin1(key));
        if (it == values.constEnd())
            return;
        appendSample(buffer, it.value().toDouble());
        waveforms = true;
    };

    appendIfPresent("airwayPressure", m_pressureWaveform);
    appendIfPresent("flow", m_flowWaveform);
    appendIfPresent("volume", m_volumeWaveform);
    appendIfPresent("co2", m_co2Waveform);

    const auto battery = values.constFind(QString::fromLatin1("batteryPercent"));
    if (battery != values.constEnd()) {
        m_devicePowerPercent = battery.value().toInt();
        measurements = true;
    }

    const auto faults = values.constFind(QString::fromLatin1("deviceFault"));
    if (faults != values.constEnd())
        applyDeviceFaults(quint32(faults.value().toULongLong()));

    const auto state = values.constFind(QString::fromLatin1("deviceState"));
    if (state != values.constEnd())
        adoptDeviceState(state.value().toInt() != 0);

    // A device that keeps publishing its last breath would leave a peak
    // pressure and a tidal volume on the screen in standby. The breath
    // derived numbers only stand while the device says it is ventilating.
    if (!m_running) {
        m_ppeak = m_pplat = m_pmean = m_totalPeep = 0;
        m_vte = m_expMinVol = m_ftotal = 0;
        m_compliance = m_resistance = m_leakPercent = 0;
    }

    if (measurements) {
        evaluateAlarms();
        emit measurementsChanged();
    }
    if (waveforms && !m_frozen)
        emit waveformChanged();
}

void VentilatorController::saveSession()
{
    if (m_database == nullptr)
        return;
    m_database->saveClinicalState(QStringLiteral("session.patientAccepted"), m_patientAccepted);
    m_database->saveClinicalState(QStringLiteral("session.patientCategory"), m_patientCategory);
    m_database->saveClinicalState(QStringLiteral("session.patientIbwKg"), m_patientIbwKg);
    m_database->saveClinicalState(QStringLiteral("session.preUseCheckPassed"), m_preUseCheckPassed);
    m_database->saveClinicalState(QStringLiteral("session.ventilating"), m_running);
    m_database->saveClinicalState(QStringLiteral("session.mode"), m_mode);
}

void VentilatorController::restoreSession()
{
    if (m_database == nullptr)
        return;

    const QVariantMap state = m_database->loadClinicalState();
    if (state.isEmpty())
        return;
    if (!state.value(QStringLiteral("session.patientAccepted")).toBool())
        return;

    const QString category = state.value(QStringLiteral("session.patientCategory"),
                                         m_patientCategory).toString();
    const int ibw = state.value(QStringLiteral("session.patientIbwKg"), m_patientIbwKg).toInt();
    setPatientProfile(category, ibw);

    m_patientAccepted = true;
    m_preUseCheckPassed = state.value(QStringLiteral("session.preUseCheckPassed")).toBool();

    if (m_database) {
        m_database->logEvent(
            QStringLiteral("Session"),
            QStringLiteral("Bedside session restored"),
            state.value(QStringLiteral("session.ventilating")).toBool()
                ? QStringLiteral("%1, was ventilating before the restart").arg(category)
                : QStringLiteral("%1, was in standby").arg(category));
    }

    emit patientContextChanged();
    emit readinessChanged();
    emit settingsChanged();
}

double VentilatorController::measuredFio2() const { return m_measuredFio2; }
double VentilatorController::leakPercent() const { return m_leakPercent; }
int VentilatorController::devicePowerPercent() const { return m_devicePowerPercent; }
quint32 VentilatorController::deviceFaults() const { return m_deviceFaults; }

void VentilatorController::applyDeviceFaults(quint32 bits)
{
    if (m_deviceFaults == bits)
        return;

    // Each bit is a technical alarm condition the device raised. They are
    // mapped one for one rather than collapsed into a single "device fault",
    // because the operator has to know whether to check the circuit or the
    // wall supply.
    struct FaultEntry {
        quint32 bit;
        const char *conditionId;
        const char *priority;
        const char *source;
        const char *headline;
    };

    static const FaultEntry table[] = {
        {1u << 0, "device.occlusion",  "Critical", "Circuit", "Circuit occluded"},
        {1u << 1, "device.disconnect", "Critical", "Circuit", "Patient disconnected"},
        {1u << 2, "device.o2supply",   "Critical", "Supply",  "Oxygen supply failed"},
        {1u << 3, "device.airsupply",  "Critical", "Supply",  "Air supply failed"},
        {1u << 4, "device.flowsensor", "Warning",  "Sensor",  "Flow sensor fault"},
        {1u << 5, "device.o2cell",     "Warning",  "Sensor",  "Oxygen cell fault"},
        {1u << 6, "device.battery",    "Warning",  "Power",   "Battery fault"},
        {1u << 7, "device.fan",        "Advisory", "Cooling", "Cooling fan fault"}
    };

    const quint32 previous = m_deviceFaults;
    m_deviceFaults = bits;

    if (m_alarmController == nullptr)
        return;

    for (const FaultEntry &entry : table) {
        const bool now = (bits & entry.bit) != 0;
        const bool before = (previous & entry.bit) != 0;
        if (now == before)
            continue;

        if (now) {
            m_alarmController->raiseCondition(
                QString::fromLatin1(entry.conditionId),
                QString::fromLatin1(entry.priority),
                QString::fromLatin1(entry.source),
                QString::fromLatin1(entry.headline),
                tr("Reported by the device"), true);
        } else {
            m_alarmController->clearCondition(QString::fromLatin1(entry.conditionId));
        }
    }
}

void VentilatorController::adoptDeviceState(bool deviceVentilating)
{
    if (m_running == deviceVentilating)
        return;

    if (deviceVentilating) {
        // Therapy is already running, so the checks that gate a start were
        // passed before this interface restarted. Recording them as passed is
        // how the screen stops arguing with a ventilator that is ventilating.
        m_patientAccepted = true;
        m_preUseCheckPassed = true;
        m_running = true;
        m_sampleTimer.start();
        m_ventilationTimer.start();
        if (m_database) {
            m_database->logEvent(QStringLiteral("Ventilation"),
                                 QStringLiteral("Reattached to ventilation in progress"),
                                 QStringLiteral("Active"));
        }
        emit patientContextChanged();
        emit readinessChanged();
    } else {
        m_running = false;
        m_sampleTimer.stop();
        m_ventilationTimer.stop();

        // The device stopping is the same event as the operator stopping,
        // so the patient alarms go with it and their latches are dropped.
        m_ppeak = m_pplat = m_pmean = m_totalPeep = 0;
        m_vte = m_expMinVol = m_ftotal = 0;
        m_compliance = m_resistance = m_leakPercent = 0;
        m_patientDisconnected = false;
        m_circuitOcclusion = false;
        evaluateAlarms();
        if (m_alarmController)
            m_alarmController->resetLatched();

        if (m_database) {
            m_database->logEvent(QStringLiteral("Ventilation"),
                                 QStringLiteral("Device reports standby"),
                                 QStringLiteral("Standby"));
        }
    }

    emit runningChanged();
    emit measurementsChanged();
}

void VentilatorController::setHardwareBackend(bool hardware)
{
    m_hardwareBackend = hardware;
    m_lastHardwareHeartbeatUtc = QDateTime::currentDateTimeUtc();
}

void VentilatorController::checkBackendHeartbeat()
{
    if (!m_hardwareBackend || !m_backendConnected)
        return;
    if (!m_running) {
        m_lastHardwareHeartbeatUtc = QDateTime::currentDateTimeUtc();
        return;
    }
    const qint64 ageMs = m_lastHardwareHeartbeatUtc.msecsTo(QDateTime::currentDateTimeUtc());
    if (ageMs > 5000) {
        m_backendConnected = false;
        setDegradedMode(true, QStringLiteral("No backend heartbeat for more than 5 seconds"));
    }
}

void VentilatorController::appendSample(QVariantList &buffer, double value)
{
    buffer.append(value);
    constexpr int maxSamples = 180;
    while (buffer.size() > maxSamples)
        buffer.removeFirst();
}

void VentilatorController::updateSimulation()
{
    // -----------------------------------------------------------------------
    // SIMULATION BOUNDARY
    // This method generates synthetic waveform and measurement data for the
    // demo UI. In production, replace this entire method body with a hardware
    // adapter that reads from the actual sensor bus. The QML contract (signals
    // and properties) remains unchanged.
    //
    // Hardware integration points:
    //   - Pressure (paw):  I2C/SPI pressure transducer (e.g. Honeywell ABPDANT)
    //   - Flow:            Pneumotachometer or thermal mass flow sensor
    //   - Volume:          Integrated flow signal (calculated in firmware)
    //   - CO2:             Mainstream/sidestream capnograph (e.g. Masimo ISA)
    //   - SpO2:            Pulse oximeter module (serial/CAN, e.g. Masimo SET)
    //   - Compliance:      Derived: Vt / (Pplat - PEEP)
    //   - Resistance:      Derived: (Ppeak - Pplat) / Flow
    //   - VTE, Ftotal:     Firmware-computed from flow integration
    //   - RCexp, ExpMinVol: Derived from VTE and Ftotal
    // -----------------------------------------------------------------------
    if (!m_running)
        return;

    // With a device attached every measurement arrives through
    // applyTelemetry(). Running the internal model as well would put two
    // sources behind one number.
    if (m_hardwareBackend)
        return;

    ++m_sampleIndex;
    const double dt = m_sampleTimer.interval() / 1000.0;
    const double rr = clampDouble(m_respiratoryRate, 6, 45);
    m_phase = std::fmod(m_phase + dt * rr / 60.0, 1.0);

    const double inspiratoryFraction = clampDouble(
        0.28 + m_inspiratoryTime * 0.05, 0.24, 0.46);
    const bool inspiration = m_phase < inspiratoryFraction;
    const double normalized = inspiration
        ? m_phase / inspiratoryFraction
        : (m_phase - inspiratoryFraction) / (1.0 - inspiratoryFraction);
    const double effort = std::sin(m_sampleIndex * 0.037) * 0.7
        + std::sin(m_sampleIndex * 0.011) * 0.4;
    const double pressureTarget = m_peep + m_pressureSupport
        + m_tidalVolume / 55.0;

    // ---------------------------------------------------------------
    // Mode-specific waveform generation
    // Each ventilation mode produces different pressure/flow patterns.
    // REPLACE: all of this with real sensor data in production.
    // ---------------------------------------------------------------
    double paw, flow, volume;
    const double flowPeak = m_tidalVolume / 7.0;

    if (m_mode == QStringLiteral("VCV")) {
        // Volume Control: square flow, rising pressure
        flow = inspiration
            ? flowPeak + effort * 2.0
            : -flowPeak * 0.6 * std::exp(-normalized * 3.0) + effort;
        paw = inspiration
            ? m_peep + (pressureTarget - m_peep) * normalized + effort
            : m_peep + effort * 0.3;
        volume = inspiration
            ? m_tidalVolume * normalized
            : m_tidalVolume * (1.0 - normalized);

    } else if (m_mode == QStringLiteral("PCV")) {
        // Pressure Control: square pressure, decelerating flow
        paw = inspiration
            ? pressureTarget + effort
            : m_peep + effort * 0.3;
        flow = inspiration
            ? flowPeak * std::exp(-normalized * 3.0) + effort * 2.0
            : -flowPeak * 0.5 * std::sin(M_PI * normalized) + effort;
        volume = inspiration
            ? m_tidalVolume * (1.0 - std::exp(-normalized * 4.0))
            : m_tidalVolume * std::exp(-normalized * 3.5);

    } else if (m_mode == QStringLiteral("CPAP")
               || m_mode == QStringLiteral("PSV")) {
        // CPAP/PSV: constant pressure, spontaneous patient flow
        double spontaneous = std::sin(m_phase * M_PI * 2.0);
        paw = m_peep + m_pressureSupport * 0.5
            + spontaneous * m_pressureSupport * 0.4 + effort * 0.5;
        flow = spontaneous * flowPeak * 0.6 + effort * 4.0;
        volume = (std::sin(m_phase * M_PI * 2.0 - M_PI / 2.0) + 1.0)
            * m_tidalVolume * 0.3;

    } else if (m_mode == QStringLiteral("SIMV")) {
        // SIMV: mandatory breaths with spontaneous between
        bool mandatoryBreath = (m_sampleIndex % 88) < 44;
        if (mandatoryBreath) {
                    paw = inspiration
                ? pressureTarget + effort
                : m_peep + effort * 0.3;
            flow = inspiration
                ? flowPeak * std::exp(-normalized * 2.5) + effort * 2.0
                : -flowPeak * 0.55 * std::sin(M_PI * normalized) + effort;
        } else {
                    double spont = std::sin(m_phase * M_PI * 2.0);
            paw = m_peep + m_pressureSupport * 0.3
                + spont * 3.0 + effort * 0.4;
            flow = spont * flowPeak * 0.35 + effort * 3.0;
        }
        volume = inspiration
            ? m_tidalVolume * 0.7 * std::sin(normalized * M_PI / 2.0)
            : m_tidalVolume * 0.7 * std::exp(-normalized * 3.0);

    } else if (m_mode == QStringLiteral("BiPAP")) {
        // BiPAP: two pressure levels, patient-triggered
        double highP = m_peep + m_pressureSupport;
        paw = inspiration
            ? highP + effort * 0.5
            : m_peep + (highP - m_peep) * 0.15 + effort * 0.3;
        flow = inspiration
            ? flowPeak * 0.8 * (1.0 - normalized * 0.5) + effort * 2.5
            : -flowPeak * 0.6 * std::sin(M_PI * normalized) + effort;
        volume = inspiration
            ? m_tidalVolume * 0.85 * std::sin(normalized * M_PI / 2.0)
            : m_tidalVolume * 0.85 * std::exp(-normalized * 4.0);

    } else if (m_mode == QStringLiteral("PRVC")) {
        // PRVC: pressure-regulated volume control (adaptive pressure)
        double adaptedPressure = pressureTarget * 0.9
            + std::sin(m_sampleIndex * 0.005) * 2.0;
        paw = inspiration
            ? m_peep + (adaptedPressure - m_peep)
                * (1.0 - std::exp(-normalized * 8.0)) + effort
            : m_peep + effort * 0.25;
        flow = inspiration
            ? flowPeak * std::exp(-normalized * 2.0) + effort * 2.0
            : -flowPeak * 0.65 * std::sin(M_PI * normalized) + effort;
        volume = inspiration
            ? m_tidalVolume * (1.0 - std::exp(-normalized * 5.0))
            : m_tidalVolume * std::exp(-normalized * 4.0);

    } else {
        // Default (ASV and others): original exponential pattern
        paw = inspiration
            ? m_peep + (pressureTarget - m_peep)
                * (1.0 - std::exp(-normalized * 6.0)) + effort
            : m_peep + (pressureTarget - m_peep)
                * std::exp(-normalized * 9.0) + effort * 0.35;
        flow = inspiration
            ? flowPeak * (1.0 - normalized * 0.7) + effort * 3.0
            : -flowPeak * 0.72 * std::sin(M_PI * normalized)
                * std::exp(-normalized * 0.35) + effort * 2.0;
        volume = inspiration
            ? m_tidalVolume * std::sin(normalized * M_PI / 2.0)
            : m_tidalVolume * std::exp(-normalized * 4.4);
    }

    const double slow = std::sin(m_sampleIndex * 0.021);
    const double fio2Effect = (m_fio2 - 21.0) / 79.0;
    m_ppeak = qRound(clampDouble(pressureTarget + 6.0 + slow * 2.2, 8, 58)); // REPLACE: read from pressure sensor
    m_pplat = qRound(clampDouble(pressureTarget + 1.5 + slow, 6, 45)); // REPLACE: read from pressure sensor (plateau hold)
    m_pmean = qRound(clampDouble(m_peep + m_pressureSupport * 0.45 + slow, 4, 35)); // REPLACE: compute mean from pressure samples
    m_spo2 = qRound(clampDouble(92.0 + fio2Effect * 8.0 - qMax(0, m_peep - 18) * 0.15 + std::sin(m_sampleIndex * 0.013), 84, 100)); // REPLACE: read from pulse oximeter module
    m_etco2 = qRound(clampDouble(31.0 + std::sin(m_sampleIndex * 0.018) * 3.0 - (m_minuteVolume - 100.0) * 0.025, 18, 55)); // REPLACE: read from capnograph sensor
    m_compliance = qRound(clampDouble(m_tidalVolume / qMax(1.0, m_pplat - m_peep) + std::sin(m_sampleIndex * 0.017) * 4.0, 12, 95)); // REPLACE: derive from real Vt/(Pplat-PEEP)
    m_resistance = qRound(clampDouble(8.0 + m_trigger * 0.8 + std::sin(m_sampleIndex * 0.029) * 2.0, 3, 28)); // REPLACE: derive from real (Ppeak-Pplat)/Flow

    // Derived respiratory mechanics (per Behance design metrics)
    m_vte = qRound(clampDouble(m_tidalVolume * (0.92 + std::sin(m_sampleIndex * 0.023) * 0.06), 50, 900)); // REPLACE: read from flow integration firmware
    m_ftotal = qRound(clampDouble(rr + std::sin(m_sampleIndex * 0.019) * 1.5, 4, 60)); // REPLACE: count from breath detection firmware
    m_rcexp = clampDouble(m_compliance * m_resistance / 1000.0 + std::sin(m_sampleIndex * 0.031) * 0.08, 0.1, 2.5); // REPLACE: derive from real compliance * resistance
    m_rcexp = std::round(m_rcexp * 100.0) / 100.0;
    m_expMinVol = clampDouble(m_vte * m_ftotal / 1000.0, 0.5, 30.0); // REPLACE: derive from real VTE * Ftotal
    m_expMinVol = std::round(m_expMinVol * 10.0) / 10.0;
    const double co2 = inspiration // REPLACE: read from CO2 capnograph
        ? qMax(0.0, m_etco2 * std::exp(-normalized * 6.0) - 2.0)
        : m_etco2 * (1.0 - std::exp(-normalized * 8.0)) + std::sin(m_sampleIndex * 0.08);

    if (!m_frozen) {
        appendSample(m_pressureWaveform, paw); // BIND: pressure sensor stream
        appendSample(m_flowWaveform, flow); // BIND: flow sensor stream
        appendSample(m_volumeWaveform, volume); // BIND: volume integration stream (mL)
        appendSample(m_co2Waveform, co2); // BIND: CO2 sensor stream
    }

    // ---------------------------------------------------------------
    // Clinical decision support metrics (simulated)
    // REPLACE: derive from real sensor data in production
    // ---------------------------------------------------------------

    // Work of breathing (J/L): area under P-V curve approximation
    // Normal 0.3-0.7 J/L; elevated in restrictive/obstructive disease
    m_workOfBreathing = clampDouble(
        0.45 + (m_resistance - 12.0) * 0.03
        + (30.0 - m_compliance) * 0.008
        + std::sin(m_sampleIndex * 0.027) * 0.08,
        0.15, 2.5);
    m_workOfBreathing = std::round(m_workOfBreathing * 100.0) / 100.0;

    // Stress index: curvature of pressure-time curve during constant flow
    // 1.0 = linear (ideal), <1.0 = tidal recruitment, >1.0 = overdistension
    m_stressIndex = clampDouble(
        1.0 + (m_ppeak - 30.0) * 0.02
        + std::sin(m_sampleIndex * 0.019) * 0.05,
        0.6, 1.8);
    m_stressIndex = std::round(m_stressIndex * 100.0) / 100.0;

    // Dead space fraction (Vd/Vt): Bohr-Enghoff equation approximation
    // Normal 0.2-0.35; elevated in PE, ARDS, low cardiac output
    m_deadSpaceFraction = clampDouble(
        0.28 + (50.0 - m_etco2) * 0.004
        + std::sin(m_sampleIndex * 0.015) * 0.02,
        0.10, 0.80);
    m_deadSpaceFraction = std::round(m_deadSpaceFraction * 100.0) / 100.0;

    // O2 toxicity timer: count minutes with FiO2 > 60%
    // Risk of absorption atelectasis and pulmonary O2 toxicity
    if (m_fio2 > 60) {
        ++m_highFio2SampleCounter;
        // ~22 samples per second at 45ms interval, 60s = ~1333 samples
        if (m_highFio2SampleCounter >= 1333) {
            m_highFio2SampleCounter = 0;
            ++m_highFio2Minutes;
        }
    } else {
        m_highFio2SampleCounter = 0;
    }

    // Patient disconnect simulation: detect from near-zero waveform amplitude
    // REPLACE: in production, compare measured vs expected flow patterns
    m_patientDisconnected = (m_ppeak < 3.0 && m_running && m_sampleIndex > 100);

    // Circuit occlusion: abnormally high pressure with near-zero flow
    m_circuitOcclusion = (m_ppeak > 55.0 && std::abs(flow) < 2.0 && m_running);

    // -----------------------------------------------------------------------
    // Populate the breath sample the mechanics engine consumes.
    //
    // In production these come from the flow and pressure channels on the
    // controller board; here they are synthesised consistently with the
    // waveform above so that the derived values behave the way the real ones
    // will - including their invalidity when a manoeuvre has not been run.
    // -----------------------------------------------------------------------

    // Track the flow extremes and the inspiratory pressure trajectory across
    // the breath, resetting at each cycle boundary.
    if (m_phase < m_breathPhase) {
        m_peakInspFlow = m_breathPeakFlow;
        m_peakExpFlow = std::abs(m_breathMinFlow);
        m_breathPeakFlow = 0.0;
        m_breathMinFlow = 0.0;
        m_inspiratoryPressures.clear();
    }
    m_breathPhase = m_phase;
    m_breathPeakFlow = std::max(m_breathPeakFlow, flow);
    m_breathMinFlow = std::min(m_breathMinFlow, flow);
    if (inspiration && m_inspiratoryPressures.size() < 512)
        m_inspiratoryPressures.append(paw);

    // Square flow only in volume control; every pressure-targeted mode
    // decelerates, which is what makes inspiratory resistance and the stress
    // index invalid there.
    m_squareFlow = (m_mode == QStringLiteral("VCV"));

    // Spontaneous modes have patient effort by definition, which invalidates
    // plateau-derived measurements.
    const sv::domain::ModeDefinition &activeMode =
        sv::domain::ModeCatalog::findOrDefault(m_mode);
    m_passivePatient =
        activeMode.controlVariable != sv::domain::ControlVariable::Spontaneous
        && activeMode.sequence != sv::domain::BreathSequence::ContinuousSpontaneous;

    m_spontaneousRate = m_passivePatient
        ? 0.0
        : clampDouble(m_ftotal * 0.8 + std::sin(m_sampleIndex * 0.009) * 1.5, 0, 60);

    // Inspired volume runs slightly above expired: a small circuit leak is the
    // normal state, and a UI that shows VTi == VTe hides the one signal that
    // reveals a growing leak.
    const double leakFraction = activeMode.nonInvasive ? 0.22 : 0.035;
    m_vti = m_vte * (1.0 + leakFraction
                     + std::sin(m_sampleIndex * 0.011) * leakFraction * 0.25);

    // Total PEEP exceeds set PEEP when expiratory time is short relative to
    // the time constant - the mechanism behind gas trapping.
    const double expiratoryTime = qMax(0.1, 60.0 / qMax(1.0, m_ftotal)
                                            - qMax(0.3, double(m_inspiratoryTime)));
    const double trappingRatio = m_rcexp > 0.0 ? expiratoryTime / (3.0 * m_rcexp) : 9.0;
    m_totalPeep = m_peep + (trappingRatio < 1.0 ? (1.0 - trappingRatio) * 6.0 : 0.0);

    m_breath.peakPressure = m_ppeak;
    m_breath.plateauPressure = m_pplat;
    m_breath.meanPressure = m_pmean;
    m_breath.setPeep = m_peep;
    m_breath.totalPeep = m_totalPeep;
    m_breath.inspiredVolume = m_vti;
    m_breath.expiredVolume = m_vte;
    m_breath.peakInspiratoryFlow = m_peakInspFlow;
    m_breath.peakExpiratoryFlow = m_peakExpFlow;
    m_breath.respiratoryRate = m_ftotal;
    m_breath.spontaneousRate = m_spontaneousRate;
    m_breath.inspiratoryTime = m_inspiratoryTime;
    m_breath.expiratoryTime = expiratoryTime;
    m_breath.squareFlow = m_squareFlow;
    m_breath.passive = m_passivePatient;
    m_breath.plateauValid = m_plateauValid;
    m_breath.totalPeepValid = m_totalPeepValid;

    // Hold-derived values go stale. Rather than displaying a five-minute-old
    // plateau beside live numbers, the validity is withdrawn.
    const double holdAge = m_mechanics.inspiratoryHoldAgeSeconds();
    if (m_plateauValid && holdAge > sv::services::RespiratoryMechanics::kManoeuvreValiditySeconds)
        m_plateauValid = false;
    const double expHoldAge = m_mechanics.expiratoryHoldAgeSeconds();
    if (m_totalPeepValid && expHoldAge > sv::services::RespiratoryMechanics::kManoeuvreValiditySeconds)
        m_totalPeepValid = false;

    evaluateAlarms();
    saveSnapshotIfDue();
    emit measurementsChanged();
    if (!m_frozen)
        emit waveformChanged();
}

void VentilatorController::evaluateAlarms()
{
    // -----------------------------------------------------------------------
    // ALARM EVALUATION
    // Every condition is detected independently and raised or cleared on its
    // own. This used to stop at the first match and return, so a second
    // condition was never detected and the first one was never cleared once a
    // higher one appeared. IEC 60601-1-8 expects independent detection with
    // the annunciator arbitrating priority, which is what AlarmController
    // does with the set it is given.
    //
    // raiseCondition() is idempotent: it logs on first appearance and on
    // escalation only, so calling it every sample tick is safe.
    // -----------------------------------------------------------------------
    if (!m_alarmController)
        return;

    // Standby means no breath is being delivered, so every condition below
    // describes something that is not happening. Real ventilators make the
    // patient alarms inactive in standby for exactly this reason; leaving
    // them on puts a Low Tidal Volume on the banner of a device that is
    // deliberately not ventilating. Technical alarms - battery, gas supply,
    // device fault, backend loss - are raised elsewhere and are untouched.
    static const char *const patientConditions[] = {
        "vent.disconnect", "vent.occlusion", "vent.paw.high", "vent.paw.low",
        "vent.mv.high", "vent.mv.low", "vent.vte.low", "vent.vte.high",
        "vent.rate.high", "vent.rate.low", "vent.spo2.low",
        "vent.etco2.high", "vent.etco2.low", "vent.fio2.high", "vent.fio2.low",
        "vent.fio2.prolonged", "vent.driving.high", "vent.apnea"
    };

    if (!m_running) {
        for (const char *id : patientConditions)
            m_alarmController->clearCondition(QString::fromLatin1(id));
        m_apneaSeconds = 0;
        return;
    }

    struct Check {
        const char *id;
        bool present;
        const char *priority;
        const char *source;
        QString headline;
        QString detail;
    };

    const double driving = drivingPressure();

    const Check checks[] = {
        {"vent.disconnect", m_patientDisconnected, "Critical", "Circuit",
         tr("Patient Disconnect"), tr("No airway pressure - check the circuit and the patient")},

        {"vent.occlusion", m_circuitOcclusion, "Critical", "Circuit",
         tr("Circuit Occlusion"), tr("High pressure with no flow - check tubing and filters")},

        {"vent.paw.high", m_ppeak > m_alarmHighPressure, "Critical", "Pressure",
         tr("High Pressure"),
         tr("Ppeak %1 cmH2O above the %2 cmH2O limit")
             .arg(qRound(m_ppeak)).arg(m_alarmHighPressure)},

        {"vent.mv.high", m_expMinVol > m_alarmHighMv, "Critical", "Volume",
         tr("High Minute Volume"),
         tr("%1 L/min above the %2 L/min limit")
             .arg(QString::number(m_expMinVol - m_alarmHighMv, 'f', 1)).arg(m_alarmHighMv)},

        {"vent.vte.low", m_vte > 0 && m_vte < m_alarmLowVt, "Critical", "Volume",
         tr("Low Tidal Volume"),
         tr("VTE %1 mL below the %2 mL limit").arg(qRound(m_vte)).arg(m_alarmLowVt)},

        {"vent.apnea", m_apneaSeconds > m_alarmApneaTime, "Critical", "Rate",
         tr("Apnea"),
         tr("No breath for %1 s, limit %2 s")
             .arg(m_apneaSeconds).arg(m_alarmApneaTime)},

        {"vent.paw.low", m_ppeak > 0 && m_ppeak < m_alarmLowPressure, "Warning", "Pressure",
         tr("Low Pressure"),
         tr("Ppeak %1 cmH2O below the %2 cmH2O limit")
             .arg(qRound(m_ppeak)).arg(m_alarmLowPressure)},

        {"vent.mv.low", m_ftotal > 0 && m_expMinVol < m_alarmLowMv, "Critical", "Volume",
         tr("Low Minute Volume"),
         tr("%1 L/min below the %2 L/min limit")
             .arg(QString::number(m_expMinVol, 'f', 1)).arg(m_alarmLowMv)},

        {"vent.vte.high", m_vte > m_alarmHighVt, "Warning", "Volume",
         tr("High Tidal Volume"),
         tr("VTE %1 mL above the %2 mL limit").arg(qRound(m_vte)).arg(m_alarmHighVt)},

        {"vent.rate.high", m_ftotal > m_alarmHighRate, "Warning", "Rate",
         tr("High Respiratory Rate"),
         tr("%1 b/min above the %2 b/min limit")
             .arg(qRound(m_ftotal)).arg(m_alarmHighRate)},

        {"vent.rate.low", m_ftotal > 0 && m_ftotal < m_alarmLowRate, "Warning", "Rate",
         tr("Low Respiratory Rate"),
         tr("%1 b/min below the %2 b/min limit")
             .arg(qRound(m_ftotal)).arg(m_alarmLowRate)},

        {"vent.spo2.low", m_spo2Monitored && m_spo2 > 0 && m_spo2 < m_alarmLowSpo2,
         "Warning", "Oximetry",
         tr("Low SpO2"),
         tr("SpO2 %1 percent below the %2 percent limit")
             .arg(qRound(m_spo2)).arg(m_alarmLowSpo2)},

        {"vent.etco2.high", m_etco2 > m_alarmHighEtco2, "Warning", "Capnography",
         tr("High EtCO2"),
         tr("End tidal carbon dioxide %1 mmHg above the %2 mmHg limit")
             .arg(qRound(m_etco2)).arg(m_alarmHighEtco2)},

        {"vent.etco2.low", m_ftotal > 0 && m_etco2 > 0 && m_etco2 < m_alarmLowEtco2,
         "Warning", "Capnography",
         tr("Low EtCO2"),
         tr("End tidal carbon dioxide %1 mmHg below the %2 mmHg limit")
             .arg(qRound(m_etco2)).arg(m_alarmLowEtco2)},

        {"vent.fio2.high", m_measuredFio2 > m_alarmHighFio2, "Warning", "Oxygen",
         tr("High Oxygen"),
         tr("Delivered %1 percent above the %2 percent limit")
             .arg(qRound(m_measuredFio2)).arg(m_alarmHighFio2)},

        {"vent.fio2.low", m_measuredFio2 > 0 && m_measuredFio2 < m_alarmLowFio2,
         "Critical", "Oxygen",
         tr("Low Oxygen"),
         tr("Delivered %1 percent below the %2 percent limit")
             .arg(qRound(m_measuredFio2)).arg(m_alarmLowFio2)},

        {"vent.fio2.prolonged", m_highFio2Minutes > 120 && m_fio2 > 60, "Warning", "Oxygen",
         tr("Prolonged High Oxygen"),
         tr("Above 60 percent for %1 minutes - consider weaning").arg(m_highFio2Minutes)},

        {"vent.driving.high", driving > 15.0, "Warning", "Pressure",
         tr("High Driving Pressure"),
         tr("%1 cmH2O, target below 15 - reduce tidal volume or raise PEEP")
             .arg(qRound(driving))}
    };

    for (const Check &check : checks) {
        const QString id = QString::fromLatin1(check.id);
        if (check.present) {
            m_alarmController->raiseCondition(id, QString::fromLatin1(check.priority),
                                              QString::fromLatin1(check.source),
                                              check.headline, check.detail, true);
        } else {
            m_alarmController->clearCondition(id);
        }
    }
}

QVariantMap VentilatorController::snapshot() const
{
    return {
        {QStringLiteral("mode"), m_mode},
        {QStringLiteral("fio2"), m_fio2},
        {QStringLiteral("peep"), m_peep},
        {QStringLiteral("pressureSupport"), m_pressureSupport},
        {QStringLiteral("respiratoryRate"), m_respiratoryRate},
        {QStringLiteral("minuteVolume"), m_minuteVolume},
        {QStringLiteral("tidalVolume"), m_tidalVolume},
        {QStringLiteral("ppeak"), m_ppeak},
        {QStringLiteral("pplat"), m_pplat},
        {QStringLiteral("pmean"), m_pmean},
        {QStringLiteral("spo2"), m_spo2},
        {QStringLiteral("etco2"), m_etco2},
        {QStringLiteral("compliance"), m_compliance},
        {QStringLiteral("resistance"), m_resistance}
    };
}

void VentilatorController::saveSnapshotIfDue()
{
    if (!m_database)
        return;
    ++m_snapshotCounter;
    if (m_snapshotCounter < 25)
        return;
    m_snapshotCounter = 0;
    m_database->saveParameterSnapshot(snapshot());
}
