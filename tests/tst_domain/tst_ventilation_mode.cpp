// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/domain/VentilationMode.h>

#include <QTest>

using namespace sv::domain;
// QCOMPARE renders whatever it is given as a char*. Argument dependent
// lookup finds sv::domain::toString for this type, which returns a QString, and
// QTest's generic fallback cannot turn one of those into a char*, so the
// test would not compile at all. This is that rendering, and it is what
// QCOMPARE prints when the comparison fails.
namespace QTest {
template <>
inline char *toString(const sv::domain::VentilationMode &value)
{
    return qstrdup(sv::domain::toString(value).toUtf8().constData());
}
} // namespace QTest



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
