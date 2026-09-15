// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/domain/Patient.h>

#include <QTest>

using namespace sv::domain;

class TestPatient : public QObject
{
    Q_OBJECT

private slots:
    void idealBodyWeight_adultMale();
    void idealBodyWeight_adultFemale();
    void idealBodyWeight_neonatal();
    void idealBodyWeight_pediatric();
    void recommendedTidalVolume_adult();
    void recommendedTidalVolume_neonatal();
    void categoryMinMaxVt_adult();
    void categoryMinMaxVt_pediatric();
    void categoryMinMaxVt_neonatal();
    void categoryMinMaxRr_adult();
    void categoryMinMaxRr_pediatric();
    void categoryMinMaxRr_neonatal();
};

void TestPatient::idealBodyWeight_adultMale()
{
    Patient p;
    p.category = QStringLiteral("Adult");
    p.gender   = QStringLiteral("Male");
    p.height   = 178;
    int ibw = idealBodyWeight(p);
    // Devine: 50 + 0.91 * (178 - 152.4) = 50 + 23.30 = 73.3 -> 73
    QCOMPARE(ibw, 73);
}

void TestPatient::idealBodyWeight_adultFemale()
{
    Patient p;
    p.category = QStringLiteral("Adult");
    p.gender   = QStringLiteral("Female");
    p.height   = 165;
    int ibw = idealBodyWeight(p);
    // Devine: 45.5 + 0.91 * (165 - 152.4) = 45.5 + 11.47 = 56.97 -> 57
    QCOMPARE(ibw, 57);
}

void TestPatient::idealBodyWeight_neonatal()
{
    Patient p;
    p.category = QStringLiteral("Neonatal");
    p.weight   = 3;
    QCOMPARE(idealBodyWeight(p), 3);
}

void TestPatient::idealBodyWeight_pediatric()
{
    Patient p;
    p.category = QStringLiteral("Pediatric");
    p.weight   = 25;
    QCOMPARE(idealBodyWeight(p), 25);
}

void TestPatient::recommendedTidalVolume_adult()
{
    Patient p;
    p.category = QStringLiteral("Adult");
    p.gender   = QStringLiteral("Male");
    p.height   = 178;
    // IBW = 73, Vt = 73 * 6 = 438
    QCOMPARE(recommendedTidalVolume(p), 438);
}

void TestPatient::recommendedTidalVolume_neonatal()
{
    Patient p;
    p.category = QStringLiteral("Neonatal");
    p.weight   = 3;
    // Neonatal: weight * 6 = 18
    QCOMPARE(recommendedTidalVolume(p), 20); // clamped to min 20
}

void TestPatient::categoryMinMaxVt_adult()
{
    Patient p;
    p.category = QStringLiteral("Adult");
    p.gender   = QStringLiteral("Male");
    p.height   = 178;
    // IBW = 73
    int minVt = categoryMinVt(p);
    int maxVt = categoryMaxVt(p);
    QCOMPARE(minVt, 73 * 4);  // 292
    QCOMPARE(maxVt, 73 * 10); // 730
}

void TestPatient::categoryMinMaxVt_pediatric()
{
    Patient p;
    p.category = QStringLiteral("Pediatric");
    p.weight   = 25;
    int minVt = categoryMinVt(p);
    int maxVt = categoryMaxVt(p);
    QCOMPARE(minVt, 25 * 5);  // 125
    QCOMPARE(maxVt, 25 * 10); // 250
}

void TestPatient::categoryMinMaxVt_neonatal()
{
    Patient p;
    p.category = QStringLiteral("Neonatal");
    p.weight   = 3;
    int minVt = categoryMinVt(p);
    int maxVt = categoryMaxVt(p);
    QCOMPARE(minVt, 12);  // max(10, 3*4=12) = 12
    QCOMPARE(maxVt, 24);  // min(80, 3*8=24) = 24
}

void TestPatient::categoryMinMaxRr_adult()
{
    Patient p;
    p.category = QStringLiteral("Adult");
    QCOMPARE(categoryMinRr(p), 4);
    QCOMPARE(categoryMaxRr(p), 35);
}

void TestPatient::categoryMinMaxRr_pediatric()
{
    Patient p;
    p.category = QStringLiteral("Pediatric");
    QCOMPARE(categoryMinRr(p), 10);
    QCOMPARE(categoryMaxRr(p), 50);
}

void TestPatient::categoryMinMaxRr_neonatal()
{
    Patient p;
    p.category = QStringLiteral("Neonatal");
    QCOMPARE(categoryMinRr(p), 20);
    QCOMPARE(categoryMaxRr(p), 80);
}

QTEST_MAIN(TestPatient)
#include "tst_patient.moc"
