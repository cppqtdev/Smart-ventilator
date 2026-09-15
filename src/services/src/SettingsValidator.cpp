// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/services/SettingsValidator.h>

#include <QSet>
#include <QtMath>

namespace sv::services {

bool SettingsValidator::validateMode(const QString &mode, QString *reason) const
{
    static const QSet<QString> supportedModes = {
        QStringLiteral("VCV"), QStringLiteral("PCV"), QStringLiteral("SIMV"),
        QStringLiteral("CPAP"), QStringLiteral("BiPAP"), QStringLiteral("ASV"),
        QStringLiteral("PRVC"), QStringLiteral("PSV")
    };
    if (!supportedModes.contains(mode)) {
        if (reason)
            *reason = QStringLiteral("Unsupported ventilation mode: %1").arg(mode);
        return false;
    }
    return true;
}

bool SettingsValidator::validateStart(const sv::domain::SetpointSet &setpoints,
                                      const sv::domain::AlarmLimits &limits,
                                      bool degradedMode,
                                      const sv::domain::Patient &patient,
                                      QString *reason) const
{
    if (degradedMode) {
        if (reason)
            *reason = QStringLiteral("Cannot start: backend communication is degraded");
        return false;
    }
    if (limits.lowPressure >= limits.highPressure) {
        if (reason)
            *reason = QStringLiteral("Cannot start: pressure alarm limits are invalid");
        return false;
    }
    if (setpoints.peep >= limits.highPressure) {
        if (reason)
            *reason = QStringLiteral("Cannot start: PEEP is above the high pressure alarm limit");
        return false;
    }
    if (setpoints.peep + setpoints.pressureSupport + 5 >= limits.highPressure) {
        if (reason)
            *reason = QStringLiteral("Cannot start: pressure support plus PEEP is too close to high pressure alarm");
        return false;
    }
    if (setpoints.tidalVolume < 20 || setpoints.respiratoryRate < 4 || setpoints.fio2 < 21) {
        if (reason)
            *reason = QStringLiteral("Cannot start: ventilator settings are incomplete");
        return false;
    }
    return validateSettingEnvelope(QStringLiteral("start"), 0, setpoints, limits, patient, reason);
}

bool SettingsValidator::validateSettingEnvelope(const QString &parameter,
                                                int value,
                                                const sv::domain::SetpointSet &setpoints,
                                                const sv::domain::AlarmLimits &limits,
                                                const sv::domain::Patient &patient,
                                                QString *reason) const
{
    const int prospectiveFio2 = parameter == QStringLiteral("fio2") ? value : setpoints.fio2;
    const int prospectivePeep = parameter == QStringLiteral("peep") ? value : setpoints.peep;
    const int prospectivePressureSupport = parameter == QStringLiteral("pressureSupport") ? value : setpoints.pressureSupport;
    const int prospectiveInspiratoryTime = parameter == QStringLiteral("inspiratoryTime") ? value : setpoints.inspiratoryTime;
    const int prospectiveRate = parameter == QStringLiteral("respiratoryRate") ? value : setpoints.respiratoryRate;
    const int prospectiveTidalVolume = parameter == QStringLiteral("tidalVolume") ? value : setpoints.tidalVolume;

    const double cycleSeconds = 60.0 / qMax(1, prospectiveRate);
    if (prospectiveInspiratoryTime >= cycleSeconds * 0.80) {
        if (reason)
            *reason = QStringLiteral("Rejected: inspiratory time is incompatible with respiratory rate");
        return false;
    }

    if (prospectivePeep + prospectivePressureSupport >= limits.highPressure - 3) {
        if (reason)
            *reason = QStringLiteral("Rejected: PEEP + pressure support is too close to high pressure alarm");
        return false;
    }

    const int minVt = sv::domain::categoryMinVt(patient);
    const int maxVt = sv::domain::categoryMaxVt(patient);
    if (prospectiveTidalVolume < minVt || prospectiveTidalVolume > maxVt) {
        if (reason)
            *reason = QStringLiteral("Rejected: tidal volume outside %1 patient safe range (%2-%3 mL)")
                .arg(patient.category).arg(minVt).arg(maxVt);
        return false;
    }

    const int minRr = sv::domain::categoryMinRr(patient);
    const int maxRr = sv::domain::categoryMaxRr(patient);
    if (prospectiveRate < minRr || prospectiveRate > maxRr) {
        if (reason)
            *reason = QStringLiteral("Rejected: respiratory rate outside %1 patient safe range (%2-%3 1/min)")
                .arg(patient.category).arg(minRr).arg(maxRr);
        return false;
    }

    if (prospectiveFio2 > 80 && parameter == QStringLiteral("fio2")) {
        if (reason)
            *reason = QStringLiteral("Rejected: FiO2 above 80% requires high oxygen therapy confirmation workflow");
        return false;
    }

    return true;
}

} // namespace sv::services
