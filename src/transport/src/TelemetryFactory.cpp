// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/transport/TelemetryFactory.h>

#include <sv/transport/CanSignal.h>
#include <sv/transport/SimulatedTelemetrySource.h>

#include <QLoggingCategory>
#include <QProcessEnvironment>

#if SV_HAS_CANBUS
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryFile>
#endif

namespace sv::transport {
namespace {

Q_LOGGING_CATEGORY(lcTransport, "sv.transport")

QString environmentValue(const char *name)
{
    return QProcessEnvironment::systemEnvironment().value(QString::fromLatin1(name));
}

#if SV_HAS_CANBUS
// QCanBus reads the DBC from a filesystem path, so a database compiled into
// the binary has to be spilled to a temporary file first.
QString materialise(const QString &path)
{
    if (!path.startsWith(QLatin1Char(':')))
        return path;
    if (!QFile::exists(path))
        return QString();

    auto *copy = new QTemporaryFile(qApp);
    if (!copy->open())
        return QString();
    QFile source(path);
    if (!source.open(QIODevice::ReadOnly))
        return QString();
    copy->write(source.readAll());
    copy->flush();
    return copy->fileName();
}
#endif // SV_HAS_CANBUS

} // namespace

bool TelemetryFactory::simulatorRequested()
{
    return environmentValue("SV_TELEMETRY").compare(QLatin1String("simulator"),
                                                    Qt::CaseInsensitive) == 0;
}

bool TelemetryFactory::udpRequested()
{
    return environmentValue("SV_TELEMETRY").compare(QLatin1String("udp"),
                                                    Qt::CaseInsensitive) == 0;
}

bool TelemetryFactory::deviceLinkRequested()
{
    const QString requested = environmentValue("SV_TELEMETRY");
    return requested.compare(QLatin1String("udp"), Qt::CaseInsensitive) == 0
        || requested.compare(QLatin1String("can"), Qt::CaseInsensitive) == 0;
}

UdpTelemetrySource::Endpoint TelemetryFactory::udpEndpointFromEnvironment()
{
    UdpTelemetrySource::Endpoint endpoint;

    const QString listenPort = environmentValue("SV_UDP_LISTEN_PORT");
    if (!listenPort.isEmpty())
        endpoint.listenPort = quint16(listenPort.toUShort());

    const QString sendPort = environmentValue("SV_UDP_SEND_PORT");
    if (!sendPort.isEmpty())
        endpoint.sendPort = quint16(sendPort.toUShort());

    return endpoint;
}

#if SV_HAS_CANBUS

CanTelemetrySource::Configuration TelemetryFactory::canConfigurationFromEnvironment(
    const QString &dbcPath)
{
    CanTelemetrySource::Configuration configuration;

    const QString plugin = environmentValue("SV_CAN_PLUGIN");
    if (!plugin.isEmpty())
        configuration.plugin = plugin;

    const QString interfaceName = environmentValue("SV_CAN_INTERFACE");
    if (!interfaceName.isEmpty())
        configuration.interfaceName = interfaceName;

    const QString nodeId = environmentValue("SV_CAN_NODE_ID");
    if (!nodeId.isEmpty())
        configuration.nodeId = nodeId.toInt(nullptr, 0);

    const QString bitrate = environmentValue("SV_CAN_BITRATE");
    if (!bitrate.isEmpty())
        configuration.bitrate = bitrate.toInt();

    configuration.databasePath = materialise(dbcPath);
    return configuration;
}

#endif // SV_HAS_CANBUS

std::unique_ptr<ITelemetrySource> TelemetryFactory::create(const Options &options,
                                                            QObject *parent)
{
    if (simulatorRequested()) {
        qCInfo(lcTransport, "SV_TELEMETRY=simulator: using the lung model");
        return std::make_unique<SimulatedTelemetrySource>(parent);
    }

    if (udpRequested()) {
        auto udpSource = std::make_unique<UdpTelemetrySource>(
            CanDatabase::defaultDatabase(), udpEndpointFromEnvironment(), parent);
        qCInfo(lcTransport, "SV_TELEMETRY=udp: %s", qPrintable(udpSource->descriptor()));
        return udpSource;
    }

#if SV_HAS_CANBUS
    auto canSource = std::make_unique<CanTelemetrySource>(
        canConfigurationFromEnvironment(options.dbcPath), parent);

    if (canSource->start()) {
        qCInfo(lcTransport, "CAN telemetry on %s", qPrintable(canSource->descriptor()));
        return canSource;
    }

    const bool forced = environmentValue("SV_TELEMETRY")
                            .compare(QLatin1String("can"), Qt::CaseInsensitive) == 0;
    if (forced || !options.allowSimulatorFallback) {
        qCCritical(lcTransport, "CAN telemetry unavailable and no fallback allowed");
        return canSource;
    }

    qCWarning(lcTransport, "CAN telemetry unavailable, falling back to the lung model");
#else
    Q_UNUSED(options)
    qCInfo(lcTransport, "built without Qt SerialBus: using the lung model");
#endif

    return std::make_unique<SimulatedTelemetrySource>(parent);
}

} // namespace sv::transport
