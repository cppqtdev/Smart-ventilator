// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/presentation/MonitoringPresenter.h>

#include "../../src/controllers/AlarmController.h"
#include "../../src/controllers/PatientController.h"
#include "../../src/controllers/VentilatorController.h"

#include <QSignalSpy>
#include <QTest>

using sv::presentation::MonitoringPresenter;

class TestMonitoringPresenter : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void detachedPresenterYieldsEmptyLists();
    void sidebarHasFiveTilesWithLabelValueAndUnit();
    void readoutsHaveSixEntries();
    void petco2IsTheAccentedReadout();
    void dialsMatchTheControllerSettings();
    void dialRangesAreSane();
    void valuesAreFormattedNotRaw();
    void patientMapCarriesTheCategory();
    void requestSettingReachesTheController();
    void requestSettingOutsideRangeIsRefused();
    void toggleFreezeFlipsTheControllerState();
    void measurementChangesNotifyTheView();

private:
    VentilatorController *m_ventilator = nullptr;
    PatientController *m_patient = nullptr;
    AlarmController *m_alarms = nullptr;
    MonitoringPresenter *m_presenter = nullptr;
};

void TestMonitoringPresenter::init()
{
    delete m_presenter;
    delete m_ventilator;
    delete m_patient;
    delete m_alarms;

    m_ventilator = new VentilatorController(nullptr, nullptr);
    m_patient = new PatientController(nullptr);
    m_alarms = new AlarmController(nullptr);
    m_presenter = new MonitoringPresenter;
    m_presenter->attach(m_ventilator, m_patient, m_alarms);
}

void TestMonitoringPresenter::detachedPresenterYieldsEmptyLists()
{
    MonitoringPresenter bare;
    QVERIFY(bare.sidebarTiles().isEmpty());
    QVERIFY(bare.readouts().isEmpty());
    QVERIFY(bare.dials().isEmpty());
    QVERIFY(bare.patient().isEmpty());
    QCOMPARE(bare.mode(), QStringLiteral("---"));
    QVERIFY(!bare.frozen());
}

void TestMonitoringPresenter::sidebarHasFiveTilesWithLabelValueAndUnit()
{
    const QVariantList tiles = m_presenter->sidebarTiles();
    QCOMPARE(tiles.size(), 5);

    for (const QVariant &entry : tiles) {
        const QVariantMap tile = entry.toMap();
        QVERIFY(tile.contains(QStringLiteral("key")));
        QVERIFY(!tile.value(QStringLiteral("label")).toString().isEmpty());
        QVERIFY(!tile.value(QStringLiteral("value")).toString().isEmpty());
        QVERIFY(tile.contains(QStringLiteral("unit")));
    }
}

void TestMonitoringPresenter::readoutsHaveSixEntries()
{
    const QVariantList readouts = m_presenter->readouts();
    QCOMPARE(readouts.size(), 6);

    QStringList keys;
    for (const QVariant &entry : readouts)
        keys << entry.toMap().value(QStringLiteral("key")).toString();

    // pcuff was here and is not any more. Cuff pressure appears on one
    // reference panel and nothing on this device measures it, so the readout
    // would have had to invent a number. The six are the set the dynamic
    // lung strip draws.
    for (const QString &expected : {QStringLiteral("totalPeep"),
                                    QStringLiteral("fspont"),
                                    QStringLiteral("petco2"),
                                    QStringLiteral("cstat"),
                                    QStringLiteral("rinsp"),
                                    QStringLiteral("spo2")}) {
        QVERIFY2(keys.contains(expected),
                 qPrintable(QStringLiteral("readout %1 is missing").arg(expected)));
    }
}

void TestMonitoringPresenter::petco2IsTheAccentedReadout()
{
    for (const QVariant &entry : m_presenter->readouts()) {
        const QVariantMap readout = entry.toMap();
        const bool accent = readout.value(QStringLiteral("accent")).toBool();
        if (readout.value(QStringLiteral("key")).toString() == QStringLiteral("petco2"))
            QVERIFY2(accent, "PetCO2 is the one accented value in the reference");
        else
            QVERIFY(!accent);
    }
}

