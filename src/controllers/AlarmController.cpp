#include "AlarmController.h"

#include "src/core/DatabaseManager.h"

#include <algorithm>
#include <utility>

namespace {

/// Conventional latching policy. High-priority physiological conditions
/// persist until acknowledged so a transient event leaves a trace; technical
/// and lower-priority conditions follow the sensor.
bool latchingPolicyPermits(AlarmController::Priority priority)
{
    return priority == AlarmController::Priority::High;
}

/// The log's priority column keeps the vendor vocabulary the existing alarm
/// screen filters on, so the normalised ranking is mapped back for display.
QString logPriorityLabel(AlarmController::Priority priority)
{
    switch (priority) {
    case AlarmController::Priority::High:   return QStringLiteral("Critical");
    case AlarmController::Priority::Medium: return QStringLiteral("Warning");
    case AlarmController::Priority::Low:    return QStringLiteral("Info");
    case AlarmController::Priority::None:   break;
    }
    return QStringLiteral("Info");
}

} // namespace

AlarmController::AlarmController(DatabaseManager *database, QObject *parent)
    : QAbstractListModel(parent)
    , m_database(database)
{
    m_audioPauseTimer.setInterval(1000);
    connect(&m_audioPauseTimer, &QTimer::timeout, this, [this]() {
        --m_audioPauseRemaining;
        if (m_audioPauseRemaining <= 0) {
            // The pause expires on its own. ISO 80601-2-12 does not allow it
            // to be extended silently; the operator must act again.
            m_audioPaused = false;
            m_audioPauseRemaining = 0;
            m_audioPauseTimer.stop();
            appendLog(QStringLiteral("Info"), QStringLiteral("low"),
                      QStringLiteral("Alarm"),
                      QStringLiteral("Audio pause expired; alarm audio restored"),
                      QStringLiteral("Closed"), QString());
        }
        emit audioChanged();
    });

    addAlarm(QStringLiteral("Info"), QStringLiteral("System"),
             QStringLiteral("Alarm system initialized"), QStringLiteral("Closed"));
}

// ---------------------------------------------------------------------------
//  Priority mapping
// ---------------------------------------------------------------------------

AlarmController::Priority AlarmController::normalisePriority(const QString &priority)
{
    const QString key = priority.trimmed().toLower();
    if (key == QLatin1String("high") || key == QLatin1String("critical"))
        return Priority::High;
    if (key == QLatin1String("medium") || key == QLatin1String("warning"))
        return Priority::Medium;
    if (key == QLatin1String("low") || key == QLatin1String("info")
        || key == QLatin1String("advisory"))
        return Priority::Low;
    return Priority::None;
}

QString AlarmController::priorityName(Priority priority)
{
    switch (priority) {
    case Priority::High:   return QStringLiteral("high");
    case Priority::Medium: return QStringLiteral("medium");
    case Priority::Low:    return QStringLiteral("low");
    case Priority::None:   break;
    }
    return QStringLiteral("none");
}

// ---------------------------------------------------------------------------
//  Model
// ---------------------------------------------------------------------------

int AlarmController::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_filteredIndices.size();
}

QVariant AlarmController::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_filteredIndices.size())
        return {};

    const int sourceRow = m_filteredIndices.at(index.row());
    if (sourceRow < 0 || sourceRow >= m_rows.size())
        return {};

    const LogRow &row = m_rows.at(sourceRow);
    switch (role) {
    case TimeRole:                 return row.time;
    case PriorityRole:             return row.priority;
    case SourceRole:               return row.source;
    case DescriptionRole:          return row.description;
    case StatusRole:               return row.status;
    case ConditionIdRole:          return row.conditionId;
    case NormalisedPriorityRole:   return row.normalisedPriority;
    default:                       return {};
    }
}

QHash<int, QByteArray> AlarmController::roleNames() const
{
    return {
        {TimeRole, "time"},
        {PriorityRole, "priority"},
        {SourceRole, "source"},
        {DescriptionRole, "description"},
        {StatusRole, "status"},
        {ConditionIdRole, "conditionId"},
        {NormalisedPriorityRole, "normalisedPriority"}
    };
}

void AlarmController::appendLog(const QString &priority,
                                const QString &normalised,
                                const QString &source,
                                const QString &description,
                                const QString &status,
                                const QString &conditionId)
{
    beginResetModel();
    m_rows.prepend({
        QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")),
        priority,
        normalised,
        source,
        description,
        status,
        conditionId
    });
    rebuildFilteredIndices();
    endResetModel();
    emit filterChanged();

    if (m_database)
        m_database->logAlarm(priority, source, description, status);
}

