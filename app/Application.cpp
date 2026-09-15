// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include "Application.h"

#include <sv/infrastructure/SimulatorAdapter.h>
#include <sv/services/SettingsValidator.h>
#include <sv/services/WaveformEngine.h>
#include <sv/services/BreathSimulator.h>
#include <sv/services/DiagnosticsService.h>
#include <sv/services/CalibrationService.h>
#include <sv/render/WaveformView.h>

#include <sv/common/AppIdentity.h>
#include <sv/common/LogBuffer.h>
#include <sv/presentation/MonitoringPresenter.h>

#include "src/core/AppSettings.h"
#include "src/core/DatabaseManager.h"
#include "src/controllers/AlarmController.h"
#include "src/controllers/ClockController.h"
#include "src/controllers/PatientController.h"
#include "src/controllers/EventController.h"
#include "src/controllers/UserController.h"
#include "src/controllers/VentilationCatalog.h"
#include "src/controllers/VentilatorController.h"

#include <QCoreApplication>
#include <QQmlContext>
#include <qqml.h>
#include <QDebug>

Application::Application(QObject *parent)
    : QObject(parent)
{
}

Application::~Application() = default;

bool Application::initialize()
{
    createInfrastructure();

    const bool dbReady = m_legacyDatabase->initialize();
    if (!dbReady)
        qCritical() << "Database initialization failed:" << m_legacyDatabase->lastError();

    createServices();
    createLegacyControllers();

    if (!dbReady) {
        m_alarmController->addAlarm(
            QStringLiteral("Critical"),
            QStringLiteral("Storage"),
            QStringLiteral("Database unavailable -- audit trail and patient persistence degraded"),
            QStringLiteral("Active"));
        m_alarmController->setActive(true);
        m_alarmController->setPriority(QStringLiteral("Critical"));
        m_alarmController->setHeadline(QStringLiteral("Storage Failure"));
        m_alarmController->setDetail(QStringLiteral("Audit trail unavailable"));
    }

    connectSignals();
    registerQmlTypes();
    registerContextProperties();
    loadQml();
    return true;
}

void Application::createInfrastructure()
{
    m_legacyDatabase = std::make_unique<DatabaseManager>();
    m_simulator = std::make_unique<sv::infrastructure::SimulatorAdapter>();
}

void Application::createServices()
{
    m_validator = std::make_unique<sv::services::SettingsValidator>();
    m_waveforms = std::make_unique<sv::services::WaveformEngine>();
    m_breathSim = std::make_unique<sv::services::BreathSimulator>();
    m_diagnosticsService = std::make_unique<sv::services::DiagnosticsService>();
    m_calibrationService = std::make_unique<sv::services::CalibrationService>();
    m_calibrationService->attach(m_legacyDatabase.get());
}

void Application::createLegacyControllers()
{
    m_logBuffer = std::make_unique<sv::common::LogBuffer>();
    m_logBuffer->installMessageHandler();
    m_logBuffer->setLogFile(sv::common::applicationDataDirectory()
                            + QStringLiteral("/smart-ventilator.log"));

    m_appSettings = std::make_unique<AppSettings>();
    m_alarmController = std::make_unique<AlarmController>(m_legacyDatabase.get());
    m_clockController = std::make_unique<ClockController>();
    m_patientController = std::make_unique<PatientController>(m_legacyDatabase.get());
    m_eventController = std::make_unique<EventController>(m_legacyDatabase.get());
    m_userController = std::make_unique<UserController>(m_legacyDatabase.get());
    m_ventilationCatalog = std::make_unique<VentilationCatalog>();
    m_ventilatorController = std::make_unique<VentilatorController>(
        m_legacyDatabase.get(), m_alarmController.get());

    m_monitoringPresenter = std::make_unique<sv::presentation::MonitoringPresenter>();
    m_monitoringPresenter->attach(m_ventilatorController.get(),
                                  m_patientController.get(),
                                  m_alarmController.get());

    m_ventilatorController->restoreSession();

    if (m_calibrationService) {
        connect(m_calibrationService.get(),
                &sv::services::CalibrationService::runChanged, this, [this]() {
            m_ventilatorController->setPreUseCheckPassed(m_calibrationService->allPassed());
        });
        m_calibrationService->setVentilating(m_ventilatorController->running());
        connect(m_ventilatorController.get(), &VentilatorController::runningChanged,
                m_calibrationService.get(), [this]() {
            m_calibrationService->setVentilating(m_ventilatorController->running());
        });
    }

    m_clockController->setTimeZoneId(m_appSettings->timeZoneId());
}

