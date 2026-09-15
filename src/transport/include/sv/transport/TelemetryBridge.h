// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class VentilatorController;

namespace sv::transport {

class ITelemetrySource;

/**
 * @brief Joins a device link to the application layer, both ways.
 *
 * Measurements arriving on the link are applied to the controller; operator
 * setting changes leaving the controller are encoded back onto the link. The
 * controller keeps no knowledge of the transport and the transport keeps none
 * of the clinical model, so either can be replaced on its own.
 *
 * The bridge also owns the meaning of "attached": while the link is up the
 * controller is told a hardware backend is present, which stands the internal
 * model down and arms the heartbeat watchdog. When the link drops, the
 * controller is told, and the degraded path it already has takes over.
 */
class TelemetryBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool linkUp READ linkUp NOTIFY linkChanged)
    Q_PROPERTY(QString descriptor READ descriptor NOTIFY linkChanged)

public:
    /** @brief Command codes on frame 0x201. */
    enum Command : quint8 {
        NoCommand = 0,
        StartVentilation = 1,
        StopVentilation = 2,
        InspiratoryHold = 3,
        ExpiratoryHold = 4,
        ManualBreath = 5,
        OxygenBoost = 6,
        Nebuliser = 7,
        AlarmReset = 8
    };
    Q_ENUM(Command)

    TelemetryBridge(ITelemetrySource *source, VentilatorController *controller,
                    QObject *parent = nullptr);

    bool linkUp() const;
    QString descriptor() const;

    /** @brief Opens the link. False when the transport could not start. */
    bool start();
    void stop();

signals:
    void linkChanged();
    void linkError(const QString &message);

private:
    void onFrame(const QVariantMap &values);
    void onLinkChanged();
    void publishSetpoints();
    void sendCommand(Command code);

    ITelemetrySource *m_source = nullptr;
    VentilatorController *m_controller = nullptr;
    bool m_adopting = false;
    bool m_everUp = false;
};

} // namespace sv::transport