void AlarmController::addAlarm(const QString &priority,
                               const QString &source,
                               const QString &description,
                               const QString &status)
{
    appendLog(priority, priorityName(normalisePriority(priority)),
              source, description, status, QString());
}

void AlarmController::rebuildFilteredIndices()
{
    m_filteredIndices.clear();
    m_filteredIndices.reserve(m_rows.size());
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_filterPriority.isEmpty() || m_rows.at(i).priority == m_filterPriority)
            m_filteredIndices.append(i);
    }
}

QString AlarmController::filterPriority() const { return m_filterPriority; }
int AlarmController::alarmCount() const { return m_filteredIndices.size(); }

void AlarmController::setFilterPriority(const QString &priority)
{
    if (m_filterPriority == priority)
        return;
    beginResetModel();
    m_filterPriority = priority;
    rebuildFilteredIndices();
    endResetModel();
    emit filterChanged();
}

// ---------------------------------------------------------------------------
//  Condition registry
// ---------------------------------------------------------------------------

void AlarmController::raiseCondition(const QString &conditionId,
                                     const QString &priority,
                                     const QString &source,
                                     const QString &headline,
                                     const QString &detail,
                                     bool latching)
{
    if (conditionId.isEmpty())
        return;

    const Priority normalised = normalisePriority(priority);
    if (normalised == Priority::None) {
        clearCondition(conditionId);
        return;
    }

    auto it = m_conditions.find(conditionId);
    const bool isNew = (it == m_conditions.end());
    const bool wasPresent = !isNew && it->conditionPresent;
    const Priority previousPriority = isNew ? Priority::None : it->priority;

    if (isNew) {
        Condition condition;
        condition.id = conditionId;
        condition.raisedAtUtc = QDateTime::currentDateTimeUtc();
        it = m_conditions.insert(conditionId, condition);
        m_order.append(conditionId);
    }

    it->priority = normalised;
    it->source = source;
    it->headline = headline;
    it->detail = detail;
    it->latching = latching && latchingPolicyPermits(normalised);
    it->conditionPresent = true;
    it->clearedAtUtc = QDateTime();

    if (!wasPresent) {
        it->acknowledged = false;
        it->raisedAtUtc = QDateTime::currentDateTimeUtc();
    }

    // Log a row on first appearance and on any escalation, but not on every
    // sensor tick - a condition that re-asserts 50 times a second must not
    // flood the log it is meant to make reviewable.
    if (!wasPresent || normalised != previousPriority) {
        appendLog(priority, priorityName(normalised), source, detail,
                  wasPresent ? QStringLiteral("Escalated") : QStringLiteral("Active"),
                  conditionId);
        emit alarmRaised(conditionId, priorityName(normalised));
    }

    refreshBanner();
    emit conditionsChanged();
}

void AlarmController::clearCondition(const QString &conditionId)
{
    auto it = m_conditions.find(conditionId);
    if (it == m_conditions.end() || !it->conditionPresent)
        return;

    it->conditionPresent = false;
    it->clearedAtUtc = QDateTime::currentDateTimeUtc();

    appendLog(logPriorityLabel(it->priority),
              priorityName(it->priority),
              it->source,
              it->headline + QStringLiteral(" condition cleared"),
              it->latching ? QStringLiteral("Cleared - awaiting reset")
                           : QStringLiteral("Closed"),
              conditionId);

    if (!it->latching) {
        m_conditions.erase(it);
        m_order.removeAll(conditionId);
    }

    refreshBanner();
    emit conditionsChanged();
}

bool AlarmController::isConditionActive(const QString &conditionId) const
{
    const auto it = m_conditions.constFind(conditionId);
    return it != m_conditions.constEnd() && it->conditionPresent;
}

void AlarmController::acknowledgeCondition(const QString &conditionId)
{
    auto it = m_conditions.find(conditionId);
    if (it == m_conditions.end() || it->acknowledged)
        return;

    it->acknowledged = true;
    appendLog(QStringLiteral("Info"), priorityName(it->priority),
              QStringLiteral("Operator"),
              it->headline + QStringLiteral(" acknowledged; indication persists "
                                            "while the condition is present"),
              QStringLiteral("Acknowledged"), conditionId);

    emit conditionsChanged();
    emit audioChanged();
}

void AlarmController::acknowledgeAll()
{
    for (const QString &id : std::as_const(m_order))
        acknowledgeCondition(id);
}

void AlarmController::resetLatched()
{
    QVector<QString> removed;
    for (auto it = m_conditions.begin(); it != m_conditions.end(); ) {
        if (!it->conditionPresent && it->latching) {
            removed.append(it->id);
            it = m_conditions.erase(it);
        } else {
            ++it;
        }
    }

    if (removed.isEmpty())
        return;

    for (const QString &id : std::as_const(removed))
        m_order.removeAll(id);

    appendLog(QStringLiteral("Info"), QStringLiteral("low"),
              QStringLiteral("Operator"),
              QStringLiteral("Reset %1 latched alarm condition(s)")
                  .arg(removed.size()),
              QStringLiteral("Closed"), QString());

    refreshBanner();
    emit conditionsChanged();
}

