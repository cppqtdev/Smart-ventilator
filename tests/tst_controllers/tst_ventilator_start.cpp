// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application
//
// The start path is the one that failed silently in the field: selecting a
// neonatal patient left adult settings loaded, validateStart() refused, and
// nothing on screen said why.
//
// A start now needs an admitted patient. These tests predate that gate and
// were setting the category and the body weight without admitting anyone,
// which is exactly the state the gate exists to refuse, so every one of them
// was refused. They admit a patient where a start is meant to succeed, and
// startRefusedUntilAPatientIsAdmitted covers the gate itself.

#include "../../src/controllers/VentilatorController.h"

#include <QSignalSpy>
#include <QTest>

class TestVentilatorStart : public QObject
{
    Q_OBJECT

private slots:
    void startRefusedUntilAPatientIsAdmitted();
    void startsFromTheDefaultAdultState();
    void startsAfterSwitchingToEveryCategory_data();
    void startsAfterSwitchingToEveryCategory();
    void categorySwitchPullsTidalVolumeIntoRange_data();
    void categorySwitchPullsTidalVolumeIntoRange();
    void categorySwitchKeepsInspiratoryTimeInsideTheCycle_data();
    void categorySwitchKeepsInspiratoryTimeInsideTheCycle();
    void rejectedStartExplainsItself();
    void rejectedStartEmitsCommandRejected();
    void invalidAlarmLimitsRefuseToStart();
    void startSetsRunning();
    void stopClearsRunning();
    void freezeTogglesWithoutStoppingVentilation();

    void categoryRangesNeverInvert_data();
    void categoryRangesNeverInvert();
    void categoryBeforeWeightDoesNotAssert();
    void setPatientProfileChangesBothAtOnce();
    void everyCategoryAndWeightStarts_data();
    void everyCategoryAndWeightStarts();
};

void TestVentilatorStart::startRefusedUntilAPatientIsAdmitted()
{
    // The gate itself: settings alone are not a patient.
    VentilatorController controller(nullptr, nullptr);
    controller.setPatientProfile(QStringLiteral("Adult"), 73);
    QVERIFY2(!controller.requestStartVentilation(),
             "a start with nobody admitted must be refused");
    QVERIFY(!controller.running());
    QVERIFY(controller.lastCommandMessage().contains(QStringLiteral("admitted")));

    controller.acceptPatient(QStringLiteral("Adult"), 73);
    QVERIFY2(controller.requestStartVentilation(),
             qPrintable(controller.lastCommandMessage()));
}

void TestVentilatorStart::startsFromTheDefaultAdultState()
{
    VentilatorController controller(nullptr, nullptr);
    controller.acceptPatient(QStringLiteral("Adult"), 73);
    QVERIFY2(controller.requestStartVentilation(),
             qPrintable(controller.lastCommandMessage()));
    QVERIFY(controller.running());
}

void TestVentilatorStart::startsAfterSwitchingToEveryCategory_data()
{
    QTest::addColumn<QString>("category");
    QTest::addColumn<int>("ibw");

    QTest::newRow("adult 73 kg")     << QStringLiteral("Adult")     << 73;
    QTest::newRow("adult 45 kg")     << QStringLiteral("Adult")     << 45;
    QTest::newRow("pediatric 20 kg") << QStringLiteral("Pediatric") << 20;
    QTest::newRow("pediatric 12 kg") << QStringLiteral("Pediatric") << 12;
    QTest::newRow("neonatal 8 kg")   << QStringLiteral("Neonatal")  << 8;
    QTest::newRow("neonatal 3 kg")   << QStringLiteral("Neonatal")  << 3;
    QTest::newRow("neonatal 1 kg")   << QStringLiteral("Neonatal")  << 1;
}

void TestVentilatorStart::startsAfterSwitchingToEveryCategory()
{
    QFETCH(QString, category);
    QFETCH(int, ibw);

    VentilatorController controller(nullptr, nullptr);
    controller.acceptPatient(category, ibw);

    QVERIFY2(controller.requestStartVentilation(),
             qPrintable(QStringLiteral("%1 %2 kg: %3")
                            .arg(category).arg(ibw)
                            .arg(controller.lastCommandMessage())));
    QVERIFY(controller.running());
}

