#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QByteArray>
#include <QDebug>

#include "src/core/AppSettings.h"
#include "src/core/DatabaseManager.h"
#include "src/controllers/AlarmController.h"
#include "src/controllers/ClockController.h"
#include "src/controllers/PatientController.h"
#include "src/controllers/EventController.h"
#include "src/controllers/UserController.h"
#include "src/controllers/VentilationCatalog.h"
#include "src/controllers/VentilatorController.h"

#include <sv/render/WaveformView.h>
#include <QStandardPaths>

#include <sv/common/LogBuffer.h>
#include <sv/presentation/MonitoringPresenter.h>
#include <sv/services/CalibrationService.h>
#include <sv/transport/TelemetryBridge.h>
#include <sv/transport/TelemetryFactory.h>

#include <qqml.h>

#include <memory>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArray("Basic"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("AlsonsTechnology"));
    QCoreApplication::setApplicationName(QStringLiteral("SmartVentilatorDemo"));

    DatabaseManager databaseManager;
    const bool databaseReady = databaseManager.initialize();
    if (!databaseReady)
        qCritical() << "Database initialization failed:" << databaseManager.lastError();
    AppSettings appSettings;
    PatientController patientController(&databaseManager);
    AlarmController alarmController(&databaseManager);
    if (!databaseReady) {
        alarmController.addAlarm(QStringLiteral("Critical"),
                                 QStringLiteral("Storage"),
                                 QStringLiteral("Database unavailable -- audit trail and patient persistence degraded"),
                                 QStringLiteral("Active"));
        alarmController.setActive(true);
        alarmController.setPriority(QStringLiteral("Critical"));
        alarmController.setHeadline(QStringLiteral("Storage Failure"));
        alarmController.setDetail(QStringLiteral("Audit trail unavailable"));
    }
    EventController eventController(&databaseManager);
    UserController userController(&databaseManager);
    VentilatorController ventilatorController(&databaseManager, &alarmController);
    ClockController clockController;
    VentilationCatalog ventilationCatalog;

    auto updateVentilatorPatientContext = [&]() {
        // The catalogue owns every parameter limit, so it has to learn the
        // patient category at the same moment the controller does.
        ventilationCatalog.setPatientCategory(patientController.category());
        ventilatorController.setPatientProfile(patientController.category(),
                                               patientController.ibw());
    };
    updateVentilatorPatientContext();
    QObject::connect(&patientController, &PatientController::patientChanged,
                     &ventilatorController, updateVentilatorPatientContext);

    // The catalogue is the authority on which controls and limits apply, so it
    // follows the ventilator's mode rather than being set independently.
    auto syncCatalogMode = [&]() { ventilationCatalog.setMode(ventilatorController.mode()); };
    syncCatalogMode();
    QObject::connect(&ventilatorController, &VentilatorController::settingsChanged,
                     &ventilationCatalog, syncCatalogMode);

    QObject::connect(&userController, &UserController::sessionChanged,
                     &ventilatorController, [&]() {
        ventilatorController.setOperatorId(userController.loggedIn()
            ? userController.currentUser()
            : QStringLiteral("unauthenticated"));
    });

    QObject::connect(&databaseManager, &DatabaseManager::errorOccurred,
                     &alarmController, [&](const QString &message) {
        alarmController.raiseAlarm(QStringLiteral("Critical"),
                                   QStringLiteral("Storage"),
                                   QStringLiteral("Storage Failure"),
                                   message);
    });
    QObject::connect(&ventilatorController, &VentilatorController::backendStateChanged,
                     &alarmController, [&]() {
        // A lost backend is a technical alarm condition, not a patient one.
        // IEC 60601-1-8 ranks it low priority, and the system strip carries
        // the detail, so it must not flash red across the header as if the
        // patient were in danger.
        if (ventilatorController.degradedMode()) {
            alarmController.raiseAlarm(QStringLiteral("Advisory"),
                                       QStringLiteral("Backend"),
                                       QStringLiteral("Backend Disconnected"),
                                       ventilatorController.backendState());
        } else {
            alarmController.clearCondition(QStringLiteral("legacy.backend.backend disconnected"));
        }
    });

    // Restore persisted timezone from QSettings.
    clockController.setTimeZoneId(appSettings.timeZoneId());

    // The project imports QML by directory rather than through
    // qt_add_qml_module, so C++ visual items are registered by hand. This must
    // happen before the engine loads main.qml.
    qmlRegisterType<sv::render::WaveformView>(
        "SmartVentilator.Render", 1, 0, "WaveformView");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appSettings"), &appSettings);
    engine.rootContext()->setContextProperty(QStringLiteral("databaseManager"), &databaseManager);
    engine.rootContext()->setContextProperty(QStringLiteral("patientController"), &patientController);
    engine.rootContext()->setContextProperty(QStringLiteral("alarmController"), &alarmController);
    engine.rootContext()->setContextProperty(QStringLiteral("eventController"), &eventController);
    engine.rootContext()->setContextProperty(QStringLiteral("userController"), &userController);
    engine.rootContext()->setContextProperty(QStringLiteral("ventilatorController"), &ventilatorController);
    engine.rootContext()->setContextProperty(QStringLiteral("clockController"), &clockController);
    engine.rootContext()->setContextProperty(QStringLiteral("ventilationCatalog"), &ventilationCatalog);

    sv::common::LogBuffer logBuffer;
    logBuffer.installMessageHandler();
    logBuffer.setLogFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QStringLiteral("/smart-ventilator.log"));
    engine.rootContext()->setContextProperty(QStringLiteral("logBuffer"), &logBuffer);

    // The device link. Without one the controller keeps running its own
    // model, which is what lets the interface be demonstrated with nothing
    // attached; with one, every measurement on screen came off the wire.
    std::unique_ptr<sv::transport::ITelemetrySource> telemetrySource;
    std::unique_ptr<sv::transport::TelemetryBridge> telemetryBridge;
    if (sv::transport::TelemetryFactory::deviceLinkRequested()) {
        telemetrySource = sv::transport::TelemetryFactory::create({}, &app);
        telemetryBridge = std::make_unique<sv::transport::TelemetryBridge>(
            telemetrySource.get(), &ventilatorController, &app);
        if (!telemetryBridge->start())
            qWarning() << "Device link did not open:" << telemetryBridge->descriptor();
        engine.rootContext()->setContextProperty(QStringLiteral("telemetryBridge"),
                                                 telemetryBridge.get());
    }

    sv::services::CalibrationService calibrationService;
    calibrationService.attach(&databaseManager);
    calibrationService.setVentilating(ventilatorController.running());
    QObject::connect(&ventilatorController, &VentilatorController::runningChanged,
                     &calibrationService, [&calibrationService, &ventilatorController]() {
        calibrationService.setVentilating(ventilatorController.running());
    });
    // A restart must not present an empty standby screen while a patient is
    // still on the circuit.
    ventilatorController.restoreSession();

    // The pre-use check is what gates the start, so the calibration result
    // has to reach the controller rather than only the page that ran it.
    QObject::connect(&calibrationService, &sv::services::CalibrationService::runChanged,
                     &ventilatorController, [&calibrationService, &ventilatorController]() {
        ventilatorController.setPreUseCheckPassed(calibrationService.allPassed());
    });

    engine.rootContext()->setContextProperty(QStringLiteral("calibrationService"),
                                             &calibrationService);

    sv::presentation::MonitoringPresenter monitoringPresenter;
    monitoringPresenter.attach(&ventilatorController, &patientController, &alarmController);
    engine.rootContext()->setContextProperty(QStringLiteral("monitoringPresenter"),
                                             &monitoringPresenter);
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
