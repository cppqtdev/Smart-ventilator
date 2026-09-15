// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/services/ClinicalMetricsCalculator.h>

#include <cmath>

namespace sv::services {

void ClinicalMetricsCalculator::update(int fio2, double ppeak, double flow, bool running, int sampleIndex)
{
    if (fio2 > 60) {
        ++m_highFio2SampleCounter;
        // ~22 samples per second at 45ms interval, 60s = ~1333 samples
        if (m_highFio2SampleCounter >= 1333) {
            m_highFio2SampleCounter = 0;
            ++m_highFio2Minutes;
        }
    } else {
        m_highFio2SampleCounter = 0;
    }
}

int ClinicalMetricsCalculator::highFio2Minutes() const
{
    return m_highFio2Minutes;
}

bool ClinicalMetricsCalculator::detectPatientDisconnect(double ppeak, bool running, int sampleIndex) const
{
    return ppeak < 3.0 && running && sampleIndex > 100;
}

bool ClinicalMetricsCalculator::detectCircuitOcclusion(double ppeak, double flow, bool running) const
{
    return ppeak > 55.0 && std::abs(flow) < 2.0 && running;
}

void ClinicalMetricsCalculator::reset()
{
    m_highFio2SampleCounter = 0;
    m_highFio2Minutes = 0;
}

} // namespace sv::services
