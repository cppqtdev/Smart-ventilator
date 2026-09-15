// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/domain/Setpoint.h>
#include <sv/domain/ClinicalMetrics.h>
#include <sv/domain/Patient.h>

#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace sv::infrastructure {
class IVentilatorHardware;
class DatabaseManager;
}

namespace sv::services {
class SettingsValidator;
class WaveformEngine;
class BreathSimulator;
}

class AlarmController;

namespace sv::backend {

class VentilatorFacade : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_DISABLE_COPY_MOVE(VentilatorFacade)

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
    Q_PROPERTY(QString patientCategory READ patientCategory WRITE setPatientContext NOTIFY patientContextChanged)
    Q_PROPERTY(int patientIbwKg READ patientIbwKg WRITE setPatientIbwKg NOTIFY patientContextChanged)
    Q_PROPERTY(bool backendConnected READ backendConnected NOTIFY backendStateChanged)
    Q_PROPERTY(bool degradedMode READ degradedMode NOTIFY backendStateChanged)
    Q_PROPERTY(QString backendState READ backendState NOTIFY backendStateChanged)

    Q_PROPERTY(double workOfBreathing READ workOfBreathing NOTIFY measurementsChanged)
    Q_PROPERTY(double stressIndex READ stressIndex NOTIFY measurementsChanged)
    Q_PROPERTY(double deadSpaceFraction READ deadSpaceFraction NOTIFY measurementsChanged)
    Q_PROPERTY(int highFio2Minutes READ highFio2Minutes NOTIFY measurementsChanged)
    Q_PROPERTY(bool patientDisconnected READ patientDisconnected NOTIFY measurementsChanged)
    Q_PROPERTY(bool circuitOcclusion READ circuitOcclusion NOTIFY measurementsChanged)

    Q_PROPERTY(int alarmHighPressure READ alarmHighPressure WRITE setAlarmHighPressure NOTIFY settingsChanged)
    Q_PROPERTY(int alarmLowPressure READ alarmLowPressure WRITE setAlarmLowPressure NOTIFY settingsChanged)
    Q_PROPERTY(int alarmApneaTime READ alarmApneaTime WRITE setAlarmApneaTime NOTIFY settingsChanged)
    Q_PROPERTY(int alarmLowVt READ alarmLowVt WRITE setAlarmLowVt NOTIFY settingsChanged)
    Q_PROPERTY(int alarmHighMv READ alarmHighMv WRITE setAlarmHighMv NOTIFY settingsChanged)
    Q_PROPERTY(int alarmLowSpo2 READ alarmLowSpo2 WRITE setAlarmLowSpo2 NOTIFY settingsChanged)
    Q_PROPERTY(bool apneaBackupEnabled READ apneaBackupEnabled WRITE setApneaBackupEnabled NOTIFY settingsChanged)
    Q_PROPERTY(int ventilationSeconds READ ventilationSeconds NOTIFY measurementsChanged)
    Q_PROPERTY(QVariantList pressureWaveform READ pressureWaveform NOTIFY waveformChanged)
    Q_PROPERTY(QVariantList flowWaveform READ flowWaveform NOTIFY waveformChanged)
    Q_PROPERTY(QVariantList volumeWaveform READ volumeWaveform NOTIFY waveformChanged)
    Q_PROPERTY(QVariantList co2Waveform READ co2Waveform NOTIFY waveformChanged)

public:
    explicit VentilatorFacade(sv::infrastructure::DatabaseManager *database,
                              AlarmController *alarmController,
                              sv::infrastructure::IVentilatorHardware *hardware,
                              sv::services::SettingsValidator *validator,
                              sv::services::WaveformEngine *waveforms,
                              sv::services::BreathSimulator *breathSim,
                              QObject *parent = nullptr);

    bool running() const;
    bool frozen() const;
    QString mode() const;
    int fio2() const;
    int peep() const;
    int pressureSupport() const;
    int inspiratoryTime() const;
    int respiratoryRate() const;
    int trigger() const;
    int minuteVolume() const;
    int tidalVolume() const;
    double ppeak() const;
    double pplat() const;
    double pmean() const;
    double spo2() const;
    double etco2() const;
    double compliance() const;
    double resistance() const;
    double vte() const;
    double ftotal() const;
    double rcexp() const;
    double expMinVol() const;
    double drivingPressure() const;
    QString ieRatio() const;
    QString ventilationTime() const;
    QString lastCommandMessage() const;
    QString operatorId() const;
    QString patientCategory() const;
    int patientIbwKg() const;
    bool backendConnected() const;
    bool degradedMode() const;
    QString backendState() const;

    double workOfBreathing() const;
    double stressIndex() const;
    double deadSpaceFraction() const;
    int highFio2Minutes() const;
    bool patientDisconnected() const;
    bool circuitOcclusion() const;

