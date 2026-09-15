// -----------------------------------------------------------------------
// File: CalibrationService.cpp
// Description: Runs the device self tests and sensor calibrations
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
#include "sv/services/CalibrationService.h"

#include "sv/common/LogBuffer.h"

#include <QMetaObject>
#include <QRandomGenerator>
#include <QtGlobal>

#include <cmath>

namespace sv::services {

namespace {

constexpr int kTickMs = 100;

} // namespace

CalibrationService::CalibrationService(QObject *parent)
    : QAbstractListModel(parent)
{
    // Durations and tolerances follow the operator manual values for a
    // turbine driven intensive care ventilator. The tightness test has to
    // hold a static pressure, which is why it is the long one.
    Procedure tightness;
    tightness.key = QStringLiteral("tightness");
    tightness.name = tr("Tightness");
    tightness.unit = QStringLiteral("l/min");
    tightness.durationMs = 12000;
    tightness.requiresStandby = true;
    tightness.nominal = 0.8;
    tightness.tolerance = 2.0;
    tightness.steps = QStringList{
        tr("Occlude the patient connection"),
        tr("Pressurising the circuit to 30 cmH2O"),
        tr("Holding pressure"),
        tr("Measuring leak")
    };

    Procedure flow;
    flow.key = QStringLiteral("flowSensor");
    flow.name = tr("Flow Sensor");
    flow.unit = QStringLiteral("l/min");
    flow.durationMs = 8000;
    flow.requiresStandby = true;
    flow.nominal = 0.05;
    flow.tolerance = 0.5;
    flow.steps = QStringList{
        tr("Checking the sensor is connected"),
        tr("Zeroing at no flow"),
        tr("Verifying the zero offset")
    };

    // A two point oxygen cell calibration runs against room air and the wall
    // supply, and does not disturb the breath delivery, so it is the one
    // procedure that is allowed while the patient is being ventilated.
    Procedure oxygen;
    oxygen.key = QStringLiteral("o2Cell");
    oxygen.name = tr("O2 Cell");
    oxygen.unit = QStringLiteral("%");
    oxygen.durationMs = 20000;
    oxygen.requiresStandby = false;
    oxygen.nominal = 21.0;
    oxygen.tolerance = 2.0;
    oxygen.steps = QStringList{
        tr("Flushing with room air"),
        tr("Reading the 21 percent point"),
        tr("Flushing with 100 percent oxygen"),
        tr("Reading the 100 percent point"),
        tr("Writing the cell curve")
    };

    m_procedures = {tightness, flow, oxygen};

    m_timer.setInterval(kTickMs);
    m_timer.setTimerType(Qt::CoarseTimer);
    connect(&m_timer, &QTimer::timeout, this, &CalibrationService::onTick);
}

CalibrationService::~CalibrationService() = default;

void CalibrationService::attach(QObject *recorder)
{
    m_recorder = recorder;
}

int CalibrationService::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_procedures.size());
}

QVariant CalibrationService::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_procedures.size())
        return {};

    const Procedure &procedure = m_procedures.at(index.row());
    switch (role) {
    case KeyRole:             return procedure.key;
    case NameRole:            return procedure.name;
    case StateRole:           return int(procedure.state);
    case StateNameRole:       return stateName(int(procedure.state));
    case StampRole:
        return procedure.stamp.isValid()
             ? procedure.stamp.toString(QStringLiteral("yyyy-MM-dd hh:mm"))
             : QString();
    case MessageRole:         return procedure.message;
    case MeasuredRole:        return procedure.measured;
    case RequiresStandbyRole: return procedure.requiresStandby;
    case DurationRole:        return procedure.durationMs;
    default:                  return {};
    }
}

QHash<int, QByteArray> CalibrationService::roleNames() const
{
    return {
        {KeyRole,             "key"},
        {NameRole,            "name"},
        {StateRole,           "state"},
        {StateNameRole,       "stateName"},
        {StampRole,           "stamp"},
        {MessageRole,         "message"},
        {MeasuredRole,        "measured"},
        {RequiresStandbyRole, "requiresStandby"},
        {DurationRole,        "durationMs"}
    };
}

