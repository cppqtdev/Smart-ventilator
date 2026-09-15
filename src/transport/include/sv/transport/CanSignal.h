// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>

#include <cstdint>

namespace sv::transport {

enum class ByteOrder { Intel, Motorola };

/**
 * @brief One signal inside a CAN frame, as a DBC file describes it.
 *
 * physical = raw * factor + offset, clamped to [minimum, maximum].
 */
struct CanSignal
{
    QString name;
    int startBit = 0;
    int bitLength = 8;
    ByteOrder byteOrder = ByteOrder::Intel;
    bool isSigned = false;
    double factor = 1.0;
    double offset = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
    QString unit;

    double decode(const QByteArray &payload) const;
    bool encode(QByteArray &payload, double physical) const;
};

/** @brief One CAN message: an identifier and the signals packed into it. */
struct CanMessage
{
    quint32 frameId = 0;
    QString name;
    int byteLength = 8;
    QList<CanSignal> signalList;
};

/**
 * @brief The message set, keyed by frame identifier.
 *
 * Built either from the compiled-in default table or from a DBC file, so a
 * field change on the hardware side is a data change rather than a rebuild.
 */
class CanDatabase
{
public:
    static CanDatabase defaultDatabase();

    /** @brief Parses a subset of DBC: BO_ message and SG_ signal lines. */
    static CanDatabase fromDbc(const QByteArray &contents, QString *errorMessage = nullptr);

    void insert(const CanMessage &message) { m_messages.insert(message.frameId, message); }
    bool contains(quint32 frameId) const { return m_messages.contains(frameId); }
    const CanMessage *message(quint32 frameId) const;

    /** @brief The frame carrying @p signalName, or nullptr. */
    const CanMessage *messageForSignal(const QString &signalName) const;

    QList<quint32> frameIds() const { return m_messages.keys(); }
    int messageCount() const { return int(m_messages.size()); }

private:
    QHash<quint32, CanMessage> m_messages;
};

} // namespace sv::transport
