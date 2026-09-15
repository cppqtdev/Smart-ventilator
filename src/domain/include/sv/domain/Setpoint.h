// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QString>

namespace sv::domain {

struct Setpoint
{
    QString name;
    int value    = 0;
    QString unit;
    int minimum  = 0;
    int maximum  = 100;
};

struct SetpointSet
{
    int fio2             = 60;
    int peep             = 15;
    int pressureSupport  = 12;
    int inspiratoryTime  = 1;
    int respiratoryRate  = 20;
    int trigger          = 3;
    int minuteVolume     = 110;
    int tidalVolume      = 420;
};

struct AlarmLimits
{
    int highPressure = 40;
    int lowPressure  = 5;
    int apneaTime    = 20;
    int lowVt        = 300;
    int highMv       = 12;
    int lowSpo2      = 90;
};

inline SetpointSet defaultSetpoints() { return {}; }
inline AlarmLimits defaultAlarmLimits() { return {}; }

} // namespace sv::domain
