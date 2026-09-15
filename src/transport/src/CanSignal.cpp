// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/transport/CanSignal.h>
#include <sv/transport/ITelemetrySource.h>

#include <sv/common/BitCodec.h>

#include <QRegularExpression>

#include <algorithm>
#include <cmath>

namespace sv::transport {
namespace {

bits::Order toBitOrder(ByteOrder order)
{
    return order == ByteOrder::Intel ? bits::Order::Intel : bits::Order::Motorola;
}

} // namespace

double CanSignal::decode(const QByteArray &payload) const
{
    const quint64 raw = bits::extract(
        reinterpret_cast<const unsigned char *>(payload.constData()),
        size_t(payload.size()), startBit, bitLength, toBitOrder(byteOrder));
    const double numeric = isSigned ? double(bits::toSigned(raw, bitLength))
                                    : double(raw);
    double physical = numeric * factor + offset;
    if (maximum > minimum)
        physical = std::clamp(physical, std::min(minimum, maximum),
                              std::max(minimum, maximum));
    return physical;
}

bool CanSignal::encode(QByteArray &payload, double physical) const
{
    if (factor == 0.0)
        return false;
    double bounded = physical;
    if (maximum > minimum)
        bounded = std::clamp(bounded, std::min(minimum, maximum),
                             std::max(minimum, maximum));

    const double numeric = (bounded - offset) / factor;
    const qint64 rounded = qint64(std::llround(numeric));

    quint64 raw = 0;
    if (isSigned) {
        const qint64 limit = qint64(1) << (bitLength - 1);
        const qint64 clamped = std::clamp<qint64>(rounded, std::min<qint64>(-limit, limit - 1),
                                                  std::max<qint64>(-limit, limit - 1));
        raw = quint64(clamped) & bits::maximumRaw(bitLength);
    } else {
        const qint64 ceiling = qint64(bits::maximumRaw(bitLength));
        raw = quint64(std::clamp<qint64>(rounded, std::min<qint64>(0, ceiling),
                                         std::max<qint64>(0, ceiling)));
    }

    bits::insert(reinterpret_cast<unsigned char *>(payload.data()),
                 size_t(payload.size()), startBit, bitLength,
                 toBitOrder(byteOrder), raw);
    return true;
}

const CanMessage *CanDatabase::message(quint32 frameId) const
{
    const auto it = m_messages.constFind(frameId);
    return it == m_messages.constEnd() ? nullptr : &(*it);
}

const CanMessage *CanDatabase::messageForSignal(const QString &signalName) const
{
    for (auto it = m_messages.constBegin(); it != m_messages.constEnd(); ++it) {
        for (const CanSignal &entry : it->signalList) {
            if (entry.name == signalName)
                return &(*it);
        }
    }
    return nullptr;
}

CanDatabase CanDatabase::defaultDatabase()
{
    using namespace sv::transport::signals_;
    CanDatabase database;

    // 0x120 - waveform samples, 100 Hz. Pressure and flow are signed because
    // flow is bidirectional and pressure can read below zero on a leak test.
    database.insert(CanMessage{0x120, QStringLiteral("VentWaveform"), 8, {
        {QString::fromLatin1(airwayPressure), 0, 16, ByteOrder::Intel, true, 0.01, 0.0, -20.0, 120.0, QStringLiteral("cmH2O")},
        {QString::fromLatin1(flow), 16, 16, ByteOrder::Intel, true, 0.1, 0.0, -300.0, 300.0, QStringLiteral("L/min")},
        {QString::fromLatin1(volume), 32, 16, ByteOrder::Intel, false, 0.5, 0.0, 0.0, 3000.0, QStringLiteral("mL")},
        {QString::fromLatin1(co2), 48, 16, ByteOrder::Intel, false, 0.01, 0.0, 0.0, 100.0, QStringLiteral("mmHg")}
    }});

    // 0x121 - breath-by-breath pressures, published at end of expiration.
    database.insert(CanMessage{0x121, QStringLiteral("VentPressures"), 8, {
        {QString::fromLatin1(peakPressure), 0, 16, ByteOrder::Intel, false, 0.1, 0.0, 0.0, 120.0, QStringLiteral("cmH2O")},
        {QString::fromLatin1(plateauPressure), 16, 16, ByteOrder::Intel, false, 0.1, 0.0, 0.0, 120.0, QStringLiteral("cmH2O")},
        {QString::fromLatin1(meanPressure), 32, 16, ByteOrder::Intel, false, 0.1, 0.0, 0.0, 120.0, QStringLiteral("cmH2O")},
        {QString::fromLatin1(peep), 48, 16, ByteOrder::Intel, false, 0.1, 0.0, 0.0, 60.0, QStringLiteral("cmH2O")}
    }});

    database.insert(CanMessage{0x122, QStringLiteral("VentVolumes"), 8, {
        {QString::fromLatin1(tidalVolumeExpired), 0, 16, ByteOrder::Intel, false, 0.5, 0.0, 0.0, 3000.0, QStringLiteral("mL")},
        {QString::fromLatin1(minuteVolume), 16, 16, ByteOrder::Intel, false, 0.01, 0.0, 0.0, 60.0, QStringLiteral("L/min")},
        {QString::fromLatin1(respiratoryRate), 32, 8, ByteOrder::Intel, false, 0.5, 0.0, 0.0, 120.0, QStringLiteral("b/min")},
        {QString::fromLatin1(leakPercent), 40, 8, ByteOrder::Intel, false, 0.5, 0.0, 0.0, 100.0, QStringLiteral("%")},
        {QString::fromLatin1(compliance), 48, 16, ByteOrder::Intel, false, 0.1, 0.0, 0.0, 300.0, QStringLiteral("mL/cmH2O")}
    }});

    database.insert(CanMessage{0x123, QStringLiteral("VentGas"), 8, {
        {QString::fromLatin1(fio2), 0, 16, ByteOrder::Intel, false, 0.1, 0.0, 0.0, 100.0, QStringLiteral("%")},
        {QString::fromLatin1(spo2), 16, 8, ByteOrder::Intel, false, 1.0, 0.0, 0.0, 100.0, QStringLiteral("%")},
        {QString::fromLatin1(etco2), 24, 16, ByteOrder::Intel, false, 0.1, 0.0, 0.0, 150.0, QStringLiteral("mmHg")},
        {QString::fromLatin1(resistance), 40, 16, ByteOrder::Intel, false, 0.1, 0.0, 0.0, 200.0, QStringLiteral("cmH2O/(L/s)")}
    }});

    // 0x124 - device status. deviceFault is a bit field; the alarm layer maps
    // each bit to a technical alarm condition.
    database.insert(CanMessage{0x124, QStringLiteral("VentStatus"), 8, {
        {QString::fromLatin1(batteryPercent), 0, 8, ByteOrder::Intel, false, 1.0, 0.0, 0.0, 100.0, QStringLiteral("%")},
        {QString::fromLatin1(deviceFault), 8, 32, ByteOrder::Intel, false, 1.0, 0.0, 0.0, 4294967295.0, QString()},
        {QString::fromLatin1(deviceState), 40, 8, ByteOrder::Intel, false, 1.0, 0.0, 0.0, 255.0, QString()}
    }});

    // 0x200 and 0x201 travel from the interface to the device: the current
    // setpoints, and a single command word for the actions that are not a
    // value. The device echoes the setpoints it accepted on 0x121 onwards,
    // so the interface never assumes a command took effect.
    database.insert(CanMessage{0x200, QStringLiteral("VentSetpoints"), 8, {
        {QString::fromLatin1(setFio2), 0, 8, ByteOrder::Intel, false, 1.0, 0.0, 21.0, 100.0, QStringLiteral("%")},
        {QString::fromLatin1(setPeep), 8, 8, ByteOrder::Intel, false, 1.0, 0.0, 0.0, 50.0, QStringLiteral("cmH2O")},
        {QString::fromLatin1(setRate), 16, 8, ByteOrder::Intel, false, 1.0, 0.0, 0.0, 120.0, QStringLiteral("b/min")},
        {QString::fromLatin1(setTidalVolume), 24, 16, ByteOrder::Intel, false, 1.0, 0.0, 0.0, 3000.0, QStringLiteral("mL")},
        {QString::fromLatin1(setPressureSupport), 40, 8, ByteOrder::Intel, false, 1.0, 0.0, 0.0, 60.0, QStringLiteral("cmH2O")},
        {QString::fromLatin1(setInspiratoryTime), 48, 8, ByteOrder::Intel, false, 0.1, 0.0, 0.0, 10.0, QStringLiteral("s")},
        {QString::fromLatin1(setTrigger), 56, 8, ByteOrder::Intel, false, 0.5, 0.0, 0.0, 40.0, QStringLiteral("L/min")}
    }});

    database.insert(CanMessage{0x201, QStringLiteral("VentCommand"), 8, {
        {QString::fromLatin1(command), 0, 8, ByteOrder::Intel, false, 1.0, 0.0, 0.0, 255.0, QString()}
    }});

    return database;
}

CanDatabase CanDatabase::fromDbc(const QByteArray &contents, QString *errorMessage)
{
    static const QRegularExpression messageLine(
        QStringLiteral("^BO_\\s+(\\d+)\\s+([A-Za-z0-9_]+)\\s*:\\s*(\\d+)"));
    static const QRegularExpression signalLine(
        QStringLiteral("^\\s*SG_\\s+([A-Za-z0-9_]+)\\s*:\\s*"
                       "(\\d+)\\|(\\d+)@([01])([+-])\\s*"
                       "\\(([^,]+),([^\\)]+)\\)\\s*"
                       "\\[([^|]*)\\|([^\\]]*)\\]\\s*\"([^\"]*)\""));

    CanDatabase database;
    CanMessage current;
    bool haveMessage = false;
    int lineNumber = 0;

    const QList<QByteArray> lines = contents.split('\n');
    for (const QByteArray &rawLine : lines) {
        ++lineNumber;
        const QString line = QString::fromUtf8(rawLine).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        const auto messageMatch = messageLine.match(line);
        if (messageMatch.hasMatch()) {
            if (haveMessage)
                database.insert(current);
            current = CanMessage{};
            current.frameId = messageMatch.captured(1).toUInt();
            current.name = messageMatch.captured(2);
            current.byteLength = messageMatch.captured(3).toInt();
            haveMessage = true;
            continue;
        }

        const auto signalMatch = signalLine.match(line);
        if (signalMatch.hasMatch()) {
            if (!haveMessage) {
                if (errorMessage != nullptr)
                    *errorMessage = QStringLiteral("line %1: SG_ before any BO_").arg(lineNumber);
                continue;
            }
            CanSignal entry;
            entry.name = signalMatch.captured(1);
            entry.startBit = signalMatch.captured(2).toInt();
            entry.bitLength = signalMatch.captured(3).toInt();
            entry.byteOrder = signalMatch.captured(4) == QLatin1String("1")
                                  ? ByteOrder::Intel : ByteOrder::Motorola;
            entry.isSigned = signalMatch.captured(5) == QLatin1String("-");
            entry.factor = signalMatch.captured(6).trimmed().toDouble();
            entry.offset = signalMatch.captured(7).trimmed().toDouble();
            entry.minimum = signalMatch.captured(8).trimmed().toDouble();
            entry.maximum = signalMatch.captured(9).trimmed().toDouble();
            entry.unit = signalMatch.captured(10);
            current.signalList.append(entry);
        }
    }

    if (haveMessage)
        database.insert(current);

    if (database.messageCount() == 0 && errorMessage != nullptr && errorMessage->isEmpty())
        *errorMessage = QStringLiteral("no BO_ message definitions found");

    return database;
}

} // namespace sv::transport
