// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/domain/Patient.h>
#include <sv/domain/Setpoint.h>

#include <QString>

namespace sv::services {

class SettingsValidator
{
public:
    /// @brief Check whether a ventilation mode string is supported.
    bool validateMode(const QString &mode, QString *reason) const;

    /// @brief Pre-start safety checks for all setpoints, limits, and patient context.
    bool validateStart(const sv::domain::SetpointSet &setpoints,
                       const sv::domain::AlarmLimits &limits,
                       bool degradedMode,
                       const sv::domain::Patient &patient,
                       QString *reason) const;

    /// @brief Inter-parameter consistency check when changing a single setting.
    bool validateSettingEnvelope(const QString &parameter,
                                int value,
                                const sv::domain::SetpointSet &setpoints,
                                const sv::domain::AlarmLimits &limits,
                                const sv::domain::Patient &patient,
                                QString *reason) const;
};

} // namespace sv::services
