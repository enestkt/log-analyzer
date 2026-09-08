#include "LogAnalysisWorker.h"

#include <QDateTime>
#include <QFileInfo>
#include <memory>
#include <utility>

#include "../core/FileLogReader.h"
#include "../core/ILogParser.h"
#include "../core/LogFilter.h"
#include "../core/ParserLibrary.h"
#include "../core/RegexLogParser.h"

GuiAnalysisResult LogAnalysisWorker::run(
    GuiAnalysisRequest request, const std::shared_ptr<std::atomic_bool> &cancelRequested)
{
    GuiAnalysisResult result;
    result.fileCount = request.filePaths.size();
    result.dateRangeEnabled = request.dateRangeEnabled;

    LogFilter filter;
    filter.setSearchPattern(request.searchPattern);
    filter.setMinLevel(request.minimumLevel);
    if (request.dateRangeEnabled)
        filter.setTimeRange(request.fromDateTime, request.toDateTime);

    LogStats stats;
    qsizetype linesSinceCancelCheck = 0;

    for (const QString &filePath : std::as_const(request.filePaths)) {
        if (cancelRequested->load(std::memory_order_relaxed)) {
            result.cancelled = true;
            return result;
        }

        std::unique_ptr<ILogParser> parser;
        if (request.customParserEnabled) {
            QString parserError;
            parser = RegexLogParser::create(request.customParserPattern,
                                            request.customTimestampFormat, parserError);
            if (!parser) {
                result.errorTitle = QStringLiteral("Pattern hatası");
                result.errorMessage = parserError;
                return result;
            }
        } else {
            QStringList sampleLines;
            FileLogReader sampleReader;
            QString sampleError;
            if (sampleReader.open(filePath, sampleError)) {
                while (!sampleReader.atEnd() && sampleLines.size() < 20)
                    sampleLines.append(sampleReader.readLine());
                sampleReader.close();
            }

            const QDateTime modified = QFileInfo(filePath).lastModified();
            const int referenceYear = modified.isValid()
                ? modified.date().year()
                : QDate::currentDate().year();
            QString detectedFormatName;
            parser = ParserLibrary::detect(sampleLines, referenceYear, detectedFormatName);
            result.detectedFormats.append(QStringLiteral("%1: %2")
                .arg(QFileInfo(filePath).fileName(), detectedFormatName));
        }

        FileLogReader reader;
        QString readerError;
        if (!reader.open(filePath, readerError)) {
            result.errorTitle = QStringLiteral("Dosya hatası");
            result.errorMessage = QStringLiteral("%1: %2").arg(filePath, readerError);
            return result;
        }

        const QString sourceName = QFileInfo(filePath).fileName();
        while (!reader.atEnd()) {
            if (++linesSinceCancelCheck >= 256) {
                linesSinceCancelCheck = 0;
                if (cancelRequested->load(std::memory_order_relaxed)) {
                    reader.close();
                    result.cancelled = true;
                    return result;
                }
            }

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
                result.entries.append(std::move(entry));
                result.sources.append(sourceName);
            }
        }
        reader.close();
    }

    result.stats = stats.result();
    return result;
}
