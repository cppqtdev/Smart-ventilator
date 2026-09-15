// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QThread>
#include <QVariantMap>
#include <QCryptographicHash>

namespace sv::infrastructure {

class DatabaseWriteWorker;

class DatabaseManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)
    Q_PROPERTY(bool ready READ ready NOTIFY storageStateChanged)
    Q_PROPERTY(bool readOnly READ readOnly NOTIFY storageStateChanged)
    Q_PROPERTY(bool degraded READ degraded NOTIFY storageStateChanged)
    Q_PROPERTY(QString storageState READ storageState NOTIFY storageStateChanged)

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;

    /// @brief Opens the SQLite database and creates missing tables.
    bool initialize();

    /// @brief Records a user/system event in the audit log.
    Q_INVOKABLE void logEvent(const QString &source,
                              const QString &description,
                              const QString &status = QStringLiteral("Recorded"));

    /// @brief Records an alarm transition in the alarm history table.
    Q_INVOKABLE void logAlarm(const QString &priority,
                              const QString &source,
                              const QString &description,
                              const QString &status);

    /// @brief Stores a ventilator parameter snapshot for trend/history use.
    void saveParameterSnapshot(const QVariantMap &snapshot);

    /// @brief Saves or updates a patient profile in the database.
    void savePatientProfile(const QVariantMap &profile);

    /// @brief Loads the most recent patient profile from the database.
    QVariantMap loadLastPatientProfile();

    /// @brief Returns the absolute SQLite database path.
    Q_INVOKABLE QString databasePath() const;

    /// @brief Most recent database error message, empty if no error.
    QString lastError() const;
    bool ready() const;
    bool readOnly() const;
    bool degraded() const;
    QString storageState() const;

    /// @brief Verifies the SHA-256 hash chain integrity of the events table.
    Q_INVOKABLE bool verifyAuditTrail();

    /// @brief Queries parameter snapshots from the last N minutes.
    Q_INVOKABLE QVariantList getParameterHistory(int minutes) const;

    /// @brief CSV-style summary of current clinical data for controlled export.
    Q_INVOKABLE QString exportClinicalSummary() const;

    /// @brief Persists a named clinical UI state value.
    Q_INVOKABLE void saveClinicalState(const QString &key, const QVariant &value);

    /// @brief All persisted clinical UI state values.
    Q_INVOKABLE QVariantMap loadClinicalState() const;

    /// @brief Records a spontaneous breathing trial session.
    Q_INVOKABLE void recordSbtSession(const QVariantMap &session);

    /// @brief Records a maintenance action against a service item.
    Q_INVOKABLE void recordMaintenance(const QString &item, const QString &action);

    /// @brief Recent spontaneous breathing trial sessions.
    Q_INVOKABLE QVariantList getSbtHistory(int limit = 20) const;

    /// @brief Recent maintenance log entries.
    Q_INVOKABLE QVariantList getMaintenanceHistory(int limit = 20) const;

    /// @brief Records a respiratory maneuver result.
    Q_INVOKABLE void recordManeuver(const QString &type, double result,
                                    const QString &unit, const QString &notes);

    /// @brief Recent respiratory maneuver results.
    Q_INVOKABLE QVariantList getManeuverHistory(int limit = 20) const;

    /// @brief Saves service schedule metadata for one maintenance item.
    Q_INVOKABLE void saveMaintenanceSchedule(const QString &item,
                                             const QString &dueDate,
                                             bool acknowledged);

    /// @brief Persisted service schedule metadata.
    Q_INVOKABLE QVariantList getMaintenanceSchedules() const;

    /// @brief Saves one central-monitor patient tile snapshot.
    Q_INVOKABLE void saveCentralPatient(const QVariantMap &patient);

    /// @brief Central-monitor patient tile snapshots.
    Q_INVOKABLE QVariantList getCentralPatients() const;

    /// @brief CSV-style audit export combining events and alarms.
    Q_INVOKABLE QString exportAuditSummary() const;

signals:
    void errorOccurred(const QString &message);
    void storageStateChanged();

private:
    void setError(const QString &message);
    void setStorageState(bool ready, bool readOnly, bool degraded, const QString &state);
    bool executeSchema();
    bool checkStorageHealth();
    void startAsyncWriter();
    void stopAsyncWriter();

    QString m_databasePath;
    QString m_lastError;
    QString m_storageState = QStringLiteral("Not initialized");
    QSqlDatabase m_database;
    QThread m_writerThread;
    DatabaseWriteWorker *m_writer = nullptr;
    bool m_ready = false;
    bool m_readOnly = false;
    bool m_degraded = true;
};

} // namespace sv::infrastructure
