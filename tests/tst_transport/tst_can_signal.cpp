// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/transport/CanSignal.h>
#include <sv/transport/ITelemetrySource.h>

#include <QFile>
#include <QTest>

using namespace sv::transport;

class TestCanSignal : public QObject
{
    Q_OBJECT

private slots:
    void encodeDecodeRoundTrip_data();
    void encodeDecodeRoundTrip();

    void signedSignalCarriesNegativeValues();
    void valueOutsideRangeIsClamped();
    void zeroFactorIsRejected();

    void defaultDatabaseHasEveryWaveformSignal();
    void defaultDatabaseResolvesSignalToItsFrame();
    void unknownSignalResolvesToNothing();

    void dbcParsesMessagesAndSignals();
    void dbcMatchesTheCompiledInDefaults();
    void dbcWithNoMessagesReportsAnError();
    void dbcSignalBeforeMessageIsReported();

    void neighbouringSignalsDoNotCorruptEachOther();
};

void TestCanSignal::encodeDecodeRoundTrip_data()
{
    QTest::addColumn<int>("startBit");
    QTest::addColumn<int>("bitLength");
    QTest::addColumn<bool>("isSigned");
    QTest::addColumn<double>("factor");
    QTest::addColumn<double>("offset");
    QTest::addColumn<double>("value");

    QTest::newRow("pressure 20.50")  << 0  << 16 << true  << 0.01 << 0.0 << 20.50;
    QTest::newRow("pressure -3.50")  << 0  << 16 << true  << 0.01 << 0.0 << -3.50;
    QTest::newRow("flow 42.5")       << 16 << 16 << true  << 0.1  << 0.0 << 42.5;
    QTest::newRow("flow -42.5")      << 16 << 16 << true  << 0.1  << 0.0 << -42.5;
    QTest::newRow("volume 520")      << 32 << 16 << false << 0.5  << 0.0 << 520.0;
    QTest::newRow("rate 18")         << 32 << 8  << false << 0.5  << 0.0 << 18.0;
    QTest::newRow("offset applied")  << 0  << 16 << false << 0.1  << -50.0 << 12.3;
    QTest::newRow("single bit high") << 7  << 1  << false << 1.0  << 0.0 << 1.0;
}

void TestCanSignal::encodeDecodeRoundTrip()
{
    QFETCH(int, startBit);
    QFETCH(int, bitLength);
    QFETCH(bool, isSigned);
    QFETCH(double, factor);
    QFETCH(double, offset);
    QFETCH(double, value);

    CanSignal entry;
    entry.startBit = startBit;
    entry.bitLength = bitLength;
    entry.isSigned = isSigned;
    entry.factor = factor;
    entry.offset = offset;
    entry.minimum = -100000.0;
    entry.maximum = 100000.0;

    QByteArray payload(8, '\0');
    QVERIFY(entry.encode(payload, value));
    QCOMPARE(payload.size(), 8);
    QCOMPARE(entry.decode(payload), value);
}

void TestCanSignal::signedSignalCarriesNegativeValues()
{
    CanSignal flow;
    flow.startBit = 16;
    flow.bitLength = 16;
    flow.isSigned = true;
    flow.factor = 0.1;
    flow.minimum = -300.0;
    flow.maximum = 300.0;

    QByteArray payload(8, '\0');
    QVERIFY(flow.encode(payload, -120.4));
    QVERIFY(flow.decode(payload) < 0.0);
    QCOMPARE(flow.decode(payload), -120.4);
}

void TestCanSignal::valueOutsideRangeIsClamped()
{
    CanSignal fio2;
    fio2.startBit = 0;
    fio2.bitLength = 16;
    fio2.factor = 0.1;
    fio2.minimum = 21.0;
    fio2.maximum = 100.0;

    QByteArray payload(8, '\0');
    QVERIFY(fio2.encode(payload, 250.0));
    QCOMPARE(fio2.decode(payload), 100.0);

    QVERIFY(fio2.encode(payload, -5.0));
    QCOMPARE(fio2.decode(payload), 21.0);
}

void TestCanSignal::zeroFactorIsRejected()
{
    CanSignal broken;
    broken.factor = 0.0;
    QByteArray payload(8, '\0');
    QVERIFY(!broken.encode(payload, 1.0));
}

void TestCanSignal::defaultDatabaseHasEveryWaveformSignal()
{
    const CanDatabase database = CanDatabase::defaultDatabase();
    const CanMessage *waveform = database.message(0x120);
    QVERIFY(waveform != nullptr);
    QCOMPARE(waveform->signalList.size(), 4);

    QStringList names;
    for (const CanSignal &entry : waveform->signalList)
        names << entry.name;

    QVERIFY(names.contains(QString::fromLatin1(signals_::airwayPressure)));
    QVERIFY(names.contains(QString::fromLatin1(signals_::flow)));
    QVERIFY(names.contains(QString::fromLatin1(signals_::volume)));
    QVERIFY(names.contains(QString::fromLatin1(signals_::co2)));
}

void TestCanSignal::defaultDatabaseResolvesSignalToItsFrame()
{
    const CanDatabase database = CanDatabase::defaultDatabase();
    const CanMessage *frame =
        database.messageForSignal(QString::fromLatin1(signals_::peep));
    QVERIFY(frame != nullptr);
    QCOMPARE(frame->frameId, 0x121u);
}

