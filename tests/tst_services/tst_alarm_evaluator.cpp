// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/services/AlarmEvaluator.h>
#include <sv/domain/AlarmDefinition.h>
#include <sv/domain/ClinicalMetrics.h>
#include <sv/domain/Setpoint.h>

#include <QTest>

using namespace sv::services;
using namespace sv::domain;
// QCOMPARE renders whatever it is given as a char*. Argument dependent
// lookup finds sv::domain::toString for this type, which returns a QString,
// and QTest's generic fallback cannot turn one of those into a char*, so the
// test would not compile at all. This is that rendering, and it is what
// QCOMPARE prints when the comparison fails.
namespace QTest {
template <>
inline char *toString(const sv::domain::AlarmPriority &value)
{
    return qstrdup(sv::domain::toString(value).toUtf8().constData());
}
} // namespace QTest


class TestAlarmEvaluator : public QObject
{
    Q_OBJECT

private slots:
    void normalMetrics_noAlarm();
    void highPressure_critical();
    void lowSpo2_warning();
    void patientDisconnected_critical();
    void circuitOcclusion_critical();
    void priorityOrdering();
};

void TestAlarmEvaluator::normalMetrics_noAlarm()
{
    AlarmEvaluator eval;
    ClinicalMetrics m;
    m.ppeak = 25;
    m.spo2  = 96;
    m.etco2 = 38;
    m.drivingPressure = 10;

    SetpointSet sp = defaultSetpoints();
    AlarmLimits lim = defaultAlarmLimits();

    AlarmState state = eval.evaluate(m, lim, sp, true);
    QVERIFY(!state.active);
    QCOMPARE(state.headline, QStringLiteral("No Active Alarms"));
}

void TestAlarmEvaluator::highPressure_critical()
{
    AlarmEvaluator eval;
    ClinicalMetrics m;
    m.ppeak = 50; // above highPressure default of 40
    m.spo2  = 96;

    AlarmState state = eval.evaluate(m, defaultAlarmLimits(), defaultSetpoints(), true);
    QVERIFY(state.active);
    QCOMPARE(state.priority, AlarmPriority::Critical);
    QCOMPARE(state.headline, QStringLiteral("High Pressure"));
}

void TestAlarmEvaluator::lowSpo2_warning()
{
    AlarmEvaluator eval;
    ClinicalMetrics m;
    m.ppeak = 20;
    m.spo2  = 85; // below lowSpo2 default of 90
    m.drivingPressure = 10;

    AlarmState state = eval.evaluate(m, defaultAlarmLimits(), defaultSetpoints(), true);
    QVERIFY(state.active);
    QCOMPARE(state.priority, AlarmPriority::Warning);
    QCOMPARE(state.headline, QStringLiteral("Low SpO2"));
}

void TestAlarmEvaluator::patientDisconnected_critical()
{
    AlarmEvaluator eval;
    ClinicalMetrics m;
    m.patientDisconnected = true;
    m.ppeak = 50; // also high pressure, but disconnect takes priority

    AlarmState state = eval.evaluate(m, defaultAlarmLimits(), defaultSetpoints(), true);
    QVERIFY(state.active);
    QCOMPARE(state.priority, AlarmPriority::Critical);
    QCOMPARE(state.headline, QStringLiteral("Patient Disconnect"));
}

void TestAlarmEvaluator::circuitOcclusion_critical()
{
    AlarmEvaluator eval;
    ClinicalMetrics m;
    m.circuitOcclusion = true;

    AlarmState state = eval.evaluate(m, defaultAlarmLimits(), defaultSetpoints(), true);
    QVERIFY(state.active);
    QCOMPARE(state.priority, AlarmPriority::Critical);
    QCOMPARE(state.headline, QStringLiteral("Circuit Occlusion"));
}

void TestAlarmEvaluator::priorityOrdering()
{
    AlarmEvaluator eval;
    ClinicalMetrics m;
    m.patientDisconnected = true;
    m.circuitOcclusion = true;
    m.ppeak = 50;

    // Disconnect has highest priority
    AlarmState state = eval.evaluate(m, defaultAlarmLimits(), defaultSetpoints(), true);
    QCOMPARE(state.headline, QStringLiteral("Patient Disconnect"));

    // Remove disconnect, occlusion is next
    m.patientDisconnected = false;
    state = eval.evaluate(m, defaultAlarmLimits(), defaultSetpoints(), true);
    QCOMPARE(state.headline, QStringLiteral("Circuit Occlusion"));

    // Remove occlusion, high pressure is next
    m.circuitOcclusion = false;
    state = eval.evaluate(m, defaultAlarmLimits(), defaultSetpoints(), true);
    QCOMPARE(state.headline, QStringLiteral("High Pressure"));
}

QTEST_MAIN(TestAlarmEvaluator)
#include "tst_alarm_evaluator.moc"
