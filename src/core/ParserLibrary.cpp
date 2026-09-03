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

std::unique_ptr<ILogParser> detect(const QStringList &sampleLines, int referenceYear,
                                   QString &detectedFormatName)
{
    auto generic = std::make_unique<GenericHeuristicParser>();
    auto syslog = std::make_unique<SyslogParser>(referenceYear);

    const int genericScore = scoreParser(*generic, sampleLines);
    const int syslogScore = scoreParser(*syslog, sampleLines);

    // Esitlik/hicbir sey eslesmeme durumunda genel (heuristic) parser'a duselim --
    // en genis kapsamli secenek o oldugu icin daha guvenli bir varsayilan.
    if (syslogScore > genericScore) {
        detectedFormatName = QStringLiteral("Syslog");
        return syslog;
    }

    // genericScore de 0 ise, aslinda HICBIR strateji bu formati tanimadi --
    // sessizce "Genel" secmek yerine bunu acikca soyluyoruz, kullanici
    // sonuclarin guvenilir olmayabilecegini bilsin.
    detectedFormatName = (genericScore > 0)
        ? QStringLiteral("Genel (zaman damgasi + seviye)")
        : QStringLiteral("Bilinmiyor (varsayilan kullaniliyor, sonuclar hatali olabilir)");
    return generic;
}

} // namespace ParserLibrary
