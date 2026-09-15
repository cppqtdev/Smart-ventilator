#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QTimer>
#include <QVariant>
#include <QVector>

class DatabaseManager;

/**
 * @brief Alarm system model and state machine.
 *
 * Implements the parts of IEC 60601-1-8 and ISO 80601-2-12 that the user
 * interface depends on:
 *
 *  - **Simultaneous conditions.** Conditions are held in a registry keyed by a
 *    stable id, not collapsed into a single banner. Clause 6.1.2 requires each
 *    active alarm condition to be indicated individually unless an intelligent
 *    alarm system deliberately suppresses it, so the UI needs the whole set and
 *    the highest priority among them, not just the most recent one.
 *
 *  - **Priority as a function of state.** raiseCondition() takes the priority
 *    each time it is called, so a condition whose severity escalates - internal
 *    power depletion goes medium to high within five minutes under
 *    ISO 80601-2-12 - is re-raised at its new priority rather than modelled as
 *    a constant.
 *
 *  - **Latching.** High-priority physiological conditions latch by default: the
 *    indication persists after the condition clears until the operator resets
 *    it, so a transient disconnection or apnoea cannot pass unnoticed.
 *    Technical and low-priority conditions do not latch. The behaviour is a
 *    per-condition flag rather than hardcoded, because it is a risk-management
 *    decision that has to be documented per condition.
 *
 *  - **Three distinct audio states.** AUDIO PAUSED is time-limited and reverts
 *    on its own; AUDIO OFF is indefinite and deliberate; normal is neither. The
 *    standard treats them as different states and so does this class. The pause
 *    ceiling is 120 s - the ventilator-specific limit from ISO 80601-2-12,
 *    which overrides the more permissive general allowance in 60601-1-8.
 *
 * The list model rows are the alarm *log* (every raise, clear, acknowledge and
 * reset, timestamped). The registry is the *live* state. Keeping them separate
 * is what lets the log satisfy the retrospective-review expectation without the
 * live view drifting out of sync with it.
 */
class AlarmController : public QAbstractListModel
{
    Q_OBJECT

    // -- Live alarm state -------------------------------------------------
    /** Normalised highest active priority: "high", "medium", "low", "none". */
    Q_PROPERTY(QString highestPriority READ highestPriority NOTIFY conditionsChanged)
    /** Number of currently active conditions. */
    Q_PROPERTY(int activeCount READ activeCount NOTIFY conditionsChanged)
    /** True when at least one latched condition is awaiting operator reset. */
    Q_PROPERTY(bool latched READ latched NOTIFY conditionsChanged)
    /** True when a reset would do something. */
    Q_PROPERTY(bool resettable READ resettable NOTIFY conditionsChanged)

    // -- Audio state ------------------------------------------------------
    /** True while AUDIO PAUSED. Time-limited, reverts automatically. */
    Q_PROPERTY(bool audioPaused READ audioPaused NOTIFY audioChanged)
    /** True while AUDIO OFF. Indefinite, requires deliberate re-enable. */
    Q_PROPERTY(bool audioOff READ audioOff NOTIFY audioChanged)
    /** Seconds left in the pause, 0 when not paused. */
    Q_PROPERTY(int audioPauseRemaining READ audioPauseRemaining NOTIFY audioChanged)
    /** The ISO 80601-2-12 ceiling, exposed so the UI never hardcodes it. */
    Q_PROPERTY(int audioPauseMaxSeconds READ audioPauseMaxSeconds CONSTANT)
    /** True when the tone should be sounding. */
    Q_PROPERTY(bool audioActive READ audioActive NOTIFY audioChanged)

    // -- Banner compatibility ---------------------------------------------
    // The highest-priority active condition, surfaced under the property
    // names the pre-rework screens already bind to.
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY bannerChanged)
    Q_PROPERTY(QString priority READ priority WRITE setPriority NOTIFY bannerChanged)
    Q_PROPERTY(QString headline READ headline WRITE setHeadline NOTIFY bannerChanged)
    Q_PROPERTY(QString detail READ detail WRITE setDetail NOTIFY bannerChanged)
    Q_PROPERTY(bool silenced READ silenced NOTIFY audioChanged)
    Q_PROPERTY(int silenceRemaining READ silenceRemaining NOTIFY audioChanged)

    // -- Log ---------------------------------------------------------------
    Q_PROPERTY(QString filterPriority READ filterPriority WRITE setFilterPriority NOTIFY filterChanged)
    Q_PROPERTY(int alarmCount READ alarmCount NOTIFY filterChanged)

    /// Top conditions as { priority: 3|2|1, text } for the header banner.
    Q_PROPERTY(QVariantList banner READ banner NOTIFY conditionsChanged)

