// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/infrastructure/SimulatedBattery.h>

namespace sv::infrastructure {

// Drain rate: 1% every 2.4 minutes = 144 seconds per 1%
// Charge rate: 1% every 1.8 minutes = 108 seconds per 1%
static constexpr double kDrainSecondsPerPercent  = 144.0;
static constexpr double kChargeSecondsPerPercent = 108.0;

SimulatedBattery::SimulatedBattery(QObject *parent)
    : QObject(parent)
{
}

sv::domain::BatteryState SimulatedBattery::currentState() const
{
    sv::domain::BatteryState state;
    state.percentage = m_percentage;
    state.acPower = m_acPower;

    if (m_acPower) {
        state.status = (m_percentage >= 100)
            ? sv::domain::BatteryStatus::Full
            : sv::domain::BatteryStatus::Charging;
        state.estimatedMinutes = 0;
    } else {
        state.status = (m_percentage < 20)
            ? sv::domain::BatteryStatus::Critical
            : sv::domain::BatteryStatus::Discharging;
        state.estimatedMinutes = static_cast<int>(m_percentage * kDrainSecondsPerPercent / 60.0);
    }

    return state;
}

void SimulatedBattery::tick()
{
    if (m_acPower) {
        if (m_percentage >= 100)
            return;
        m_fractionalAccumulator += 1.0 / kChargeSecondsPerPercent;
    } else {
        if (m_percentage <= 0)
            return;
        m_fractionalAccumulator += 1.0 / kDrainSecondsPerPercent;
    }

    if (m_fractionalAccumulator >= 1.0) {
        m_fractionalAccumulator -= 1.0;
        if (m_acPower) {
            if (m_percentage < 100)
                ++m_percentage;
        } else {
            if (m_percentage > 0)
                --m_percentage;
        }
    }
}

void SimulatedBattery::setAcPower(bool connected)
{
    if (m_acPower == connected)
        return;
    m_acPower = connected;
    m_fractionalAccumulator = 0.0;
}

} // namespace sv::infrastructure
