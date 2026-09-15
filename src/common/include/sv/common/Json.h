// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <optional>
#include <type_traits>

namespace sv::json {

/**
 * @brief Collects every problem found while reading a document.
 *
 * A configuration file is read once at start-up, so failing on the first
 * bad field wastes a whole restart cycle to surface the second one. Reader
 * keeps going and reports the full list.
 */
class Diagnostics
{
public:
    void missing(const QString &path)
    {
        m_errors.append(QStringLiteral("%1: required field is missing").arg(path));
    }

    void wrongType(const QString &path, const QString &expected)
    {
        m_errors.append(QStringLiteral("%1: expected %2").arg(path, expected));
    }

    void outOfRange(const QString &path, double value, double minimum, double maximum)
    {
        m_errors.append(QStringLiteral("%1: %2 is outside [%3, %4]")
                            .arg(path)
                            .arg(value)
                            .arg(minimum)
                            .arg(maximum));
    }

    void custom(const QString &path, const QString &message)
    {
        m_errors.append(QStringLiteral("%1: %2").arg(path, message));
    }

    bool ok() const { return m_errors.isEmpty(); }
    const QStringList &errors() const { return m_errors; }
    QString summary() const { return m_errors.join(QStringLiteral("; ")); }
    void clear() { m_errors.clear(); }

private:
    QStringList m_errors;
};

/**
 * @brief Cursor over one JSON object, with a path for diagnostics.
 *
 * Reader does not throw and does not stop. A field that is absent or of the
 * wrong type leaves the target untouched and records one diagnostic, so the
 * target keeps whatever default the struct gave it.
 */
class Reader
{
public:
    Reader(const QJsonObject &object, Diagnostics &diagnostics, QString path = {})
        : m_object(object)
        , m_diagnostics(&diagnostics)
        , m_path(std::move(path))
    {
    }

    bool has(QLatin1String key) const { return m_object.contains(key); }

    QString pathOf(QLatin1String key) const
    {
        return m_path.isEmpty() ? QString(key) : m_path + QLatin1Char('.') + QString(key);
    }

    Reader child(QLatin1String key, bool required = true) const
    {
        const QJsonValue value = m_object.value(key);
        if (!value.isObject()) {
            if (required) {
                if (value.isUndefined())
                    m_diagnostics->missing(pathOf(key));
                else
                    m_diagnostics->wrongType(pathOf(key), QStringLiteral("an object"));
            }
            return Reader(QJsonObject(), *m_diagnostics, pathOf(key));
        }
        return Reader(value.toObject(), *m_diagnostics, pathOf(key));
    }

    QJsonArray array(QLatin1String key, bool required = true) const
    {
        const QJsonValue value = m_object.value(key);
        if (!value.isArray()) {
            if (required) {
                if (value.isUndefined())
                    m_diagnostics->missing(pathOf(key));
                else
                    m_diagnostics->wrongType(pathOf(key), QStringLiteral("an array"));
            }
            return QJsonArray();
        }
        return value.toArray();
    }

    template <typename T>
    bool read(QLatin1String key, T &target, bool required = true) const
    {
        const QJsonValue value = m_object.value(key);
        if (value.isUndefined() || value.isNull()) {
            if (required)
                m_diagnostics->missing(pathOf(key));
            return false;
        }
        return assign(value, key, target);
    }

    template <typename T>
    bool readClamped(QLatin1String key, T &target, T minimum, T maximum,
                     bool required = true) const
    {
        T candidate = target;
        if (!read(key, candidate, required))
            return false;
        if (candidate < minimum || candidate > maximum) {
            m_diagnostics->outOfRange(pathOf(key), double(candidate),
                                      double(minimum), double(maximum));
            return false;
        }
        target = candidate;
        return true;
    }

    Diagnostics &diagnostics() const { return *m_diagnostics; }
    const QJsonObject &object() const { return m_object; }

private:
    bool assign(const QJsonValue &value, QLatin1String key, bool &target) const
    {
        if (!value.isBool()) {
            m_diagnostics->wrongType(pathOf(key), QStringLiteral("a boolean"));
            return false;
        }
        target = value.toBool();
        return true;
    }

