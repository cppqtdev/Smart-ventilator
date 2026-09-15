#pragma once

#include <sv/services/RespiratoryMechanics.h>

#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class AlarmController;
class DatabaseManager;

/**
 * @brief Simulates a real ventilator hardware data stream for the demo UI.
 *
 * VentilatorController exposes setpoints, measurements, rolling waveform
 * buffers, and operator actions to QML. The class intentionally mirrors a
 * future hardware integration boundary: in production, the simulation loop can
 * be replaced with a serial/CAN/Ethernet device adapter while keeping the QML
 * contract stable.
 */
class VentilatorController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool frozen READ frozen NOTIFY frozenChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY settingsChanged)
    Q_PROPERTY(int fio2 READ fio2 WRITE setFio2 NOTIFY settingsChanged)
    Q_PROPERTY(int peep READ peep WRITE setPeep NOTIFY settingsChanged)
    Q_PROPERTY(int pressureSupport READ pressureSupport WRITE setPressureSupport NOTIFY settingsChanged)
    Q_PROPERTY(int inspiratoryTime READ inspiratoryTime WRITE setInspiratoryTime NOTIFY settingsChanged)
    Q_PROPERTY(int respiratoryRate READ respiratoryRate WRITE setRespiratoryRate NOTIFY settingsChanged)
    Q_PROPERTY(int trigger READ trigger WRITE setTrigger NOTIFY settingsChanged)
    Q_PROPERTY(int minuteVolume READ minuteVolume WRITE setMinuteVolume NOTIFY settingsChanged)
    Q_PROPERTY(int tidalVolume READ tidalVolume WRITE setTidalVolume NOTIFY settingsChanged)
    Q_PROPERTY(double ppeak READ ppeak NOTIFY measurementsChanged)
    Q_PROPERTY(double pplat READ pplat NOTIFY measurementsChanged)
    Q_PROPERTY(double pmean READ pmean NOTIFY measurementsChanged)
    Q_PROPERTY(double spo2 READ spo2 NOTIFY measurementsChanged)
    Q_PROPERTY(double etco2 READ etco2 NOTIFY measurementsChanged)
    Q_PROPERTY(double compliance READ compliance NOTIFY measurementsChanged)
    Q_PROPERTY(double resistance READ resistance NOTIFY measurementsChanged)
    Q_PROPERTY(double vte READ vte NOTIFY measurementsChanged)
    Q_PROPERTY(double ftotal READ ftotal NOTIFY measurementsChanged)
    Q_PROPERTY(double rcexp READ rcexp NOTIFY measurementsChanged)
    Q_PROPERTY(double expMinVol READ expMinVol NOTIFY measurementsChanged)
    Q_PROPERTY(double drivingPressure READ drivingPressure NOTIFY measurementsChanged)
    Q_PROPERTY(QString ieRatio READ ieRatio NOTIFY measurementsChanged)
    Q_PROPERTY(QString ventilationTime READ ventilationTime NOTIFY measurementsChanged)
    Q_PROPERTY(QString lastCommandMessage READ lastCommandMessage NOTIFY commandMessageChanged)
    Q_PROPERTY(QString operatorId READ operatorId WRITE setOperatorId NOTIFY operatorChanged)
    Q_PROPERTY(bool patientAccepted READ patientAccepted NOTIFY patientContextChanged)
    Q_PROPERTY(bool preUseCheckPassed READ preUseCheckPassed NOTIFY readinessChanged)
    Q_PROPERTY(bool readyToVentilate READ readyToVentilate NOTIFY readinessChanged)
    Q_PROPERTY(QString readinessReason READ readinessReason NOTIFY readinessChanged)
    Q_PROPERTY(QString patientCategory READ patientCategory WRITE setPatientContext NOTIFY patientContextChanged)
    Q_PROPERTY(int patientIbwKg READ patientIbwKg WRITE setPatientIbwKg NOTIFY patientContextChanged)
    Q_PROPERTY(bool backendConnected READ backendConnected NOTIFY backendStateChanged)
    Q_PROPERTY(bool degradedMode READ degradedMode NOTIFY backendStateChanged)
    Q_PROPERTY(QString backendState READ backendState NOTIFY backendStateChanged)

    // Clinical decision support metrics (simulated)
    Q_PROPERTY(double workOfBreathing READ workOfBreathing NOTIFY measurementsChanged)
    Q_PROPERTY(double stressIndex READ stressIndex NOTIFY measurementsChanged)
    Q_PROPERTY(double deadSpaceFraction READ deadSpaceFraction NOTIFY measurementsChanged)
    Q_PROPERTY(int highFio2Minutes READ highFio2Minutes NOTIFY measurementsChanged)
    Q_PROPERTY(bool patientDisconnected READ patientDisconnected NOTIFY measurementsChanged)
    Q_PROPERTY(bool circuitOcclusion READ circuitOcclusion NOTIFY measurementsChanged)

    Q_PROPERTY(int alarmHighPressure READ alarmHighPressure WRITE setAlarmHighPressure NOTIFY settingsChanged)
    Q_PROPERTY(int alarmLowPressure READ alarmLowPressure WRITE setAlarmLowPressure NOTIFY settingsChanged)
    Q_PROPERTY(int alarmApneaTime READ alarmApneaTime WRITE setAlarmApneaTime NOTIFY settingsChanged)
    Q_PROPERTY(int apneaSeconds READ apneaSeconds NOTIFY measurementsChanged)
    Q_PROPERTY(int alarmLowVt READ alarmLowVt WRITE setAlarmLowVt NOTIFY settingsChanged)
    Q_PROPERTY(int alarmHighMv READ alarmHighMv WRITE setAlarmHighMv NOTIFY settingsChanged)
    Q_PROPERTY(int alarmLowSpo2 READ alarmLowSpo2 WRITE setAlarmLowSpo2 NOTIFY settingsChanged)
    Q_PROPERTY(int alarmLowMv READ alarmLowMv WRITE setAlarmLowMv NOTIFY settingsChanged)
    Q_PROPERTY(int alarmHighVt READ alarmHighVt WRITE setAlarmHighVt NOTIFY settingsChanged)
    Q_PROPERTY(int alarmHighRate READ alarmHighRate WRITE setAlarmHighRate NOTIFY settingsChanged)
    Q_PROPERTY(int alarmLowRate READ alarmLowRate WRITE setAlarmLowRate NOTIFY settingsChanged)
    Q_PROPERTY(int alarmHighFio2 READ alarmHighFio2 WRITE setAlarmHighFio2 NOTIFY settingsChanged)
    Q_PROPERTY(int alarmLowFio2 READ alarmLowFio2 WRITE setAlarmLowFio2 NOTIFY settingsChanged)
    Q_PROPERTY(int alarmHighEtco2 READ alarmHighEtco2 WRITE setAlarmHighEtco2 NOTIFY settingsChanged)
    Q_PROPERTY(int alarmLowEtco2 READ alarmLowEtco2 WRITE setAlarmLowEtco2 NOTIFY settingsChanged)
    Q_PROPERTY(bool spo2Monitored READ spo2Monitored WRITE setSpo2Monitored NOTIFY settingsChanged)
    Q_PROPERTY(bool apneaBackupEnabled READ apneaBackupEnabled WRITE setApneaBackupEnabled NOTIFY settingsChanged)
    Q_PROPERTY(int ventilationSeconds READ ventilationSeconds NOTIFY measurementsChanged)
    Q_PROPERTY(QVariantList pressureWaveform READ pressureWaveform NOTIFY waveformChanged)
    Q_PROPERTY(QVariantList flowWaveform READ flowWaveform NOTIFY waveformChanged)
    Q_PROPERTY(QVariantList volumeWaveform READ volumeWaveform NOTIFY waveformChanged)
    Q_PROPERTY(QVariantList co2Waveform READ co2Waveform NOTIFY waveformChanged)

    // -- Mode identity, read from the declarative mode catalogue ----------
    Q_PROPERTY(QString modeDescription READ modeDescription NOTIFY settingsChanged)
    Q_PROPERTY(bool nonInvasive READ nonInvasive NOTIFY settingsChanged)

    // -- Measured inputs the mechanics engine needs -----------------------
    Q_PROPERTY(double vti READ vti NOTIFY measurementsChanged)
    Q_PROPERTY(double totalPeep READ totalPeep NOTIFY measurementsChanged)
    Q_PROPERTY(double peakInspiratoryFlow READ peakInspiratoryFlow NOTIFY measurementsChanged)
    Q_PROPERTY(double peakExpiratoryFlow READ peakExpiratoryFlow NOTIFY measurementsChanged)
    Q_PROPERTY(double spontaneousRate READ spontaneousRate NOTIFY measurementsChanged)
    Q_PROPERTY(bool squareFlow READ squareFlow NOTIFY measurementsChanged)
    Q_PROPERTY(bool passivePatient READ passivePatient NOTIFY measurementsChanged)
    Q_PROPERTY(bool plateauValid READ plateauValid NOTIFY measurementsChanged)
    Q_PROPERTY(bool totalPeepValid READ totalPeepValid NOTIFY measurementsChanged)

    /// True while a hold manoeuvre is running. A property, not just an
    /// invokable, so the manoeuvre buttons actually disable during one.
    Q_PROPERTY(double measuredFio2 READ measuredFio2 NOTIFY measurementsChanged)
    Q_PROPERTY(double leakPercent READ leakPercent NOTIFY measurementsChanged)
    Q_PROPERTY(int devicePowerPercent READ devicePowerPercent NOTIFY measurementsChanged)
    Q_PROPERTY(quint32 deviceFaults READ deviceFaults NOTIFY measurementsChanged)
    Q_PROPERTY(bool holdInProgress READ holdInProgress NOTIFY measurementsChanged)

    Q_PROPERTY(bool oxygenBoostActive READ oxygenBoostActive NOTIFY manoeuvreChanged)
    Q_PROPERTY(int oxygenBoostRemaining READ oxygenBoostRemaining NOTIFY manoeuvreChanged)
    Q_PROPERTY(bool nebuliserActive READ nebuliserActive NOTIFY manoeuvreChanged)
    Q_PROPERTY(int nebuliserRemaining READ nebuliserRemaining NOTIFY manoeuvreChanged)

    /**
     * Every derived respiratory-mechanics quantity, keyed by name. Each entry
     * is a map of { value, valid, reason, manoeuvre, age } - the validity
     * travels with the number so a screen cannot display an invalid
     * measurement by forgetting to check a separate flag.
     */
    Q_PROPERTY(QVariantMap mechanics READ mechanics NOTIFY measurementsChanged)

