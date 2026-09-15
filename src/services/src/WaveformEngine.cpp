// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/services/WaveformEngine.h>

namespace sv::services {

void WaveformEngine::appendSample(const sv::domain::WaveformSample &sample)
{
    if (m_frozen)
        return;
    m_pressure.push(sample.pressure);
    m_flow.push(sample.flow);
    m_volume.push(sample.volume);
    m_co2.push(sample.co2);
}

void WaveformEngine::clear()
{
    m_pressure.clear();
    m_flow.clear();
    m_volume.clear();
    m_co2.clear();
}

QVariantList WaveformEngine::pressureWaveform() const { return m_pressure.toList(); }
QVariantList WaveformEngine::flowWaveform() const { return m_flow.toList(); }
QVariantList WaveformEngine::volumeWaveform() const { return m_volume.toList(); }
QVariantList WaveformEngine::co2Waveform() const { return m_co2.toList(); }

bool WaveformEngine::frozen() const { return m_frozen; }
void WaveformEngine::setFrozen(bool frozen) { m_frozen = frozen; }

} // namespace sv::services