    bool assign(const QJsonValue &value, QLatin1String key, int &target) const
    {
        if (!value.isDouble()) {
            m_diagnostics->wrongType(pathOf(key), QStringLiteral("a number"));
            return false;
        }
        target = value.toInt();
        return true;
    }

    bool assign(const QJsonValue &value, QLatin1String key, double &target) const
    {
        if (!value.isDouble()) {
            m_diagnostics->wrongType(pathOf(key), QStringLiteral("a number"));
            return false;
        }
        target = value.toDouble();
        return true;
    }

    bool assign(const QJsonValue &value, QLatin1String key, QString &target) const
    {
        if (!value.isString()) {
            m_diagnostics->wrongType(pathOf(key), QStringLiteral("a string"));
            return false;
        }
        target = value.toString();
        return true;
    }

    bool assign(const QJsonValue &value, QLatin1String key, QStringList &target) const
    {
        if (!value.isArray()) {
            m_diagnostics->wrongType(pathOf(key), QStringLiteral("an array of strings"));
            return false;
        }
        QStringList collected;
        const QJsonArray entries = value.toArray();
        collected.reserve(entries.size());
        for (const QJsonValue &entry : entries) {
            if (!entry.isString()) {
                m_diagnostics->wrongType(pathOf(key), QStringLiteral("an array of strings"));
                return false;
            }
            collected.append(entry.toString());
        }
        target = collected;
        return true;
    }

    QJsonObject m_object;
    Diagnostics *m_diagnostics;
    QString m_path;
};

/** @brief Parses raw bytes, reporting a syntax error through @p diagnostics. */
inline std::optional<QJsonObject> parseObject(const QByteArray &bytes,
                                              Diagnostics &diagnostics,
                                              const QString &sourceName = {})
{
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError) {
        diagnostics.custom(sourceName.isEmpty() ? QStringLiteral("<json>") : sourceName,
                           QStringLiteral("offset %1: %2")
                               .arg(error.offset)
                               .arg(error.errorString()));
        return std::nullopt;
    }
    if (!document.isObject()) {
        diagnostics.wrongType(sourceName.isEmpty() ? QStringLiteral("<json>") : sourceName,
                              QStringLiteral("an object at the top level"));
        return std::nullopt;
    }
    return document.object();
}

} // namespace sv::json

// ---------------------------------------------------------------------------
//  Field macros
// ---------------------------------------------------------------------------
//
// Declaring a struct's JSON mapping once, next to the struct, keeps the field
// name, the member and the range in one place. Writing them out by hand is
// how a renamed member ends up silently reading nothing.
//
//   struct Limits { double high = 40.0; double low = 5.0; };
//
//   SV_JSON_BEGIN(Limits)
//       SV_JSON_RANGE(high, 0.0, 100.0)
//       SV_JSON_FIELD(low)
//   SV_JSON_END()
//
// which defines: bool readLimits(const sv::json::Reader &, Limits &)

#define SV_JSON_BEGIN(Type)                                                    \
    inline bool read##Type(const ::sv::json::Reader &reader, Type &target)     \
    {                                                                          \
        const int errorsBefore = reader.diagnostics().errors().size();

#define SV_JSON_FIELD(member)                                                  \
        reader.read(QLatin1String(#member), target.member, true);

#define SV_JSON_OPTIONAL(member)                                               \
        reader.read(QLatin1String(#member), target.member, false);

#define SV_JSON_NAMED(key, member)                                             \
        reader.read(QLatin1String(key), target.member, true);

#define SV_JSON_RANGE(member, minimum, maximum)                                \
        reader.readClamped(QLatin1String(#member), target.member,              \
                           decltype(target.member)(minimum),                   \
                           decltype(target.member)(maximum), true);

#define SV_JSON_OPTIONAL_RANGE(member, minimum, maximum)                       \
        reader.readClamped(QLatin1String(#member), target.member,              \
                           decltype(target.member)(minimum),                   \
                           decltype(target.member)(maximum), false);

#define SV_JSON_END()                                                          \
        return reader.diagnostics().errors().size() == errorsBefore;           \
    }
