#pragma once

#include <QDateTime>
#include <QString>

#include "LogLevel.h"

struct LogEntry
{
    QDateTime timestamp;
    LogLevel level = LogLevel::Unknown;
    QString message;
    QString rawLine;
    bool isValid = false;
};
