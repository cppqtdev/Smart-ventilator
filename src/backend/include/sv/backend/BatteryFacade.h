// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/domain/BatteryState.h>

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

namespace sv::infrastructure {
class IBatteryMonitor;
}

namespace sv::backend {

class BatteryFacade : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_DISABLE_COPY_MOVE(BatteryFacade)

    Q_PROPERTY(int percentage READ percentage NOTIFY batteryChanged)
    Q_PROPERTY(QString status READ status NOTIFY batteryChanged)
    Q_PROPERTY(int estimatedMinutes READ estimatedMinutes NOTIFY batteryChanged)
    Q_PROPERTY(bool acPower READ acPower NOTIFY batteryChanged)

    // Being on mains and being charging are different states, and the UI
    // draws them differently: a full pack on mains shows no bolt. Deriving
    // this in QML from acPower alone would collapse the distinction.
    Q_PROPERTY(bool isCharging READ isCharging NOTIFY batteryChanged)

    // Alias for estimatedMinutes, named for what the header actually shows:
    // remaining runtime on the internal source.
    Q_PROPERTY(int runtimeMinutes READ runtimeMinutes NOTIFY batteryChanged)

    // ISO 80601-2-12 makes internal power depletion a MEDIUM priority alarm
    // that escalates to HIGH within five minutes, so the UI needs the
    // condition, not just the percentage.
    Q_PROPERTY(bool depleted READ depleted NOTIFY batteryChanged)

public:
    explicit BatteryFacade(sv::infrastructure::IBatteryMonitor *monitor,
                           QObject *parent = nullptr);

    int percentage() const;
    QString status() const;
    int estimatedMinutes() const;
    bool acPower() const;
    bool isCharging() const;
    int runtimeMinutes() const;
    bool depleted() const;

    Q_INVOKABLE void setAcPower(bool connected);

signals:
    void batteryChanged();
    void criticalBattery();

private slots:
    void tick();

private:
    sv::infrastructure::IBatteryMonitor *m_monitor = nullptr;
    QTimer m_timer;
    int m_lastPercentage = 100;
    sv::domain::BatteryStatus m_lastStatus = sv::domain::BatteryStatus::Unknown;
};

} // namespace sv::backend