public:
    /**
     * @param database Pointer to the application database manager.
     * @param alarmController Pointer to the alarm controller for threshold evaluation.
     * @param parent Optional parent QObject for ownership.
     */
    explicit VentilatorController(DatabaseManager *database,
                                  AlarmController *alarmController,
                                  QObject *parent = nullptr);

    /** @return True if the ventilator simulation is actively running. */
    bool running() const;
    /** @return True if waveform display is frozen. */
    bool frozen() const;
    /** @return Current ventilation mode identifier (e.g. "ASV", "PCV"). */
    QString mode() const;
    /** @return Fraction of inspired oxygen setpoint in percent. */
    int fio2() const;
    /** @return Positive end-expiratory pressure setpoint in cmH2O. */
    int peep() const;
    /** @return Pressure support setpoint in cmH2O. */
    int pressureSupport() const;
    /** @return Inspiratory time setpoint in seconds. */
    int inspiratoryTime() const;
    /** @return Respiratory rate setpoint in breaths per minute. */
    int respiratoryRate() const;
    /** @return Flow trigger sensitivity setpoint in L/min. */
    int trigger() const;
    /** @return Minute volume setpoint in mL/min. */
    int minuteVolume() const;
    /** @return Tidal volume setpoint in mL. */
    int tidalVolume() const;
    /** @return Measured peak airway pressure in cmH2O. */
    double ppeak() const;
    /** @return Measured plateau airway pressure in cmH2O. */
    double pplat() const;
    /** @return Measured mean airway pressure in cmH2O. */
    double pmean() const;
    /** @return Measured peripheral oxygen saturation in percent. */
    double spo2() const;
    /** @return Measured end-tidal CO2 in mmHg. */
    double etco2() const;
    /** @return Measured lung compliance in mL/cmH2O. */
    double compliance() const;
    /** @return Measured airway resistance in cmH2O/(L/s). */
    double resistance() const;
    /** @return Measured expired tidal volume in mL. */
    double vte() const;
    /** @return Total respiratory frequency in breaths/min. */
    double ftotal() const;
    /** @return Expiratory time constant in seconds. */
    double rcexp() const;
    /** @return Measured expired minute volume in L/min. */
    double expMinVol() const;
    /** @return Driving pressure in cmH2O (Pplat - PEEP). Key lung protection metric. */
    double drivingPressure() const;
    /** @return Inspiratory:Expiratory ratio as formatted string (e.g. "1:2.0"). */
    QString ieRatio() const;
    /** @return Formatted ventilation elapsed time as HH:MM:SS. */
    QString ventilationTime() const;
    /** @return Last accepted/rejected operator command status. */
    QString lastCommandMessage() const;
    QString operatorId() const;
    QString patientCategory() const;
    int patientIbwKg() const;
    bool backendConnected() const;
    bool degradedMode() const;
    QString backendState() const;

    /** @return Simulated work of breathing in J/L. Normal 0.3-0.7. */
    double workOfBreathing() const;
    /** @return Stress index (1.0 = linear, <1 = recruitment, >1 = overdistension). */
    double stressIndex() const;
    /** @return Dead space fraction (Vd/Vt). Normal 0.2-0.35. */
    double deadSpaceFraction() const;
    /** @return Minutes spent with FiO2 above 60%. O2 toxicity risk. */
    int highFio2Minutes() const;
    /** @return True if waveforms suggest patient circuit disconnection. */
    bool patientDisconnected() const;
    /** @return True if pressure pattern suggests circuit occlusion. */
    bool circuitOcclusion() const;

    /** @return Ventilation elapsed time in seconds. */
    int ventilationSeconds() const;

    int alarmHighPressure() const;
    int alarmLowPressure() const;
    int alarmApneaTime() const;

    /** @return Seconds since the last detected breath, 0 while breathing. */
    int apneaSeconds() const;
    int alarmLowVt() const;
    int alarmHighMv() const;
    int alarmLowSpo2() const;
    int alarmLowMv() const;
    int alarmHighVt() const;
    int alarmHighRate() const;
    int alarmLowRate() const;
    int alarmHighFio2() const;
    int alarmLowFio2() const;
    int alarmHighEtco2() const;
    int alarmLowEtco2() const;

    /** False when the oximeter is not in use, which takes its alarms out. */
    bool spo2Monitored() const;
    bool apneaBackupEnabled() const;

    /** @return Rolling pressure waveform sample buffer. */
    QVariantList pressureWaveform() const;
    /** @return Rolling flow waveform sample buffer. */
    QVariantList flowWaveform() const;
    /** @return Rolling volume waveform sample buffer. */
    QVariantList volumeWaveform() const;
    /** @return Rolling CO2 waveform sample buffer. */
    QVariantList co2Waveform() const;

    QString modeDescription() const;
    bool nonInvasive() const;

    double vti() const;
    double totalPeep() const;
    double peakInspiratoryFlow() const;
    double peakExpiratoryFlow() const;
    double spontaneousRate() const;
    bool squareFlow() const;
    bool passivePatient() const;
    bool plateauValid() const;
    bool totalPeepValid() const;

    /** @return All derived mechanics, each with its validity. */
    QVariantMap mechanics() const;

    /**
     * @brief Performs an inspiratory hold to measure plateau pressure.
     *
     * Static compliance, airway resistance and driving pressure all need a
     * plateau and are reported as unmeasurable until this runs.
     */
    Q_INVOKABLE void performInspiratoryHold(int milliseconds = 1500);

    /**
     * @brief Performs an expiratory hold to measure total PEEP.
     *
     * Auto-PEEP is the difference between the result and the set PEEP. The
     * measurement is invalid if the patient makes an effort during the hold.
     */
    Q_INVOKABLE void performExpiratoryHold(int milliseconds = 2000);

    /** @return True while a hold manoeuvre is in progress. */
    bool holdInProgress() const;

    /** @brief Starts the ventilator simulation loop. */
    Q_INVOKABLE void startVentilation();
    /** @brief Validates and starts ventilation through the safe command path. */
    Q_INVOKABLE bool requestStartVentilation();
    /** @brief Stops the ventilator simulation loop. */
    Q_INVOKABLE void stopVentilation();
    /** @brief Toggles waveform freeze on or off. */
    Q_INVOKABLE void toggleFreeze();
    /** @brief Runs a simulated sensor calibration sequence. */
    Q_INVOKABLE void runCalibration();
    /**
     * @brief Validated operator command for changing a ventilator parameter.
     * @param parameter Stable parameter id such as "fio2", "peep", or "tidalVolume".
     * @param value Requested integer value.
     * @return True if accepted and applied to the simulator state.
     */
    Q_INVOKABLE bool requestParameterChange(const QString &parameter, int value);
    /** @brief Validated operator command for changing an alarm limit. */
    Q_INVOKABLE bool requestAlarmLimitChange(const QString &limit, int value);
    /** @brief Validated operator command for changing ventilation mode. */
    Q_INVOKABLE bool requestModeChange(const QString &mode);

    /** @return True when this build can actually deliver the mode. */
    Q_INVOKABLE bool isModeSupported(const QString &mode) const;

    bool patientAccepted() const;
    bool preUseCheckPassed() const;
    bool readyToVentilate() const;
    QString readinessReason() const;

    /**
     * @brief Records that a patient has been admitted at this bedside.
     *
     * Ventilation cannot start before this. Real devices ask the same
     * question, because the patient category decides every safe range and a
     * carried-over category from the previous patient is a hazard.
     */
    Q_INVOKABLE void acceptPatient(const QString &category, int ibwKg);

    /** @brief Clears the admitted patient, which returns the device to standby. */
    Q_INVOKABLE void dischargePatient();

    /** @brief Records the outcome of the pre-use check. */
    Q_INVOKABLE void setPreUseCheckPassed(bool passed);

    /**
     * @brief Starts without a passing pre-use check, for an emergency.
     *
     * The reason is mandatory and is written to the audit trail, because a
     * device that can be started unchecked silently is a device that is
     * always started unchecked.
     */
    Q_INVOKABLE bool overridePreUseCheck(const QString &reason);
    /**
     * @brief The editable range of one alarm limit.
     * @return Keys value, minimum, maximum, label and unit; empty if the
     *         limit is not one this controller knows.
     */
    Q_INVOKABLE QVariantMap alarmLimitRange(const QString &limit);

    /** @brief Validated operator command for enabling/disabling apnea backup. */
    Q_INVOKABLE bool requestApneaBackupChange(bool enabled);

    /** @return Oxygen the device reports delivering, not the setpoint. */
    double measuredFio2() const;

    /** @return Circuit leak as a percentage of delivered volume. */
    double leakPercent() const;

    /** @return Battery the device reports, or -1 when it reports none. */
    int devicePowerPercent() const;

    /** @return The device fault bit field from frame 0x124. */
    quint32 deviceFaults() const;

    bool oxygenBoostActive() const;
    int oxygenBoostRemaining() const;
    bool nebuliserActive() const;
    int nebuliserRemaining() const;

    /**
     * @brief Delivers one mandatory breath immediately.
     *
     * Refused while a hold manoeuvre is running, because the valves are shut.
     */
    Q_INVOKABLE bool deliverManualBreath();

    /**
     * @brief Raises FiO2 to 100 percent for a fixed interval.
     *
     * The pre-suction oxygenation control. The previous setting is restored
     * when the interval ends, so a forgotten boost cannot leave the patient
     * on pure oxygen.
     */
    Q_INVOKABLE bool startOxygenBoost(int seconds = 120);

    /** @brief Ends the oxygen boost early and restores the FiO2 setting. */
    Q_INVOKABLE void cancelOxygenBoost();

    /** @brief Runs the nebuliser for a fixed number of minutes. */
    Q_INVOKABLE bool startNebuliser(int minutes = 10);

    /** @brief Stops the nebuliser early. */
    Q_INVOKABLE void cancelNebuliser();

    /** @brief Simulated future hardware heartbeat. */
    Q_INVOKABLE void recordHardwareHeartbeat();
    /** @brief Simulates hardware link loss/recovery for demo and tests. */
    /**
     * @brief Sets the patient category and ideal body weight in one step.
     *
     * Setting them separately leaves the controller holding a category and a
     * weight that do not belong together for one call, and every range
     * derived from the pair is wrong for that moment.
     */
    Q_INVOKABLE void setPatientProfile(const QString &category, int ibwKg);

    /**
     * @brief Declares that telemetry comes from real hardware.
     *
     * The heartbeat watchdog only runs when it does. On the internal
     * simulator the application would be watching its own pulse, and would
     * raise a disconnect alarm against itself.
     */
    Q_INVOKABLE void setHardwareBackend(bool hardware);

    /**
     * @brief Applies one decoded frame from the device.
     *
     * Keys are the signal names in sv/transport/ITelemetrySource.h. Only the
     * keys present are written, because each frame carries its own subset.
     * While a hardware backend is attached these values replace the internal
     * model rather than being blended with it: two sources for one number is
     * how a display comes to disagree with the device.
     */
    Q_INVOKABLE void applyTelemetry(const QVariantMap &values);

    /**
     * @brief Follows the device into or out of ventilation.
     *
     * The device is a separate processor and keeps ventilating when this
     * interface restarts. On reattaching it must adopt what the device is
     * already doing, never command it to match a freshly initialised
     * interface: a restarted screen must not stop a running therapy.
     */
    void adoptDeviceState(bool deviceVentilating);

    /**
     * @brief Reloads the bedside session written by the previous run.
     *
     * Covers the case where this interface restarts and there is no device
     * link to ask: the admitted patient, the category and the pre-use check
     * come back, so a crash does not present the operator with an empty
     * standby screen and a patient still on the circuit. A device link, when
     * there is one, overrides this through adoptDeviceState().
     */
    Q_INVOKABLE void restoreSession();

    Q_INVOKABLE void setBackendConnected(bool connected);