void TestVentilatorStart::categorySwitchPullsTidalVolumeIntoRange_data()
{
    QTest::addColumn<QString>("category");
    QTest::addColumn<int>("ibw");
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");

    QTest::newRow("neonatal 8 kg")   << QStringLiteral("Neonatal")  << 8  << 32  << 64;
    QTest::newRow("neonatal 3 kg")   << QStringLiteral("Neonatal")  << 3  << 12  << 24;
    QTest::newRow("pediatric 20 kg") << QStringLiteral("Pediatric") << 20 << 100 << 200;
}

void TestVentilatorStart::categorySwitchPullsTidalVolumeIntoRange()
{
    QFETCH(QString, category);
    QFETCH(int, ibw);
    QFETCH(int, minimum);
    QFETCH(int, maximum);

    VentilatorController controller(nullptr, nullptr);
    QCOMPARE(controller.tidalVolume(), 420);

    controller.setPatientContext(category);
    controller.setPatientIbwKg(ibw);

    QVERIFY2(controller.tidalVolume() >= minimum && controller.tidalVolume() <= maximum,
             qPrintable(QStringLiteral("%1 mL is outside %2-%3 mL")
                            .arg(controller.tidalVolume()).arg(minimum).arg(maximum)));
}

void TestVentilatorStart::categorySwitchKeepsInspiratoryTimeInsideTheCycle_data()
{
    QTest::addColumn<QString>("category");
    QTest::addColumn<int>("ibw");

    QTest::newRow("adult")     << QStringLiteral("Adult")     << 73;
    QTest::newRow("pediatric") << QStringLiteral("Pediatric") << 18;
    QTest::newRow("neonatal")  << QStringLiteral("Neonatal")  << 4;
}

void TestVentilatorStart::categorySwitchKeepsInspiratoryTimeInsideTheCycle()
{
    QFETCH(QString, category);
    QFETCH(int, ibw);

    VentilatorController controller(nullptr, nullptr);
    controller.setPatientContext(category);
    controller.setPatientIbwKg(ibw);

    const double cycleSeconds = 60.0 / qMax(1, controller.respiratoryRate());
    QVERIFY2(controller.inspiratoryTime() < cycleSeconds * 0.80,
             qPrintable(QStringLiteral("Ti %1 s does not fit a %2 s cycle")
                            .arg(controller.inspiratoryTime()).arg(cycleSeconds)));
}

void TestVentilatorStart::rejectedStartExplainsItself()
{
    // The patient is admitted first, or this passes because nobody was
    // admitted rather than because the limits are impossible.
    VentilatorController controller(nullptr, nullptr);
    controller.acceptPatient(QStringLiteral("Adult"), 73);
    // Drive the alarm limits into an impossible state.
    controller.setAlarmHighPressure(10);
    controller.setAlarmLowPressure(20);

    QVERIFY(!controller.requestStartVentilation());
    QVERIFY2(!controller.lastCommandMessage().isEmpty(),
             "a refused start must say why");
    QVERIFY(!controller.running());
}

void TestVentilatorStart::rejectedStartEmitsCommandRejected()
{
    VentilatorController controller(nullptr, nullptr);
    controller.acceptPatient(QStringLiteral("Adult"), 73);
    controller.setAlarmHighPressure(10);
    controller.setAlarmLowPressure(20);

    QSignalSpy spy(&controller, &VentilatorController::commandRejected);
    QVERIFY(!controller.requestStartVentilation());
    QCOMPARE(spy.count(), 1);
    QVERIFY(!spy.first().at(0).toString().isEmpty());
}

void TestVentilatorStart::invalidAlarmLimitsRefuseToStart()
{
    VentilatorController controller(nullptr, nullptr);
    controller.acceptPatient(QStringLiteral("Adult"), 73);
    controller.setAlarmHighPressure(12);
    QVERIFY(!controller.requestStartVentilation());
}

void TestVentilatorStart::startSetsRunning()
{
    VentilatorController controller(nullptr, nullptr);
    controller.acceptPatient(QStringLiteral("Adult"), 73);
    QVERIFY(!controller.running());
    QVERIFY(controller.requestStartVentilation());
    QVERIFY(controller.running());
}

