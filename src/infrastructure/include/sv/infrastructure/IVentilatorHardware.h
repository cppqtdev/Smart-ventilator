// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/domain/WaveformSample.h>
#include <sv/domain/ClinicalMetrics.h>
#include <sv/domain/Setpoint.h>

#include <QString>

namespace sv::infrastructure {

class IVentilatorHardware
{
public:
    virtual ~IVentilatorHardware() = default;

    /// @brief Generate one simulation/hardware tick. Returns raw waveform + metrics.
    virtual sv::domain::WaveformSample tick(double dt, double phase, bool inspiration,
                                            double normalized, const sv::domain::SetpointSet &setpoints,
                                            const QString &mode) = 0;

    /// @brief Compute clinical metrics from current state.
    virtual sv::domain::ClinicalMetrics computeMetrics(const sv::domain::SetpointSet &setpoints,
                                                       int sampleIndex) = 0;
};

} // namespace sv::infrastructure
