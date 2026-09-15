#include "EventController.h"
#include "../core/DatabaseManager.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include <utility>

EventController::EventController(DatabaseManager *database, QObject *parent)
    : QAbstractListModel(parent)
    , m_database(database)
{
    loadFromDatabase();
    applyFilter();

    if (m_database) {
        connect(m_database, &DatabaseManager::eventLogged,
                this, &EventController::appendRow);
    }
}

int EventController::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant EventController::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};

    const EventRow &row = m_rows.at(index.row());

    switch (role) {
    case TimeRole:        return row.time;
    case SourceRole:      return row.source;
    case DescriptionRole: return row.description;
    case SeverityRole:    return row.severity;
    default:              return {};
    }
}

QHash<int, QByteArray> EventController::roleNames() const
{
    return {
        { TimeRole,        "time" },
        { SourceRole,      "source" },
        { DescriptionRole, "description" },
        { SeverityRole,    "severity" }
    };
}

void EventController::addEvent(const QString &source,
                                const QString &description,
                                const QString &severity)
{
    // The database echoes every accepted write back through eventLogged, so
    // the row is added there rather than twice.
    if (m_database) {
        m_database->logEvent(source, description, severity);
        return;
    }

    appendRow(source, description, severity);
}

void EventController::appendRow(const QString &source,
                                 const QString &description,
                                 const QString &severity)
{
    const QString timestamp = QDateTime::currentDateTime().toString(
        QStringLiteral("HH:mm:ss"));

    QString level = severity.toLower();
    if (level != QStringLiteral("critical") && level != QStringLiteral("warning"))
        level = QStringLiteral("normal");

    const EventRow row{ timestamp, source, description, level };
    m_all.prepend(row);

    if (!matches(row))
        return;

    beginInsertRows(QModelIndex(), 0, 0);
    m_rows.prepend(row);
    endInsertRows();
    emit countChanged();
}

void EventController::refresh()
{
    m_all.clear();
    loadFromDatabase();
    applyFilter();
}

void EventController::setFilter(const QString &filter)
{
    const QString key = filter.isEmpty() ? QStringLiteral("all") : filter;
    if (key == m_filter)
        return;

    m_filter = key;
    emit filterChanged();
    applyFilter();
}

bool EventController::matches(const EventRow &row) const
{
    if (m_filter == QStringLiteral("all"))
        return true;

    const QString source = row.source.toLower();

    if (m_filter == QStringLiteral("alarm")) {
        return source == QStringLiteral("alarm")
            || source == QStringLiteral("safety")
            || row.severity == QStringLiteral("critical")
            || row.severity == QStringLiteral("warning");
    }

    if (m_filter == QStringLiteral("setting")) {
        return source == QStringLiteral("setting")
            || source == QStringLiteral("mode")
            || source == QStringLiteral("parameter")
            || source == QStringLiteral("ventilation")
            || source == QStringLiteral("patient")
            || source == QStringLiteral("calibration");
    }

    return true;
}

void EventController::applyFilter()
{
    beginResetModel();
    m_rows.clear();
    m_rows.reserve(m_all.size());
    for (const EventRow &row : std::as_const(m_all)) {
        if (matches(row))
            m_rows.append(row);
    }
    endResetModel();
    emit countChanged();
}

void EventController::loadFromDatabase()
{
    if (!m_database)
        return;

    QSqlQuery query(QSqlDatabase::database(
        QStringLiteral("SmartVentilatorConnection")));

    if (!query.exec(QStringLiteral(
            "SELECT created_at, source, description, status "
            "FROM events ORDER BY id DESC LIMIT 200"))) {
        qWarning() << "Unable to load events:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        const QDateTime dt = QDateTime::fromString(
            query.value(0).toString(), Qt::ISODate);
        const QString timeStr = dt.isValid()
            ? dt.toLocalTime().toString(QStringLiteral("HH:mm:ss"))
            : query.value(0).toString();

        // Map legacy status values to severity for row coloring.
        // Old records may store "Recorded", "Active", "Passed", etc.
        QString severity = query.value(3).toString().toLower();
        if (severity != QStringLiteral("critical")
            && severity != QStringLiteral("warning")) {
            severity = QStringLiteral("normal");
        }

        m_all.append({
            timeStr,
            query.value(1).toString(),
            query.value(2).toString(),
            severity
        });
    }
}
