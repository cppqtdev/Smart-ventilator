// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/common/AppIdentity.h>

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace sv::common {

void applyApplicationIdentity()
{
    QCoreApplication::setOrganizationName(identity::organizationName());
    QCoreApplication::setApplicationName(identity::applicationName());
}

QString applicationDataDirectory()
{
    // AppDataLocation is derived from the names above, so a caller that
    // reaches this before applyApplicationIdentity would be handed a
    // directory belonging to nobody. Setting them again here is cheap and
    // makes the order of calls in main something that cannot go wrong.
    if (QCoreApplication::organizationName() != identity::organizationName()
        || QCoreApplication::applicationName() != identity::applicationName()) {
        applyApplicationIdentity();
    }

    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directory.isEmpty() || !QDir().mkpath(directory))
        return {};

    return directory;
}

} // namespace sv::common
