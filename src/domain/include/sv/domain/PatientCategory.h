// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QString>
#include <QStringList>

namespace sv::domain {

/**
 * @brief Patient size class.
 *
 * This is the single most load-bearing setting in the device. It selects the
 * limit tier for every ventilation parameter, the alarm defaults, and how body
 * weight is obtained - which differs by design, not by accident:
 *
 *  - Adult and paediatric: **body height** is entered and predicted body
 *    weight is computed from it (ARDSNet), because tidal volume targets are
 *    defined against PBW and PBW tracks height, not scale weight.
 *  - Neonatal: **body weight** is entered directly. The ARDSNet relation is
 *    not validated below 152.4 cm and goes negative at extreme short stature,
 *    so applying it to a neonate is a dosing error, not an approximation.
 *
 * Dräger's V500 workflow makes the same split, and miscalculated PBW is a
 * documented cause of unsafe ventilator settings (Chase et al., PMC5606514).
 */
enum class PatientCategory {
    Adult = 0,
    Pediatric = 1,
    Neonatal = 2
};

/// Number of categories - the array extent used by the limit tables.
inline constexpr int patientCategoryCount = 3;

/// Index into the per-category limit arrays.
inline constexpr int categoryIndex(PatientCategory category)
{
    return static_cast<int>(category);
}

/// Display name, e.g. "Adult".
QString toString(PatientCategory category);

/// Parses "Adult" / "Pediatric" / "Paediatric" / "Neonatal" / "Neo".
/// Returns Adult for unrecognised input, which is the safest default for
/// limit *width* but must never be relied on silently - validate the input.
PatientCategory patientCategoryFromString(const QString &text);

/// True when weight is entered directly rather than derived from height.
bool usesDirectWeight(PatientCategory category);

/// All category names, in display order.
QStringList patientCategoryNames();

/**
 * @brief ARDSNet predicted body weight in kilograms.
 *
 *   Male:   PBW = 50.0 + 0.91 * (height_cm - 152.4)
 *   Female: PBW = 45.5 + 0.91 * (height_cm - 152.4)
 *
 * Numerically identical to Devine IBW for adults; the names differ by
 * context. Not validated below 152.4 cm, so the result is clamped to a
 * physiologically sane floor rather than allowed to go negative.
 *
 * @param heightCm Standing height in centimetres.
 * @param female   True for the female relation.
 */
double predictedBodyWeightKg(double heightCm, bool female);

/**
 * @brief Tidal volume target range in mL/kg for a category.
 *
 * Adult and paediatric dose against PBW; neonatal doses against actual body
 * weight. Lung-protective targets are narrower than these - see
 * lungProtectiveTidalVolumePerKg().
 */
void tidalVolumePerKgRange(PatientCategory category, double *lowOut, double *highOut);

/// ARDS / PARDS lung-protective target in mL/kg for the category.
double lungProtectiveTidalVolumePerKg(PatientCategory category);

} // namespace sv::domain
