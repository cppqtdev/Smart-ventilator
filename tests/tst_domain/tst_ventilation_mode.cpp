// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/domain/VentilationMode.h>

#include <QTest>

using namespace sv::domain;

class TestVentilationMode : public QObject
{
    Q_OBJECT

private slots:
    void toString_asv();
    void toString_vcv();
    void toString_allModes();
    void fromString_pcv();
    void fromString_invalid();
    void isSupported_allModes();
    void supportedModeNames_count();
};

void TestVentilationMode::toString_asv()
{
    QCOMPARE(toString(VentilationMode::ASV), QStringLiteral("ASV"));
}

void TestVentilationMode::toString_vcv()
{
    QCOMPARE(toString(VentilationMode::VCV), QStringLiteral("VCV"));
}

void TestVentilationMode::toString_allModes()
{
    QCOMPARE(toString(VentilationMode::PCV),   QStringLiteral("PCV"));
    QCOMPARE(toString(VentilationMode::SIMV),  QStringLiteral("SIMV"));
    QCOMPARE(toString(VentilationMode::CPAP),  QStringLiteral("CPAP"));
    QCOMPARE(toString(VentilationMode::BiPAP), QStringLiteral("BiPAP"));
    QCOMPARE(toString(VentilationMode::PRVC),  QStringLiteral("PRVC"));
    QCOMPARE(toString(VentilationMode::PSV),   QStringLiteral("PSV"));
}

void TestVentilationMode::fromString_pcv()
{
    QCOMPARE(fromString(QStringLiteral("PCV")), VentilationMode::PCV);
}

void TestVentilationMode::fromString_invalid()
{
    QCOMPARE(fromString(QStringLiteral("INVALID")), VentilationMode::ASV);
}

void TestVentilationMode::isSupported_allModes()
{
    QVERIFY(isSupported(VentilationMode::ASV));
    QVERIFY(isSupported(VentilationMode::VCV));
    QVERIFY(isSupported(VentilationMode::PCV));
    QVERIFY(isSupported(VentilationMode::SIMV));
    QVERIFY(isSupported(VentilationMode::CPAP));
    QVERIFY(isSupported(VentilationMode::BiPAP));
    QVERIFY(isSupported(VentilationMode::PRVC));
    QVERIFY(isSupported(VentilationMode::PSV));
}

void TestVentilationMode::supportedModeNames_count()
{
    QStringList names = supportedModeNames();
    QCOMPARE(names.size(), 8);
    QVERIFY(names.contains(QStringLiteral("VCV")));
    QVERIFY(names.contains(QStringLiteral("BiPAP")));
}

QTEST_MAIN(TestVentilationMode)
#include "tst_ventilation_mode.moc"
