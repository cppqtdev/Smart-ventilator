#include "ClockController.h"

#include <sv/common/LogBuffer.h>

#include <QDate>
#include <QDateTime>
#include <QLocale>

ClockController::ClockController(QObject *parent)
    : QObject(parent)
    , m_timeZone("Asia/Kolkata")
{
    refresh();
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &ClockController::refresh);
    m_timer.start();
}

QString ClockController::dateText() const
{
    return m_dateText;
}

QString ClockController::timeText() const
{
    return m_timeText;
}

QString ClockController::dateTimeText() const
{
    return m_dateText + QLatin1Char('\n') + m_timeText;
}

QString ClockController::timeZoneId() const
{
    return QString::fromLatin1(m_timeZone.id());
}

void ClockController::setTimeZoneId(const QString &id)
{
    const QTimeZone tz(id.toLatin1());
    if (!tz.isValid())
        return;
    if (m_timeZone == tz)
        return;
    m_timeZone = tz;
    emit timeZoneChanged();
    refresh();
}

QStringList ClockController::availableTimeZones() const
{
    return {
        QStringLiteral("Asia/Kolkata"),
        QStringLiteral("UTC"),
        QStringLiteral("America/New_York"),
        QStringLiteral("America/Chicago"),
        QStringLiteral("America/Los_Angeles"),
        QStringLiteral("Europe/London"),
        QStringLiteral("Europe/Berlin"),
        QStringLiteral("Asia/Tokyo"),
        QStringLiteral("Australia/Sydney")
    };
}

int ClockController::year() const
{
    return currentZonedTime().date().year();
}

int ClockController::month() const
{
    return currentZonedTime().date().month();
}

int ClockController::day() const
{
    return currentZonedTime().date().day();
}

int ClockController::hour() const
{
    return currentZonedTime().time().hour();
}

int ClockController::minute() const
{
    return currentZonedTime().time().minute();
}

int ClockController::second() const
{
    return currentZonedTime().time().second();
}

QString ClockController::isoDate() const
{
    return currentZonedTime().date().toString(QStringLiteral("yyyy-MM-dd"));
}

QString ClockController::isoTime() const
{
    return currentZonedTime().time().toString(QStringLiteral("hh:mm:ss"));
}

qint64 ClockController::offsetSeconds() const
{
    return m_offsetSeconds;
}

bool ClockController::adjusted() const
{
    return m_offsetSeconds != 0;
}

int ClockController::daysInMonth(int year, int month) const
{
    if (month < 1 || month > 12)
        return 31;
    return QDate(year < 1 ? 2000 : year, month, 1).daysInMonth();
}

bool ClockController::applyDateTime(int year, int month, int day, int hour, int minute)
{
    // HARDWARE: on the device this writes the RTC. Here the operator setting
    // is kept as an offset from the host clock, because an application cannot
    // and should not move the clock of the machine it is demonstrated on.
    const QDate date(year, month, day);
    const QTime time(hour, minute, 0);
    if (!date.isValid() || !time.isValid())
        return false;

    QDateTime wanted(date, time, m_timeZone);
    if (!wanted.isValid())
        return false;

    const QString before = currentZonedTime().toString(Qt::ISODate);
    m_offsetSeconds = QDateTime::currentDateTimeUtc().secsTo(wanted.toUTC());

    refresh();
    emit offsetChanged();
    emit timeChanged();
    emit clockSet(wanted.toString(Qt::ISODate));

    if (auto *log = sv::common::LogBuffer::instance()) {
        log->noteControl(QStringLiteral("deviceClock"), before,
                         wanted.toString(Qt::ISODate), true);
    }
    return true;
}

void ClockController::resetToHostClock()
{
    if (m_offsetSeconds == 0)
        return;
    m_offsetSeconds = 0;
    refresh();
    emit offsetChanged();
    emit timeChanged();
}

void ClockController::refresh()
{
    // HARDWARE: Replace system clock with hardware RTC (e.g. DS3231 over I2C).
    // In production, synchronize via NTP when network is available.
    const QDateTime now = currentZonedTime();
    const QString nextDate = QLocale(QLocale::English).toString(
        now.date(), QStringLiteral("dd-MMM-yyyy"));
    const QString nextTime = QLocale(QLocale::English).toString(
        now.time(), QStringLiteral("hh:mm AP"));

    m_dateText = nextDate;
    m_timeText = nextTime;

    // second, isoTime and the date parts all carry this signal, so it fires
    // every tick rather than only when the displayed minute rolls over.
    emit timeChanged();
}

QDateTime ClockController::currentZonedTime() const
{
    return QDateTime::currentDateTimeUtc()
        .addSecs(m_offsetSeconds)
        .toTimeZone(m_timeZone);
}
