// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace sv::services {
class DiagnosticsService;
class CalibrationService;
}

namespace sv::backend {

class DiagnosticsFacade : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_DISABLE_COPY_MOVE(DiagnosticsFacade)

    Q_PROPERTY(QVariantList sensorStatus READ sensorStatus NOTIFY sensorStatusChanged)
    Q_PROPERTY(QVariantList testResults READ testResults NOTIFY testResultsChanged)
    Q_PROPERTY(bool allTestsPassed READ allTestsPassed NOTIFY testResultsChanged)

public:
    explicit DiagnosticsFacade(sv::services::DiagnosticsService *diagnostics,
                               sv::services::CalibrationService *calibration,
                               QObject *parent = nullptr);

    QVariantList sensorStatus() const;
    QVariantList testResults() const;
    bool allTestsPassed() const;

    Q_INVOKABLE void runTest(const QString &testName);
    Q_INVOKABLE void runAllTests();
    Q_INVOKABLE void toggleSensor(const QString &name);
    Q_INVOKABLE void runSelfTest();

signals:
    void sensorStatusChanged();
    void testResultsChanged();

private:
    sv::services::DiagnosticsService *m_diagnostics = nullptr;
    sv::services::CalibrationService *m_calibration = nullptr;
};

} // namespace sv::backend
