#include "ParserLibrary.h"

#include "GenericHeuristicParser.h"
#include "LogEntry.h"
#include "SyslogParser.h"

namespace {

int scoreParser(const ILogParser &parser, const QStringList &sampleLines)
{
    int score = 0;
    for (const QString &line : sampleLines) {
        LogEntry entry;
        if (parser.parseLine(line, entry))
            ++score;
    }
    return score;
}

} // namespace

namespace ParserLibrary {

std::unique_ptr<ILogParser> detect(const QStringList &sampleLines, QString &detectedFormatName)
{
    auto generic = std::make_unique<GenericHeuristicParser>();
    auto syslog = std::make_unique<SyslogParser>();

    const int genericScore = scoreParser(*generic, sampleLines);
    const int syslogScore = scoreParser(*syslog, sampleLines);

    // Esitlik/hicbir sey eslesmeme durumunda genel (heuristic) parser'a duselim --
    // en genis kapsamli secenek o oldugu icin daha guvenli bir varsayilan.
    if (syslogScore > genericScore) {
        detectedFormatName = QStringLiteral("Syslog");
        return syslog;
    }

    detectedFormatName = QStringLiteral("Genel (zaman damgasi + seviye)");
    return generic;
}

} // namespace ParserLibrary
