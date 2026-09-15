// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QString>

namespace sv::common {

/**
 * @brief The organisation and application names the whole device answers to.
 *
 * Every persisted thing on this device is found through these two strings.
 * QSettings keys its store by them, and QStandardPaths::AppDataLocation
 * builds the directory that holds the SQLite database and the log file from
 * them. Two bootstraps setting them differently is two devices sharing one
 * machine: the audit trail splits, and which half an event lands in depends
 * on which binary was started.
 *
 * They were set in three places and disagreed in all three - the qmake
 * bootstrap said AlsonsTechnology/SmartVentilatorDemo, the CMake one said
 * TechCoderHub LLP/SmartVentilator, and AppSettings built its own QSettings
 * with the first pair regardless of either. The names below are the first
 * pair, because that is where the data written so far actually is, and the
 * organisation matches the company the device is badged with.
 */
namespace identity {

inline const QString &organizationName()
{
    static const QString name = QStringLiteral("AlsonsTechnology");
    return name;
}

inline const QString &applicationName()
{
    static const QString name = QStringLiteral("SmartVentilatorDemo");
    return name;
}

} // namespace identity

/**
 * @brief Sets the names on QCoreApplication. Call this first in main, before
 *        anything opens a database, a settings store or a log file.
 */
void applyApplicationIdentity();

/**
 * @return The directory that holds the database and the log, created if it
 *         is missing. Empty when the platform will not give one.
 */
QString applicationDataDirectory();

} // namespace sv::common
