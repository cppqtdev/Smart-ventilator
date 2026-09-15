// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/transport/ITelemetrySource.h>

#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace sv::transport {

/**
 * @brief Lung model driving the same signals the CAN link publishes.
 *
 * A single-compartment model: the ventilator applies a pressure or delivers
 * a volume, the lung responds through its compliance and resistance, and the
 * derived values fall out of the resulting waveform. It is not a validated
 * physiological model and must never be presented as patient data, but it
 * exercises the whole decode-to-display path with no hardware attached.
 */
class SimulatedTelemetrySource : public ITelemetrySource
{
    Q_OBJECT

public:
    struct Lung
    {
        double complianceMlPerCmH2O = 50.0;
        double resistanceCmH2OPerLPerS = 10.0;
        double intrinsicPeep = 0.0;
        double spontaneousEffortCmH2O = 0.0;
    };

    struct Ventilator
    {
        double peep = 5.0;
        double pressureControl = 15.0;
        double tidalVolumeMl = 500.0;
        double respiratoryRate = 14.0;
        double inspiratoryTimeS = 1.0;
        double fio2 = 60.0;
        bool volumeControlled = true;
    };

    explicit SimulatedTelemetrySource(QObject *parent = nullptr);
    ~SimulatedTelemetrySource() override;

    bool start() override;
    void stop() override;
    bool isConnected() const override;
    QString descriptor() const override;
    bool writeSetting(const QString &signalName, double value) override;

    void setLung(const Lung &lung) { m_lung = lung; }
    const Lung &lung() const { return m_lung; }

    void setVentilator(const Ventilator &settings) { m_ventilator = settings; }
    const Ventilator &ventilator() const { return m_ventilator; }

    /** @brief Sample interval in milliseconds. 10 ms gives a 100 Hz waveform. */
    void setSampleIntervalMs(int milliseconds);

private:
    void tick();
    void publishBreath();

    Lung m_lung;
    Ventilator m_ventilator;

    QTimer *m_timer = nullptr;
    QElapsedTimer m_uptime;
    int m_sampleIntervalMs = 10;

    double m_breathPhaseS = 0.0;
    double m_lungVolumeMl = 0.0;
    double m_airwayPressure = 5.0;
    double m_flowLpm = 0.0;

    double m_peakPressure = 0.0;
    double m_meanPressureSum = 0.0;
    int m_meanPressureCount = 0;
    double m_peakVolume = 0.0;
    bool m_wasInspiring = false;
    bool m_connected = false;
};

} // namespace sv::transport
