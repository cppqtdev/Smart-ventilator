// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#ifndef SV_HAS_CANBUS
#define SV_HAS_CANBUS 1
#endif

#if SV_HAS_CANBUS

#include <sv/transport/CanSignal.h>
#include <sv/transport/ITelemetrySource.h>

#include <QElapsedTimer>
#include <QString>

#include <memory>

QT_BEGIN_NAMESPACE
class QCanBusDevice;
class QTimer;
QT_END_NAMESPACE

namespace sv::transport {

/**
 * @brief QCanBus link to the ventilator controller board.
 *
 * Plugin and interface are configuration, not code: socketcan/can0 on the
 * target, virtualcan/can0 on a developer desktop. Nothing else changes
 * between the two, so the desktop exercises the real decode path.
 *
 * ### Threading
 *
 * The device is created on whatever thread this object lives on and is never
 * touched from another. Move the whole object to a dedicated thread if the
 * GUI thread cannot keep up with the frame rate; do not share the device.
 *
 * ### Heartbeat
 *
 * CANopen NMT heartbeats arrive on 0x700 + node id. Missing @c
 * heartbeatTimeoutMs of them drops `connected`, which the alarm layer turns
 * into a technical alarm. An EMCY frame on 0x080 + node id is forwarded as a
 * device fault.
 */
class CanTelemetrySource : public ITelemetrySource
{
    Q_OBJECT

public:
    struct Configuration
    {
        QString plugin = QStringLiteral("socketcan");
        QString interfaceName = QStringLiteral("can0");
        QString databasePath;
        int nodeId = 0x01;
        int heartbeatTimeoutMs = 1500;
        int bitrate = 500000;
    };

    explicit CanTelemetrySource(Configuration configuration, QObject *parent = nullptr);
    ~CanTelemetrySource() override;

    bool start() override;
    void stop() override;
    bool isConnected() const override;
    QString descriptor() const override;
    bool writeSetting(const QString &signalName, double value) override;

    const CanDatabase &database() const { return m_database; }

    /** @brief Lists the plugins the Qt build actually provides. */
    static QStringList availablePlugins();

private:
    void readFrames();
    void handleDeviceError();
    void setConnected(bool connected);
    void loadDatabase();

    Configuration m_configuration;
    CanDatabase m_database;
    std::unique_ptr<QCanBusDevice> m_device;
    QTimer *m_heartbeatTimer = nullptr;
    QElapsedTimer m_uptime;
    bool m_connected = false;
};

} // namespace sv::transport

#endif // SV_HAS_CANBUS
