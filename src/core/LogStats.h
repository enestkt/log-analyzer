#pragma once

#include <QMap>
#include <QString>

#include "LogLevel.h"

struct LogEntry;

struct LogStatsResult
{
    qint64 totalLines = 0;
    qint64 parsedLines = 0;
    qint64 unparsedLines = 0;
    qint64 filteredInCount = 0;

    QMap<LogLevel, qint64> countsByLevel;

    QMap<QString, qint64> countsByHourBucket;
};

class LogStats
{
public:
    void addRawLineSeen();
    void addUnparsedLine();
    void addEntry(const LogEntry &entry, bool passedFilter);

    const LogStatsResult &result() const { return m_result; }

private:
    LogStatsResult m_result;
};