void TestVentilatorStart::stopClearsRunning()
{
    VentilatorController controller(nullptr, nullptr);
    controller.acceptPatient(QStringLiteral("Adult"), 73);
    QVERIFY(controller.requestStartVentilation());
    controller.stopVentilation();
    QVERIFY(!controller.running());
}

void TestVentilatorStart::freezeTogglesWithoutStoppingVentilation()
{
    VentilatorController controller(nullptr, nullptr);
    controller.acceptPatient(QStringLiteral("Adult"), 73);
    QVERIFY(controller.requestStartVentilation());
    const bool before = controller.frozen();
    controller.toggleFreeze();
    QCOMPARE(controller.frozen(), !before);
    QVERIFY(controller.running());
    controller.toggleFreeze();
    QCOMPARE(controller.frozen(), before);
}

void TestVentilatorStart::categoryRangesNeverInvert_data()
{
    QTest::addColumn<QString>("category");
    QTest::addColumn<int>("ibw");

    const QStringList categories{QStringLiteral("Adult"),
                                 QStringLiteral("Pediatric"),
                                 QStringLiteral("Neonatal")};
    for (const QString &category : categories) {
        for (int ibw : {1, 2, 3, 5, 8, 12, 20, 30, 45, 73, 120, 180}) {
            QTest::newRow(qPrintable(QStringLiteral("%1 %2 kg").arg(category).arg(ibw)))
                << category << ibw;
        }
    }
}

void TestVentilatorStart::categoryRangesNeverInvert()
{
    QFETCH(QString, category);
    QFETCH(int, ibw);

    // qBound asserts in a debug build when the range is inverted, so the
    // bounds themselves have to be ordered for every pairing - including the
    // ones a clinician would never choose.
    VentilatorController controller(nullptr, nullptr);
    controller.acceptPatient(category, ibw);

    QVERIFY2(controller.tidalVolume() > 0, "tidal volume collapsed to zero");
    QVERIFY2(controller.respiratoryRate() > 0, "respiratory rate collapsed to zero");
    QVERIFY2(controller.requestStartVentilation(),
             qPrintable(controller.lastCommandMessage()));
}

void TestVentilatorStart::categoryBeforeWeightDoesNotAssert()
{
    // This is the launch crash: the start-up sync set the category while the
    // previous patient's body weight was still loaded, so for one call the
    // controller held Neonatal at 73 kg and every derived range inverted.
    VentilatorController controller(nullptr, nullptr);
    controller.setPatientContext(QStringLiteral("Neonatal"));
    controller.setPatientIbwKg(8);
    controller.acceptPatient(QStringLiteral("Neonatal"), 8);
    QVERIFY(controller.requestStartVentilation());

    VentilatorController other(nullptr, nullptr);
    other.setPatientContext(QStringLiteral("Adult"));
    other.setPatientIbwKg(1);
    other.acceptPatient(QStringLiteral("Adult"), 1);
    QVERIFY(other.requestStartVentilation());
}

void TestVentilatorStart::setPatientProfileChangesBothAtOnce()
{
    VentilatorController controller(nullptr, nullptr);
    QSignalSpy spy(&controller, &VentilatorController::patientContextChanged);

    controller.setPatientProfile(QStringLiteral("Neonatal"), 3);
    QCOMPARE(controller.patientCategory(), QStringLiteral("Neonatal"));
    QCOMPARE(controller.patientIbwKg(), 3);
    QCOMPARE(spy.count(), 1);

    // Setting the same profile again must not churn.
    controller.setPatientProfile(QStringLiteral("Neonatal"), 3);
    QCOMPARE(spy.count(), 1);
}

void TestVentilatorStart::everyCategoryAndWeightStarts_data()
{
    categoryRangesNeverInvert_data();
}

void TestVentilatorStart::everyCategoryAndWeightStarts()
{
    QFETCH(QString, category);
    QFETCH(int, ibw);

    VentilatorController controller(nullptr, nullptr);
    controller.acceptPatient(category, ibw);
    QVERIFY2(controller.requestStartVentilation(),
             qPrintable(QStringLiteral("%1 %2 kg refused: %3")
                            .arg(category).arg(ibw)
                            .arg(controller.lastCommandMessage())));
    QVERIFY(controller.running());
}

QTEST_MAIN(TestVentilatorStart)
#include "tst_ventilator_start.moc"
