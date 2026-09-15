// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

namespace sv::domain {

enum class BatteryStatus
{
    Charging,
    Discharging,
    Full,
    Critical,
    Unknown
};

struct BatteryState
{
    int percentage          = 100;
    BatteryStatus status    = BatteryStatus::Full;
    int estimatedMinutes    = 240;
    bool acPower            = true;
};

} // namespace sv::domain