const AlarmController::Condition *AlarmController::dominantCondition() const
{
    const Condition *best = nullptr;
    for (const QString &id : m_order) {
        const auto it = m_conditions.constFind(id);
        if (it == m_conditions.constEnd())
            continue;
        // Present conditions outrank latched-but-cleared ones at equal
        // priority: what is happening now matters more than what happened.
        if (!best
            || it->priority > best->priority
            || (it->priority == best->priority
                && it->conditionPresent && !best->conditionPresent)) {
            best = &(*it);
        }
    }
    return best;
}

QVariantList AlarmController::banner() const
{
    const QVariantList conditions = activeConditions();
    QVariantList rows;
    rows.reserve(conditions.size());
    for (const QVariant &entry : conditions) {
        const QVariantMap condition = entry.toMap();
        const QString name = condition.value(QStringLiteral("priority")).toString();
        int rank = 0;
        if (name == QLatin1String("high"))
            rank = 3;
        else if (name == QLatin1String("medium"))
            rank = 2;
        else if (name == QLatin1String("low"))
            rank = 1;
        if (rank == 0)
            continue;
        rows.append(QVariantMap{
            {QStringLiteral("priority"), rank},
            {QStringLiteral("text"), condition.value(QStringLiteral("headline")).toString()},
            {QStringLiteral("conditionId"), condition.value(QStringLiteral("conditionId"))}
        });
    }
    return rows;
}

QVariantList AlarmController::activeConditions() const
{
    QVector<const Condition *> sorted;
    sorted.reserve(m_order.size());
    for (const QString &id : m_order) {
        const auto it = m_conditions.constFind(id);
        if (it != m_conditions.constEnd())
            sorted.append(&(*it));
    }

    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Condition *a, const Condition *b) {
                         if (a->priority != b->priority)
                             return a->priority > b->priority;
                         return a->conditionPresent && !b->conditionPresent;
                     });

    QVariantList list;
    list.reserve(sorted.size());
    for (const Condition *condition : std::as_const(sorted)) {
        list.append(QVariantMap{
            {QStringLiteral("conditionId"), condition->id},
            {QStringLiteral("priority"), priorityName(condition->priority)},
            {QStringLiteral("source"), condition->source},
            {QStringLiteral("headline"), condition->headline},
            {QStringLiteral("detail"), condition->detail},
            {QStringLiteral("present"), condition->conditionPresent},
            {QStringLiteral("latched"), !condition->conditionPresent && condition->latching},
            {QStringLiteral("acknowledged"), condition->acknowledged},
            {QStringLiteral("raisedAt"), condition->raisedAtUtc.toLocalTime()
                                             .toString(QStringLiteral("hh:mm:ss"))}
        });
    }
    return list;
}

void AlarmController::refreshBanner()
{
    const Condition *dominant = dominantCondition();
    const bool nextActive = dominant != nullptr;
    const QString nextPriority = dominant ? priorityName(dominant->priority)
                                          : QStringLiteral("Normal");
    const QString nextHeadline = dominant ? dominant->headline
                                          : QStringLiteral("No Active Alarms");
    const QString nextDetail = dominant ? dominant->detail
                                        : QStringLiteral("System normal");

    const bool changed = m_active != nextActive
                      || m_priority != nextPriority
                      || m_headline != nextHeadline
                      || m_detail != nextDetail;
    if (!changed)
        return;

    m_active = nextActive;
    m_priority = nextPriority;
    m_headline = nextHeadline;
    m_detail = nextDetail;
    emit bannerChanged();
    emit audioChanged();
}

// ---------------------------------------------------------------------------
//  Derived live state
// ---------------------------------------------------------------------------

QString AlarmController::highestPriority() const
{
    const Condition *dominant = dominantCondition();
    return dominant ? priorityName(dominant->priority) : QStringLiteral("none");
}

int AlarmController::activeCount() const
{
    int count = 0;
    for (const Condition &condition : m_conditions) {
        if (condition.conditionPresent)
            ++count;
    }
    return count;
}

bool AlarmController::latched() const
{
    for (const Condition &condition : m_conditions) {
        if (!condition.conditionPresent && condition.latching)
            return true;
    }
    return false;
}

bool AlarmController::resettable() const { return latched(); }

// ---------------------------------------------------------------------------
//  Audio state machine
// ---------------------------------------------------------------------------

