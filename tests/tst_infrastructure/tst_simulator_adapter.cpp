// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/infrastructure/SimulatorAdapter.h>
#include <sv/domain/Setpoint.h>
#include <sv/domain/WaveformSample.h>
#include <sv/domain/ClinicalMetrics.h>

#include <QTest>

using namespace sv::infrastructure;
using namespace sv::domain;

class TestSimulatorAdapter : public QObject
{
    Q_OBJECT

private slots:
    void tick_returnsNonZeroSample_VCV();
    void tick_returnsNonZeroSample_PCV();
    void tick_returnsNonZeroSample_ASV();
    void tick_returnsNonZeroSample_allModes();
    void computeMetrics_valuesInRange();
    void tick_pressurePositiveDuringInspiration_VCV();
};

void TestSimulatorAdapter::tick_returnsNonZeroSample_VCV()
{
    SimulatorAdapter adapter;
    SetpointSet sp = defaultSetpoints();
    WaveformSample s = adapter.tick(0.02, 0.3, true, 0.5, sp, QStringLiteral("VCV"));

    // At least one channel should be non-zero during inspiration
    bool anyNonZero = (s.pressure != 0.0) || (s.flow != 0.0)
                   || (s.volume != 0.0) || (s.co2 != 0.0);
    QVERIFY(anyNonZero);
}

void TestSimulatorAdapter::tick_returnsNonZeroSample_PCV()
{
    SimulatorAdapter adapter;
    SetpointSet sp = defaultSetpoints();
    WaveformSample s = adapter.tick(0.02, 0.3, true, 0.5, sp, QStringLiteral("PCV"));

    bool anyNonZero = (s.pressure != 0.0) || (s.flow != 0.0)
                   || (s.volume != 0.0) || (s.co2 != 0.0);
    QVERIFY(anyNonZero);
}

void TestSimulatorAdapter::tick_returnsNonZeroSample_ASV()
{
    SimulatorAdapter adapter;
    SetpointSet sp = defaultSetpoints();
    WaveformSample s = adapter.tick(0.02, 0.3, true, 0.5, sp, QStringLiteral("ASV"));

    bool anyNonZero = (s.pressure != 0.0) || (s.flow != 0.0)
                   || (s.volume != 0.0) || (s.co2 != 0.0);
    QVERIFY(anyNonZero);
}

void TestSimulatorAdapter::tick_returnsNonZeroSample_allModes()
{
    const QStringList modes = {
        QStringLiteral("VCV"), QStringLiteral("PCV"), QStringLiteral("SIMV"),
        QStringLiteral("CPAP"), QStringLiteral("BiPAP"), QStringLiteral("PRVC"),
        QStringLiteral("PSV"), QStringLiteral("ASV")
    };
    SetpointSet sp = defaultSetpoints();

    for (const QString &mode : modes) {
        SimulatorAdapter adapter;
        WaveformSample s = adapter.tick(0.02, 0.3, true, 0.5, sp, mode);
        bool anyNonZero = (s.pressure != 0.0) || (s.flow != 0.0)
                       || (s.volume != 0.0) || (s.co2 != 0.0);
        QVERIFY2(anyNonZero, qPrintable(QStringLiteral("Zero sample for mode: %1").arg(mode)));
    }
}

void TestSimulatorAdapter::computeMetrics_valuesInRange()
{
    SimulatorAdapter adapter;
    SetpointSet sp = defaultSetpoints();

    // Run a few ticks to initialise internal state
    for (int i = 0; i < 10; ++i)
        adapter.tick(0.02, double(i) / 10.0, i < 5, double(i) / 10.0, sp, QStringLiteral("VCV"));

    ClinicalMetrics m = adapter.computeMetrics(sp, 10);
    QVERIFY(m.ppeak >= 0 && m.ppeak <= 60);
    QVERIFY(m.spo2 >= 80 && m.spo2 <= 100);
    QVERIFY(m.etco2 >= 15 && m.etco2 <= 60);
    QVERIFY(m.compliance >= 10 && m.compliance <= 100);
    QVERIFY(m.resistance >= 2 && m.resistance <= 30);
}

void TestSimulatorAdapter::tick_pressurePositiveDuringInspiration_VCV()
{
    SimulatorAdapter adapter;
    SetpointSet sp = defaultSetpoints();

    // Generate several samples during inspiration and verify pressure > 0
    for (int i = 0; i < 20; ++i) {
        double normalized = double(i) / 20.0;
        WaveformSample s = adapter.tick(0.02, 0.3, true, normalized, sp, QStringLiteral("VCV"));
        QVERIFY2(s.pressure > 0, qPrintable(
            QStringLiteral("Pressure should be > 0 during inspiration at sample %1, got %2")
                .arg(i).arg(s.pressure)));
    }
}

QTEST_MAIN(TestSimulatorAdapter)
#include "tst_simulator_adapter.moc"
