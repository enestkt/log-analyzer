#include "JsonExporter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "../core/LogEntry.h"
#include "../core/LogLevel.h"
#include "../core/LogStats.h"

bool JsonExporter::exportTo(const QVector<LogEntry> &entries,
                          const LogStatsResult &stats,
                          const QString &path,
                          QString &error) const
{
    QJsonArray entriesArray;
    for(const LogEntry &entry : entries){
        QJsonObject obj;
        obj["timestamp"] = entry.timestamp.toString(Qt::ISODate);
        obj["level"] = logLevelToString(entry.level);
        obj["message"] = entry.message;
        entriesArray.append(obj);
    }

    QJsonObject countsByLevelObj;
    for(auto it = stats.countsByLevel.constBegin();
         it != stats.countsByLevel.constEnd(); ++it)
        countsByLevelObj[logLevelToString(it.key())] = it.value();

    QJsonObject countsByHourObj;
    for(auto it = stats.countsByHourBucket.constBegin();
         it != stats.countsByHourBucket.constEnd(); ++it)
        countsByHourObj[it.key()] = it.value();

    QJsonObject statsObj;
    statsObj["totalLines"] = stats.totalLines;
    statsObj["parsedLines"] = stats.parsedLines;
    statsObj["unparsedLines"] = stats.unparsedLines;
    statsObj["filteredInCount"] = stats.filteredInCount;
    statsObj["countsByLevel"] = countsByLevelObj;
    statsObj["countsByHourBucket"] = countsByHourObj;

    QJsonObject root;
    root["entries"] = entriesArray;
    root["stats"] = statsObj;

    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)){
        error = file.errorString();
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;


































}