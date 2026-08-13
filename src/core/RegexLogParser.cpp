#include "RegexLogParser.h"
#include <QStringList>
#include "LogEntry.h"
#include "LogLevel.h"

RegexLogParser::RegexLogParser(const QRegularExpression &pattern, const QString &timestampFormat)
    : m_pattern(pattern)
    , m_timestampFormat(timestampFormat)
{
}

std::unique_ptr<RegexLogParser> RegexLogParser::create(const QRegularExpression &pattern,
                                                       const QString &timestampFormat,
                                                       QString &error)
{
    if (!pattern.isValid()) {
        error = QStringLiteral("Parser pattern gecersiz: %1").arg(pattern.errorString());
        return nullptr;
    }

    const QStringList requiredGroups = { QStringLiteral("timestamp"),
                                        QStringLiteral("level"),
                                        QStringLiteral("message") };
    const QStringList namedGroups = pattern.namedCaptureGroups();

    for (const QString &group : requiredGroups) {
        if (!namedGroups.contains(group)) {
            error = QStringLiteral("Parser pattern '%1' adinda bir yakalama grubu icermiyor "
                                   "(timestamp/level/message hepsi gerekli).")
                        .arg(group);
            return nullptr;
        }
    }

    // std::unique_ptr::reset ile private constructor'a erismek icin new kullaniyoruz,
    // make_unique burada calismaz cunku constructor private.
    return std::unique_ptr<RegexLogParser>(new RegexLogParser(pattern, timestampFormat));
}

QRegularExpression RegexLogParser::defaultPattern()
{
    return QRegularExpression(
        QStringLiteral(R"(^\[(?<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\]\s+(?<level>\w+):\s*(?<message>.*)$)"));
}

QString RegexLogParser::defaultTimestampFormat()
{
    return QStringLiteral("yyyy-MM-dd HH:mm:ss");
}

bool RegexLogParser::parseLine(const QString &line, LogEntry &out) const
{
    const QRegularExpressionMatch match = m_pattern.match(line);

    if (!match.hasMatch()) {
        out = LogEntry{};
        out.rawLine = line;
        out.isValid = false;
        return false;
    }

    const QDateTime timestamp = QDateTime::fromString(match.captured("timestamp"), m_timestampFormat);

    out.timestamp = timestamp;
    out.level = logLevelFromString(match.captured("level"));
    out.message = match.captured("message");
    out.rawLine = line;
    // Grup eslesse bile zaman damgasi parse edilemediyse, filtreleme icin kullanilamaz -> gecersiz sayiyoruz.
    out.isValid = timestamp.isValid();

    return out.isValid;
}