#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class DatabaseManager;

/**
 * @brief QML list model for the clinical event timeline.
 *
 * EventController reads event records from the SQLite database and exposes
 * them as a QAbstractListModel for the EventsScreen. New events are inserted
 * through the addEvent method and immediately appear in the model.
 */
class EventController : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum EventRoles {
        TimeRole = Qt::UserRole + 1,
        SourceRole,
        DescriptionRole,
        SeverityRole
    };

    /**
     * @param database  Shared database manager for event persistence.
     * @param parent  Optional QObject parent for ownership.
     */
    explicit EventController(DatabaseManager *database, QObject *parent = nullptr);

    /** @return Number of event rows in the model. */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /** @return Data for the given event row and role. */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /** @return Mapping of EventRoles to QML role name strings. */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Inserts a new event at the top of the model and persists it to the database.
     * @param source  Subsystem that generated the event (e.g. "Mode", "Parameter", "Alarm").
     * @param description  Human-readable event detail.
     * @param severity  Visual severity hint ("normal", "warning", "critical").
     */
    Q_INVOKABLE void addEvent(const QString &source,
                               const QString &description,
                               const QString &severity = QStringLiteral("normal"));

    /** @brief Reloads all events from the database into the model. */
    Q_INVOKABLE void refresh();

    /** @brief Adds one already-persisted event to the top of the model. */
    void appendRow(const QString &source,
                   const QString &description,
                   const QString &severity);

    /** @return The active tab key: "all", "alarm" or "setting". */
    QString filter() const { return m_filter; }

    /** @brief Shows only the rows belonging to @p filter. */
    void setFilter(const QString &filter);

signals:
    void filterChanged();
    void countChanged();

private:
    void loadFromDatabase();

    struct EventRow {
        QString time;
        QString source;
        QString description;
        QString severity;
    };

    /** @return true when @p row belongs in the current tab. */
    bool matches(const EventRow &row) const;

    /** @brief Rebuilds the visible rows from the loaded set. */
    void applyFilter();

    // Every loaded row is kept, because a tab change is a view change and
    // must not cost a database round trip.
    QVector<EventRow> m_all;
    QVector<EventRow> m_rows;
    QString m_filter = QStringLiteral("all");
    DatabaseManager *m_database = nullptr;
};
