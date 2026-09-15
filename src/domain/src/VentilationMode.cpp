// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include "sv/domain/VentilationMode.h"

namespace sv::domain {

QString toString(VentilationMode mode)
{
    switch (mode) {
    case VentilationMode::ASV:   return QStringLiteral("ASV");
    case VentilationMode::VCV:   return QStringLiteral("VCV");
    case VentilationMode::PCV:   return QStringLiteral("PCV");
    case VentilationMode::SIMV:  return QStringLiteral("SIMV");
    case VentilationMode::CPAP:  return QStringLiteral("CPAP");
    case VentilationMode::BiPAP: return QStringLiteral("BiPAP");
    case VentilationMode::PRVC:  return QStringLiteral("PRVC");
    case VentilationMode::PSV:   return QStringLiteral("PSV");
    }
    return QStringLiteral("ASV");
}

VentilationMode fromString(const QString &text)
{
    if (text == QStringLiteral("ASV"))   return VentilationMode::ASV;
    if (text == QStringLiteral("VCV"))   return VentilationMode::VCV;
    if (text == QStringLiteral("PCV"))   return VentilationMode::PCV;
    if (text == QStringLiteral("SIMV"))  return VentilationMode::SIMV;
    if (text == QStringLiteral("CPAP"))  return VentilationMode::CPAP;
    if (text == QStringLiteral("BiPAP")) return VentilationMode::BiPAP;
    if (text == QStringLiteral("PRVC"))  return VentilationMode::PRVC;
    if (text == QStringLiteral("PSV"))   return VentilationMode::PSV;
    return VentilationMode::ASV;
}

bool isSupported(VentilationMode mode)
{
    switch (mode) {
    case VentilationMode::ASV:
    case VentilationMode::VCV:
    case VentilationMode::PCV:
    case VentilationMode::SIMV:
    case VentilationMode::CPAP:
    case VentilationMode::BiPAP:
    case VentilationMode::PRVC:
    case VentilationMode::PSV:
        return true;
    }
    return false;
}

QStringList supportedModeNames()
{
    return {
        QStringLiteral("ASV"),
        QStringLiteral("VCV"),
        QStringLiteral("PCV"),
        QStringLiteral("SIMV"),
        QStringLiteral("CPAP"),
        QStringLiteral("BiPAP"),
        QStringLiteral("PRVC"),
        QStringLiteral("PSV")
    };
}

} // namespace sv::domain
