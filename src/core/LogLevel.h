#pragma once
#include <QString>

enum class LogLevel
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
    Unknown
};

QString logLevelToString(LogLevel level);

LogLevel logLevelFromString(const QString &text);
