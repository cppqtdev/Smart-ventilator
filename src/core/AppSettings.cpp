#include "AppSettings.h"

#include <sv/common/AppIdentity.h>

#include <QDebug>

namespace {
void syncSettings(QSettings &settings, const QString &key)
{
    settings.sync();
    if (settings.status() != QSettings::NoError)
        qWarning() << "Failed to persist setting" << key << "status" << settings.status();
}
}

namespace {
// Writing the counter every minute would be hundreds of thousands of flash
// writes a year for no benefit. Five minutes loses at most five minutes of
// running time if the device loses power.
constexpr int kPersistIntervalMs = 5 * 60 * 1000;
}

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(sv::common::identity::organizationName(),
                 sv::common::identity::applicationName())
{
    // The default used to be 82.11, a number nobody measured, printed on the
    // splash screen as if the device had run that long. A device that has
    // never run has run for no hours.
    m_baseHours = m_settings.value(QStringLiteral("device/operatingHours"), 0.0).toDouble();
    if (m_baseHours < 0.0)
        m_baseHours = 0.0;
    m_session.start();

    m_persistTimer.setInterval(kPersistIntervalMs);
    connect(&m_persistTimer, &QTimer::timeout, this, [this]() {
        persistOperatingHours();
        emit operatingHoursChanged();
    });
    m_persistTimer.start();
}

AppSettings::~AppSettings()
{
    persistOperatingHours();
}

void AppSettings::persistOperatingHours()
{
    const double total = operatingHours();
    m_settings.setValue(QStringLiteral("device/operatingHours"), total);
    syncSettings(m_settings, QStringLiteral("device/operatingHours"));
    m_baseHours = total;
    m_session.restart();
}

QString AppSettings::serialNumber() const
{
    return m_settings.value(QStringLiteral("device/serialNumber"),
                            QStringLiteral("not set")).toString();
}

void AppSettings::setSerialNumber(const QString &serial)
{
    if (serialNumber() == serial)
        return;
    m_settings.setValue(QStringLiteral("device/serialNumber"), serial);
    syncSettings(m_settings, QStringLiteral("device/serialNumber"));
    emit serialNumberChanged();
}

QString AppSettings::softwareVersion() const
{
    return QStringLiteral(APP_VERSION);
}

QString AppSettings::buildId() const
{
    return QStringLiteral(BUILD_ID);
}

double AppSettings::operatingHours() const
{
    const double thisSession = m_session.isValid()
        ? m_session.elapsed() / (1000.0 * 60.0 * 60.0)
        : 0.0;
    return m_baseHours + thisSession;
}

int AppSettings::brightness() const
{
    return m_settings.value(QStringLiteral("ui/brightness"), 85).toInt();
}

int AppSettings::audioVolume() const
{
    return m_settings.value(QStringLiteral("ui/audioVolume"), 70).toInt();
}

QString AppSettings::language() const
{
    return m_settings.value(QStringLiteral("ui/language"), QStringLiteral("English")).toString();
}

QString AppSettings::dayNightMode() const
{
    return m_settings.value(QStringLiteral("ui/dayNightMode"), QStringLiteral("Day")).toString();
}

QString AppSettings::timeZoneId() const
{
    return m_settings.value(QStringLiteral("ui/timeZoneId"), QStringLiteral("Asia/Kolkata")).toString();
}

int AppSettings::monitoringLayout() const
{
    return m_settings.value(QStringLiteral("ui/monitoringLayout"), 1).toInt();
}

int AppSettings::nightStartHour() const
{
    return m_settings.value(QStringLiteral("ui/nightStartHour"), 20).toInt();
}

int AppSettings::dayStartHour() const
{
    return m_settings.value(QStringLiteral("ui/dayStartHour"), 6).toInt();
}

void AppSettings::setOperatingHours(double hours)
{
    // A service action after a board swap, not something the counter itself
    // does. The session restarts from the new total.
    if (hours < 0.0)
        hours = 0.0;
    if (qFuzzyCompare(operatingHours(), hours))
        return;
    m_baseHours = hours;
    m_session.restart();
    m_settings.setValue(QStringLiteral("device/operatingHours"), hours);
    syncSettings(m_settings, QStringLiteral("device/operatingHours"));
    emit operatingHoursChanged();
}

void AppSettings::setBrightness(int value)
{
    if (brightness() == value)
        return;
    m_settings.setValue(QStringLiteral("ui/brightness"), value);
    syncSettings(m_settings, QStringLiteral("ui/brightness"));
    emit brightnessChanged();
}

void AppSettings::setAudioVolume(int value)
{
    if (audioVolume() == value)
        return;
    m_settings.setValue(QStringLiteral("ui/audioVolume"), value);
    syncSettings(m_settings, QStringLiteral("ui/audioVolume"));
    emit audioVolumeChanged();
}

void AppSettings::setLanguage(const QString &value)
{
    if (language() == value)
        return;
    m_settings.setValue(QStringLiteral("ui/language"), value);
    syncSettings(m_settings, QStringLiteral("ui/language"));
    emit languageChanged();
}

void AppSettings::setDayNightMode(const QString &value)
{
    if (dayNightMode() == value)
        return;
    m_settings.setValue(QStringLiteral("ui/dayNightMode"), value);
    syncSettings(m_settings, QStringLiteral("ui/dayNightMode"));
    emit dayNightModeChanged();
}

void AppSettings::setTimeZoneId(const QString &value)
{
    if (timeZoneId() == value)
        return;
    m_settings.setValue(QStringLiteral("ui/timeZoneId"), value);
    syncSettings(m_settings, QStringLiteral("ui/timeZoneId"));
    emit timeZoneIdChanged();
}

// The number of arrangements ui/Screens/Home/HomeLayouts.qml offers. A
// layout id above this is clamped away, which leaves the picker showing the
// old selection lit beside the one that was pressed. scripts/layout_count.py
// compares the two, because the last time they drifted apart nothing said so.
static constexpr int kMonitoringLayoutCount = 7;

void AppSettings::setMonitoringLayout(int value)
{
    value = qBound(1, value, kMonitoringLayoutCount);
    if (monitoringLayout() == value)
        return;
    m_settings.setValue(QStringLiteral("ui/monitoringLayout"), value);
    syncSettings(m_settings, QStringLiteral("ui/monitoringLayout"));
    emit monitoringLayoutChanged();
}

void AppSettings::setNightStartHour(int value)
{
    value = qBound(0, value, 23);
    if (nightStartHour() == value)
        return;
    m_settings.setValue(QStringLiteral("ui/nightStartHour"), value);
    syncSettings(m_settings, QStringLiteral("ui/nightStartHour"));
    emit dayNightScheduleChanged();
}

void AppSettings::setDayStartHour(int value)
{
    value = qBound(0, value, 23);
    if (dayStartHour() == value)
        return;
    m_settings.setValue(QStringLiteral("ui/dayStartHour"), value);
    syncSettings(m_settings, QStringLiteral("ui/dayStartHour"));
    emit dayNightScheduleChanged();
}
