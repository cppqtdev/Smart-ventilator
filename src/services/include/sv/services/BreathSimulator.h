// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

namespace sv::services {

struct BreathPhase
{
    double phase = 0.0;
    bool inspiration = true;
    double normalized = 0.0;
    double inspiratoryFraction = 0.33;
};

class BreathSimulator
{
public:
    /// @brief Advance the breath phase by dt seconds and return the new phase state.
    BreathPhase advancePhase(double currentPhase, double dt, int respiratoryRate, int inspiratoryTime);
};

} // namespace sv::services
