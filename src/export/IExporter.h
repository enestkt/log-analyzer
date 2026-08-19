#pragma once
#include <QString>
#include <QStringList>
#include <QVector>

struct LogEntry;
struct LogStatsResult;

class IExporter
{
public:
    virtual ~IExporter() = default;

    // Basarili olursa true doner. Basarisiz olursa false doner ve error'u doldurur
    // (dosya acilamadi, yazma hatasi vb.) -- hicbir zaman exception atmaz.
    // sources: entries ile ayni sirada, her satirin hangi dosyadan geldigi.
    virtual bool exportTo(const QVector<LogEntry> &entries,
                          const QStringList &sources,
                          const LogStatsResult &stats,
                          const QString &path,
                          QString &error) const = 0;
};
