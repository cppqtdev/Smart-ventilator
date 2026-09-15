#pragma once

#include <QObject>
#include <QTimer>
#include <QTimeZone>

/**
 * @brief Publishes the device date and time in the configured clinical timezone.
 *
 * The demo uses India Standard Time and a 12-hour clock to match the requested
 * ventilator display format. The class is intentionally tiny so it can later be
 * replaced by an RTC/NTP-backed device clock service.
 */
class ClockController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString dateText READ dateText NOTIFY timeChanged)
    Q_PROPERTY(QString timeText READ timeText NOTIFY timeChanged)
    Q_PROPERTY(QString dateTimeText READ dateTimeText NOTIFY timeChanged)
    Q_PROPERTY(QString timeZoneId READ timeZoneId WRITE setTimeZoneId NOTIFY timeZoneChanged)
    Q_PROPERTY(int year READ year NOTIFY timeChanged)
    Q_PROPERTY(int month READ month NOTIFY timeChanged)
    Q_PROPERTY(int day READ day NOTIFY timeChanged)
    Q_PROPERTY(int hour READ hour NOTIFY timeChanged)
    Q_PROPERTY(int minute READ minute NOTIFY timeChanged)
    Q_PROPERTY(int second READ second NOTIFY timeChanged)
    Q_PROPERTY(QString isoDate READ isoDate NOTIFY timeChanged)
    Q_PROPERTY(QString isoTime READ isoTime NOTIFY timeChanged)
    Q_PROPERTY(qint64 offsetSeconds READ offsetSeconds NOTIFY offsetChanged)
    Q_PROPERTY(bool adjusted READ adjusted NOTIFY offsetChanged)

public:
    explicit ClockController(QObject *parent = nullptr);

    /** @return Formatted date string in the configured timezone. */
    QString dateText() const;
    /** @return Formatted time string in the configured timezone. */
    QString timeText() const;
    /** @return Combined date and time string in the configured timezone. */
    QString dateTimeText() const;
    /** @return Current IANA timezone identifier (e.g. "Asia/Kolkata"). */
    QString timeZoneId() const;

    /** @brief Sets the display timezone by IANA identifier (e.g. "America/New_York"). */
    Q_INVOKABLE void setTimeZoneId(const QString &id);

    /** @return List of common IANA timezone identifiers for UI selection. */
    Q_INVOKABLE QStringList availableTimeZones() const;

    int year() const;
    int month() const;
    int day() const;
    int hour() const;
    int minute() const;
    int second() const;

    /** @return Date as yyyy-MM-dd in the configured timezone. */
    QString isoDate() const;
    /** @return Time as hh:mm:ss in the configured timezone. */
    QString isoTime() const;

    /** @return How far the device clock is set from the host clock. */
    qint64 offsetSeconds() const;
    /** @return True once an operator has set the clock. */
    bool adjusted() const;

    /** @return Days in the given month, so the day dial can stop at the end. */
    Q_INVOKABLE int daysInMonth(int year, int month) const;

    /**
     * @brief Sets the device clock.
     *
     * Rejects a date that does not exist, such as the 31st of February.
     * @return false when the date is not valid; nothing is changed.
     */
    Q_INVOKABLE bool applyDateTime(int year, int month, int day, int hour, int minute);

    /** @brief Drops the operator offset and follows the host clock again. */
    Q_INVOKABLE void resetToHostClock();

signals:
    void timeChanged();
    void timeZoneChanged();
    void offsetChanged();
    void clockSet(const QString &isoDateTime);

private slots:
    void refresh();

private:
    QDateTime currentZonedTime() const;

    QTimer m_timer;
    QTimeZone m_timeZone;
    qint64 m_offsetSeconds = 0;
    QString m_dateText;
    QString m_timeText;
};
