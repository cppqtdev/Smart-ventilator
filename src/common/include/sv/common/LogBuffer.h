// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QFile>
#include <QString>
#include <QVector>

#include <memory>

namespace sv::common {

/**
 * @brief In-memory ring of log records, exposed to QML as a list model.
 *
 * Installs itself as the Qt message handler, so every qCInfo/qCWarning in the
 * application lands here as well as on the console. A device in a ward has no
 * console to read, and a fault that only reproduces at 3 a.m. is a fault
 * nobody can describe - the log has to be on the screen and on the disk.
 *
 * Also takes explicit records through note(), which is how a control action
 * gets logged with its old and new value rather than as free text.
 */
class LogBuffer : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int capacity READ capacity WRITE setCapacity NOTIFY capacityChanged)
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)

public:
    enum Level { Debug, Info, Notice, Warning, Critical };
    Q_ENUM(Level)

    enum Roles {
        TimeRole = Qt::UserRole + 1,
        LevelRole,
        LevelNameRole,
        CategoryRole,
        MessageRole,
        DetailRole
    };

    explicit LogBuffer(QObject *parent = nullptr);
    ~LogBuffer() override;

    /** @brief The one buffer the message handler writes into. */
    static LogBuffer *instance();

    /** @brief Routes Qt's message stream into this buffer. */
    void installMessageHandler();

    /** @brief Also mirrors every record to @p path. Empty disables the file. */
    Q_INVOKABLE bool setLogFile(const QString &path);
    QString filePath() const { return m_filePath; }

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int capacity() const { return m_capacity; }
    void setCapacity(int capacity);

    /** @brief Records one entry directly, without going through qDebug. */
    Q_INVOKABLE void note(Level level, const QString &category,
                          const QString &message, const QString &detail = {});

    /** @brief Records a control action with its before and after values. */
    Q_INVOKABLE void noteControl(const QString &control, const QVariant &from,
                                 const QVariant &to, bool accepted,
                                 const QString &reason = {});

    Q_INVOKABLE void clear();

    /** @brief The most recent @p limit records, newest first, for export. */
    Q_INVOKABLE QString toPlainText(int limit = 0) const;

signals:
    void countChanged();
    void capacityChanged();
    void filePathChanged();
    void recordAdded(int level, const QString &category, const QString &message);

private:
    struct Record
    {
        QDateTime at;
        Level level = Info;
        QString category;
        QString message;
        QString detail;
    };

    void append(Record record);

    // The Qt message handler runs on whichever thread emitted the message,
    // including the database writer thread, so a record raised off the owning
    // thread is queued rather than touching the model in place.
    void post(Record record);
    void writeToFile(const Record &record);

    QVector<Record> m_records;
    int m_capacity = 2000;
    QString m_filePath;
    std::unique_ptr<QFile> m_file;
};

} // namespace sv::common
