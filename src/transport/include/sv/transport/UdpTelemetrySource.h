// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <sv/transport/CanSignal.h>
#include <sv/transport/ITelemetrySource.h>

#include <QByteArray>
#include <QDateTime>
#include <QHostAddress>
#include <QTimer>
#include <QUdpSocket>

namespace sv::transport {

/**
 * @brief The device link carried over loopback datagrams instead of CAN.
 *
 * The device speaks CAN. Development machines do not have a CAN interface,
 * and macOS has no SocketCAN at all, so the same frames travel as datagrams:
 * four bytes of identifier, one of length, then the eight payload bytes, in
 * exactly the layout data/ventilator.dbc describes. Decoding, encoding, the
 * heartbeat and the setpoint path are therefore the code the device runs; only
 * the wire underneath changes when this is swapped for CanTelemetrySource.
 *
 * Datagram layout, little endian:
 *   [0..3] frame identifier
 *   [4]    payload length, 0 to 8
 *   [5..]  payload
 */
// Declared at namespace scope: a nested struct with default member
// initializers cannot be used as a default argument inside the class that
// encloses it, because the initializers are not complete until the class is.
struct UdpEndpoint
{
    QHostAddress address = QHostAddress::LocalHost;
    quint16 listenPort = 35200;
    quint16 sendPort = 35201;
};

class UdpTelemetrySource : public ITelemetrySource
{
    Q_OBJECT

public:
    using Endpoint = UdpEndpoint;

    explicit UdpTelemetrySource(const CanDatabase &database,
                                const UdpEndpoint &endpoint = UdpEndpoint{},
                                QObject *parent = nullptr);
    ~UdpTelemetrySource() override;

    bool start() override;
    void stop() override;
    bool isConnected() const override;
    QString descriptor() const override;
    bool writeSetting(const QString &signalName, double value) override;

    /** @brief Sends the command word on 0x201. */
    bool writeCommand(quint8 code);

    /** @brief Frames seen since start, for the diagnostics page. */
    quint64 framesReceived() const { return m_framesReceived; }

signals:
    /** @brief A frame arrived that the database does not describe. */
    void unknownFrame(quint32 frameId);

private slots:
    void readPending();
    void checkHeartbeat();

private:
    bool sendFrame(quint32 frameId, const QByteArray &payload);
    void setConnected(bool connected);

    CanDatabase m_database;
    UdpEndpoint m_endpoint;
    QUdpSocket m_socket;
    QTimer m_heartbeat;

    // The outgoing setpoint frame is kept whole: one signal changing must not
    // blank the others, because the device applies the frame as a set.
    QByteArray m_setpointPayload;

    QDateTime m_lastFrameUtc;
    quint64 m_framesReceived = 0;
    bool m_connected = false;
};

} // namespace sv::transport