bool CalibrationService::busy() const
{
    return m_activeRow >= 0;
}

QString CalibrationService::activeKey() const
{
    if (m_activeRow < 0 || m_activeRow >= m_procedures.size())
        return {};
    return m_procedures.at(m_activeRow).key;
}

QString CalibrationService::activeName() const
{
    if (m_activeRow < 0 || m_activeRow >= m_procedures.size())
        return {};
    return m_procedures.at(m_activeRow).name;
}

int CalibrationService::progress() const
{
    return m_progress;
}

QString CalibrationService::statusText() const
{
    return m_statusText;
}

bool CalibrationService::ventilating() const
{
    return m_ventilating;
}

void CalibrationService::setVentilating(bool ventilating)
{
    if (m_ventilating == ventilating)
        return;
    m_ventilating = ventilating;
    emit ventilatingChanged();

    // Starting ventilation during a procedure that needs standby ends it.
    // Silently continuing would report a leak figure measured against a
    // moving circuit pressure.
    if (m_ventilating && m_activeRow >= 0
            && m_procedures.at(m_activeRow).requiresStandby) {
        abortRun(Aborted, tr("Stopped because ventilation started"));
    }
}

bool CalibrationService::allPassed() const
{
    for (const Procedure &procedure : m_procedures) {
        if (procedure.state != Passed)
            return false;
    }
    return true;
}

QString CalibrationService::lastRejection() const
{
    return m_lastRejection;
}

QString CalibrationService::stateName(int state)
{
    switch (state) {
    case Running: return tr("Running");
    case Passed:  return tr("Passed");
    case Failed:  return tr("Failed");
    case Aborted: return tr("Stopped");
    default:      return tr("Not run");
    }
}

int CalibrationService::indexOf(const QString &key) const
{
    for (int row = 0; row < m_procedures.size(); ++row) {
        if (m_procedures.at(row).key == key)
            return row;
    }
    return -1;
}

bool CalibrationService::start(const QString &key)
{
    const int row = indexOf(key);
    if (row < 0) {
        reject(key, tr("No such procedure"));
        return false;
    }
    if (busy()) {
        reject(key, tr("%1 is already running").arg(activeName()));
        return false;
    }
    if (m_procedures.at(row).requiresStandby && m_ventilating) {
        reject(key, tr("%1 needs the ventilator in standby")
                        .arg(m_procedures.at(row).name));
        return false;
    }

    m_queue.clear();
    beginRun(row);
    return true;
}

bool CalibrationService::startAll()
{
    if (busy()) {
        reject(QString(), tr("%1 is already running").arg(activeName()));
        return false;
    }

    m_queue.clear();
    for (const Procedure &procedure : m_procedures) {
        if (procedure.requiresStandby && m_ventilating)
            continue;
        m_queue.append(procedure.key);
    }
    if (m_queue.isEmpty()) {
        reject(QString(), tr("Every procedure needs the ventilator in standby"));
        return false;
    }

    const QString first = m_queue.takeFirst();
    beginRun(indexOf(first));
    return true;
}

void CalibrationService::beginRun(int row)
{
    if (row < 0 || row >= m_procedures.size())
        return;

    m_activeRow = row;
    m_progress = 0;
    Procedure &procedure = m_procedures[row];
    procedure.state = Running;
    procedure.message = procedure.steps.isEmpty() ? QString() : procedure.steps.first();
    m_statusText = procedure.message;
    m_lastRejection.clear();

    m_clock.restart();
    m_timer.start();

    if (auto *log = sv::common::LogBuffer::instance()) {
        log->note(sv::common::LogBuffer::Info, QStringLiteral("Calibration"),
                  tr("%1 started").arg(procedure.name));
    }

    publishRow(row);
    emit rejectionChanged();
    emit runChanged();
    emit resultsChanged();
    emit progressChanged();
}

