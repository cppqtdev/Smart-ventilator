#include "AlarmAudio.h"

#include "AlarmController.h"

#include <QLoggingCategory>

#include <algorithm>

namespace {

// A burst repeats until the condition is dealt with. IEC 60601-1-8 allows
// 2.5 s to 15 s for high and medium priority; high sits at the urgent end.
constexpr int kHighIntervalMs = 5000;
constexpr int kMediumIntervalMs = 12000;
// Low priority need not repeat at all under the standard. It does here, well
// apart, so that a condition left standing is not forgotten.
constexpr int kLowIntervalMs = 30000;

// An alarm that can be turned down to inaudible is an alarm that can be
// missed, so the setting scales between this floor and full.
constexpr int kMinimumVolumePercent = 35;

} // namespace

AlarmAudio::AlarmAudio(AlarmController *alarms, QObject *parent)
    : QObject(parent)
    , m_alarms(alarms)
{
    m_high.setSource(QUrl(QStringLiteral("qrc:/ui/Assets/alarm_high.wav")));
    m_medium.setSource(QUrl(QStringLiteral("qrc:/ui/Assets/alarm_medium.wav")));
    m_low.setSource(QUrl(QStringLiteral("qrc:/ui/Assets/alarm_low.wav")));
    m_test.setSource(QUrl(QStringLiteral("qrc:/ui/Assets/alarm_test.wav")));

    setVolume(m_volume);

    m_repeat.setSingleShot(false);
    connect(&m_repeat, &QTimer::timeout, this, &AlarmAudio::startBurst);

    // A source that never becomes ready means no output device took it. That
    // is a technical alarm condition in its own right: the operator has to be
    // told that the ventilator cannot sound.
    const auto watchStatus = [this](QSoundEffect *effect) {
        connect(effect, &QSoundEffect::statusChanged, this, [this, effect]() {
            if (effect->status() != QSoundEffect::Error)
                return;
            if (!m_available)
                return;
            m_available = false;
            emit availableChanged();
            qCritical("AlarmAudio: the alarm tone could not be loaded - "
                      "the ventilator cannot sound an alarm");
            if (m_alarms != nullptr) {
                m_alarms->raiseAlarm(QStringLiteral("Critical"),
                                     QStringLiteral("Audio"),
                                     QStringLiteral("Alarm sound unavailable"),
                                     QStringLiteral("Active"));
            }
        });
    };
    watchStatus(&m_high);
    watchStatus(&m_medium);
    watchStatus(&m_low);
    watchStatus(&m_test);

    // A speaker that has failed is only discovered by using it. The device
    // sounds one pulse shortly after start so the operator hears, every time
    // it is switched on, that it still can.
    QTimer::singleShot(1200, this, &AlarmAudio::playTestTone);

    if (m_alarms != nullptr) {
        connect(m_alarms, &AlarmController::audioChanged,
                this, &AlarmAudio::refresh);
        connect(m_alarms, &AlarmController::conditionsChanged,
                this, &AlarmAudio::refresh);
    }
    refresh();
}

bool AlarmAudio::sounding() const
{
    return m_repeat.isActive();
}

int AlarmAudio::volume() const
{
    return m_volume;
}

bool AlarmAudio::available() const
{
    return m_available;
}

int AlarmAudio::minimumVolumePercent()
{
    return kMinimumVolumePercent;
}

void AlarmAudio::setVolume(int percent)
{
    const int clamped = std::clamp(percent, 0, 100);
    const bool changed = clamped != m_volume;
    m_volume = clamped;

    const qreal span = (100.0 - kMinimumVolumePercent) / 100.0;
    const qreal linear = (kMinimumVolumePercent + m_volume * span) / 100.0;
    m_high.setVolume(linear);
    m_medium.setVolume(linear);
    m_low.setVolume(linear);
    m_test.setVolume(linear);

    if (changed)
        emit volumeChanged();
}

void AlarmAudio::playTestTone()
{
    m_test.play();
}

QSoundEffect *AlarmAudio::effectFor(const QString &priority)
{
    if (priority == QLatin1String("high"))
        return &m_high;
    if (priority == QLatin1String("medium"))
        return &m_medium;
    if (priority == QLatin1String("low"))
        return &m_low;
    return nullptr;
}

int AlarmAudio::intervalFor(const QString &priority)
{
    if (priority == QLatin1String("high"))
        return kHighIntervalMs;
    if (priority == QLatin1String("medium"))
        return kMediumIntervalMs;
    return kLowIntervalMs;
}

void AlarmAudio::refresh()
{
    if (m_alarms == nullptr) {
        stopBurst();
        return;
    }

    // audioActive is the state machine's own answer to "should a tone be
    // sounding", and it already accounts for paused, off and latched.
    if (!m_alarms->audioActive()) {
        stopBurst();
        return;
    }

    const QString priority = m_alarms->highestPriority();
    if (effectFor(priority) == nullptr) {
        stopBurst();
        return;
    }

    // A condition that escalates has to be heard at its new priority without
    // waiting for the old burst interval to run out.
    if (priority != m_playingPriority) {
        m_playingPriority = priority;
        m_repeat.stop();
        m_repeat.setInterval(intervalFor(priority));
        startBurst();
        m_repeat.start();
        emit soundingChanged();
    } else if (!m_repeat.isActive()) {
        m_repeat.setInterval(intervalFor(priority));
        startBurst();
        m_repeat.start();
        emit soundingChanged();
    }
}

void AlarmAudio::startBurst()
{
    QSoundEffect *effect = effectFor(m_playingPriority);
    if (effect != nullptr)
        effect->play();
}

void AlarmAudio::stopBurst()
{
    if (!m_repeat.isActive() && m_playingPriority.isEmpty())
        return;

    m_repeat.stop();
    m_playingPriority.clear();
    m_high.stop();
    m_medium.stop();
    m_low.stop();
    emit soundingChanged();
}
