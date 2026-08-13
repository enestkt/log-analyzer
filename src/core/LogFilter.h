#pragma once

#include <QDateTime>
#include <QRegularExpression>

#include "LogLevel.h"

struct LogEntry;

class LogFilter
{
public:
    // from/to gecersizse (QDateTime::isValid() == false) o yondeki sinir uygulanmaz.

    void setTimeRange(const QDateTime &from, const QDateTime &to);

    // LogLevel::Unknown verilirse seviye filtresi devre disi kalir.
    void setMinLevel(LogLevel level);

    // Gecersiz/bos pattern verilirse arama filtresi devre disi kalir.
    void setSearchPattern(const QRegularExpression &pattern);

    bool matches(const LogEntry &entry) const;

private:
    QDateTime m_from;
    QDateTime m_to;
    LogLevel m_minLevel = LogLevel::Unknown;
    QRegularExpression m_searchPattern;
};
