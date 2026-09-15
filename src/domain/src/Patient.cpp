// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include "sv/domain/Patient.h"

#include <QtMath>
#include <algorithm>

namespace sv::domain {

int idealBodyWeight(const Patient &p)
{
    if (p.category == QStringLiteral("Neonatal"))
        return std::clamp(p.weight, 1, 8);

    if (p.category == QStringLiteral("Pediatric"))
        return std::clamp(p.weight, 3, 60);

    // Devine formula (1974) for adult ideal body weight
    const double base = (p.gender == QStringLiteral("Male")) ? 50.0 : 45.5;
    return std::clamp(qRound(base + 0.91 * (p.height - 152.4)), 20, 160);
}

int recommendedTidalVolume(const Patient &p)
{
    const int ibw = idealBodyWeight(p);

    if (p.category == QStringLiteral("Neonatal"))
        return std::clamp(p.weight * 6, 20, 60);

    if (p.category == QStringLiteral("Pediatric"))
        return std::clamp(ibw * 7, 30, 450);

    return std::clamp(ibw * 6, 150, 900);
}

int recommendedRate(const Patient &p)
{
    if (p.category == QStringLiteral("Neonatal"))
        return 36;
    if (p.category == QStringLiteral("Pediatric"))
        return 24;
    return 16;
}

namespace {

// The floor and the ceiling come from different terms - a per-kilogram figure
// against a fixed category limit - so a category paired with a body weight it
// does not belong to used to give a floor above the ceiling. Callers then fed
// an inverted range to qBound, which asserts. The ceiling is reconciled with
// the floor here, once, so no caller has to know.
int ceilingVt(const Patient &p, int ibw)
{
    if (p.category == QStringLiteral("Neonatal"))
        return qMin(80, qMax(12, ibw * 8));
    if (p.category == QStringLiteral("Pediatric"))
        return qMin(500, qMax(40, ibw * 10));
    return qMin(900, qMax(160, ibw * 10));
}

} // namespace

int categoryMinVt(const Patient &p)
{
    const int ibw = idealBodyWeight(p);

    int floorMl = 150;
    int perKilogram = ibw * 4;
    if (p.category == QStringLiteral("Neonatal")) {
        floorMl = 10;
    } else if (p.category == QStringLiteral("Pediatric")) {
        floorMl = 30;
        perKilogram = ibw * 5;
    }
    return qMin(qMax(floorMl, perKilogram), ceilingVt(p, ibw));
}

int categoryMaxVt(const Patient &p)
{
    return qMax(categoryMinVt(p), ceilingVt(p, idealBodyWeight(p)));
}

int categoryMinRr(const Patient &p)
{
    if (p.category == QStringLiteral("Neonatal"))
        return 20;
    if (p.category == QStringLiteral("Pediatric"))
        return 10;
    return 4;
}

int categoryMaxRr(const Patient &p)
{
    int ceiling = 35;
    if (p.category == QStringLiteral("Neonatal"))
        ceiling = 80;
    else if (p.category == QStringLiteral("Pediatric"))
        ceiling = 50;
    return qMax(categoryMinRr(p), ceiling);
}

} // namespace sv::domain
