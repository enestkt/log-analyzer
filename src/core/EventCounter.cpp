#include "EventCounter.h"

#include "LogEntry.h"

void EventCounter::addEventPattern(const QString &name, const QRegularExpression &pattern)
{
    m_patterns.append({name,pattern});
}

void EventCounter::observe(const LogEntry &entry)
{
    if(!entry.isValid)
        return;

    const QString monthBucket = entry.timestamp.toString(QStringLiteral("yyyy-MM"));

    for(const NamedEventPattern &np : std::as_const(m_patterns)) {
        if(np.pattern.match(entry.message).hasMatch())
            ++m_counts[np.name][monthBucket];
    }
}