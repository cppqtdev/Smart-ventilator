// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/common/RingBuffer.h>
#include <sv/domain/WaveformSample.h>

#include <QVariantList>

namespace sv::services {

class WaveformEngine
{
public:
    /// @brief Append a single sample to all four waveform buffers.
    void appendSample(const sv::domain::WaveformSample &sample);

    /// @brief Clear all waveform buffers.
    void clear();

    /// @brief Return pressure waveform as QVariantList for QML.
    QVariantList pressureWaveform() const;

    /// @brief Return flow waveform as QVariantList for QML.
    QVariantList flowWaveform() const;

    /// @brief Return volume waveform as QVariantList for QML.
    QVariantList volumeWaveform() const;

    /// @brief Return CO2 waveform as QVariantList for QML.
    QVariantList co2Waveform() const;

    /// @brief Whether waveform updates are frozen.
    bool frozen() const;

    /// @brief Set the frozen state.
    void setFrozen(bool frozen);

private:
    sv::common::RingBuffer<double, 180> m_pressure;
    sv::common::RingBuffer<double, 180> m_flow;
    sv::common::RingBuffer<double, 180> m_volume;
    sv::common::RingBuffer<double, 180> m_co2;
    bool m_frozen = false;
};

} // namespace sv::services
