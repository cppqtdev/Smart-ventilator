// -----------------------------------------------------------------------
// File: CalibrationService.h
// Description: Runs the device self tests and sensor calibrations
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The three procedures on the Tests and Calibration page are real device
// preconditions, not decoration. A tightness test that says "passed" without
// having pressurised anything is worse than no test at all, so this class
// owns the whole run: the precondition check, the timed sequence, the
// measured result, the pass or fail decision and the record that is written
// afterwards. The view only starts a run and draws what this reports.
//
// TODO: pressure, flow and oxygen readings come from the simulator. Replace
// readMeasurement() with the real sensor reads before any clinical use.
//
#ifndef SV_SERVICES_CALIBRATIONSERVICE_H
#define SV_SERVICES_CALIBRATIONSERVICE_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QElapsedTimer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>
#include <QVector>

#include <sv/domain/CalibrationResult.h>

namespace sv::services {

class CalibrationService : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ busy NOTIFY runChanged)
    Q_PROPERTY(QString activeKey READ activeKey NOTIFY runChanged)
    Q_PROPERTY(QString activeName READ activeName NOTIFY runChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY progressChanged)
    Q_PROPERTY(bool ventilating READ ventilating WRITE setVentilating NOTIFY ventilatingChanged)
    Q_PROPERTY(bool allPassed READ allPassed NOTIFY runChanged)
    Q_PROPERTY(QString lastRejection READ lastRejection NOTIFY rejectionChanged)

public:
    enum State {
        Idle,
        Running,
        Passed,
        Failed,
        Aborted
    };
    Q_ENUM(State)

    enum Roles {
        KeyRole = Qt::UserRole + 1,
        NameRole,
        StateRole,
        StateNameRole,
        StampRole,
        MessageRole,
        MeasuredRole,
        RequiresStandbyRole,
        DurationRole
    };

    explicit CalibrationService(QObject *parent = nullptr);
    ~CalibrationService() override;

    /**
     * @brief Where results are recorded.
     *
     * Taken as a plain QObject and called through the meta object, so this
     * service does not have to know which of the two database classes the
     * build it lands in actually uses. May be null in tests.
     */
    void attach(QObject *recorder);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool busy() const;
    QString activeKey() const;
    QString activeName() const;
    int progress() const;
    QString statusText() const;
    bool ventilating() const;
    void setVentilating(bool ventilating);
    bool allPassed() const;
    QString lastRejection() const;

    /**
     * @brief Starts one procedure.
     * @return false when a precondition blocks it; lastRejection says why.
     */
    Q_INVOKABLE bool start(const QString &key);

    /** @brief Runs every procedure in order, stopping on the first failure. */
    Q_INVOKABLE bool startAll();

    /** @brief Stops the run in progress and marks it aborted. */
    Q_INVOKABLE void cancel();

    /**
     * @brief Marks a procedure's sensor as faulty so the next run fails.
     *
     * The service reads a healthy value by default. This is how the failure
     * path is exercised in a test and how a real sensor fault, once the
     * hardware read lands, will be reported.
     */
    Q_INVOKABLE void setSensorFaulty(const QString &key, bool faulty);

    /** @brief One row as a map, for a view that wants a single entry. */
    Q_INVOKABLE QVariantMap entryFor(const QString &key) const;

    /** @brief Human readable name for a state value. */
    Q_INVOKABLE static QString stateName(int state);

    /** @brief Every procedure as a domain result, for the backend facade. */
    QVector<sv::domain::CalibrationResult> results() const;

    /** @brief Name based entry point kept for the backend facade. */
    void runTest(const QString &testName);

    /** @brief Name based entry point kept for the backend facade. */
    void runAllTests();

signals:
    void runChanged();
    void resultsChanged();
    void progressChanged();
    void ventilatingChanged();
    void rejectionChanged();
    void finished(const QString &key, bool passed, const QString &message);
    void rejected(const QString &key, const QString &reason);

private slots:
    void onTick();

private:
    struct Procedure {
        QString key;
        QString name;
        QString unit;
        int durationMs = 0;
        bool requiresStandby = false;
        double nominal = 0.0;
        double tolerance = 0.0;
        QStringList steps;

        bool faulty = false;

        State state = Idle;
        QDateTime stamp;
        QString message;
        double measured = 0.0;
    };

    int indexOf(const QString &key) const;
    void beginRun(int row);
    void completeRun();
    void abortRun(State reason, const QString &message);
    double readMeasurement(const Procedure &procedure) const;
    void publishRow(int row);
    void recordResult(const Procedure &procedure);
    void reject(const QString &key, const QString &reason);

    QVector<Procedure> m_procedures;
    QObject *m_recorder = nullptr;

    QTimer m_timer;
    QElapsedTimer m_clock;
    int m_activeRow = -1;
    int m_progress = 0;
    QString m_statusText;
    QString m_lastRejection;
    bool m_ventilating = false;
    QStringList m_queue;
};

} // namespace sv::services

#endif // SV_SERVICES_CALIBRATIONSERVICE_H
