// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QString>

namespace sv::domain {

enum class CalibrationStatus
{
    Passed,
    Failed,
    NotRun,
    InProgress
};

struct CalibrationResult
{
    QString testName;
    CalibrationStatus status  = CalibrationStatus::NotRun;
    QString timestamp;
    QString details;
};

} // namespace sv::domain
