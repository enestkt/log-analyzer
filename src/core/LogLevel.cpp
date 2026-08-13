#include "LogLevel.h"

QString logLevelToString(LogLevel level)
{
    switch(level) {
    case LogLevel::Trace:    return QStringLiteral("TRACE");
    case LogLevel::Debug:    return QStringLiteral("DEBUG");
    case LogLevel::Info:     return QStringLiteral("INFO");
    case LogLevel::Warning:  return QStringLiteral("WARNING");
    case LogLevel::Error:    return QStringLiteral("ERROR");
    case LogLevel::Critical: return QStringLiteral("CRITICAL");
    case LogLevel::Unknown:  break;
    }
    return QStringLiteral("UNKNOWN");
}

LogLevel logLevelFromString(const QString &text)
{
    const QString normalized = text.trimmed().toUpper();

    if (normalized == QStringLiteral("TRACE"))    return LogLevel::Trace;
    if (normalized == QStringLiteral("DEBUG"))    return LogLevel::Debug;
    if (normalized == QStringLiteral("INFO"))     return LogLevel::Info;
    if (normalized == QStringLiteral("WARN") ||
        normalized == QStringLiteral("WARNING"))  return LogLevel::Warning;
    if (normalized == QStringLiteral("ERROR"))    return LogLevel::Error;
    if (normalized == QStringLiteral("CRITICAL") ||
        normalized == QStringLiteral("FATAL"))    return LogLevel::Critical;

    return LogLevel::Unknown;
}