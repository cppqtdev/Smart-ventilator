// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/transport/SimulatedTelemetrySource.h>
#include <sv/transport/TelemetryFactory.h>

#include <QSignalSpy>
#include <QTest>

#include <cmath>

using namespace sv::transport;

class TestSimulatedSource : public QObject
{
    Q_OBJECT

private slots:
    void startsAndReportsConnected();
    void stopClearsConnected();
    void emitsWaveformSignalsWhileRunning();
    void pressureStaysAbovePeep();
    void volumeReturnsTowardZeroDuringExpiration();
    void breathSummaryArrivesOncePerBreath();
    void writeSettingChangesTheModel();
    void unknownSettingIsRejected();
    void factoryFallsBackToTheSimulator();
    void factoryHonoursTheEnvironmentOverride();
};

void TestSimulatedSource::startsAndReportsConnected()
{
    SimulatedTelemetrySource source;
    QVERIFY(!source.isConnected());

    QSignalSpy spy(&source, &ITelemetrySource::connectedChanged);
    QVERIFY(source.start());
    QCOMPARE(spy.count(), 1);
    QVERIFY(source.isConnected());
    source.stop();
}

void TestSimulatedSource::stopClearsConnected()
{
    SimulatedTelemetrySource source;
    QVERIFY(source.start());
    source.stop();
    QVERIFY(!source.isConnected());
}

void TestSimulatedSource::emitsWaveformSignalsWhileRunning()
{
    SimulatedTelemetrySource source;
    source.setSampleIntervalMs(5);
    QSignalSpy spy(&source, &ITelemetrySource::signalReceived);
    QVERIFY(source.start());

    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 8, 2000);

    QStringList seen;
    for (const QList<QVariant> &call : spy) {
        const QString name = call.at(0).toString();
        if (!seen.contains(name))
            seen << name;
    }
    QVERIFY(seen.contains(QString::fromLatin1(signals_::airwayPressure)));
    QVERIFY(seen.contains(QString::fromLatin1(signals_::flow)));
    QVERIFY(seen.contains(QString::fromLatin1(signals_::volume)));
    source.stop();
}

void TestSimulatedSource::pressureStaysAbovePeep()
{
    SimulatedTelemetrySource source;
    SimulatedTelemetrySource::Ventilator settings;
    settings.peep = 8.0;
    settings.respiratoryRate = 20.0;
    source.setVentilator(settings);
    source.setSampleIntervalMs(5);

    QSignalSpy spy(&source, &ITelemetrySource::signalReceived);
    QVERIFY(source.start());
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 200, 4000);
    source.stop();

    int samples = 0;
    for (const QList<QVariant> &call : spy) {
        if (call.at(0).toString() != QString::fromLatin1(signals_::airwayPressure))
            continue;
        const double pressure = call.at(1).toDouble();
        QVERIFY2(pressure > settings.peep - 1.0,
                 qPrintable(QStringLiteral("pressure %1 fell below PEEP %2")
                                .arg(pressure).arg(settings.peep)));
        QVERIFY(std::isfinite(pressure));
        ++samples;
    }
    QVERIFY(samples > 0);
}

void TestSimulatedSource::volumeReturnsTowardZeroDuringExpiration()
{
    SimulatedTelemetrySource source;
    source.setSampleIntervalMs(5);

    double maximum = 0.0;
    double minimumAfterPeak = 1e9;
    bool seenPeak = false;

    connect(&source, &ITelemetrySource::signalReceived, &source,
            [&](const QString &name, double value, quint64) {
                if (name != QString::fromLatin1(signals_::volume))
                    return;
                if (value > maximum) {
                    maximum = value;
                    seenPeak = maximum > 100.0;
                } else if (seenPeak) {
                    minimumAfterPeak = std::min(minimumAfterPeak, value);
                }
            });

    QVERIFY(source.start());
    QTest::qWait(4000);
    source.stop();

    QVERIFY2(maximum > 100.0, "the lung never filled");
    QVERIFY2(minimumAfterPeak < maximum * 0.5, "the lung never emptied");
}

void TestSimulatedSource::breathSummaryArrivesOncePerBreath()
{
    SimulatedTelemetrySource source;
    SimulatedTelemetrySource::Ventilator settings;
    settings.respiratoryRate = 30.0;
    source.setVentilator(settings);
    source.setSampleIntervalMs(5);

    QSignalSpy spy(&source, &ITelemetrySource::frameDecoded);
    QVERIFY(source.start());
    QTest::qWait(4500);
    source.stop();

    // 30 breaths per minute over roughly 4.5 s is about two breaths.
    QVERIFY2(spy.count() >= 1, "no breath summary was published");
    QVERIFY2(spy.count() <= 5,
             qPrintable(QStringLiteral("too many breath summaries: %1").arg(spy.count())));

    const QVariantMap first = spy.first().at(0).toMap();
    QVERIFY(first.contains(QString::fromLatin1(signals_::peakPressure)));
    QVERIFY(first.contains(QString::fromLatin1(signals_::tidalVolumeExpired)));
    QVERIFY(first.contains(QString::fromLatin1(signals_::minuteVolume)));
    QVERIFY(first.value(QString::fromLatin1(signals_::peakPressure)).toDouble() > 0.0);
}

void TestSimulatedSource::writeSettingChangesTheModel()
{
    SimulatedTelemetrySource source;
    QVERIFY(source.writeSetting(QString::fromLatin1(signals_::peep), 12.0));
    QCOMPARE(source.ventilator().peep, 12.0);

    QVERIFY(source.writeSetting(QString::fromLatin1(signals_::fio2), 45.0));
    QCOMPARE(source.ventilator().fio2, 45.0);

    QVERIFY(source.writeSetting(QString::fromLatin1(signals_::respiratoryRate), 24.0));
    QCOMPARE(source.ventilator().respiratoryRate, 24.0);
}

void TestSimulatedSource::unknownSettingIsRejected()
{
    SimulatedTelemetrySource source;
    QVERIFY(!source.writeSetting(QStringLiteral("notASetting"), 1.0));
}

void TestSimulatedSource::factoryFallsBackToTheSimulator()
{
    qputenv("SV_TELEMETRY", QByteArray());
    auto source = TelemetryFactory::create();
    QVERIFY(source != nullptr);
    // Without Qt SerialBus, or with no bus present, the lung model must be
    // what comes back so the application still runs.
    if (!TelemetryFactory::canSupported())
        QVERIFY(dynamic_cast<SimulatedTelemetrySource *>(source.get()) != nullptr);
}

void TestSimulatedSource::factoryHonoursTheEnvironmentOverride()
{
    qputenv("SV_TELEMETRY", "simulator");
    QVERIFY(TelemetryFactory::simulatorRequested());
    auto source = TelemetryFactory::create();
    QVERIFY(dynamic_cast<SimulatedTelemetrySource *>(source.get()) != nullptr);
    qputenv("SV_TELEMETRY", QByteArray());
}

QTEST_MAIN(TestSimulatedSource)
#include "tst_simulated_source.moc"
