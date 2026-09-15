// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/services/AlarmEvaluator.h>

#include <QtMath>

namespace sv::services {

sv::domain::AlarmState AlarmEvaluator::evaluate(const sv::domain::ClinicalMetrics &metrics,
                                                const sv::domain::AlarmLimits &limits,
                                                const sv::domain::SetpointSet &setpoints,
                                                bool running) const
{
    using namespace sv::domain;

    AlarmState result;

    if (metrics.patientDisconnected) {
        result.active = true;
        result.priority = AlarmPriority::Critical;
        result.headline = QStringLiteral("Patient Disconnect");
        result.detail = QStringLiteral("Check circuit and patient");
        return result;
    }

    if (metrics.circuitOcclusion) {
        result.active = true;
        result.priority = AlarmPriority::Critical;
        result.headline = QStringLiteral("Circuit Occlusion");
        result.detail = QStringLiteral("Check tubing and filters");
        return result;
    }

    if (metrics.ppeak > limits.highPressure) {
        result.active = true;
        result.priority = AlarmPriority::Critical;
        result.headline = QStringLiteral("High Pressure");
        result.detail = QStringLiteral("Paw above limit");
        return result;
    }

    if (setpoints.minuteVolume > limits.highMv * 10) {
        result.active = true;
        result.priority = AlarmPriority::Critical;
        result.headline = QStringLiteral("High Minute Volume");
        result.detail = QStringLiteral("CT Low");
        return result;
    }

    if (metrics.spo2 < limits.lowSpo2 && metrics.spo2 > 0) {
        result.active = true;
        result.priority = AlarmPriority::Warning;
        result.headline = QStringLiteral("Low SpO2");
        result.detail = QStringLiteral("Oxygen saturation below 90%");
        return result;
    }

    if (metrics.etco2 > 50) {
        result.active = true;
        result.priority = AlarmPriority::Warning;
        result.headline = QStringLiteral("High EtCO2");
        result.detail = QStringLiteral("End-tidal CO2 elevated");
        return result;
    }

    if (metrics.highFio2Minutes > 120 && setpoints.fio2 > 60) {
        result.active = true;
        result.priority = AlarmPriority::Warning;
        result.headline = QStringLiteral("O2 Toxicity Risk");
        result.detail = QStringLiteral("Prolonged high FiO2 exposure");
        return result;
    }

    if (metrics.drivingPressure > 15.0) {
        result.active = true;
        result.priority = AlarmPriority::Warning;
        result.headline = QStringLiteral("High Driving Pressure");
        result.detail = QStringLiteral("Lung injury risk -- reduce Vt or increase PEEP");
        return result;
    }

    result.active = false;
    result.priority = AlarmPriority::Info;
    result.headline = QStringLiteral("No Active Alarms");
    result.detail = QStringLiteral("System normal");
    return result;
}

} // namespace sv::services
