// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace sv::transport {

/**
 * @brief Signal identifiers carried on the bus.
 *
 * One list, used by the CAN decoder, the simulator and the application layer
 * alike. A signal the decoder does not know is dropped at the edge rather
 * than travelling as a loose string.
 */
namespace signals_ {
inline constexpr auto airwayPressure = "airwayPressure";
inline constexpr auto flow = "flow";
inline constexpr auto volume = "volume";
inline constexpr auto co2 = "co2";
inline constexpr auto peakPressure = "peakPressure";
inline constexpr auto plateauPressure = "plateauPressure";
inline constexpr auto meanPressure = "meanPressure";
inline constexpr auto peep = "peep";
inline constexpr auto tidalVolumeExpired = "tidalVolumeExpired";
inline constexpr auto minuteVolume = "minuteVolume";
inline constexpr auto respiratoryRate = "respiratoryRate";
inline constexpr auto fio2 = "fio2";
inline constexpr auto spo2 = "spo2";
inline constexpr auto etco2 = "etco2";
inline constexpr auto compliance = "compliance";
inline constexpr auto resistance = "resistance";
inline constexpr auto leakPercent = "leakPercent";
inline constexpr auto batteryPercent = "batteryPercent";
inline constexpr auto deviceFault = "deviceFault";
inline constexpr auto deviceState = "deviceState";

// Remaining runtime on battery, in minutes. 65535 means the device does not
// know, which is not the same as nearly empty and must not read as it.
inline constexpr auto batteryMinutes = "batteryMinutes";
inline constexpr quint16 batteryMinutesUnknown = 65535;

// Setpoints travel the other way, from the interface to the device. They
// carry their own names so a frame lookup by signal name cannot confuse a
// commanded value with a measured one.
inline constexpr auto setFio2 = "setFio2";
inline constexpr auto setPeep = "setPeep";
inline constexpr auto setRate = "setRate";
inline constexpr auto setTidalVolume = "setTidalVolume";
inline constexpr auto setPressureSupport = "setPressureSupport";
inline constexpr auto setInspiratoryTime = "setInspiratoryTime";
inline constexpr auto setTrigger = "setTrigger";
inline constexpr auto command = "command";
} // namespace signals_

/**
 * @brief Everything the application layer needs from the ventilator hardware.
 *
 * The CAN link and the desktop simulator both implement this, so nothing
 * above the transport layer knows or cares which one is running. That is what
 * lets the UI be developed and demonstrated with no hardware attached and
 * still exercise exactly the code path the device uses.
 */
class ITelemetrySource : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString descriptor READ descriptor CONSTANT)

public:
    using QObject::QObject;
    ~ITelemetrySource() override = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isConnected() const = 0;
    virtual QString descriptor() const = 0;

    /** @brief Sends a setting to the hardware. Returns false if not connected. */
    virtual bool writeSetting(const QString &signalName, double value) = 0;

signals:
    void connectedChanged();

    /** @brief One decoded signal, in the canonical units from sv/common/Units.h. */
    void signalReceived(const QString &signalName, double value, quint64 timestampMs);

    /** @brief A batch decoded from one frame, for callers that want them together. */
    void frameDecoded(const QVariantMap &values, quint64 timestampMs);

    void transportError(const QString &message);
};

} // namespace sv::transport
