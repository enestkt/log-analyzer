#include "SyslogYearDetector.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

namespace {

const QStringList &monthNames()
{
    static const QStringList months = {
        QStringLiteral("Jan"), QStringLiteral("Feb"), QStringLiteral("Mar"),
        QStringLiteral("Apr"), QStringLiteral("May"), QStringLiteral("Jun"),
        QStringLiteral("Jul"), QStringLiteral("Aug"), QStringLiteral("Sep"),
        QStringLiteral("Oct"), QStringLiteral("Nov"), QStringLiteral("Dec")
    };
    return months;
}

int monthNumber(const QString &name)
{
    return monthNames().indexOf(name) + 1;
}

const QRegularExpression &prefixPattern()
{
    static const QRegularExpression pattern(QStringLiteral(
        R"(^(?<month>Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+)"
        R"((?<day>\d{1,2})\s+(?<time>\d{2}:\d{2}:\d{2})\s+)"));
    return pattern;
}

const QRegularExpression &embeddedDatePattern()
{
    static const QRegularExpression pattern(QStringLiteral(
        R"((?:Sun|Mon|Tue|Wed|Thu|Fri|Sat)\s+)"
        R"((?<month>Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+)"
        R"((?<day>\d{1,2})\s+(?<time>\d{2}:\d{2}:\d{2}))"
        R"((?:\s+[A-Z]{2,5})?\s+(?<year>(?:19|20)\d{2})\b)"));
    return pattern;
}

} // namespace

void SyslogYearDetector::inspectLine(const QString &line)
{
    const QRegularExpressionMatch prefix = prefixPattern().match(line);
    if (!prefix.hasMatch())
        return;

    const int prefixMonth = monthNumber(prefix.captured(QStringLiteral("month")));
    // Ay en az 6 ay geriye sicradiysa yil donmustur -- SyslogParser ile birebir
    // ayni kural (bkz. SyslogParser::parseLine). Kucuk sira bozukluklari sayilmaz.
    if (m_previousMonth > 0 && m_previousMonth - prefixMonth >= 6)
        ++m_rolloversSeen;
    m_previousMonth = prefixMonth;

    QRegularExpressionMatchIterator dates = embeddedDatePattern().globalMatch(line);
    while (dates.hasNext()) {
        const QRegularExpressionMatch date = dates.next();
        // Derleme tarihi gibi mesaj icindeki ilgisiz yillari kullanma. Capalama
        // tarihi, satirin Syslog basligiyla ay/gun/saat olarak birebir eslesmeli.
        if (date.captured(QStringLiteral("month")) != prefix.captured(QStringLiteral("month"))
            || date.captured(QStringLiteral("day")).toInt()
                != prefix.captured(QStringLiteral("day")).toInt()
            || date.captured(QStringLiteral("time"))
                != prefix.captured(QStringLiteral("time")))
            continue;

        const int anchorYear = date.captured(QStringLiteral("year")).toInt();
        const int firstYear = anchorYear - m_rolloversSeen;
        if (firstYear >= 1000 && firstYear <= 9999)
            ++m_candidateCounts[firstYear];
    }
}

int SyslogYearDetector::inferredFirstYear() const
{
    int bestYear = 0;
    int bestCount = 0;
    for (auto it = m_candidateCounts.constBegin(); it != m_candidateCounts.constEnd(); ++it) {
        if (it.value() > bestCount || (it.value() == bestCount && it.key() < bestYear)) {
            bestYear = it.key();
            bestCount = it.value();
        }
    }
    return bestYear;
}

int syslogYearFromFileName(const QString &path)
{
    static const QRegularExpression yearPattern(
        QStringLiteral(R"((?:^|\D)((?:19|20)\d{2})(?:\D|$))"));
    const QRegularExpressionMatch match = yearPattern.match(QFileInfo(path).completeBaseName());
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}