public:
    enum AlarmRoles {
        TimeRole = Qt::UserRole + 1,
        PriorityRole,
        SourceRole,
        DescriptionRole,
        StatusRole,
        ConditionIdRole,
        NormalisedPriorityRole
    };
    Q_ENUM(AlarmRoles)

    /** Normalised priority ranking. Higher is more urgent. */
    enum class Priority {
        None = 0,
        Low = 1,
        Medium = 2,
        High = 3
    };
    Q_ENUM(Priority)

    explicit AlarmController(DatabaseManager *database, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // -- Condition lifecycle ----------------------------------------------
    /**
     * @brief Raises a condition, or updates it if already active.
     * @param conditionId Stable identifier, e.g. "paw.high", "circuit.disconnect".
     *        Re-raising the same id updates in place instead of duplicating.
     * @param priority "High"/"Critical", "Medium"/"Warning", or "Low"/"Info".
     * @param source Subsystem that detected it, e.g. "Pressure".
     * @param headline Short operator-facing condition name.
     * @param detail One line of actionable context.
     * @param latching Request latching. Honoured only where the priority
     *        policy permits it - currently high priority alone - so passing
     *        true on a medium condition still gives non-latching behaviour.
     */
    Q_INVOKABLE void raiseCondition(const QString &conditionId,
                                    const QString &priority,
                                    const QString &source,
                                    const QString &headline,
                                    const QString &detail,
                                    bool latching = true);

    /**
     * @brief Marks a condition as no longer detected.
     *
     * A non-latching condition disappears. A latching one stays indicated,
     * flagged as cleared, until resetLatched().
     */
    Q_INVOKABLE void clearCondition(const QString &conditionId);

    /** @return True if the named condition is currently active. */
    Q_INVOKABLE bool isConditionActive(const QString &conditionId) const;

    /** @brief Acknowledges every active condition without clearing any. */
    Q_INVOKABLE void acknowledgeAll();

    /** @brief Acknowledges one condition. */
    Q_INVOKABLE void acknowledgeCondition(const QString &conditionId);

    /** @brief Drops every latched-but-cleared condition. */
    Q_INVOKABLE void resetLatched();

    /** @return Active conditions, highest priority first, as QML-friendly maps. */
    QVariantList banner() const;

    Q_INVOKABLE QVariantList activeConditions() const;

    // -- Audio state machine ----------------------------------------------
    /**
     * @brief Enters AUDIO PAUSED.
     * @param seconds Clamped to [10, 120]. The 120 s ceiling is the
     *        ISO 80601-2-12 ventilator limit, not the looser general one.
     */
    Q_INVOKABLE void pauseAudio(int seconds = 120);
    /** @brief Leaves AUDIO PAUSED immediately. */
    Q_INVOKABLE void resumeAudio();
    /** @brief Enters or leaves AUDIO OFF. Indefinite and deliberate. */
    Q_INVOKABLE void setAudioOff(bool off);

    QString highestPriority() const;
    int activeCount() const;
    bool latched() const;
    bool resettable() const;

    bool audioPaused() const;
    bool audioOff() const;
    int audioPauseRemaining() const;
    int audioPauseMaxSeconds() const;
    bool audioActive() const;

    bool active() const;
    QString priority() const;
    QString headline() const;
    QString detail() const;
    bool silenced() const;
    int silenceRemaining() const;

    QString filterPriority() const;
    int alarmCount() const;
    Q_INVOKABLE void setFilterPriority(const QString &priority);

    /** @brief Appends a row to the alarm log without touching live state. */
    Q_INVOKABLE void addAlarm(const QString &priority,
                              const QString &source,
                              const QString &description,
                              const QString &status);

    // -- Pre-rework API ----------------------------------------------------
    /** @deprecated Use raiseCondition(). Derives an id from source + headline. */
    Q_INVOKABLE void raiseAlarm(const QString &priority,
                                const QString &source,
                                const QString &headline,
                                const QString &detail);
    /** @deprecated Use acknowledgeAll(). */

    /** @brief Maps a vendor priority string onto the normalised ranking. */
    static Priority normalisePriority(const QString &priority);
    /** @brief Lowercase name of a normalised priority, as QML consumes it. */
    static QString priorityName(Priority priority);

public slots:
    void setActive(bool value);
    void setPriority(const QString &value);
    void setHeadline(const QString &value);
    void setDetail(const QString &value);

signals:
    void conditionsChanged();
    void bannerChanged();
    void audioChanged();
    void filterChanged();
    /** Emitted once per transition into an active state, for the tone engine. */
    void alarmRaised(const QString &conditionId, const QString &priority);

private:
    struct Condition {
        QString id;
        Priority priority = Priority::None;
        QString source;
        QString headline;
        QString detail;
        bool latching = true;
        bool conditionPresent = false;   // sensor still reports it
        bool acknowledged = false;
        QDateTime raisedAtUtc;
        QDateTime clearedAtUtc;
    };

    struct LogRow {
        QString time;
        QString priority;          // as supplied, for the existing filter UI
        QString normalisedPriority;
        QString source;
        QString description;
        QString status;
        QString conditionId;
    };

    void appendLog(const QString &priority,
                   const QString &normalised,
                   const QString &source,
                   const QString &description,
                   const QString &status,
                   const QString &conditionId);
    void rebuildFilteredIndices();
    void refreshBanner();
    const Condition *dominantCondition() const;

    QVector<LogRow> m_rows;
    QVector<int> m_filteredIndices;
    QString m_filterPriority;

    QHash<QString, Condition> m_conditions;
    QVector<QString> m_order;   // insertion order, for stable tie-breaking

    DatabaseManager *m_database = nullptr;

    // Banner mirror of the dominant condition.
    bool m_active = false;
    QString m_priority = QStringLiteral("Normal");
    QString m_headline = QStringLiteral("No Active Alarms");
    QString m_detail = QStringLiteral("System normal");

    bool m_audioPaused = false;
    bool m_audioOff = false;
    int m_audioPauseRemaining = 0;
    QTimer m_audioPauseTimer;

    static constexpr int kAudioPauseMaxSeconds = 120;  // ISO 80601-2-12
    static constexpr int kAudioPauseMinSeconds = 10;
};
