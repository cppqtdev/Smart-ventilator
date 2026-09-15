// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QString>

namespace sv::domain {

struct Patient {
    QString category;
    QString gender;
    int age = 0;
    int height = 170;
    int weight = 70;
    QString patientId;
    QString bedNumber;
    QString physician;
    QString admitDate;
};

/// Ideal body weight using the Devine formula (adults) or actual weight for neonates/pediatrics.
int idealBodyWeight(const Patient &p);

/// Recommended tidal volume in mL (ARDSNet 6-8 mL/kg IBW).
int recommendedTidalVolume(const Patient &p);

/// Recommended respiratory rate in breaths per minute.
int recommendedRate(const Patient &p);

/// Minimum safe tidal volume for the patient category in mL.
int categoryMinVt(const Patient &p);

/// Maximum safe tidal volume for the patient category in mL.
int categoryMaxVt(const Patient &p);

/// Minimum safe respiratory rate for the patient category in bpm.
int categoryMinRr(const Patient &p);

/// Maximum safe respiratory rate for the patient category in bpm.
int categoryMaxRr(const Patient &p);

} // namespace sv::domain
