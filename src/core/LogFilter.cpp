#include "LogFilter.h"
#include "LogEntry.h"

void LogFilter::setTimeRange(const QDateTime &from, const QDateTime &to)
{
    m_from = from;
    m_to = to;
}

void LogFilter::setMinLevel(LogLevel level)
{
    m_minLevel = level;
}

void LogFilter::setSearchPattern(const QRegularExpression &pattern)
{
    m_searchPattern = pattern;
}

bool LogFilter::matches(const LogEntry &entry) const
{
    if(!entry.isValid)
        return false;

    if(m_from.isValid() && entry.timestamp < m_from)
        return false;
    if(m_to.isValid() && entry.timestamp > m_to)
        return false;

    if(m_minLevel != LogLevel::Unknown){
        if(entry.level == LogLevel::Unknown)
            return false;
        if(static_cast<int>(entry.level) < static_cast<int>(m_minLevel))
            return false;
    }

    if(m_searchPattern.isValid() && !m_searchPattern.pattern().isEmpty()) {
        if(!m_searchPattern.match(entry.message).hasMatch())
            return false;
    }
    return true;
}
