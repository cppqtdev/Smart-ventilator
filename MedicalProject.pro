QT += quick quickcontrols2 sql multimedia network

APP_VERSION = $$(APP_VERSION)
isEmpty(APP_VERSION): APP_VERSION = 0.1.0-dev
BUILD_ID = $$(BUILD_ID)
isEmpty(BUILD_ID): BUILD_ID = local
DEFINES += APP_VERSION=\\\"$${APP_VERSION}\\\" BUILD_ID=\\\"$${BUILD_ID}\\\"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

SOURCES += \
        main.cpp \
        src/common/src/AppIdentity.cpp \
        src/common/src/LogBuffer.cpp \
        src/core/AppSettings.cpp \
        src/core/DatabaseManager.cpp \
        src/controllers/AlarmController.cpp \
        src/controllers/ClockController.cpp \
        src/controllers/EventController.cpp \
        src/controllers/UserController.cpp \
        src/controllers/PatientController.cpp \
        src/controllers/VentilationCatalog.cpp \
        src/controllers/VentilatorController.cpp \
        src/domain/src/ModeCatalog.cpp \
        src/domain/src/Patient.cpp \
        src/domain/src/PatientCategory.cpp \
        src/domain/src/VentilationMode.cpp \
        src/domain/src/VentilationParameter.cpp \
        src/services/src/CalibrationService.cpp \
        src/services/src/RespiratoryMechanics.cpp \
        src/render/src/WaveformView.cpp \
        src/presentation/src/MonitoringPresenter.cpp \
        src/transport/src/CanSignal.cpp \
        src/transport/src/TelemetryBridge.cpp \
        src/transport/src/UdpTelemetrySource.cpp \
        src/transport/src/SimulatedTelemetrySource.cpp \
        src/transport/src/TelemetryFactory.cpp

HEADERS += \
        src/common/include/sv/common/AppIdentity.h \
        src/common/include/sv/common/LogBuffer.h \
        src/core/AppSettings.h \
        src/core/DatabaseManager.h \
        src/controllers/AlarmController.h \
        src/controllers/ClockController.h \
        src/controllers/EventController.h \
        src/controllers/UserController.h \
        src/controllers/PatientController.h \
        src/controllers/VentilationCatalog.h \
        src/controllers/VentilatorController.h \
        src/domain/include/sv/domain/ModeCatalog.h \
        src/domain/include/sv/domain/Patient.h \
        src/domain/include/sv/domain/PatientCategory.h \
        src/domain/include/sv/domain/VentilationMode.h \
        src/domain/include/sv/domain/VentilationParameter.h \
        src/services/include/sv/services/CalibrationService.h \
        src/services/include/sv/services/RespiratoryMechanics.h \
        src/render/include/sv/render/WaveformView.h \
        src/presentation/include/sv/presentation/MonitoringPresenter.h \
        src/transport/include/sv/transport/CanSignal.h \
        src/transport/include/sv/transport/TelemetryBridge.h \
        src/transport/include/sv/transport/UdpTelemetrySource.h \
        src/transport/include/sv/transport/ITelemetrySource.h \
        src/transport/include/sv/transport/SimulatedTelemetrySource.h \
        src/transport/include/sv/transport/TelemetryFactory.h

# The libraries expose their headers under <sv/<lib>/...>, so each library's
# include root goes on the path. This mirrors what the CMake build does with
# target_include_directories; the two build systems must agree or a header
# moves and only one of them notices.
INCLUDEPATH += . \
    src/common/include \
    src/domain/include \
    src/services/include \
    src/render/include \
    src/presentation/include \
    src/transport/include


# SerialBus is an optional Qt module. Without it the CAN link is compiled out
# and the simulator is the only telemetry source, so the kit still builds.
qtHaveModule(serialbus) {
    QT += serialbus
    DEFINES += SV_HAS_CANBUS=1
    SOURCES += src/transport/src/CanTelemetrySource.cpp
    HEADERS += src/transport/include/sv/transport/CanTelemetrySource.h
} else {
    DEFINES += SV_HAS_CANBUS=0
    message("Qt SerialBus not available: building without the CAN link")
}

RESOURCES += qml.qrc \
    src/transport/data/transport.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
