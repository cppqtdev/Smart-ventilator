// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/domain/DiagnosticReport.h>

#include <QObject>
#include <QVector>

namespace sv::services {

class DiagnosticsService : public QObject
{
    Q_OBJECT

public:
    explicit DiagnosticsService(QObject *parent = nullptr);

    /// @brief Run a full self-test across all sensors and return a report.
    Q_INVOKABLE sv::domain::DiagnosticReport runSelfTest();

    /// @brief Return the current sensor health status list.
    QVector<sv::domain::SensorHealth> sensorStatus() const;

    /// @brief Toggle a sensor online/offline for testing purposes.
    Q_INVOKABLE void toggleSensor(const QString &name);

signals:
    void sensorStatusChanged();

private:
    QVector<sv::domain::SensorHealth> m_sensors;
};

} // namespace sv::services
