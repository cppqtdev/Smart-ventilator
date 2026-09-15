// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/common/LogBuffer.h>

#include <QDir>
#include <QFileInfo>
#include <QThread>

#include <utility>
#include <QTextStream>

#include <cstdio>

namespace sv::common {
namespace {

LogBuffer *g_instance = nullptr;
QtMessageHandler g_previousHandler = nullptr;

LogBuffer::Level levelFor(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return LogBuffer::Debug;
    case QtInfoMsg:     return LogBuffer::Info;
    case QtWarningMsg:  return LogBuffer::Warning;
    case QtCriticalMsg: return LogBuffer::Critical;
    case QtFatalMsg:    return LogBuffer::Critical;
    }
    return LogBuffer::Info;
}

const char *levelName(LogBuffer::Level level)
{
    switch (level) {
    case LogBuffer::Debug:    return "DEBUG";
    case LogBuffer::Info:     return "INFO";
    case LogBuffer::Notice:   return "NOTICE";
    case LogBuffer::Warning:  return "WARNING";
    case LogBuffer::Critical: return "CRITICAL";
    }
    return "INFO";
}

void handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (g_previousHandler != nullptr)
        g_previousHandler(type, context, message);

    if (g_instance == nullptr)
        return;

    const QString category = context.category != nullptr
        ? QString::fromLatin1(context.category)
        : QStringLiteral("default");

    // Qt's audio back end reports one warning per unrecognised channel on
    // every device it enumerates. On a machine with several sound devices
    // that is a hundred records before the app has drawn a frame, which
    // pushes the entries a clinician needs off the screen.
    // It arrives on the default category, so the message itself is matched.
    if (type != QtCriticalMsg && type != QtFatalMsg
        && (category.startsWith(QLatin1String("qt.multimedia"))
            || message.startsWith(QLatin1String("audio device has unrecognized channel")))) {
        return;
    }

    QString detail;
    if (context.file != nullptr) {
        detail = QStringLiteral("%1:%2")
                     .arg(QFileInfo(QString::fromLatin1(context.file)).fileName())
                     .arg(context.line);
    }

    g_instance->note(levelFor(type), category, message, detail);
}

} // namespace

LogBuffer::LogBuffer(QObject *parent)
    : QAbstractListModel(parent)
{
    if (g_instance == nullptr)
        g_instance = this;
}

LogBuffer::~LogBuffer()
{
    if (g_instance == this) {
        qInstallMessageHandler(g_previousHandler);
        g_instance = nullptr;
        g_previousHandler = nullptr;
    }
}

LogBuffer *LogBuffer::instance()
{
    return g_instance;
}

void LogBuffer::installMessageHandler()
{
    g_instance = this;
    g_previousHandler = qInstallMessageHandler(handler);
}

bool LogBuffer::setLogFile(const QString &path)
{
    m_file.reset();
    m_filePath.clear();

    if (path.isEmpty()) {
        emit filePathChanged();
        return true;
    }

    QDir().mkpath(QFileInfo(path).absolutePath());

    auto file = std::make_unique<QFile>(path);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        note(Warning, QStringLiteral("sv.log"),
             tr("Cannot open the log file"), file->errorString());
        return false;
    }

    m_file = std::move(file);
    m_filePath = path;
    emit filePathChanged();
    note(Info, QStringLiteral("sv.log"), tr("Logging to %1").arg(path));
    return true;
}

int LogBuffer::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_records.size());
}

QVariant LogBuffer::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_records.size())
        return {};

    // Newest first: a clinician reading a log wants the last thing that
    // happened, not the first.
    const Record &record = m_records.at(m_records.size() - 1 - index.row());

    switch (role) {
    case TimeRole:      return record.at.toString(QStringLiteral("hh:mm:ss.zzz"));
    case LevelRole:     return int(record.level);
    case LevelNameRole: return QString::fromLatin1(levelName(record.level));
    case CategoryRole:  return record.category;
    case MessageRole:   return record.message;
    case DetailRole:    return record.detail;
    default:            return {};
    }
}

QHash<int, QByteArray> LogBuffer::roleNames() const
{
    return {
        {TimeRole, "time"},
        {LevelRole, "level"},
        {LevelNameRole, "levelName"},
        {CategoryRole, "category"},
        {MessageRole, "message"},
        {DetailRole, "detail"}
    };
}

void LogBuffer::setCapacity(int capacity)
{
    const int clamped = qBound(100, capacity, 100000);
    if (m_capacity == clamped)
        return;
    m_capacity = clamped;
    while (m_records.size() > m_capacity) {
        beginRemoveRows({}, rowCount() - 1, rowCount() - 1);
        m_records.removeFirst();
        endRemoveRows();
    }
    emit capacityChanged();
}

void LogBuffer::append(Record record)
{
    if (m_records.size() >= m_capacity) {
        // The oldest record is the last row, because row 0 is the newest.
        beginRemoveRows({}, rowCount() - 1, rowCount() - 1);
        m_records.removeFirst();
        endRemoveRows();
    }

    beginInsertRows({}, 0, 0);
    m_records.append(record);
    endInsertRows();

    writeToFile(record);
    emit countChanged();
    emit recordAdded(int(record.level), record.category, record.message);
}

void LogBuffer::writeToFile(const Record &record)
{
    if (!m_file)
        return;
    QTextStream stream(m_file.get());
    stream << record.at.toString(Qt::ISODateWithMs) << ' '
           << levelName(record.level) << ' '
           << record.category << ' '
           << record.message;
    if (!record.detail.isEmpty())
        stream << QStringLiteral(" [") << record.detail << QLatin1Char(']');
    stream << '\n';
    stream.flush();
}

void LogBuffer::post(Record record)
{
    if (QThread::currentThread() == thread()) {
        append(std::move(record));
        return;
    }
    QMetaObject::invokeMethod(this, [this, record]() { append(record); },
                              Qt::QueuedConnection);
}

void LogBuffer::note(Level level, const QString &category,
                     const QString &message, const QString &detail)
{
    post(Record{QDateTime::currentDateTime(), level, category, message, detail});
}

void LogBuffer::noteControl(const QString &control, const QVariant &from,
                            const QVariant &to, bool accepted,
                            const QString &reason)
{
    const QString message = accepted
        ? tr("%1: %2 to %3").arg(control, from.toString(), to.toString())
        : tr("%1: %2 to %3 refused").arg(control, from.toString(), to.toString());

    post(Record{QDateTime::currentDateTime(),
                accepted ? Notice : Warning,
                QStringLiteral("sv.control"),
                message,
                reason});
}

void LogBuffer::clear()
{
    if (m_records.isEmpty())
        return;
    beginResetModel();
    m_records.clear();
    endResetModel();
    emit countChanged();
}

QString LogBuffer::toPlainText(int limit) const
{
    const int wanted = limit > 0 ? qMin(limit, int(m_records.size()))
                                 : int(m_records.size());
    QString text;
    for (int i = 0; i < wanted; ++i) {
        const Record &record = m_records.at(m_records.size() - 1 - i);
        text += QStringLiteral("%1 %2 %3 %4\n")
                    .arg(record.at.toString(Qt::ISODateWithMs),
                         QString::fromLatin1(levelName(record.level)),
                         record.category,
                         record.message);
    }
    return text;
}

} // namespace sv::common
