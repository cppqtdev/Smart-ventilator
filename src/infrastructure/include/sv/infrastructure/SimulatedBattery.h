// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/infrastructure/IBatteryMonitor.h>

#include <QObject>

namespace sv::infrastructure {

class SimulatedBattery : public QObject, public IBatteryMonitor
{
    Q_OBJECT

public:
    explicit SimulatedBattery(QObject *parent = nullptr);

    /// @brief Return the current battery state snapshot.
    sv::domain::BatteryState currentState() const override;

    /// @brief Advance the battery simulation by one second.
    void tick() override;

    /// @brief Set AC power connected state for testing.
    Q_INVOKABLE void setAcPower(bool connected);

private:
    int m_percentage = 100;
    bool m_acPower = true;
    double m_fractionalAccumulator = 0.0;
};

} // namespace sv::infrastructure