void AlarmController::pauseAudio(int seconds)
{
    // HARDWARE: pausing must mute the amplifier only. The visual indicators
    // and the remote-nurse-call output stay live - a paused alarm is still an
    // alarm.
    if (m_audioOff)
        return;

    const int clamped = std::clamp(seconds, std::min(kAudioPauseMinSeconds, kAudioPauseMaxSeconds),
                                    std::max(kAudioPauseMinSeconds, kAudioPauseMaxSeconds));
    m_audioPaused = true;
    m_audioPauseRemaining = clamped;
    m_audioPauseTimer.start();

    appendLog(QStringLiteral("Info"), QStringLiteral("low"), QStringLiteral("Operator"),
              QStringLiteral("Alarm audio paused for %1 s").arg(clamped),
              QStringLiteral("Closed"), QString());

    emit audioChanged();

    if (m_database) {
        m_database->logEvent(QStringLiteral("Alarm"),
                             QStringLiteral("Alarm audio paused for %1 seconds")
                                 .arg(clamped));
    }
}

void AlarmController::resumeAudio()
{
    if (!m_audioPaused)
        return;
    m_audioPaused = false;
    m_audioPauseRemaining = 0;
    m_audioPauseTimer.stop();

    appendLog(QStringLiteral("Info"), QStringLiteral("low"), QStringLiteral("Operator"),
              QStringLiteral("Alarm audio pause cancelled"),
              QStringLiteral("Closed"), QString());

    emit audioChanged();

    if (m_database) {
        m_database->logEvent(QStringLiteral("Alarm"),
                             QStringLiteral("Alarm audio pause cancelled by operator"));
    }
}

void AlarmController::setAudioOff(bool off)
{
    if (m_audioOff == off)
        return;
    m_audioOff = off;
    if (off) {
        m_audioPaused = false;
        m_audioPauseRemaining = 0;
        m_audioPauseTimer.stop();
    }

    // AUDIO OFF is indefinite, so it is logged at a higher level than a pause
    // and is expected to be visible on the alarm screen for as long as it lasts.
    appendLog(off ? QStringLiteral("Warning") : QStringLiteral("Info"),
              off ? QStringLiteral("medium") : QStringLiteral("low"),
              QStringLiteral("Operator"),
              off ? QStringLiteral("Alarm audio switched OFF indefinitely")
                  : QStringLiteral("Alarm audio switched back ON"),
              QStringLiteral("Closed"), QString());

    emit audioChanged();

    if (m_database) {
        m_database->logEvent(QStringLiteral("Alarm"),
                             off ? QStringLiteral("Alarm audio disabled")
                                 : QStringLiteral("Alarm audio enabled"));
    }
}

bool AlarmController::audioPaused() const { return m_audioPaused; }
bool AlarmController::audioOff() const { return m_audioOff; }
int AlarmController::audioPauseRemaining() const { return m_audioPauseRemaining; }
int AlarmController::audioPauseMaxSeconds() const { return kAudioPauseMaxSeconds; }

bool AlarmController::audioActive() const
{
    return m_active && !m_audioPaused && !m_audioOff;
}

bool AlarmController::silenced() const { return m_audioPaused || m_audioOff; }
int AlarmController::silenceRemaining() const { return m_audioPauseRemaining; }

// ---------------------------------------------------------------------------
//  Banner accessors
// ---------------------------------------------------------------------------

bool AlarmController::active() const { return m_active; }
QString AlarmController::priority() const { return m_priority; }
QString AlarmController::headline() const { return m_headline; }
QString AlarmController::detail() const { return m_detail; }

void AlarmController::setActive(bool value)
{
    if (m_active == value)
        return;
    m_active = value;
    emit bannerChanged();
    emit audioChanged();
}

void AlarmController::setPriority(const QString &value)
{
    if (m_priority == value)
        return;
    m_priority = value;
    emit bannerChanged();
}

void AlarmController::setHeadline(const QString &value)
{
    if (m_headline == value)
        return;
    m_headline = value;
    emit bannerChanged();
}

void AlarmController::setDetail(const QString &value)
{
    if (m_detail == value)
        return;
    m_detail = value;
    emit bannerChanged();
}

// ---------------------------------------------------------------------------
//  Pre-rework API
// ---------------------------------------------------------------------------

void AlarmController::raiseAlarm(const QString &priority,
                                 const QString &source,
                                 const QString &headline,
                                 const QString &detail)
{
    // Synthesises a stable id so repeated calls from the old code path update
    // one condition instead of accumulating duplicates.
    const QString conditionId = QStringLiteral("legacy.%1.%2")
                                    .arg(source.toLower(), headline.toLower());
    raiseCondition(conditionId, priority, source, headline, detail,
                   true);
}

