// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/domain/PatientCategory.h>

#include <algorithm>

namespace sv::domain {

QString toString(PatientCategory category)
{
    switch (category) {
    case PatientCategory::Adult:     return QStringLiteral("Adult");
    case PatientCategory::Pediatric: return QStringLiteral("Pediatric");
    case PatientCategory::Neonatal:  return QStringLiteral("Neonatal");
    }
    return QStringLiteral("Adult");
}

PatientCategory patientCategoryFromString(const QString &text)
{
    const QString key = text.trimmed().toLower();
    if (key == QLatin1String("pediatric") || key == QLatin1String("paediatric")
        || key == QLatin1String("ped") || key == QLatin1String("child")
        || key == QLatin1String("infant"))
        return PatientCategory::Pediatric;
    if (key == QLatin1String("neonatal") || key == QLatin1String("neonate")
        || key == QLatin1String("neo") || key == QLatin1String("newborn"))
        return PatientCategory::Neonatal;
    return PatientCategory::Adult;
}

bool usesDirectWeight(PatientCategory category)
{
    return category == PatientCategory::Neonatal;
}

QStringList patientCategoryNames()
{
    return { QStringLiteral("Adult"),
             QStringLiteral("Pediatric"),
             QStringLiteral("Neonatal") };
}

double predictedBodyWeightKg(double heightCm, bool female)
{
    // ARDSNet, metric form. Identical to Devine IBW for adults.
    const double base = female ? 45.5 : 50.0;
    const double pbw = base + 0.91 * (heightCm - 152.4);

    // The relation is not validated below 152.4 cm and goes negative at
    // extreme short stature. Clamping here rather than at the call site means
    // no caller can accidentally dose a tidal volume against a negative
    // weight - which is the failure mode the PBW-accuracy literature reports.
    return std::clamp(pbw, 3.0, 200.0);
}

void tidalVolumePerKgRange(PatientCategory category, double *lowOut, double *highOut)
{
    double low = 6.0;
    double high = 8.0;
    switch (category) {
    case PatientCategory::Adult:
    case PatientCategory::Pediatric:
        low = 6.0;
        high = 8.0;
        break;
    case PatientCategory::Neonatal:
        // Dosed against ACTUAL body weight, not PBW.
        low = 4.0;
        high = 6.0;
        break;
    }
    if (lowOut)
        *lowOut = low;
    if (highOut)
        *highOut = high;
}

double lungProtectiveTidalVolumePerKg(PatientCategory category)
{
    switch (category) {
    case PatientCategory::Adult:     return 6.0;   // ARDSNet ARMA
    case PatientCategory::Pediatric: return 5.0;   // severe PARDS, 4-6 mL/kg
    case PatientCategory::Neonatal:  return 4.5;
    }
    return 6.0;
}

} // namespace sv::domain