void TestMonitoringPresenter::dialsMatchTheControllerSettings()
{
    const QVariantList dials = m_presenter->dials();
    QCOMPARE(dials.size(), 3);

    const QVariantMap oxygen = dials.at(0).toMap();
    QCOMPARE(oxygen.value(QStringLiteral("key")).toString(), QStringLiteral("fio2"));
    QCOMPARE(oxygen.value(QStringLiteral("value")).toDouble(), double(m_ventilator->fio2()));

    const QVariantMap peep = dials.at(1).toMap();
    QCOMPARE(peep.value(QStringLiteral("key")).toString(), QStringLiteral("peep"));
    QCOMPARE(peep.value(QStringLiteral("value")).toDouble(), double(m_ventilator->peep()));
}

void TestMonitoringPresenter::dialRangesAreSane()
{
    for (const QVariant &entry : m_presenter->dials()) {
        const QVariantMap dial = entry.toMap();
        const double from = dial.value(QStringLiteral("from")).toDouble();
        const double to = dial.value(QStringLiteral("to")).toDouble();
        const double value = dial.value(QStringLiteral("value")).toDouble();
        const double step = dial.value(QStringLiteral("step")).toDouble();

        QVERIFY2(to > from, "a dial with an empty range cannot be turned");
        QVERIFY2(step > 0.0, "a dial needs a non-zero step");
        QVERIFY2(value >= from && value <= to,
                 qPrintable(QStringLiteral("%1 is outside %2..%3")
                                .arg(value).arg(from).arg(to)));
    }
}

void TestMonitoringPresenter::valuesAreFormattedNotRaw()
{
    // Cstat carries one decimal in the reference; the sidebar MV does too.
    for (const QVariant &entry : m_presenter->readouts()) {
        const QVariantMap readout = entry.toMap();
        if (readout.value(QStringLiteral("key")).toString() != QStringLiteral("cstat"))
            continue;
        const QString value = readout.value(QStringLiteral("value")).toString();
        QVERIFY2(value == QStringLiteral("---") || value.contains(QLatin1Char('.')),
                 qPrintable(QStringLiteral("Cstat '%1' lost its decimal").arg(value)));
    }
}

void TestMonitoringPresenter::patientMapCarriesTheCategory()
{
    const QVariantMap patient = m_presenter->patient();
    QVERIFY(patient.contains(QStringLiteral("category")));
    QVERIFY(patient.contains(QStringLiteral("ibw")));
    QVERIFY(patient.contains(QStringLiteral("height")));
}

void TestMonitoringPresenter::requestSettingReachesTheController()
{
    const int before = m_ventilator->fio2();
    const int target = before >= 90 ? 40 : before + 5;

    QVERIFY(m_presenter->requestSetting(QStringLiteral("fio2"), target));
    QCOMPARE(m_ventilator->fio2(), target);
}

void TestMonitoringPresenter::requestSettingOutsideRangeIsRefused()
{
    const int before = m_ventilator->peep();
    QVERIFY(!m_presenter->requestSetting(QStringLiteral("peep"), 400.0));
    QCOMPARE(m_ventilator->peep(), before);
}

void TestMonitoringPresenter::toggleFreezeFlipsTheControllerState()
{
    // A start needs an admitted patient, and freezing needs a running one.
    m_ventilator->acceptPatient(QStringLiteral("Adult"), 73);
    QVERIFY2(m_ventilator->requestStartVentilation(),
             qPrintable(m_ventilator->lastCommandMessage()));
    const bool before = m_ventilator->frozen();
    m_presenter->toggleFreeze();
    QCOMPARE(m_ventilator->frozen(), !before);
    QCOMPARE(m_presenter->frozen(), m_ventilator->frozen());
}

void TestMonitoringPresenter::measurementChangesNotifyTheView()
{
    QSignalSpy spy(m_presenter, &MonitoringPresenter::settingsChanged);
    m_ventilator->setFio2(m_ventilator->fio2() == 50 ? 55 : 50);
    QVERIFY2(spy.count() >= 1, "the view is never told the setting moved");
}

QTEST_MAIN(TestMonitoringPresenter)
#include "tst_monitoring_presenter.moc"
