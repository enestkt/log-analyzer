#include "CsvExporter.h"

#include <QFile>
#include <QTextStream>

#include "../core/LogEntry.h"
#include "../core/LogLevel.h"
#include "../core/LogStats.h"

namespace {

QString csvEscape(const QString &field)
{
    if(!field.contains(';') && !field.contains('"') && !field.contains('\n'))
        return field;

    QString escaped = field;
    escaped.replace(QStringLiteral("\""), QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(escaped);
}
}

bool CsvExporter::exportTo(const QVector<LogEntry> &entries,
                           const LogStatsResult &stats,
                           const QString &path,
                           QString &error) const
{
    Q_UNUSED(stats);

    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        error = file.errorString();
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out.setGenerateByteOrderMark(true);

    out <<"timestamp; level; message \n";
    for(const LogEntry &entry : entries) {
        out<<csvEscape(entry.timestamp.toString(Qt::ISODate)) << ';'
            << csvEscape(logLevelToString(entry.level)) << ';'
            << csvEscape(entry.message) << '\n';
    }
    return true;

























}