// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/transport/UdpTelemetrySource.h>

#include <QLoggingCategory>
#include <QNetworkDatagram>
#include <QVariantMap>

namespace sv::transport {

namespace {

Q_LOGGING_CATEGORY(lcUdpLink, "sv.transport.udp")

constexpr int kHeaderBytes = 5;
constexpr int kMaxPayload = 8;
constexpr int kHeartbeatIntervalMs = 500;
constexpr qint64 kSilenceLimitMs = 2000;

quint32 readIdentifier(const QByteArray &datagram)
{
    return quint32(quint8(datagram.at(0)))
         | quint32(quint8(datagram.at(1))) << 8
         | quint32(quint8(datagram.at(2))) << 16
         | quint32(quint8(datagram.at(3))) << 24;
}

QByteArray frameHeader(quint32 frameId, int length)
{
    QByteArray header(kHeaderBytes, char(0));
    header[0] = char(frameId & 0xFF);
    header[1] = char((frameId >> 8) & 0xFF);
    header[2] = char((frameId >> 16) & 0xFF);
    header[3] = char((frameId >> 24) & 0xFF);
    header[4] = char(length & 0xFF);
    return header;
}

} // namespace

UdpTelemetrySource::UdpTelemetrySource(const CanDatabase &database,
                                       const UdpEndpoint &endpoint,
                                       QObject *parent)
    : ITelemetrySource(parent)
    , m_database(database)
    , m_endpoint(endpoint)
    , m_setpointPayload(kMaxPayload, char(0))
{
    connect(&m_socket, &QUdpSocket::readyRead,
            this, &UdpTelemetrySource::readPending);

    m_heartbeat.setInterval(kHeartbeatIntervalMs);
    connect(&m_heartbeat, &QTimer::timeout,
            this, &UdpTelemetrySource::checkHeartbeat);
}

UdpTelemetrySource::~UdpTelemetrySource()
{
    UdpTelemetrySource::stop();
}

bool UdpTelemetrySource::start()
{
    if (m_socket.state() == QAbstractSocket::BoundState)
        return true;

    if (!m_socket.bind(m_endpoint.address, m_endpoint.listenPort,
                       QUdpSocket::DontShareAddress)) {
        const QString message = tr("Cannot listen on %1:%2 - %3")
                                    .arg(m_endpoint.address.toString())
                                    .arg(m_endpoint.listenPort)
                                    .arg(m_socket.errorString());
        qCWarning(lcUdpLink) << message;
        emit transportError(message);
        return false;
    }

    m_framesReceived = 0;
    m_lastFrameUtc = QDateTime();
    m_heartbeat.start();
    qCInfo(lcUdpLink) << "listening on" << m_endpoint.listenPort
                      << "sending to" << m_endpoint.sendPort;
    return true;
}

void UdpTelemetrySource::stop()
{
    m_heartbeat.stop();
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.close();
    setConnected(false);
}

bool UdpTelemetrySource::isConnected() const
{
    return m_connected;
}

QString UdpTelemetrySource::descriptor() const
{
    return tr("UDP frame link on %1, listening %2, sending %3")
        .arg(m_endpoint.address.toString())
        .arg(m_endpoint.listenPort)
        .arg(m_endpoint.sendPort);
}

void UdpTelemetrySource::readPending()
{
    while (m_socket.hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket.receiveDatagram();
        const QByteArray data = datagram.data();
        if (data.size() < kHeaderBytes)
            continue;

        const quint32 frameId = readIdentifier(data);
        const int length = qMin(int(quint8(data.at(4))), kMaxPayload);
        if (data.size() < kHeaderBytes + length)
            continue;

        const QByteArray payload = data.mid(kHeaderBytes, length);
        const CanMessage *message = m_database.message(frameId);
        if (message == nullptr) {
            emit unknownFrame(frameId);
            continue;
        }

        ++m_framesReceived;
        m_lastFrameUtc = QDateTime::currentDateTimeUtc();
        setConnected(true);

        const quint64 stamp = quint64(m_lastFrameUtc.toMSecsSinceEpoch());
        QVariantMap decoded;
        for (const CanSignal &entry : message->signalList) {
            const double value = entry.decode(payload);
            decoded.insert(entry.name, value);
            emit signalReceived(entry.name, value, stamp);
        }
        emit frameDecoded(decoded, stamp);
    }
}

void UdpTelemetrySource::checkHeartbeat()
{
    if (!m_lastFrameUtc.isValid()) {
        setConnected(false);
        return;
    }
    const qint64 age = m_lastFrameUtc.msecsTo(QDateTime::currentDateTimeUtc());
    setConnected(age <= kSilenceLimitMs);
}

bool UdpTelemetrySource::writeSetting(const QString &signalName, double value)
{
    const CanMessage *message = m_database.messageForSignal(signalName);
    if (message == nullptr)
        return false;

    // Only the setpoint frame is kept whole between writes; anything else is
    // sent on its own.
    QByteArray payload = message->frameId == 0x200
        ? m_setpointPayload
        : QByteArray(message->byteLength, char(0));

    bool packed = false;
    for (const CanSignal &entry : message->signalList) {
        if (entry.name != signalName)
            continue;
        packed = entry.encode(payload, value);
        break;
    }
    if (!packed)
        return false;

    if (message->frameId == 0x200)
        m_setpointPayload = payload;

    return sendFrame(message->frameId, payload);
}

bool UdpTelemetrySource::writeCommand(quint8 code)
{
    QByteArray payload(kMaxPayload, char(0));
    payload[0] = char(code);
    return sendFrame(0x201, payload);
}

bool UdpTelemetrySource::sendFrame(quint32 frameId, const QByteArray &payload)
{
    if (m_socket.state() != QAbstractSocket::BoundState)
        return false;

    const QByteArray datagram = frameHeader(frameId, int(payload.size())) + payload;
    const qint64 written = m_socket.writeDatagram(datagram, m_endpoint.address,
                                                  m_endpoint.sendPort);
    if (written != datagram.size()) {
        emit transportError(tr("Frame %1 was not sent: %2")
                                .arg(frameId, 3, 16, QLatin1Char('0'))
                                .arg(m_socket.errorString()));
        return false;
    }
    return true;
}

void UdpTelemetrySource::setConnected(bool connected)
{
    if (m_connected == connected)
        return;
    m_connected = connected;
    emit connectedChanged();
}

} // namespace sv::transport