void CalibrationService::onTick()
{
    if (m_activeRow < 0) {
        m_timer.stop();
        return;
    }

    Procedure &procedure = m_procedures[m_activeRow];
    const qint64 elapsed = m_clock.elapsed();
    const qint64 span = procedure.durationMs > 0 ? procedure.durationMs : 1;
    const qint64 scaled = elapsed * 100 / span;
    const int percent = int(scaled < 0 ? 0 : (scaled > 100 ? 100 : scaled));

    if (percent != m_progress) {
        m_progress = percent;

        if (!procedure.steps.isEmpty()) {
            const int stepCount = int(procedure.steps.size());
            const int step = qBound(0, m_progress * stepCount / 100, qMax(0, stepCount - 1));
            const QString text = procedure.steps.at(step);
            if (text != m_statusText) {
                m_statusText = text;
                procedure.message = text;
                publishRow(m_activeRow);
            }
        }
        emit progressChanged();
    }

    if (elapsed >= procedure.durationMs)
        completeRun();
}

double CalibrationService::readMeasurement(const Procedure &procedure) const
{
    // TODO: replace with the real sensor read. A healthy circuit lands well
    // inside the tolerance band and moves between runs the way a measured
    // value does; a sensor marked faulty lands outside it.
    const double spread = procedure.tolerance * (procedure.faulty ? 1.6 : 0.1);
    const double offset = QRandomGenerator::global()->generateDouble() * spread;
    const double sign = procedure.faulty || QRandomGenerator::global()->bounded(2) == 0 ? 1.0 : -1.0;
    return procedure.nominal + sign * (procedure.faulty ? procedure.tolerance + offset : offset);
}

void CalibrationService::setSensorFaulty(const QString &key, bool faulty)
{
    const int row = indexOf(key);
    if (row < 0)
        return;
    m_procedures[row].faulty = faulty;
}

void CalibrationService::completeRun()
{
    m_timer.stop();

    const int row = m_activeRow;
    Procedure &procedure = m_procedures[row];

    procedure.measured = readMeasurement(procedure);
    const double deviation = std::fabs(procedure.measured - procedure.nominal);
    const bool passed = deviation <= procedure.tolerance;

    procedure.state = passed ? Passed : Failed;
    procedure.stamp = QDateTime::currentDateTime();
    procedure.message = passed
        ? tr("%1 %2, within %3 %2")
              .arg(QString::number(procedure.measured, 'f', 2),
                   procedure.unit,
                   QString::number(procedure.tolerance, 'f', 2))
        : tr("%1 %2, outside %3 %2")
              .arg(QString::number(procedure.measured, 'f', 2),
                   procedure.unit,
                   QString::number(procedure.tolerance, 'f', 2));

    m_activeRow = -1;
    m_progress = 100;
    m_statusText = procedure.message;

    recordResult(procedure);
    publishRow(row);
    emit progressChanged();
    emit runChanged();
    emit resultsChanged();
    emit finished(procedure.key, passed, procedure.message);

    // A queued run only continues while the results are good. Calibrating a
    // flow sensor on a circuit that just failed its leak test measures the
    // leak, not the sensor.
    if (passed && !m_queue.isEmpty()) {
        const QString next = m_queue.takeFirst();
        const int nextRow = indexOf(next);
        if (nextRow >= 0)
            beginRun(nextRow);
    } else {
        m_queue.clear();
    }
}

void CalibrationService::abortRun(State reason, const QString &message)
{
    if (m_activeRow < 0)
        return;

    m_timer.stop();
    const int row = m_activeRow;
    Procedure &procedure = m_procedures[row];
    procedure.state = reason;
    procedure.message = message;

    m_activeRow = -1;
    m_progress = 0;
    m_statusText = message;
    m_queue.clear();

    if (auto *log = sv::common::LogBuffer::instance()) {
        log->note(sv::common::LogBuffer::Warning, QStringLiteral("Calibration"),
                  QStringLiteral("%1: %2").arg(procedure.name, message));
    }
    if (m_recorder != nullptr) {
        QMetaObject::invokeMethod(m_recorder, "logEvent",
                                  Q_ARG(QString, QStringLiteral("Calibration")),
                                  Q_ARG(QString, QStringLiteral("%1 stopped").arg(procedure.name)),
                                  Q_ARG(QString, message));
    }

    publishRow(row);
    emit progressChanged();
    emit runChanged();
    emit resultsChanged();
    emit finished(procedure.key, false, message);
}

