// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QString>
#include <QStringList>

namespace sv::domain {

enum class VentilationMode {
    ASV,
    VCV,
    PCV,
    SIMV,
    CPAP,
    BiPAP,
    PRVC,
    PSV
};

QString toString(VentilationMode mode);

/// Parse a string into a VentilationMode. Returns ASV on unrecognised input.
VentilationMode fromString(const QString &text);

/// True if the mode is in the supported set.
bool isSupported(VentilationMode mode);

/// Names of all supported modes suitable for UI combo boxes.
QStringList supportedModeNames();

} // namespace sv::domain