void Application::connectSignals()
{
    auto updatePatientContext = [this]() {
        m_ventilationCatalog->setPatientCategory(m_patientController->category());
        m_ventilatorController->setPatientProfile(m_patientController->category(),
                                                  m_patientController->ibw());
    };
    updatePatientContext();
    connect(m_patientController.get(), &PatientController::patientChanged,
            this, updatePatientContext);

    connect(m_userController.get(), &UserController::sessionChanged,
            this, [this]() {
        const QString opId = m_userController->loggedIn()
            ? m_userController->currentUser()
            : QStringLiteral("unauthenticated");
        m_ventilatorController->setOperatorId(opId);
    });

    connect(m_legacyDatabase.get(), &DatabaseManager::errorOccurred,
            this, [this](const QString &message) {
        m_alarmController->raiseAlarm(QStringLiteral("Critical"),
                                      QStringLiteral("Storage"),
                                      QStringLiteral("Storage Failure"),
                                      message);
    });

    // The catalogue is the authority on which controls and limits apply, so
    // it must follow the ventilator's mode rather than being set separately.
    auto syncMode = [this]() { m_ventilationCatalog->setMode(m_ventilatorController->mode()); };
    syncMode();
    connect(m_ventilatorController.get(), &VentilatorController::settingsChanged,
            this, syncMode);

    connect(m_ventilatorController.get(), &VentilatorController::backendStateChanged,
            this, [this]() {
        // A lost backend is a technical alarm condition, not a patient one.
        // IEC 60601-1-8 ranks it low priority, and the system strip carries
        // the detail, so it must not flash red across the header as if the
        // patient were in danger.
        if (m_ventilatorController->degradedMode()) {
            m_alarmController->raiseAlarm(QStringLiteral("Advisory"),
                                          QStringLiteral("Backend"),
                                          QStringLiteral("Backend Disconnected"),
                                          m_ventilatorController->backendState());
        } else {
            m_alarmController->clearCondition(QStringLiteral("legacy.backend.backend disconnected"));
        }
    });
}

void Application::registerQmlTypes()
{
    // The project imports QML by directory rather than through
    // qt_add_qml_module, so C++ visual items are registered by hand here.
    // This has to happen before the engine loads main.qml.
    qmlRegisterType<sv::render::WaveformView>(
        "SmartVentilator.Render", 1, 0, "WaveformView");
}

void Application::registerContextProperties()
{
    auto *ctx = m_engine.rootContext();

    // Legacy controllers for existing QML screens (transition period)
    ctx->setContextProperty(QStringLiteral("appSettings"), m_appSettings.get());
    ctx->setContextProperty(QStringLiteral("databaseManager"), m_legacyDatabase.get());
    ctx->setContextProperty(QStringLiteral("patientController"), m_patientController.get());
    ctx->setContextProperty(QStringLiteral("alarmController"), m_alarmController.get());
    ctx->setContextProperty(QStringLiteral("eventController"), m_eventController.get());
    ctx->setContextProperty(QStringLiteral("userController"), m_userController.get());
    ctx->setContextProperty(QStringLiteral("ventilatorController"), m_ventilatorController.get());
    ctx->setContextProperty(QStringLiteral("clockController"), m_clockController.get());

    // Declarative mode / parameter / limit tables. Screens read their control
    // set and their ranges from here rather than hardcoding either.
    ctx->setContextProperty(QStringLiteral("ventilationCatalog"), m_ventilationCatalog.get());
    ctx->setContextProperty(QStringLiteral("logBuffer"), m_logBuffer.get());
    ctx->setContextProperty(QStringLiteral("monitoringPresenter"),
                            m_monitoringPresenter.get());
    ctx->setContextProperty(QStringLiteral("calibrationService"),
                            m_calibrationService.get());
}

void Application::loadQml()
{
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(
        &m_engine,
        &QQmlApplicationEngine::objectCreated,
        this,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    m_engine.load(url);
}
