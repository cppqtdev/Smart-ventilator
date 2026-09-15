// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include "CalibrationResult.h"
#include <QString>
#include <QVector>

namespace sv::domain {

struct SensorHealth
{
    QString name;
    bool online        = false;
    QString lastChecked;
};

struct DiagnosticReport
{
    QVector<SensorHealth> sensors;
    QVector<CalibrationResult> tests;
    bool allPassed = false;
};

} // namespace sv::domain