public slots:
    /** @param value Ventilation mode identifier to apply. */
    void setMode(const QString &value);
    /** @param value FiO2 setpoint in percent. */
    void setFio2(int value);
    /** @param value PEEP setpoint in cmH2O. */
    void setPeep(int value);
    /** @param value Pressure support setpoint in cmH2O. */
    void setPressureSupport(int value);
    /** @param value Inspiratory time setpoint in seconds. */
    void setInspiratoryTime(int value);
    /** @param value Respiratory rate setpoint in breaths per minute. */
    void setRespiratoryRate(int value);
    /** @param value Flow trigger sensitivity in L/min. */
    void setTrigger(int value);
    /** @param value Minute volume setpoint in mL/min. */
    void setMinuteVolume(int value);
    /** @param value Tidal volume setpoint in mL. */
    void setTidalVolume(int value);

    void setAlarmHighPressure(int value);
    void setAlarmLowPressure(int value);
    void setAlarmApneaTime(int value);
    void setAlarmLowVt(int value);
    void setAlarmHighMv(int value);
    void setAlarmLowSpo2(int value);
    void setAlarmLowMv(int value);
    void setAlarmHighVt(int value);
    void setAlarmHighRate(int value);
    void setAlarmLowRate(int value);
    void setAlarmHighFio2(int value);
    void setAlarmLowFio2(int value);
    void setAlarmHighEtco2(int value);
    void setAlarmLowEtco2(int value);
    void setSpo2Monitored(bool value);
    void setApneaBackupEnabled(bool value);
    void setOperatorId(const QString &operatorId);
    void setPatientContext(const QString &category);
    void setPatientIbwKg(int ibwKg);

