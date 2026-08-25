#include "SyslogParser.h"

#include <QDate>
#include <QDateTime>
#include <QRegularExpression>
#include <QStringList>

#include "LogEntry.h"
#include "LogLevel.h"

namespace {

const QRegularExpression &syslogPattern()
{
    static const QRegularExpression pattern(QStringLiteral(
        R"(^(?<month>Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+(?<day>\d{1,2})\s+)"
        R"((?<time>\d{2}:\d{2}:\d{2})\s+(?<host>\S+)\s+(?<process>[^:\[]+)(?:\[\d+\])?:\s*(?<message>.*)$)"));
    return pattern;
}

int monthFromName(const QString &name)
{
    static const QStringList months = {
        QStringLiteral("Jan"), QStringLiteral("Feb"), QStringLiteral("Mar"), QStringLiteral("Apr"),
        QStringLiteral("May"), QStringLiteral("Jun"), QStringLiteral("Jul"), QStringLiteral("Aug"),
        QStringLiteral("Sep"), QStringLiteral("Oct"), QStringLiteral("Nov"), QStringLiteral("Dec")
    };
    return months.indexOf(name) + 1; // bulunamazsa indexOf -1 doner, +1 ile 0 olur
}

} // namespace

bool SyslogParser::parseLine(const QString &line, LogEntry &out) const
{
    const QRegularExpressionMatch match = syslogPattern().match(line);
    if (!match.hasMatch()) {
        out = LogEntry{};
        out.rawLine = line;
        out.isValid = false;
        return false;
    }

    const int month = monthFromName(match.captured(QStringLiteral("month")));
    const int day = match.captured(QStringLiteral("day")).toInt();
    const QString timeText = match.captured(QStringLiteral("time"));

    // Syslog satirlarinda yil hic yazilmaz -- icinde bulunulan yil varsayilir
    // (rsyslog gibi araclarin standart davranisi). Eski/arsivlenmis loglarda
    // bu varsayim yanlis olabilir; bu bilinen bir sinirdir.
    const int year = QDate::currentDate().year();
    const QDateTime timestamp = QDateTime::fromString(
        QStringLiteral("%1-%2-%3 %4")
            .arg(year)
            .arg(month, 2, 10, QLatin1Char('0'))
            .arg(day, 2, 10, QLatin1Char('0'))
            .arg(timeText),
        QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    out.timestamp = timestamp;
    out.level = LogLevel::Unknown; // syslog'da seviye kavrami yok
    out.message = match.captured(QStringLiteral("message")).trimmed();
    out.rawLine = line;
    out.isValid = timestamp.isValid() && month > 0;
    return out.isValid;
}
