// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/domain/BatteryState.h>

namespace sv::infrastructure {

class IBatteryMonitor
{
public:
    virtual ~IBatteryMonitor() = default;

    /// @brief Return the current battery state snapshot.
    virtual sv::domain::BatteryState currentState() const = 0;

    /// @brief Advance the battery simulation by one second.
    virtual void tick() = 0;
};

} // namespace sv::infrastructure
