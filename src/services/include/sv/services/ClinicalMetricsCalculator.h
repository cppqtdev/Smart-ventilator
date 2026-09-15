// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

namespace sv::services {

class ClinicalMetricsCalculator
{
public:
    /// @brief Update O2 toxicity timer, disconnect and occlusion detection.
    void update(int fio2, double ppeak, double flow, bool running, int sampleIndex);

    /// @brief Minutes of sustained FiO2 above 60%.
    int highFio2Minutes() const;

    /// @brief Detect patient disconnect from near-zero pressure.
    bool detectPatientDisconnect(double ppeak, bool running, int sampleIndex) const;

    /// @brief Detect circuit occlusion from high pressure with near-zero flow.
    bool detectCircuitOcclusion(double ppeak, double flow, bool running) const;

    /// @brief Reset all internal counters.
    void reset();

private:
    int m_highFio2SampleCounter = 0;
    int m_highFio2Minutes = 0;
};

} // namespace sv::services
