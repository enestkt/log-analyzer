#pragma once

#include <QDateTime>
#include <QRegularExpression>
#include <QString>

#include "../core/LogLevel.h"

class QCoreApplication;

class CliOptions
{
public:

    bool parse(const QCoreApplication &app, QString &error);

    QString filePath() const { return m_filePath; }

    QDateTime fromTimestamp() const { return m_from; }
    QDateTime toTimestamp() const { return m_to; }

    LogLevel minLevel() const { return m_minLevel; }

    QRegularExpression parserPattern() const { return m_parserPattern; }
    QString timestampFormat() const { return m_timestampFormat; }

    QRegularExpression searchPattern() const { return m_searchPattern; }

    bool exportRequested() const { return !m_exportFormat.isEmpty(); }
    QString exportFormat() const { return m_exportFormat; }
    QString outputPath() const { return m_outputPath; }

    // --parser-pattern kullanici tarafindan gercekten verildi mi (yoksa varsayilan mi kullanildi)
    bool parserPatternExplicitlySet() const { return m_parserPatternExplicitlySet; }

private:
    QString m_filePath;
    QDateTime m_from;
    QDateTime m_to;
    LogLevel m_minLevel = LogLevel::Unknown;
    QRegularExpression m_parserPattern;
    bool m_parserPatternExplicitlySet = false;
    QString m_timestampFormat;
    QRegularExpression m_searchPattern;
    QString m_exportFormat;
    QString m_outputPath;
};