signals:
    void runningChanged();
    void frozenChanged();
    void settingsChanged();
    void measurementsChanged();
    void manoeuvreChanged();
    void waveformChanged();
    void commandMessageChanged();
    void commandRejected(const QString &message);
    void operatorChanged();
    void patientContextChanged();
    void readinessChanged();
    void backendStateChanged();

private slots:
    void updateSimulation();
    /// Pulls tidal volume and rate into the current category's safe envelope.
    /// Selecting a neonate must not leave adult settings loaded.
    void reseedForPatientCategory();

    void checkBackendHeartbeat();

private:
    void appendSample(QVariantList &buffer, double value);
    void evaluateAlarms();
    void saveSnapshotIfDue();
    QVariantMap snapshot() const;
    bool applyParameterChange(const QString &parameter, int value, bool audited);
    bool applyAlarmLimitChange(const QString &limit, int value, bool audited);

    /**
     * @brief The one place a limit's bounds are written down.
     * @return Pointer to the member the limit stores itself in, or nullptr
     *         when the name is not a limit. The out parameters carry the
     *         bounds, which depend on the neighbouring limits.
     */
    int *alarmLimitTarget(const QString &limit, int *low, int *high,
                          QString *label, QString *unit);
    bool validateMode(const QString &mode, QString *reason) const;
    bool validateStart(QString *reason) const;
    bool validateSettingEnvelope(const QString &parameter, int value, QString *reason) const;
    int categoryMinVt() const;
    int categoryMaxVt() const;
    int categoryMinRr() const;
    int categoryMaxRr() const;

    /// Raw ceilings before they are reconciled with the floor.
    int categoryCeilingVt() const;
    int categoryCeilingRr() const;
    void setCommandMessage(const QString &message);
    void setDegradedMode(bool degraded, const QString &state);
    void logSettingChange(const QString &parameter, const QVariant &oldValue, const QVariant &newValue);
    void saveSession();
    void applyDeviceFaults(quint32 bits);

    DatabaseManager *m_database = nullptr;
    AlarmController *m_alarmController = nullptr;
    QTimer m_sampleTimer;
    bool m_running = false;
    bool m_frozen = false;
    bool m_backendConnected = true;
    bool m_hardwareBackend = false;
    bool m_degradedMode = false;
    QString m_mode = QStringLiteral("ASV");
    QString m_lastCommandMessage;
    QString m_operatorId = QStringLiteral("unauthenticated");
    QString m_patientCategory = QStringLiteral("Adult");
    bool m_patientAccepted = false;
    bool m_preUseCheckPassed = false;
    bool m_preUseCheckOverridden = false;
    QString m_backendState = QStringLiteral("Simulator connected");
    QDateTime m_lastHardwareHeartbeatUtc;
    int m_fio2 = 60;
    int m_patientIbwKg = 73;
    int m_peep = 15;
    int m_pressureSupport = 12;
    int m_inspiratoryTime = 1;
    int m_respiratoryRate = 20;
    int m_trigger = 3;
    int m_minuteVolume = 110;
    int m_tidalVolume = 420;
    double m_ppeak = 0;
    double m_pplat = 0;
    double m_pmean = 0;
    double m_spo2 = 0;
    double m_etco2 = 0;
    double m_compliance = 0;
    double m_resistance = 0;
    double m_vte = 0;
    double m_ftotal = 0;
    double m_rcexp = 0;
    double m_expMinVol = 0;
    double m_measuredFio2 = 0;
    double m_leakPercent = 0;
    int m_devicePowerPercent = -1;
    quint32 m_deviceFaults = 0;
    double m_workOfBreathing = 0;
    double m_stressIndex = 1.0;
    double m_deadSpaceFraction = 0.3;
    int m_highFio2Minutes = 0;
    bool m_patientDisconnected = false;
    bool m_circuitOcclusion = false;
    int m_highFio2SampleCounter = 0;
    int m_alarmHighPressure = 40;
    int m_alarmLowPressure = 5;
    int m_alarmApneaTime = 20;
    int m_apneaSeconds = 0;
    int m_alarmLowVt = 300;
    int m_alarmHighMv = 12;
    int m_alarmLowSpo2 = 90;
    int m_alarmLowMv = 4;
    int m_alarmHighVt = 800;
    int m_alarmHighRate = 40;
    int m_alarmLowRate = 5;
    int m_alarmHighFio2 = 60;
    int m_alarmLowFio2 = 21;
    int m_alarmHighEtco2 = 60;
    int m_alarmLowEtco2 = 30;
    bool m_spo2Monitored = true;
    bool m_apneaBackupEnabled = true;
    double m_phase = 0;
    int m_sampleIndex = 0;
    int m_snapshotCounter = 0;
    int m_ventilationSeconds = 0;
    QTimer m_ventilationTimer;
    QTimer m_backendWatchdogTimer;
    // Respiratory mechanics engine and the breath it last consumed.
    sv::services::RespiratoryMechanics m_mechanics;
    sv::services::BreathSample m_breath;
    QTimer m_holdTimer;
    bool m_holdInProgress = false;

    QTimer m_oxygenBoostTimer;
    QTimer m_nebuliserTimer;
    int m_fio2BeforeBoost = 0;
    bool m_holdIsInspiratory = false;
    double m_vti = 0;
    double m_totalPeep = 0;
    double m_peakInspFlow = 0;
    double m_peakExpFlow = 0;
    double m_spontaneousRate = 0;
    bool m_squareFlow = false;
    bool m_passivePatient = true;
    bool m_plateauValid = false;
    bool m_totalPeepValid = false;
    double m_breathPeakFlow = 0;
    double m_breathMinFlow = 0;
    double m_breathPhase = 0;
    QVector<double> m_inspiratoryPressures;

    QVariantList m_pressureWaveform;
    QVariantList m_flowWaveform;
    QVariantList m_volumeWaveform;
    QVariantList m_co2Waveform;
};
