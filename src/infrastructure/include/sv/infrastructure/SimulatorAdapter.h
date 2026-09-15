// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/infrastructure/IVentilatorHardware.h>

namespace sv::infrastructure {

class SimulatorAdapter : public IVentilatorHardware
{
public:
    SimulatorAdapter() = default;

    /// @brief Generate one simulation tick producing pressure, flow, volume, and CO2 waveform values.
    sv::domain::WaveformSample tick(double dt, double phase, bool inspiration,
                                    double normalized, const sv::domain::SetpointSet &setpoints,
                                    const QString &mode) override;

    /// @brief Compute clinical metrics (ppeak, spo2, compliance, etc.) from current simulation state.
    sv::domain::ClinicalMetrics computeMetrics(const sv::domain::SetpointSet &setpoints,
                                               int sampleIndex) override;

private:
    static double clamp(double value, double lo, double hi);

    int m_sampleIndex = 0;
    int m_highFio2SampleCounter = 0;
    int m_highFio2Minutes = 0;
    double m_lastPpeak = 0.0;
    double m_lastFlow = 0.0;
};

} // namespace sv::infrastructure
