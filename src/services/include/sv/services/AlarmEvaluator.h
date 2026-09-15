// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/domain/AlarmDefinition.h>
#include <sv/domain/AlarmState.h>
#include <sv/domain/ClinicalMetrics.h>
#include <sv/domain/Setpoint.h>

namespace sv::services {

class AlarmEvaluator
{
public:
    /// @brief Evaluate current metrics and return the highest-priority active alarm.
    sv::domain::AlarmState evaluate(const sv::domain::ClinicalMetrics &metrics,
                                    const sv::domain::AlarmLimits &limits,
                                    const sv::domain::SetpointSet &setpoints,
                                    bool running) const;
};

} // namespace sv::services
