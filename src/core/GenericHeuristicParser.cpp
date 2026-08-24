#include "GenericHeuristicParser.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QVector>

#include "LogEntry.h"
#include "LogLevel.h"

namespace {

struct TimestampPattern
{
    QRegularExpression regex;
    QString qtFormat;
};

const QVector<TimestampPattern> &timestampPatterns()
{
    static const QVector<TimestampPattern> patterns = {
        { QRegularExpression(R"(\d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2}[.,]\d{1,6})"),
          QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz") },
        { QRegularExpression(R"(\d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2})"),
          QStringLiteral("yyyy-MM-dd HH:mm:ss") },
        { QRegularExpression(R"(\d{2}/\d{2}/\d{4} \d{2}:\d{2}:\d{2})"),
          QStringLiteral("MM/dd/yyyy HH:mm:ss") },
    };
    return patterns;
}

const QRegularExpression &levelPattern()
{
    static const QRegularExpression pattern(
        QStringLiteral(R"(\b(TRACE|DEBUG|INFO|WARN(?:ING)?|ERROR|CRITICAL|FATAL)\b)"),
        QRegularExpression::CaseInsensitiveOption);
    return pattern;
}

} // namespace

bool GenericHeuristicParser::parseLine(const QString &line, LogEntry &out) const
{
    // 1) Zaman damgasini bul -- birkac yaygin kalibi sirayla dener, ilk eslesen kazanir.
    QString timestampText;
    QString timestampFormat;
    int timestampEnd = -1;
    for (const TimestampPattern &tp : timestampPatterns()) {
        const QRegularExpressionMatch m = tp.regex.match(line);
        if (m.hasMatch()) {
            timestampText = m.captured(0);
            timestampFormat = tp.qtFormat;
            timestampEnd = m.capturedEnd(0);
            break;
        }
    }

    if (timestampEnd < 0) {
        out = LogEntry{};
        out.rawLine = line;
        out.isValid = false;
        return false;
    }

    // 2) Zaman damgasindan sonraki kisimda bir seviye kelimesi ara.
    const QString afterTimestamp = line.mid(timestampEnd);
    const QRegularExpressionMatch levelMatch = levelPattern().match(afterTimestamp);

    LogLevel level = LogLevel::Unknown;
    int messageSearchStart = timestampEnd;
    if (levelMatch.hasMatch()) {
        level = logLevelFromString(levelMatch.captured(1));
        messageSearchStart = timestampEnd + levelMatch.capturedEnd(0);
    }

    // 3) Mesaj: seviyeden sonraki kisimda ilk ':' karakterinden sonrasi --
    //    thread adi/logger sinifi gibi ara alanlar (Hadoop tarzi) boylece atlanir.
    //    ':' yoksa kalan kismin tamami mesaj sayilir.
    QString remainder = line.mid(messageSearchStart);
    const int firstColon = remainder.indexOf(QLatin1Char(':'));
    QString message = (firstColon >= 0) ? remainder.mid(firstColon + 1) : remainder;
    message = message.trimmed();
    while (!message.isEmpty() && (message.at(0) == QLatin1Char(']') || message.at(0) == QLatin1Char('-')))
        message = message.mid(1).trimmed();

    QString normalizedTimestamp = timestampText;
    normalizedTimestamp.replace(QLatin1Char(','), QLatin1Char('.'));
    const QDateTime timestamp = QDateTime::fromString(normalizedTimestamp, timestampFormat);

    out.timestamp = timestamp;
    out.level = level;
    out.message = message.isEmpty() ? remainder.trimmed() : message;
    out.rawLine = line;
    out.isValid = timestamp.isValid();
    return out.isValid;
}
