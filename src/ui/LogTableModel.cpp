#include "LogTableModel.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <algorithm>
#include <numeric>
#include <utility>

#include "../core/LogLevel.h"

namespace {

QColor levelColor(LogLevel level)
{
    switch (level) {
    case LogLevel::Warning:  return QColor(0xb4, 0x53, 0x09);
    case LogLevel::Error:    return QColor(0xdc, 0x26, 0x26);
    case LogLevel::Critical: return QColor(0x99, 0x1b, 0x1b);
    default:                 return QColor(0x33, 0x41, 0x55);
    }
}

bool isAttentionLevel(LogLevel level)
{
    return level == LogLevel::Warning || level == LogLevel::Error
        || level == LogLevel::Critical;
}

} // namespace

LogTableModel::LogTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int LogTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

int LogTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant LogTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const LogEntry &entry = m_entries.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case TimestampColumn: return entry.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
        case LevelColumn:     return logLevelToString(entry.level);
        case MessageColumn:   return entry.message;
        case SourceColumn:    return m_sources.value(index.row());
        default:              return {};
        }
    }

    if (role == Qt::ForegroundRole && index.column() == LevelColumn)
        return QBrush(levelColor(entry.level));

    if (role == Qt::FontRole && index.column() == LevelColumn && isAttentionLevel(entry.level)) {
        QFont font;
        font.setBold(true);
        return font;
    }

    if (role == Qt::TextAlignmentRole
        && (index.column() == TimestampColumn || index.column() == LevelColumn))
        return QVariant::fromValue(Qt::Alignment(Qt::AlignVCenter | Qt::AlignLeft));

    return {};
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case TimestampColumn: return QStringLiteral("Tarih");
    case LevelColumn:     return QStringLiteral("Seviye");
    case MessageColumn:   return QStringLiteral("Mesaj");
    case SourceColumn:    return QStringLiteral("Kaynak");
    default:              return {};
    }
}

void LogTableModel::setResults(QVector<LogEntry> entries, QStringList sources)
{
    beginResetModel();
    m_entries = std::move(entries);
    m_sources = std::move(sources);
    endResetModel();
}

void LogTableModel::clear()
{
    setResults({}, {});
}

void LogTableModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= ColumnCount || m_entries.size() < 2)
        return;

    beginResetModel();
    QVector<qsizetype> indices(m_entries.size());
    std::iota(indices.begin(), indices.end(), qsizetype(0));

    const auto less = [this, column](qsizetype left, qsizetype right) {
        const LogEntry &a = m_entries.at(left);
        const LogEntry &b = m_entries.at(right);
        switch (column) {
        case TimestampColumn: return a.timestamp < b.timestamp;
        case LevelColumn:     return static_cast<int>(a.level) < static_cast<int>(b.level);
        case MessageColumn:   return a.message.localeAwareCompare(b.message) < 0;
        case SourceColumn:    return m_sources.at(left).localeAwareCompare(m_sources.at(right)) < 0;
        default:              return false;
        }
    };

    std::stable_sort(indices.begin(), indices.end(), [&](qsizetype a, qsizetype b) {
        return order == Qt::AscendingOrder ? less(a, b) : less(b, a);
    });

    QVector<LogEntry> sortedEntries;
    QStringList sortedSources;
    sortedEntries.reserve(m_entries.size());
    sortedSources.reserve(m_sources.size());
    for (qsizetype index : std::as_const(indices)) {
        sortedEntries.append(std::move(m_entries[index]));
        sortedSources.append(std::move(m_sources[index]));
    }
    m_entries = std::move(sortedEntries);
    m_sources = std::move(sortedSources);
    endResetModel();
}
