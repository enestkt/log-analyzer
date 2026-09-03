#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>
#include <QVector>
#include <memory>

#include "cli/CliOptions.h"
#include "core/FileLogReader.h"
#include "core/LogEntry.h"
#include "core/LogFilter.h"
#include "core/LogLevel.h"
#include "core/LogStats.h"
#include "core/ILogParser.h"
#include "core/ParserLibrary.h"
#include "core/RegexLogParser.h"
#include "export/CsvExporter.h"
#include "export/IExporter.h"
#include "export/JsonExporter.h"

namespace {

void printSummary(const LogStatsResult &stats, QTextStream &out)
{
    out << "----- Ozet -----\n";
    out << "Toplam satir     : " << stats.totalLines << '\n';
    out << "Parse edilen     : " << stats.parsedLines << '\n';
    out << "Parse edilemeyen : " << stats.unparsedLines << '\n';
    out << "Filtreyi gecen   : " << stats.filteredInCount << '\n';

    out << "\nSeviyeye gore dagilim:\n";
    for (auto it = stats.countsByLevel.constBegin(); it != stats.countsByLevel.constEnd(); ++it)
        out << "  " << logLevelToString(it.key()) << ": " << it.value() << '\n';

    out << "\nSaatlik dagilim:\n";
    for (auto it = stats.countsByHourBucket.constBegin(); it != stats.countsByHourBucket.constEnd(); ++it)
        out << "  " << it.key() << ": " << it.value() << '\n';
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("LogAnalyzer"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    QTextStream cout(stdout);
    QTextStream cerr(stderr);

    // --- CLI argumanlarini oku ve dogrula ---
    CliOptions options;
    QString error;
    if (!options.parse(app, error)) {
        cerr << error << '\n';
        return 1;
    }

    // --- Composition root: soyutlamalarin arkasina somut siniflari koy ---
    std::unique_ptr<ILogParser> parser;
    if (options.parserPatternExplicitlySet()) {
        QString parserError;
        std::unique_ptr<RegexLogParser> regexParser =
            RegexLogParser::create(options.parserPattern(), options.timestampFormat(), parserError);
        if (!regexParser) {
            cerr << parserError << '\n';
            return 1;
        }
        parser = std::move(regexParser);
    } else {
        QStringList sampleLines;
        FileLogReader sampleReader;
        QString sampleError;
        if (sampleReader.open(options.filePath(), sampleError)) {
            while (!sampleReader.atEnd() && sampleLines.size() < 20)
                sampleLines.append(sampleReader.readLine());
            sampleReader.close();
        }
        const QDateTime modified = QFileInfo(options.filePath()).lastModified();
        const int referenceYear = modified.isValid() ? modified.date().year() : QDate::currentDate().year();
        QString detectedFormatName;
        parser = ParserLibrary::detect(sampleLines, referenceYear, detectedFormatName);
        cout << "Algilanan log formati: " << detectedFormatName << "\n";
    }

    FileLogReader reader;
    QString readerError;
    if (!reader.open(options.filePath(), readerError)) {
        cerr << readerError << '\n';
        return 2;
    }

    LogFilter filter;
    filter.setTimeRange(options.fromTimestamp(), options.toTimestamp());
    filter.setMinLevel(options.minLevel());
    filter.setSearchPattern(options.searchPattern());

    LogStats stats;
    QVector<LogEntry> filteredEntries;
    QStringList sources;   // filteredEntries ile ayni sirada, hangi dosyadan geldigi
    const QString sourceName = QFileInfo(options.filePath()).fileName();

    // --- Tek gecisli (single-pass) akis: dosya asla tamami belleğe alinmadan okunur ---
    while (!reader.atEnd()) {
        const QString line = reader.readLine();
        stats.addRawLineSeen();

        LogEntry entry;
        if (!parser->parseLine(line, entry)) {
            stats.addUnparsedLine();
            continue;
        }

        const bool passedFilter = filter.matches(entry);
        stats.addEntry(entry, passedFilter);

        if (passedFilter) {
            filteredEntries.append(entry);
            sources.append(sourceName);
        }
    }

    reader.close();

    printSummary(stats.result(), cout);

    // --- Istenirse disa aktar ---
    if (options.exportRequested()) {
        std::unique_ptr<IExporter> exporter = options.exportFormat() == QStringLiteral("csv")
        ? std::unique_ptr<IExporter>(std::make_unique<CsvExporter>())
        : std::unique_ptr<IExporter>(std::make_unique<JsonExporter>());

        QString exportError;
        if (!exporter->exportTo(filteredEntries, sources, stats.result(), options.outputPath(), exportError)) {
            cerr << exportError << '\n';
            return 3;
        }

        cout << "\nDisa aktarildi: " << options.outputPath() << '\n';
    }

    return 0;
}