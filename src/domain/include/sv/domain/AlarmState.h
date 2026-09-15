// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include "AlarmDefinition.h"

namespace sv::domain {

struct AlarmState
{
    bool active              = false;
    AlarmPriority priority   = AlarmPriority::Info;
    QString headline         = QStringLiteral("No Active Alarms");
    QString detail           = QStringLiteral("System normal");
    bool silenced            = false;
    int silenceRemaining     = 0;
};

} // namespace sv::domain