void CalibrationService::cancel()
{
    abortRun(Aborted, tr("Stopped by the operator"));
}

void CalibrationService::recordResult(const Procedure &procedure)
{
    const QString verdict = procedure.state == Passed ? tr("passed") : tr("failed");

    if (auto *log = sv::common::LogBuffer::instance()) {
        log->note(procedure.state == Passed ? sv::common::LogBuffer::Info
                                            : sv::common::LogBuffer::Warning,
                  QStringLiteral("Calibration"),
                  QStringLiteral("%1 %2 - %3").arg(procedure.name, verdict, procedure.message));
    }

    if (m_recorder == nullptr)
        return;

    QMetaObject::invokeMethod(m_recorder, "logEvent",
                              Q_ARG(QString, QStringLiteral("Calibration")),
                              Q_ARG(QString, QStringLiteral("%1 %2").arg(procedure.name, verdict)),
                              Q_ARG(QString, procedure.message));
    QMetaObject::invokeMethod(m_recorder, "recordMaintenance",
                              Q_ARG(QString, procedure.name),
                              Q_ARG(QString, QStringLiteral("%1 (%2)").arg(verdict, procedure.message)));
}

void CalibrationService::reject(const QString &key, const QString &reason)
{
    m_lastRejection = reason;
    emit rejectionChanged();
    emit rejected(key, reason);

    if (auto *log = sv::common::LogBuffer::instance()) {
        log->note(sv::common::LogBuffer::Warning, QStringLiteral("Calibration"), reason);
    }
}

QVariantMap CalibrationService::entryFor(const QString &key) const
{
    const int row = indexOf(key);
    if (row < 0)
        return {};

    const Procedure &procedure = m_procedures.at(row);
    return QVariantMap{
        {QStringLiteral("key"), procedure.key},
        {QStringLiteral("name"), procedure.name},
        {QStringLiteral("state"), int(procedure.state)},
        {QStringLiteral("stateName"), stateName(int(procedure.state))},
        {QStringLiteral("stamp"), procedure.stamp.isValid()
             ? procedure.stamp.toString(QStringLiteral("yyyy-MM-dd hh:mm")) : QString()},
        {QStringLiteral("message"), procedure.message},
        {QStringLiteral("measured"), procedure.measured},
        {QStringLiteral("requiresStandby"), procedure.requiresStandby}
    };
}

QVector<sv::domain::CalibrationResult> CalibrationService::results() const
{
    QVector<sv::domain::CalibrationResult> out;
    out.reserve(m_procedures.size());
    for (const Procedure &procedure : m_procedures) {
        sv::domain::CalibrationResult result;
        result.testName = procedure.name;
        result.timestamp = procedure.stamp.isValid()
            ? procedure.stamp.toString(QStringLiteral("yyyy-MM-dd hh:mm")) : QString();
        result.details = procedure.message;
        switch (procedure.state) {
        case Passed:  result.status = sv::domain::CalibrationStatus::Passed; break;
        case Failed:
        case Aborted: result.status = sv::domain::CalibrationStatus::Failed; break;
        case Running: result.status = sv::domain::CalibrationStatus::InProgress; break;
        default:      result.status = sv::domain::CalibrationStatus::NotRun; break;
        }
        out.append(result);
    }
    return out;
}

void CalibrationService::runTest(const QString &testName)
{
    for (const Procedure &procedure : m_procedures) {
        if (procedure.name.compare(testName, Qt::CaseInsensitive) == 0
                || procedure.key.compare(testName, Qt::CaseInsensitive) == 0) {
            start(procedure.key);
            return;
        }
    }
    reject(testName, tr("No such procedure"));
}

void CalibrationService::runAllTests()
{
    startAll();
}

void CalibrationService::publishRow(int row)
{
    if (row < 0 || row >= m_procedures.size())
        return;
    const QModelIndex at = index(row, 0);
    emit dataChanged(at, at);
}

} // namespace sv::services
