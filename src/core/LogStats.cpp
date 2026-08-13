#include "LogStats.h"

#include "LogEntry.h"

void LogStats::addRawLineSeen()
{
    ++m_result.totalLines;
}

void LogStats::addUnparsedLine()
{
    ++m_result.unparsedLines;
}

void LogStats::addEntry(const LogEntry &entry, bool passedFilter)
{
    ++m_result.parsedLines;

    if (!passedFilter)
        return;

    ++m_result.filteredInCount;
    ++m_result.countsByLevel[entry.level];

    const QString hourBucket = entry.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:00"));
    ++m_result.countsByHourBucket[hourBucket];
}