// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/transport/CanTelemetrySource.h>

#if SV_HAS_CANBUS

#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusFrame>
#include <QFile>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

namespace sv::transport {
namespace {

constexpr quint32 kHeartbeatBase = 0x700;
constexpr quint32 kEmergencyBase = 0x080;

} // namespace

CanTelemetrySource::CanTelemetrySource(Configuration configuration, QObject *parent)
    : ITelemetrySource(parent)
    , m_configuration(std::move(configuration))
    , m_database(CanDatabase::defaultDatabase())
{
    loadDatabase();

    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setSingleShot(true);
    m_heartbeatTimer->setInterval(m_configuration.heartbeatTimeoutMs);
    connect(m_heartbeatTimer, &QTimer::timeout, this, [this]() {
        setConnected(false);
        emit transportError(tr("No heartbeat from node %1 for %2 ms")
                                .arg(m_configuration.nodeId)
                                .arg(m_configuration.heartbeatTimeoutMs));
    });
}

CanTelemetrySource::~CanTelemetrySource()
{
    stop();
}

void CanTelemetrySource::loadDatabase()
{
    if (m_configuration.databasePath.isEmpty())
        return;

    QFile file(m_configuration.databasePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit transportError(tr("Cannot open %1: %2")
                                .arg(m_configuration.databasePath, file.errorString()));
        return;
    }

    QString parseError;
    CanDatabase parsed = CanDatabase::fromDbc(file.readAll(), &parseError);
    if (parsed.messageCount() == 0) {
        emit transportError(tr("%1: %2").arg(m_configuration.databasePath, parseError));
        return;
    }
    m_database = parsed;
}

QStringList CanTelemetrySource::availablePlugins()
{
    return QCanBus::instance()->plugins();
}

bool CanTelemetrySource::start()
{
    if (m_device)
        return true;

    QString error;
    m_device.reset(QCanBus::instance()->createDevice(
        m_configuration.plugin.toLatin1(), m_configuration.interfaceName, &error));

    if (!m_device) {
        emit transportError(tr("Cannot create %1/%2: %3")
                                .arg(m_configuration.plugin,
                                     m_configuration.interfaceName,
                                     error));
        return false;
    }

    m_device->setConfigurationParameter(QCanBusDevice::BitRateKey, m_configuration.bitrate);

    connect(m_device.get(), &QCanBusDevice::framesReceived,
            this, &CanTelemetrySource::readFrames);
    connect(m_device.get(), &QCanBusDevice::errorOccurred,
            this, &CanTelemetrySource::handleDeviceError);

    if (!m_device->connectDevice()) {
        emit transportError(tr("Cannot connect %1/%2: %3")
                                .arg(m_configuration.plugin,
                                     m_configuration.interfaceName,
                                     m_device->errorString()));
        m_device.reset();
        return false;
    }

    m_uptime.start();
    m_heartbeatTimer->start();
    return true;
}

void CanTelemetrySource::stop()
{
    if (m_heartbeatTimer != nullptr)
        m_heartbeatTimer->stop();
    if (m_device) {
        m_device->disconnectDevice();
        m_device.reset();
    }
    setConnected(false);
}

bool CanTelemetrySource::isConnected() const
{
    return m_connected;
}

QString CanTelemetrySource::descriptor() const
{
    return QStringLiteral("%1/%2 @ %3 bit/s")
        .arg(m_configuration.plugin, m_configuration.interfaceName)
        .arg(m_configuration.bitrate);
}

void CanTelemetrySource::setConnected(bool connected)
{
    if (m_connected == connected)
        return;
    m_connected = connected;
    emit connectedChanged();
}

void CanTelemetrySource::readFrames()
{
    if (!m_device)
        return;

    const quint64 now = m_uptime.isValid() ? quint64(m_uptime.elapsed()) : 0;

    while (m_device->framesAvailable() > 0) {
        const QCanBusFrame frame = m_device->readFrame();
        if (!frame.isValid())
            continue;

        const quint32 frameId = frame.frameId();

        if (frameId == kHeartbeatBase + quint32(m_configuration.nodeId)) {
            m_heartbeatTimer->start();
            setConnected(true);
            continue;
        }

        if (frameId == kEmergencyBase + quint32(m_configuration.nodeId)) {
            const QByteArray payload = frame.payload();
            const quint32 code = payload.size() >= 2
                ? quint32(uchar(payload.at(0))) | (quint32(uchar(payload.at(1))) << 8)
                : 0;
            emit signalReceived(QString::fromLatin1(signals_::deviceFault), double(code), now);
            continue;
        }

        const CanMessage *message = m_database.message(frameId);
        if (message == nullptr)
            continue;

        const QByteArray payload = frame.payload();
        QVariantMap decoded;
        for (const CanSignal &entry : message->signalList) {
            const double value = entry.decode(payload);
            decoded.insert(entry.name, value);
            emit signalReceived(entry.name, value, now);
        }
        if (!decoded.isEmpty())
            emit frameDecoded(decoded, now);

        m_heartbeatTimer->start();
        setConnected(true);
    }
}

void CanTelemetrySource::handleDeviceError()
{
    if (!m_device)
        return;
    emit transportError(m_device->errorString());
    if (m_device->state() != QCanBusDevice::ConnectedState)
        setConnected(false);
}

bool CanTelemetrySource::writeSetting(const QString &signalName, double value)
{
    if (!m_device || !m_connected)
        return false;

    const CanMessage *message = m_database.messageForSignal(signalName);
    if (message == nullptr)
        return false;

    QByteArray payload(message->byteLength, '\0');
    for (const CanSignal &entry : message->signalList) {
        if (entry.name == signalName) {
            if (!entry.encode(payload, value))
                return false;
            break;
        }
    }

    QCanBusFrame frame(message->frameId, payload);
    return m_device->writeFrame(frame);
}

} // namespace sv::transport

#endif // SV_HAS_CANBUS
