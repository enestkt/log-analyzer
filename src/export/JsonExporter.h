#pragma once
#include "IExporter.h"

class JsonExporter : public IExporter
{

public:
    bool exportTo(const QVector<LogEntry> &entries,
                  const LogStatsResult &stats,
                  const QString &path,
                  QString &error) const override;
};
