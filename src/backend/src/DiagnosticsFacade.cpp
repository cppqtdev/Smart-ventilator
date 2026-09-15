// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/backend/DiagnosticsFacade.h>
#include <sv/services/DiagnosticsService.h>
#include <sv/services/CalibrationService.h>
#include <sv/domain/DiagnosticReport.h>
#include <sv/domain/CalibrationResult.h>

#include <QVariantMap>

namespace sv::backend {

DiagnosticsFacade::DiagnosticsFacade(sv::services::DiagnosticsService *diagnostics,
                                     sv::services::CalibrationService *calibration,
                                     QObject *parent)
    : QObject(parent)
    , m_diagnostics(diagnostics)
    , m_calibration(calibration)
{
    if (m_diagnostics) {
        connect(m_diagnostics, &sv::services::DiagnosticsService::sensorStatusChanged,
                this, &DiagnosticsFacade::sensorStatusChanged);
    }
    if (m_calibration) {
        connect(m_calibration, &sv::services::CalibrationService::resultsChanged,
                this, &DiagnosticsFacade::testResultsChanged);
    }
}

QVariantList DiagnosticsFacade::sensorStatus() const
{
    QVariantList result;
    if (!m_diagnostics)
        return result;
    const auto sensors = m_diagnostics->sensorStatus();
    for (const auto &s : sensors) {
        QVariantMap entry;
        entry[QStringLiteral("name")] = s.name;
        entry[QStringLiteral("online")] = s.online;
        entry[QStringLiteral("lastChecked")] = s.lastChecked;
        result.append(entry);
    }
    return result;
}

QVariantList DiagnosticsFacade::testResults() const
{
    QVariantList result;
    if (!m_calibration)
        return result;
    const auto tests = m_calibration->results();
    for (const auto &t : tests) {
        QVariantMap entry;
        entry[QStringLiteral("testName")] = t.testName;
        entry[QStringLiteral("timestamp")] = t.timestamp;
        entry[QStringLiteral("details")] = t.details;
        QString statusStr;
        switch (t.status) {
        case sv::domain::CalibrationStatus::Passed:     statusStr = QStringLiteral("Passed"); break;
        case sv::domain::CalibrationStatus::Failed:     statusStr = QStringLiteral("Failed"); break;
        case sv::domain::CalibrationStatus::NotRun:     statusStr = QStringLiteral("Not Run"); break;
        case sv::domain::CalibrationStatus::InProgress: statusStr = QStringLiteral("In Progress"); break;
        }
        entry[QStringLiteral("status")] = statusStr;
        result.append(entry);
    }
    return result;
}

bool DiagnosticsFacade::allTestsPassed() const
{
    return m_calibration ? m_calibration->allPassed() : false;
}

void DiagnosticsFacade::runTest(const QString &testName)
{
    if (m_calibration)
        m_calibration->runTest(testName);
}

void DiagnosticsFacade::runAllTests()
{
    if (m_calibration)
        m_calibration->runAllTests();
}

void DiagnosticsFacade::toggleSensor(const QString &name)
{
    if (m_diagnostics)
        m_diagnostics->toggleSensor(name);
}

void DiagnosticsFacade::runSelfTest()
{
    if (m_diagnostics)
        m_diagnostics->runSelfTest();
    emit sensorStatusChanged();
    emit testResultsChanged();
}

} // namespace sv::backend
