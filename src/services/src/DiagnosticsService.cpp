// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/services/DiagnosticsService.h>

#include <QDateTime>

namespace sv::services {

DiagnosticsService::DiagnosticsService(QObject *parent)
    : QObject(parent)
{
    const QStringList names = {
        QStringLiteral("O2 Cell"),
        QStringLiteral("Flow Sensor"),
        QStringLiteral("Pressure Sensor"),
        QStringLiteral("CO2 Sensor"),
        QStringLiteral("Temperature"),
        QStringLiteral("Humidity")
    };

    for (const auto &name : names) {
        sv::domain::SensorHealth s;
        s.name = name;
        s.online = true;
        m_sensors.append(s);
    }
}

Q_INVOKABLE sv::domain::DiagnosticReport DiagnosticsService::runSelfTest()
{
    const QString now = QDateTime::currentDateTime().toString(Qt::ISODate);

    sv::domain::DiagnosticReport report;
    report.allPassed = true;

    for (auto &sensor : m_sensors) {
        sensor.lastChecked = now;
        if (!sensor.online)
            report.allPassed = false;
    }

    report.sensors = m_sensors;

    for (const auto &sensor : m_sensors) {
        sv::domain::CalibrationResult test;
        test.testName = sensor.name;
        test.timestamp = now;
        if (sensor.online) {
            test.status = sv::domain::CalibrationStatus::Passed;
            test.details = QStringLiteral("Sensor responding normally");
        } else {
            test.status = sv::domain::CalibrationStatus::Failed;
            test.details = QStringLiteral("Sensor offline");
            report.allPassed = false;
        }
        report.tests.append(test);
    }

    emit sensorStatusChanged();
    return report;
}

QVector<sv::domain::SensorHealth> DiagnosticsService::sensorStatus() const
{
    return m_sensors;
}

Q_INVOKABLE void DiagnosticsService::toggleSensor(const QString &name)
{
    for (auto &sensor : m_sensors) {
        if (sensor.name == name) {
            sensor.online = !sensor.online;
            emit sensorStatusChanged();
            return;
        }
    }
}

} // namespace sv::services