    int ventilationSeconds() const;
    int alarmHighPressure() const;
    int alarmLowPressure() const;
    int alarmApneaTime() const;
    int alarmLowVt() const;
    int alarmHighMv() const;
    int alarmLowSpo2() const;
    bool apneaBackupEnabled() const;

    QVariantList pressureWaveform() const;
    QVariantList flowWaveform() const;
    QVariantList volumeWaveform() const;
    QVariantList co2Waveform() const;

    Q_INVOKABLE void startVentilation();
    Q_INVOKABLE bool requestStartVentilation();
    Q_INVOKABLE void stopVentilation();
    Q_INVOKABLE void toggleFreeze();
    Q_INVOKABLE void runCalibration();
    Q_INVOKABLE bool requestParameterChange(const QString &parameter, int value);
    Q_INVOKABLE bool requestAlarmLimitChange(const QString &limit, int value);
    Q_INVOKABLE bool requestModeChange(const QString &mode);
    Q_INVOKABLE bool requestApneaBackupChange(bool enabled);
    Q_INVOKABLE void recordHardwareHeartbeat();
    Q_INVOKABLE void setBackendConnected(bool connected);

public slots:
    void setMode(const QString &value);
    void setFio2(int value);
    void setPeep(int value);
    void setPressureSupport(int value);
    void setInspiratoryTime(int value);
    void setRespiratoryRate(int value);
    void setTrigger(int value);
    void setMinuteVolume(int value);
    void setTidalVolume(int value);
    void setAlarmHighPressure(int value);
    void setAlarmLowPressure(int value);
    void setAlarmApneaTime(int value);
    void setAlarmLowVt(int value);
    void setAlarmHighMv(int value);
    void setAlarmLowSpo2(int value);
    void setApneaBackupEnabled(bool value);
    void setOperatorId(const QString &operatorId);
    void setPatientContext(const QString &category);
    void setPatientIbwKg(int ibwKg);

signals:
    void runningChanged();
    void frozenChanged();
    void settingsChanged();
    void measurementsChanged();
    void waveformChanged();
    void commandMessageChanged();
    void commandRejected(const QString &message);
    void operatorChanged();
    void patientContextChanged();
    void backendStateChanged();

private slots:
    void updateSimulation();
    void checkBackendHeartbeat();

private:
    void evaluateAlarms();
    void saveSnapshotIfDue();
    QVariantMap snapshot() const;
    bool applyParameterChange(const QString &parameter, int value, bool audited);
    bool applyAlarmLimitChange(const QString &limit, int value, bool audited);
    void setCommandMessage(const QString &message);
    void setDegradedMode(bool degraded, const QString &state);
    void logSettingChange(const QString &parameter, const QVariant &oldValue, const QVariant &newValue);
    sv::domain::SetpointSet currentSetpoints() const;
    sv::domain::AlarmLimits currentAlarmLimits() const;
    sv::domain::Patient currentPatient() const;

    sv::infrastructure::DatabaseManager *m_database = nullptr;
    AlarmController *m_alarmController = nullptr;
    sv::infrastructure::IVentilatorHardware *m_hardware = nullptr;
    sv::services::SettingsValidator *m_validator = nullptr;
    sv::services::WaveformEngine *m_waveforms = nullptr;
    sv::services::BreathSimulator *m_breathSim = nullptr;

    QTimer m_sampleTimer;
    QTimer m_ventilationTimer;
    QTimer m_backendWatchdogTimer;
    QDateTime m_lastHardwareHeartbeatUtc;

    bool m_running = false;
    bool m_frozen = false;
    bool m_backendConnected = true;
    bool m_degradedMode = false;
    QString m_mode = QStringLiteral("ASV");
    QString m_lastCommandMessage;
    QString m_operatorId = QStringLiteral("unauthenticated");
    QString m_patientCategory = QStringLiteral("Adult");
    QString m_backendState = QStringLiteral("Simulator connected");

    int m_fio2 = 60;
    int m_patientIbwKg = 73;
    int m_peep = 15;
    int m_pressureSupport = 12;
    int m_inspiratoryTime = 1;
    int m_respiratoryRate = 20;
    int m_trigger = 3;
    int m_minuteVolume = 110;
    int m_tidalVolume = 420;

    sv::domain::ClinicalMetrics m_metrics;

    int m_alarmHighPressure = 40;
    int m_alarmLowPressure = 5;
    int m_alarmApneaTime = 20;
    int m_alarmLowVt = 300;
    int m_alarmHighMv = 12;
    int m_alarmLowSpo2 = 90;
    bool m_apneaBackupEnabled = true;

    double m_phase = 0.0;
    int m_sampleIndex = 0;
    int m_snapshotCounter = 0;
    int m_ventilationSeconds = 0;
};

} // namespace sv::backend
