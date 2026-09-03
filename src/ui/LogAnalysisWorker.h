#pragma once

#include <QDateTime>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVector>
#include <atomic>
#include <memory>

#include "../core/LogEntry.h"
#include "../core/LogLevel.h"
#include "../core/LogStats.h"

struct GuiAnalysisRequest
{
    QStringList filePaths;
    QRegularExpression searchPattern;
    LogLevel minimumLevel = LogLevel::Unknown;
    bool dateRangeEnabled = false;
    QDateTime fromDateTime;
    QDateTime toDateTime;
    QRegularExpression customParserPattern;
    QString customTimestampFormat;
    bool customParserEnabled = false;
};

struct GuiAnalysisResult
{
    QVector<LogEntry> entries;
    QStringList sources;
    LogStatsResult stats;
    int fileCount = 0;
    bool dateRangeEnabled = false;
    bool cancelled = false;
    QString errorTitle;
    QString errorMessage;
};

class LogAnalysisWorker
{
public:
    static GuiAnalysisResult run(GuiAnalysisRequest request,
                                 const std::shared_ptr<std::atomic_bool> &cancelRequested);
};
