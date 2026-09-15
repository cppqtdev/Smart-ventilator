// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QString>

namespace sv::domain {

enum class AlarmPriority {
    Critical,
    Warning,
    Info
};

struct AlarmDefinition {
    AlarmPriority priority = AlarmPriority::Info;
    QString source;
    QString headline;
    QString detail;
};

/// Convert alarm priority to a display string.
inline QString toString(AlarmPriority priority)
{
    switch (priority) {
    case AlarmPriority::Critical: return QStringLiteral("Critical");
    case AlarmPriority::Warning:  return QStringLiteral("Warning");
    case AlarmPriority::Info:     return QStringLiteral("Info");
    }
    return QStringLiteral("Info");
}

} // namespace sv::domain
