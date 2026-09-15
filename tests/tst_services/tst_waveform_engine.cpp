// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/services/WaveformEngine.h>
#include <sv/domain/WaveformSample.h>

#include <QTest>

using namespace sv::services;
using namespace sv::domain;

class TestWaveformEngine : public QObject
{
    Q_OBJECT

private slots:
    void appendSample_addsToBuffers();
    void pressureWaveform_returnsCorrectList();
    void setFrozen_preventsNewSamples();
    void clear_emptiesAllBuffers();
    void bufferDoesNotExceed180();
};

void TestWaveformEngine::appendSample_addsToBuffers()
{
    WaveformEngine engine;
    WaveformSample s{12.5, 30.0, 200.0, 4.5};
    engine.appendSample(s);

    QCOMPARE(engine.pressureWaveform().size(), 1);
    QCOMPARE(engine.flowWaveform().size(), 1);
    QCOMPARE(engine.volumeWaveform().size(), 1);
    QCOMPARE(engine.co2Waveform().size(), 1);
}

void TestWaveformEngine::pressureWaveform_returnsCorrectList()
{
    WaveformEngine engine;
    engine.appendSample({10.0, 0, 0, 0});
    engine.appendSample({20.0, 0, 0, 0});
    engine.appendSample({30.0, 0, 0, 0});

    QVariantList pw = engine.pressureWaveform();
    QCOMPARE(pw.size(), 3);
    QCOMPARE(pw.at(0).toDouble(), 10.0);
    QCOMPARE(pw.at(1).toDouble(), 20.0);
    QCOMPARE(pw.at(2).toDouble(), 30.0);
}

void TestWaveformEngine::setFrozen_preventsNewSamples()
{
    WaveformEngine engine;
    engine.appendSample({5.0, 1.0, 2.0, 3.0});
    QCOMPARE(engine.pressureWaveform().size(), 1);

    engine.setFrozen(true);
    QVERIFY(engine.frozen());
    engine.appendSample({15.0, 2.0, 3.0, 4.0});
    // Size should not increase while frozen
    QCOMPARE(engine.pressureWaveform().size(), 1);

    engine.setFrozen(false);
    engine.appendSample({25.0, 3.0, 4.0, 5.0});
    QCOMPARE(engine.pressureWaveform().size(), 2);
}

void TestWaveformEngine::clear_emptiesAllBuffers()
{
    WaveformEngine engine;
    for (int i = 0; i < 10; ++i)
        engine.appendSample({double(i), double(i), double(i), double(i)});

    QCOMPARE(engine.pressureWaveform().size(), 10);
    engine.clear();
    QCOMPARE(engine.pressureWaveform().size(), 0);
    QCOMPARE(engine.flowWaveform().size(), 0);
    QCOMPARE(engine.volumeWaveform().size(), 0);
    QCOMPARE(engine.co2Waveform().size(), 0);
}

void TestWaveformEngine::bufferDoesNotExceed180()
{
    WaveformEngine engine;
    for (int i = 0; i < 250; ++i)
        engine.appendSample({double(i), double(i), double(i), double(i)});

    QVERIFY(engine.pressureWaveform().size() <= 180);
    QVERIFY(engine.flowWaveform().size() <= 180);
    QVERIFY(engine.volumeWaveform().size() <= 180);
    QVERIFY(engine.co2Waveform().size() <= 180);
}

QTEST_MAIN(TestWaveformEngine)
#include "tst_waveform_engine.moc"
