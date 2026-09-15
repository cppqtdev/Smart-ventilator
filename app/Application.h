// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <memory>

class AppSettings;
class AlarmController;
class ClockController;
class PatientController;
class EventController;
class UserController;
class VentilationCatalog;

namespace sv::presentation { class MonitoringPresenter; }
namespace sv::common { class LogBuffer; }
class VentilatorController;
class DatabaseManager;

namespace sv::infrastructure {
class DatabaseManager;
class SimulatorAdapter;
class SimulatedBattery;
}

namespace sv::services {
class SettingsValidator;
class WaveformEngine;
class BreathSimulator;
class DiagnosticsService;
class CalibrationService;
}

namespace sv::backend {
class VentilatorFacade;
class BatteryFacade;
class DiagnosticsFacade;
}

class Application : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Application)

public:
    explicit Application(QObject *parent = nullptr);
    ~Application() override;

    bool initialize();

private:
    void createInfrastructure();
    void createServices();
    void createFacades();
    void createLegacyControllers();
    void registerQmlTypes();
    void registerContextProperties();
    void connectSignals();
    void loadQml();

    QQmlApplicationEngine m_engine;

    std::unique_ptr<sv::infrastructure::DatabaseManager> m_database;
    std::unique_ptr<sv::infrastructure::SimulatorAdapter> m_simulator;
    std::unique_ptr<sv::infrastructure::SimulatedBattery> m_battery;

    std::unique_ptr<DatabaseManager> m_legacyDatabase;

    std::unique_ptr<sv::services::SettingsValidator> m_validator;
    std::unique_ptr<sv::services::WaveformEngine> m_waveforms;
    std::unique_ptr<sv::services::BreathSimulator> m_breathSim;
    std::unique_ptr<sv::services::DiagnosticsService> m_diagnosticsService;
    std::unique_ptr<sv::services::CalibrationService> m_calibrationService;

    std::unique_ptr<sv::backend::VentilatorFacade> m_ventilatorFacade;
    std::unique_ptr<sv::backend::BatteryFacade> m_batteryFacade;
    std::unique_ptr<sv::backend::DiagnosticsFacade> m_diagnosticsFacade;

    std::unique_ptr<AppSettings> m_appSettings;
    std::unique_ptr<AlarmController> m_alarmController;
    std::unique_ptr<ClockController> m_clockController;
    std::unique_ptr<PatientController> m_patientController;
    std::unique_ptr<EventController> m_eventController;
    std::unique_ptr<UserController> m_userController;
    std::unique_ptr<VentilationCatalog> m_ventilationCatalog;
    std::unique_ptr<sv::presentation::MonitoringPresenter> m_monitoringPresenter;
    std::unique_ptr<sv::common::LogBuffer> m_logBuffer;
    std::unique_ptr<VentilatorController> m_ventilatorController;
};
