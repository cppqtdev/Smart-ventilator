// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/services/BreathSimulator.h>

#include <algorithm>
#include <cmath>

namespace sv::services {

namespace {
double clampDouble(double value, double low, double high)
{
    return std::max(low, std::min(high, value));
}
} // namespace

BreathPhase BreathSimulator::advancePhase(double currentPhase, double dt, int respiratoryRate, int inspiratoryTime)
{
    const double rr = clampDouble(respiratoryRate, 6.0, 45.0);
    double phase = std::fmod(currentPhase + dt * rr / 60.0, 1.0);

    const double inspiratoryFraction = clampDouble(
        0.28 + inspiratoryTime * 0.05, 0.24, 0.46);
    const bool inspiration = phase < inspiratoryFraction;
    const double normalized = inspiration
        ? phase / inspiratoryFraction
        : (phase - inspiratoryFraction) / (1.0 - inspiratoryFraction);

    return BreathPhase{phase, inspiration, normalized, inspiratoryFraction};
}

} // namespace sv::services
