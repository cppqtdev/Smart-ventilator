// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/transport/SimulatedTelemetrySource.h>

#include <sv/common/Units.h>

#include <QTimer>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace sv::transport {
namespace {

constexpr double kMinimumCompliance = 1.0;
constexpr double kMinimumResistance = 0.5;

} // namespace

SimulatedTelemetrySource::SimulatedTelemetrySource(QObject *parent)
    : ITelemetrySource(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(m_sampleIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &SimulatedTelemetrySource::tick);
}

SimulatedTelemetrySource::~SimulatedTelemetrySource() = default;

void SimulatedTelemetrySource::setSampleIntervalMs(int milliseconds)
{
    m_sampleIntervalMs = std::clamp(milliseconds, 2, 100);
    m_timer->setInterval(m_sampleIntervalMs);
}

bool SimulatedTelemetrySource::start()
{
    if (m_timer->isActive())
        return true;
    m_uptime.start();
    m_lungVolumeMl = 0.0;
    m_breathPhaseS = 0.0;
    m_timer->start();
    m_connected = true;
    emit connectedChanged();
    return true;
}

void SimulatedTelemetrySource::stop()
{
    m_timer->stop();
    if (m_connected) {
        m_connected = false;
        emit connectedChanged();
    }
}

bool SimulatedTelemetrySource::isConnected() const
{
    return m_connected;
}

QString SimulatedTelemetrySource::descriptor() const
{
    return tr("Simulated lung, C %1 mL/cmH2O, R %2 cmH2O/(L/s)")
        .arg(m_lung.complianceMlPerCmH2O, 0, 'f', 0)
        .arg(m_lung.resistanceCmH2OPerLPerS, 0, 'f', 0);
}

bool SimulatedTelemetrySource::writeSetting(const QString &signalName, double value)
{
    if (signalName == QLatin1String(signals_::peep)) {
        m_ventilator.peep = value;
    } else if (signalName == QLatin1String(signals_::fio2)) {
        m_ventilator.fio2 = value;
    } else if (signalName == QLatin1String(signals_::respiratoryRate)) {
        m_ventilator.respiratoryRate = std::max(1.0, value);
    } else if (signalName == QLatin1String(signals_::tidalVolumeExpired)) {
        m_ventilator.tidalVolumeMl = value;
    } else {
        return false;
    }
    return true;
}

void SimulatedTelemetrySource::tick()
{
    const double dt = double(m_sampleIntervalMs) / units::millisecondsPerSecond;
    const double periodS = units::secondsPerMinute
                           / std::max(1.0, m_ventilator.respiratoryRate);
    const double inspiratoryTime = std::clamp(m_ventilator.inspiratoryTimeS,
                                              0.2, periodS * 0.8);

    m_breathPhaseS += dt;
    if (m_breathPhaseS >= periodS) {
        m_breathPhaseS -= periodS;
        publishBreath();
    }

    const bool inspiring = m_breathPhaseS < inspiratoryTime;
    const double compliance = std::max(kMinimumCompliance, m_lung.complianceMlPerCmH2O);
    const double resistance = std::max(kMinimumResistance, m_lung.resistanceCmH2OPerLPerS);

    double flowMlPerSecond = 0.0;

    if (inspiring) {
        if (m_ventilator.volumeControlled) {
            // Constant inspiratory flow: the square wave the reference draws.
            flowMlPerSecond = m_ventilator.tidalVolumeMl / inspiratoryTime;
        } else {
            // Pressure control: flow decays as the lung fills toward target.
            const double target = m_ventilator.pressureControl * compliance;
            const double deficit = std::max(0.0, target - m_lungVolumeMl);
            const double timeConstant = resistance * compliance / 1000.0;
            flowMlPerSecond = deficit / std::max(0.05, timeConstant);
        }
    } else {
        // Passive expiration with the same time constant.
        const double timeConstant = resistance * compliance / 1000.0;
        flowMlPerSecond = -m_lungVolumeMl / std::max(0.05, timeConstant);
    }

    m_lungVolumeMl = std::clamp(m_lungVolumeMl + flowMlPerSecond * dt, 0.0, 3000.0);
    m_flowLpm = units::flowToLitresPerMinute(flowMlPerSecond);

    const double elasticPressure = m_lungVolumeMl / compliance;
    const double resistivePressure = (flowMlPerSecond / 1000.0) * resistance;
    m_airwayPressure = m_ventilator.peep + m_lung.intrinsicPeep
                       + elasticPressure + resistivePressure
                       - m_lung.spontaneousEffortCmH2O;

    m_peakPressure = std::max(m_peakPressure, m_airwayPressure);
    m_peakVolume = std::max(m_peakVolume, m_lungVolumeMl);
    m_meanPressureSum += m_airwayPressure;
    ++m_meanPressureCount;
    m_wasInspiring = inspiring;

    const double co2 = inspiring
        ? 0.0
        : 38.0 * (1.0 - std::exp(-(m_breathPhaseS - inspiratoryTime) * 12.0));

    const quint64 now = quint64(m_uptime.elapsed());
    emit signalReceived(QString::fromLatin1(signals_::airwayPressure), m_airwayPressure, now);
    emit signalReceived(QString::fromLatin1(signals_::flow), m_flowLpm, now);
    emit signalReceived(QString::fromLatin1(signals_::volume), m_lungVolumeMl, now);
    emit signalReceived(QString::fromLatin1(signals_::co2), co2, now);
}

void SimulatedTelemetrySource::publishBreath()
{
    const double compliance = std::max(kMinimumCompliance, m_lung.complianceMlPerCmH2O);
    const double meanPressure = m_meanPressureCount > 0
        ? m_meanPressureSum / double(m_meanPressureCount)
        : m_ventilator.peep;
    const double minuteVolume = units::toLitres(m_peakVolume) * m_ventilator.respiratoryRate;
    const double plateau = m_ventilator.peep + m_peakVolume / compliance;

    const quint64 now = quint64(m_uptime.elapsed());
    const QVariantMap values{
        {QString::fromLatin1(signals_::peakPressure), m_peakPressure},
        {QString::fromLatin1(signals_::plateauPressure), plateau},
        {QString::fromLatin1(signals_::meanPressure), meanPressure},
        {QString::fromLatin1(signals_::peep), m_ventilator.peep + m_lung.intrinsicPeep},
        {QString::fromLatin1(signals_::tidalVolumeExpired), m_peakVolume},
        {QString::fromLatin1(signals_::minuteVolume), minuteVolume},
        {QString::fromLatin1(signals_::respiratoryRate), m_ventilator.respiratoryRate},
        {QString::fromLatin1(signals_::compliance), compliance},
        {QString::fromLatin1(signals_::resistance), m_lung.resistanceCmH2OPerLPerS},
        {QString::fromLatin1(signals_::fio2), m_ventilator.fio2},
        {QString::fromLatin1(signals_::spo2), 97.0},
        {QString::fromLatin1(signals_::etco2), 38.0},
        {QString::fromLatin1(signals_::leakPercent), 0.0}
    };

    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
        emit signalReceived(it.key(), it.value().toDouble(), now);
    emit frameDecoded(values, now);

    m_peakPressure = 0.0;
    m_peakVolume = 0.0;
    m_meanPressureSum = 0.0;
    m_meanPressureCount = 0;
}

} // namespace sv::transport
