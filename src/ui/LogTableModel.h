#pragma once

#include <QAbstractTableModel>
#include <QStringList>
#include <QVector>

#include "../core/LogEntry.h"

class LogTableModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column { TimestampColumn, LevelColumn, MessageColumn, SourceColumn, ColumnCount };

    explicit LogTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void setResults(QVector<LogEntry> entries, QStringList sources);
    void clear();

    const QVector<LogEntry> &entries() const { return m_entries; }
    const QStringList &sources() const { return m_sources; }

private:
    QVector<LogEntry> m_entries;
    QStringList m_sources;
};
