// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class VentilatorController;
class PatientController;
class AlarmController;

namespace sv::presentation {

/**
 * @brief View model for the home screen.
 *
 * The presentation layer owns every decision about how a value is shown -
 * which tiles appear, how many decimals, which unit symbol, what counts as
 * an accent colour. The application layer below it (VentilatorController and
 * friends) owns what the values ARE and what a change request does. QML binds
 * only to this class, so a display change never reaches into the ventilation
 * logic and a ventilation change never has to know about the screen.
 */
class MonitoringPresenter : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList sidebarTiles READ sidebarTiles NOTIFY measurementsChanged)
    Q_PROPERTY(QVariantList readouts READ readouts NOTIFY measurementsChanged)
    Q_PROPERTY(QVariantList dials READ dials NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList banner READ banner NOTIFY bannerChanged)
    Q_PROPERTY(QVariantMap patient READ patient NOTIFY patientChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY settingsChanged)
    Q_PROPERTY(bool ventilating READ ventilating NOTIFY settingsChanged)
    Q_PROPERTY(int measuredRate READ measuredRate NOTIFY measurementsChanged)
    Q_PROPERTY(QVariantMap asvTarget READ asvTarget NOTIFY measurementsChanged)
    Q_PROPERTY(QString readinessReason READ readinessReason NOTIFY settingsChanged)
    Q_PROPERTY(bool frozen READ frozen NOTIFY settingsChanged)
    Q_PROPERTY(bool nonInvasive READ nonInvasive NOTIFY settingsChanged)
    Q_PROPERTY(bool spontaneous READ spontaneous NOTIFY measurementsChanged)

public:
    explicit MonitoringPresenter(QObject *parent = nullptr);
    ~MonitoringPresenter() override;

    void attach(VentilatorController *ventilator,
                PatientController *patient,
                AlarmController *alarms);

    QVariantList sidebarTiles() const;
    QVariantList readouts() const;
    QVariantList dials() const;
    QVariantList banner() const;
    QVariantMap patient() const;
    QString mode() const;
    bool ventilating() const;

    /** @return Total respiratory rate the device reports, for the lung motion. */
    int measuredRate() const;

    /**
     * @brief The operating point of the adaptive target and the window it
     *        is allowed to move in.
     * @return Keys minuteVolume, rate, tidalVolume, minRate, maxRate,
     *         minVolume and maxVolume. The chart draws the window as a box,
     *         the constant minute volume through it as a curve, and the
     *         operating point as a dot.
     */
    QVariantMap asvTarget() const;
    QString readinessReason() const;
    bool frozen() const;
    bool nonInvasive() const;
    bool spontaneous() const;

    /** @brief Forwards a dial change to the application layer. */
    Q_INVOKABLE bool requestSetting(const QString &key, double value);

    /** @brief Starts ventilation. False when a precondition refused it. */
    Q_INVOKABLE bool requestStart();

    /** @brief Stops ventilation and returns the device to standby. */
    Q_INVOKABLE void requestStop();

    /** @brief Toggles the frozen display state. */
    Q_INVOKABLE void toggleFreeze();

    /** @brief Whole buffer for a channel, for a view that has just appeared. */
    Q_INVOKABLE QVariantList waveform(const QString &channelKey) const;

signals:
    /**
     * @brief One new sample on one channel.
     *
     * The controller appends exactly one sample per channel per tick, so the
     * views append rather than re-reading the whole buffer. Pushing a
     * QVariantList of several hundred points at 22 Hz is the expensive path
     * WaveformView warns about.
     */
    void sampleAppended(const QString &channelKey, double value);

    void measurementsChanged();
    void settingsChanged();
    void bannerChanged();
    void patientChanged();

private slots:
    void publishLatestSamples();

private:
    QVariantMap tile(const QString &key, const QString &label, double value,
                     int decimals, const QString &unit,
                     const QString &upper, const QString &lower) const;

    QVariantMap readout(const QString &key, const QString &label, double value,
                        int decimals, const QString &unit, bool accent = false) const;

    QVariantMap dial(const QString &key, const QString &label, double value,
                     double from, double to, double step,
                     const QString &unit, int decimals = 0) const;

    VentilatorController *m_ventilator = nullptr;
    PatientController *m_patient = nullptr;
    AlarmController *m_alarms = nullptr;
};

} // namespace sv::presentation
