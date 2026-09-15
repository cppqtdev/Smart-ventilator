// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

namespace sv::domain {

struct WaveformSample
{
    double pressure = 0.0;
    double flow     = 0.0;
    double volume   = 0.0;
    double co2      = 0.0;
};

} // namespace sv::domain
