// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/infrastructure/SimulatorAdapter.h>

#include <QtMath>
#include <algorithm>
#include <cmath>

namespace sv::infrastructure {

double SimulatorAdapter::clamp(double value, double lo, double hi)
{
    return std::max(lo, std::min(hi, value));
}

sv::domain::WaveformSample SimulatorAdapter::tick(double dt, double phase, bool inspiration,
                                                  double normalized,
                                                  const sv::domain::SetpointSet &setpoints,
                                                  const QString &mode)
{
    ++m_sampleIndex;

    const double effort = std::sin(m_sampleIndex * 0.037) * 0.7
                        + std::sin(m_sampleIndex * 0.011) * 0.4;
    const double pressureTarget = setpoints.peep + setpoints.pressureSupport
                                + setpoints.tidalVolume / 55.0;
    const double flowPeak = setpoints.tidalVolume / 7.0;

    double paw, flow, volume;

    if (mode == QStringLiteral("VCV")) {
        flow = inspiration
            ? flowPeak + effort * 2.0
            : -flowPeak * 0.6 * std::exp(-normalized * 3.0) + effort;
        paw = inspiration
            ? setpoints.peep + (pressureTarget - setpoints.peep) * normalized + effort
            : setpoints.peep + effort * 0.3;
        volume = inspiration
            ? setpoints.tidalVolume * normalized
            : setpoints.tidalVolume * (1.0 - normalized);

    } else if (mode == QStringLiteral("PCV")) {
        paw = inspiration
            ? pressureTarget + effort
            : setpoints.peep + effort * 0.3;
        flow = inspiration
            ? flowPeak * std::exp(-normalized * 3.0) + effort * 2.0
            : -flowPeak * 0.5 * std::sin(M_PI * normalized) + effort;
        volume = inspiration
            ? setpoints.tidalVolume * (1.0 - std::exp(-normalized * 4.0))
            : setpoints.tidalVolume * std::exp(-normalized * 3.5);

    } else if (mode == QStringLiteral("CPAP") || mode == QStringLiteral("PSV")) {
        double spontaneous = std::sin(phase * M_PI * 2.0);
        paw = setpoints.peep + setpoints.pressureSupport * 0.5
            + spontaneous * setpoints.pressureSupport * 0.4 + effort * 0.5;
        flow = spontaneous * flowPeak * 0.6 + effort * 4.0;
        volume = (std::sin(phase * M_PI * 2.0 - M_PI / 2.0) + 1.0)
               * setpoints.tidalVolume * 0.3;

    } else if (mode == QStringLiteral("SIMV")) {
        bool mandatoryBreath = (m_sampleIndex % 88) < 44;
        if (mandatoryBreath) {
            paw = inspiration
                ? pressureTarget + effort
                : setpoints.peep + effort * 0.3;
            flow = inspiration
                ? flowPeak * std::exp(-normalized * 2.5) + effort * 2.0
                : -flowPeak * 0.55 * std::sin(M_PI * normalized) + effort;
        } else {
            double spont = std::sin(phase * M_PI * 2.0);
            paw = setpoints.peep + setpoints.pressureSupport * 0.3
                + spont * 3.0 + effort * 0.4;
            flow = spont * flowPeak * 0.35 + effort * 3.0;
        }
        volume = inspiration
            ? setpoints.tidalVolume * 0.7 * std::sin(normalized * M_PI / 2.0)
            : setpoints.tidalVolume * 0.7 * std::exp(-normalized * 3.0);

    } else if (mode == QStringLiteral("BiPAP")) {
        double highP = setpoints.peep + setpoints.pressureSupport;
        paw = inspiration
            ? highP + effort * 0.5
            : setpoints.peep + (highP - setpoints.peep) * 0.15 + effort * 0.3;
        flow = inspiration
            ? flowPeak * 0.8 * (1.0 - normalized * 0.5) + effort * 2.5
            : -flowPeak * 0.6 * std::sin(M_PI * normalized) + effort;
        volume = inspiration
            ? setpoints.tidalVolume * 0.85 * std::sin(normalized * M_PI / 2.0)
            : setpoints.tidalVolume * 0.85 * std::exp(-normalized * 4.0);

    } else if (mode == QStringLiteral("PRVC")) {
        double adaptedPressure = pressureTarget * 0.9
            + std::sin(m_sampleIndex * 0.005) * 2.0;
        paw = inspiration
            ? setpoints.peep + (adaptedPressure - setpoints.peep)
                * (1.0 - std::exp(-normalized * 8.0)) + effort
            : setpoints.peep + effort * 0.25;
        flow = inspiration
            ? flowPeak * std::exp(-normalized * 2.0) + effort * 2.0
            : -flowPeak * 0.65 * std::sin(M_PI * normalized) + effort;
        volume = inspiration
            ? setpoints.tidalVolume * (1.0 - std::exp(-normalized * 5.0))
            : setpoints.tidalVolume * std::exp(-normalized * 4.0);

    } else {
        // Default (ASV and others)
        paw = inspiration
            ? setpoints.peep + (pressureTarget - setpoints.peep)
                * (1.0 - std::exp(-normalized * 6.0)) + effort
            : setpoints.peep + (pressureTarget - setpoints.peep)
                * std::exp(-normalized * 9.0) + effort * 0.35;
        flow = inspiration
            ? flowPeak * (1.0 - normalized * 0.7) + effort * 3.0
            : -flowPeak * 0.72 * std::sin(M_PI * normalized)
                * std::exp(-normalized * 0.35) + effort * 2.0;
        volume = inspiration
            ? setpoints.tidalVolume * std::sin(normalized * M_PI / 2.0)
            : setpoints.tidalVolume * std::exp(-normalized * 4.4);
    }

    const double co2 = inspiration
        ? qMax(0.0, 35.0 * std::exp(-normalized * 6.0) - 2.0)
        : 35.0 * (1.0 - std::exp(-normalized * 8.0)) + std::sin(m_sampleIndex * 0.08);

    m_lastPpeak = paw;
    m_lastFlow = flow;

    return { paw, flow, volume, co2 };
}

sv::domain::ClinicalMetrics SimulatorAdapter::computeMetrics(const sv::domain::SetpointSet &setpoints,
                                                             int sampleIndex)
{
    sv::domain::ClinicalMetrics m;

    const double slow = std::sin(sampleIndex * 0.021);
    const double fio2Effect = (setpoints.fio2 - 21.0) / 79.0;
    const double rr = clamp(setpoints.respiratoryRate, 6, 45);
    const double pressureTarget = setpoints.peep + setpoints.pressureSupport
                                + setpoints.tidalVolume / 55.0;

    m.ppeak = qRound(clamp(pressureTarget + 6.0 + slow * 2.2, 8, 58));
    m.pplat = qRound(clamp(pressureTarget + 1.5 + slow, 6, 45));
    m.pmean = qRound(clamp(setpoints.peep + setpoints.pressureSupport * 0.45 + slow, 4, 35));
    m.spo2 = qRound(clamp(92.0 + fio2Effect * 8.0
                           - qMax(0, setpoints.peep - 18) * 0.15
                           + std::sin(sampleIndex * 0.013), 84, 100));
    m.etco2 = qRound(clamp(31.0 + std::sin(sampleIndex * 0.018) * 3.0
                            - (setpoints.minuteVolume - 100.0) * 0.025, 18, 55));
    m.compliance = qRound(clamp(setpoints.tidalVolume / qMax(1.0, m.pplat - setpoints.peep)
                                + std::sin(sampleIndex * 0.017) * 4.0, 12, 95));
    m.resistance = qRound(clamp(8.0 + setpoints.trigger * 0.8
                                + std::sin(sampleIndex * 0.029) * 2.0, 3, 28));

    m.vte = qRound(clamp(setpoints.tidalVolume
                          * (0.92 + std::sin(sampleIndex * 0.023) * 0.06), 50, 900));
    m.ftotal = qRound(clamp(rr + std::sin(sampleIndex * 0.019) * 1.5, 4, 60));
    m.rcexp = clamp(m.compliance * m.resistance / 1000.0
                    + std::sin(sampleIndex * 0.031) * 0.08, 0.1, 2.5);
    m.rcexp = std::round(m.rcexp * 100.0) / 100.0;
    m.expMinVol = clamp(m.vte * m.ftotal / 1000.0, 0.5, 30.0);
    m.expMinVol = std::round(m.expMinVol * 10.0) / 10.0;

    // Clinical decision support metrics
    m.workOfBreathing = clamp(
        0.45 + (m.resistance - 12.0) * 0.03
        + (30.0 - m.compliance) * 0.008
        + std::sin(sampleIndex * 0.027) * 0.08,
        0.15, 2.5);
    m.workOfBreathing = std::round(m.workOfBreathing * 100.0) / 100.0;

    m.stressIndex = clamp(
        1.0 + (m.ppeak - 30.0) * 0.02
        + std::sin(sampleIndex * 0.019) * 0.05,
        0.6, 1.8);
    m.stressIndex = std::round(m.stressIndex * 100.0) / 100.0;

    m.deadSpaceFraction = clamp(
        0.28 + (50.0 - m.etco2) * 0.004
        + std::sin(sampleIndex * 0.015) * 0.02,
        0.10, 0.80);
    m.deadSpaceFraction = std::round(m.deadSpaceFraction * 100.0) / 100.0;

    // O2 toxicity tracking
    if (setpoints.fio2 > 60) {
        ++m_highFio2SampleCounter;
        if (m_highFio2SampleCounter >= 1333) {
            m_highFio2SampleCounter = 0;
            ++m_highFio2Minutes;
        }
    } else {
        m_highFio2SampleCounter = 0;
    }
    m.highFio2Minutes = m_highFio2Minutes;

    m.patientDisconnected = (m.ppeak < 3.0 && sampleIndex > 100);
    m.circuitOcclusion = (m.ppeak > 55.0 && std::abs(m_lastFlow) < 2.0);

    return m;
}

} // namespace sv::infrastructure