void TestCanSignal::unknownSignalResolvesToNothing()
{
    const CanDatabase database = CanDatabase::defaultDatabase();
    QVERIFY(database.messageForSignal(QStringLiteral("notASignal")) == nullptr);
    QVERIFY(database.message(0x7FF) == nullptr);
}

void TestCanSignal::dbcParsesMessagesAndSignals()
{
    const QByteArray dbc =
        "BO_ 288 VentWaveform: 8 VENT\n"
        " SG_ airwayPressure : 0|16@1- (0.01,0) [-20|120] \"cmH2O\" HMI\n"
        " SG_ flow : 16|16@1- (0.1,0) [-300|300] \"L/min\" HMI\n";

    QString error;
    const CanDatabase database = CanDatabase::fromDbc(dbc, &error);
    QCOMPARE(database.messageCount(), 1);

    const CanMessage *frame = database.message(288);
    QVERIFY(frame != nullptr);
    QCOMPARE(frame->name, QStringLiteral("VentWaveform"));
    QCOMPARE(frame->byteLength, 8);
    QCOMPARE(frame->signalList.size(), 2);

    const CanSignal &pressure = frame->signalList.first();
    QCOMPARE(pressure.name, QStringLiteral("airwayPressure"));
    QCOMPARE(pressure.startBit, 0);
    QCOMPARE(pressure.bitLength, 16);
    QCOMPARE(pressure.byteOrder, ByteOrder::Intel);
    QVERIFY(pressure.isSigned);
    QCOMPARE(pressure.factor, 0.01);
    QCOMPARE(pressure.minimum, -20.0);
    QCOMPARE(pressure.maximum, 120.0);
    QCOMPARE(pressure.unit, QStringLiteral("cmH2O"));
}

void TestCanSignal::dbcMatchesTheCompiledInDefaults()
{
    QFile file(QStringLiteral(":/data/ventilator.dbc"));
    QVERIFY2(file.open(QIODevice::ReadOnly), "ventilator.dbc is not in the resources");

    QString error;
    const CanDatabase parsed = CanDatabase::fromDbc(file.readAll(), &error);
    const CanDatabase builtIn = CanDatabase::defaultDatabase();

    QCOMPARE(parsed.messageCount(), builtIn.messageCount());

    // Every frame in the file must decode identically to the compiled table,
    // or the two drift apart the first time someone edits one of them.
    for (quint32 frameId : builtIn.frameIds()) {
        const CanMessage *a = builtIn.message(frameId);
        const CanMessage *b = parsed.message(frameId);
        QVERIFY2(b != nullptr, qPrintable(QStringLiteral("frame 0x%1 missing from the DBC")
                                              .arg(frameId, 0, 16)));
        QCOMPARE(b->signalList.size(), a->signalList.size());
        for (int i = 0; i < a->signalList.size(); ++i) {
            QCOMPARE(b->signalList.at(i).name, a->signalList.at(i).name);
            QCOMPARE(b->signalList.at(i).startBit, a->signalList.at(i).startBit);
            QCOMPARE(b->signalList.at(i).bitLength, a->signalList.at(i).bitLength);
            QCOMPARE(b->signalList.at(i).isSigned, a->signalList.at(i).isSigned);
            QCOMPARE(b->signalList.at(i).factor, a->signalList.at(i).factor);
        }
    }
}

void TestCanSignal::dbcWithNoMessagesReportsAnError()
{
    QString error;
    const CanDatabase database = CanDatabase::fromDbc("VERSION \"nothing here\"\n", &error);
    QCOMPARE(database.messageCount(), 0);
    QVERIFY(!error.isEmpty());
}

void TestCanSignal::dbcSignalBeforeMessageIsReported()
{
    QString error;
    const QByteArray dbc = " SG_ orphan : 0|8@1+ (1,0) [0|255] \"\" HMI\n";
    const CanDatabase database = CanDatabase::fromDbc(dbc, &error);
    QCOMPARE(database.messageCount(), 0);
    QVERIFY(error.contains(QStringLiteral("SG_ before any BO_")));
}

void TestCanSignal::neighbouringSignalsDoNotCorruptEachOther()
{
    const CanDatabase database = CanDatabase::defaultDatabase();
    const CanMessage *frame = database.message(0x122);
    QVERIFY(frame != nullptr);

    QByteArray payload(frame->byteLength, '\0');
    const QList<double> written{620.0, 8.40, 18.0, 12.0, 47.0};
    QCOMPARE(written.size(), frame->signalList.size());

    for (int i = 0; i < frame->signalList.size(); ++i)
        QVERIFY(frame->signalList.at(i).encode(payload, written.at(i)));

    for (int i = 0; i < frame->signalList.size(); ++i) {
        const double back = frame->signalList.at(i).decode(payload);
        QVERIFY2(qAbs(back - written.at(i)) < 0.51,
                 qPrintable(QStringLiteral("%1: wrote %2 read %3")
                                .arg(frame->signalList.at(i).name)
                                .arg(written.at(i))
                                .arg(back)));
    }
}

QTEST_MAIN(TestCanSignal)
#include "tst_can_signal.moc"
