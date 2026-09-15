// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/services/SettingsValidator.h>
#include <sv/domain/Patient.h>
#include <sv/domain/Setpoint.h>

#include <QTest>

using namespace sv::services;
using namespace sv::domain;

class TestSettingsValidator : public QObject
{
    Q_OBJECT

private slots:
    void validateMode_valid();
    void validateMode_invalid();
    void validateStart_degradedRejects();
    void validateStart_validAccepts();
    void validateSettingEnvelope_peepPsTooClose();
    void validateSettingEnvelope_fio2Above80();
    void validateSettingEnvelope_normalAccepts();
    void validateSettingEnvelope_tidalVolumeOutOfRange();
};

void TestSettingsValidator::validateMode_valid()
{
    SettingsValidator v;
    QString reason;
    QVERIFY(v.validateMode(QStringLiteral("VCV"), &reason));
}

void TestSettingsValidator::validateMode_invalid()
{
    SettingsValidator v;
    QString reason;
    QVERIFY(!v.validateMode(QStringLiteral("INVALID"), &reason));
    QVERIFY(reason.contains(QStringLiteral("Unsupported")));
}

void TestSettingsValidator::validateStart_degradedRejects()
{
    SettingsValidator v;
    Patient patient;
    patient.category = QStringLiteral("Adult");
    patient.gender   = QStringLiteral("Male");
    patient.height   = 178;

    SetpointSet sp = defaultSetpoints();
    AlarmLimits lim = defaultAlarmLimits();
    QString reason;

    QVERIFY(!v.validateStart(sp, lim, true, patient, &reason));
    QVERIFY(reason.contains(QStringLiteral("degraded")));
}

void TestSettingsValidator::validateStart_validAccepts()
{
    SettingsValidator v;
    Patient patient;
    patient.category = QStringLiteral("Adult");
    patient.gender   = QStringLiteral("Male");
    patient.height   = 178;

    SetpointSet sp = defaultSetpoints();
    AlarmLimits lim = defaultAlarmLimits();
    // Ensure PEEP + PS + 5 < highPressure (40)
    sp.peep = 5;
    sp.pressureSupport = 10;
    sp.tidalVolume = 420;
    sp.respiratoryRate = 16;
    sp.fio2 = 60;
    lim.highPressure = 45;
    QString reason;

    QVERIFY(v.validateStart(sp, lim, false, patient, &reason));
}

void TestSettingsValidator::validateSettingEnvelope_peepPsTooClose()
{
    SettingsValidator v;
    Patient patient;
    patient.category = QStringLiteral("Adult");
    patient.gender   = QStringLiteral("Male");
    patient.height   = 178;

    SetpointSet sp = defaultSetpoints();
    sp.peep = 5;
    sp.pressureSupport = 10;
    sp.tidalVolume = 420;
    sp.respiratoryRate = 16;
    AlarmLimits lim = defaultAlarmLimits();
    lim.highPressure = 17; // 5 + 10 = 15 >= 17 - 3 = 14 -> reject
    QString reason;

    QVERIFY(!v.validateSettingEnvelope(QStringLiteral("peep"), 5, sp, lim, patient, &reason));
    QVERIFY(reason.contains(QStringLiteral("too close")));
}

void TestSettingsValidator::validateSettingEnvelope_fio2Above80()
{
    SettingsValidator v;
    Patient patient;
    patient.category = QStringLiteral("Adult");
    patient.gender   = QStringLiteral("Male");
    patient.height   = 178;

    SetpointSet sp = defaultSetpoints();
    sp.peep = 5;
    sp.pressureSupport = 10;
    sp.tidalVolume = 420;
    sp.respiratoryRate = 16;
    AlarmLimits lim = defaultAlarmLimits();
    lim.highPressure = 45;
    QString reason;

    QVERIFY(!v.validateSettingEnvelope(QStringLiteral("fio2"), 85, sp, lim, patient, &reason));
    QVERIFY(reason.contains(QStringLiteral("FiO2")));
}

void TestSettingsValidator::validateSettingEnvelope_normalAccepts()
{
    SettingsValidator v;
    Patient patient;
    patient.category = QStringLiteral("Adult");
    patient.gender   = QStringLiteral("Male");
    patient.height   = 178;

    SetpointSet sp = defaultSetpoints();
    sp.peep = 5;
    sp.pressureSupport = 10;
    sp.tidalVolume = 420;
    sp.respiratoryRate = 16;
    AlarmLimits lim = defaultAlarmLimits();
    lim.highPressure = 45;
    QString reason;

    QVERIFY(v.validateSettingEnvelope(QStringLiteral("fio2"), 60, sp, lim, patient, &reason));
}

void TestSettingsValidator::validateSettingEnvelope_tidalVolumeOutOfRange()
{
    SettingsValidator v;
    Patient patient;
    patient.category = QStringLiteral("Neonatal");
    patient.weight   = 3;

    SetpointSet sp = defaultSetpoints();
    sp.peep = 5;
    sp.pressureSupport = 5;
    sp.respiratoryRate = 30;
    sp.tidalVolume = 20; // base; we test prospective value
    AlarmLimits lim = defaultAlarmLimits();
    lim.highPressure = 45;
    QString reason;

    // Neonatal 3kg: minVt=12, maxVt=24. Setting 50 is out of range.
    QVERIFY(!v.validateSettingEnvelope(QStringLiteral("tidalVolume"), 50, sp, lim, patient, &reason));
    QVERIFY(reason.contains(QStringLiteral("tidal volume")));
}

QTEST_MAIN(TestSettingsValidator)
#include "tst_settings_validator.moc"
