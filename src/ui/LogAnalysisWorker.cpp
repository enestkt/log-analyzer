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
#include "../core/SyslogYearDetector.h"

namespace {

int inferSyslogYear(const QString &filePath,
                    const std::shared_ptr<std::atomic_bool> &cancelRequested,
                    bool &cancelled)
{
    const int fileNameYear = syslogYearFromFileName(filePath);
    if (fileNameYear > 0)
        return fileNameYear;

    FileLogReader reader;
    QString error;
    if (!reader.open(filePath, error))
        return 0;

    SyslogYearDetector detector;
    qsizetype linesSinceCancelCheck = 0;
    while (!reader.atEnd()) {
        detector.inspectLine(reader.readLine());
        if (++linesSinceCancelCheck >= 256) {
            linesSinceCancelCheck = 0;
            if (cancelRequested->load(std::memory_order_relaxed)) {
                cancelled = true;
                reader.close();
                return 0;
            }
        }
    }
    reader.close();
    return detector.inferredFirstYear();
}

} // namespace

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

            QString detectedFormatName;
            bool usesReferenceYear = false;
            parser = ParserLibrary::detect(sampleLines, QDate::currentDate().year(),
                                           detectedFormatName,
                                           &usesReferenceYear);
            if (usesReferenceYear) {
                bool cancelledDuringDetection = false;
                int referenceYear = inferSyslogYear(
                    filePath, cancelRequested, cancelledDuringDetection);
                if (cancelledDuringDetection) {
                    result.cancelled = true;
                    return result;
                }
                QString yearNote = QStringLiteral("otomatik");
                if (referenceYear == 0) {
                    // Icerikte tam tarih capasi yok, dosya adinda yil yok. Son care:
                    // dosyanin degistirilme yili. Ay/gun/saat dosyadan dogru geliyor,
                    // yalnizca yil tahmin -- sonuc satirinda "TAHMINI" olarak gorunur.
                    const QDateTime modified = QFileInfo(filePath).lastModified();
                    referenceYear = modified.isValid()
                                        ? modified.date().year()
                                        : QDate::currentDate().year();
                    yearNote = QStringLiteral("dosya tarihinden, TAHMINI");
                }
                parser = ParserLibrary::detect(sampleLines, referenceYear,
                                               detectedFormatName);
                detectedFormatName += QStringLiteral(" (başlangıç yılı: %1 - %2)")
                    .arg(referenceYear).arg(yearNote);
            }
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
