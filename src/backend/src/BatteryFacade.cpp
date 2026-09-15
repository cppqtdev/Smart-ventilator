// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/backend/BatteryFacade.h>
#include <sv/infrastructure/IBatteryMonitor.h>
#include <sv/domain/BatteryState.h>

namespace sv::backend {

BatteryFacade::BatteryFacade(sv::infrastructure::IBatteryMonitor *monitor,
                             QObject *parent)
    : QObject(parent)
    , m_monitor(monitor)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &BatteryFacade::tick);
    m_timer.start();
}

int BatteryFacade::percentage() const
{
    return m_monitor ? m_monitor->currentState().percentage : 100;
}

QString BatteryFacade::status() const
{
    if (!m_monitor)
        return QStringLiteral("Unknown");

    switch (m_monitor->currentState().status) {
    case sv::domain::BatteryStatus::Charging:     return QStringLiteral("Charging");
    case sv::domain::BatteryStatus::Discharging:  return QStringLiteral("Discharging");
    case sv::domain::BatteryStatus::Full:          return QStringLiteral("Full");
    case sv::domain::BatteryStatus::Critical:      return QStringLiteral("Critical");
    case sv::domain::BatteryStatus::Unknown:       return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

int BatteryFacade::estimatedMinutes() const
{
    return m_monitor ? m_monitor->currentState().estimatedMinutes : 0;
}

bool BatteryFacade::acPower() const
{
    return m_monitor ? m_monitor->currentState().acPower : false;
}

bool BatteryFacade::isCharging() const
{
    if (!m_monitor)
        return false;
    return m_monitor->currentState().status == sv::domain::BatteryStatus::Charging;
}

int BatteryFacade::runtimeMinutes() const
{
    // Only meaningful while running on the internal source; on mains the
    // number is not a countdown and showing it would be misleading.
    return acPower() ? -1 : estimatedMinutes();
}

bool BatteryFacade::depleted() const
{
    if (!m_monitor)
        return false;
    const auto state = m_monitor->currentState();
    return !state.acPower
        && (state.status == sv::domain::BatteryStatus::Critical
            || state.percentage < 20);
}

void BatteryFacade::setAcPower(bool connected)
{
    if (!m_monitor)
        return;
    auto *simBattery = dynamic_cast<QObject *>(m_monitor);
    if (simBattery)
        QMetaObject::invokeMethod(simBattery, "setAcPower", Q_ARG(bool, connected));
    tick();
}

void BatteryFacade::tick()
{
    if (!m_monitor)
        return;
    m_monitor->tick();

    const auto state = m_monitor->currentState();
    const bool wasCritical = m_lastStatus == sv::domain::BatteryStatus::Critical;
    m_lastPercentage = state.percentage;
    m_lastStatus = state.status;

    emit batteryChanged();

    // The alarm is raised on the transition. Emitting it on every tick while
    // the condition lasts would re-announce the same alarm once a second.
    if (!wasCritical && state.status == sv::domain::BatteryStatus::Critical)
        emit criticalBattery();
}

} // namespace sv::backend
