// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QString>

namespace sv::domain {

struct ClinicalMetrics
{
    double ppeak              = 0.0;
    double pplat              = 0.0;
    double pmean              = 0.0;
    double spo2               = 0.0;
    double etco2              = 0.0;
    double compliance         = 0.0;
    double resistance         = 0.0;
    double vte                = 0.0;
    double ftotal             = 0.0;
    double rcexp              = 0.0;
    double expMinVol          = 0.0;
    double drivingPressure    = 0.0;
    double workOfBreathing    = 0.0;
    double stressIndex        = 1.0;
    double deadSpaceFraction  = 0.3;
    int highFio2Minutes       = 0;
    bool patientDisconnected  = false;
    bool circuitOcclusion     = false;
    QString ieRatio           = QStringLiteral("1:2.0");
};

} // namespace sv::domain
