#include <QCoreApplication>
#include <QDate>
#include <QTemporaryFile>
#include <QTextStream>
#include <memory>
#include <QFileInfo>

#include "core/LogEntry.h"
#include "core/ParserLibrary.h"
#include "core/SyslogParser.h"
#include "core/SyslogYearDetector.h"
#include "ui/LogAnalysisWorker.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Secilen yil ilk kayda uygulanmali, dosya sistemi tarihi kullanilmamali.
    SyslogParser parser(2007);
    LogEntry december;
    if (!parser.parseLine(
            QStringLiteral("Dec 31 23:59:59 host sshd[1]: last record"), december))
        return 1;
    if (december.timestamp.date().year() != 2007)
        return 2;

    // Kronolojik dosyada Aralik -> Ocak gecisi yeni yila tasinmali.
    LogEntry january;
    if (!parser.parseLine(
            QStringLiteral("Jan  1 00:00:01 host sshd[1]: first record"), january))
        return 3;
    if (january.timestamp.date().year() != 2008)
        return 4;

    const QStringList samples = {
        QStringLiteral("Dec 31 23:59:59 host job[1]: completed at Mon Dec 31 23:59:59 2007"),
        QStringLiteral("Jan  1 00:00:01 host sshd[1]: first record")
    };
    QString formatName;
    bool usesReferenceYear = false;
    std::unique_ptr<ILogParser> detected = ParserLibrary::detect(
        samples, 2007, formatName, &usesReferenceYear);
    if (!usesReferenceYear || formatName != QStringLiteral("Syslog"))
        return 5;

    // Algilama icin yapilan ornek okuma, asil parser'in yil durumunu kirletmemeli.
    LogEntry detectedFirst;
    if (!detected->parseLine(samples.first(), detectedFirst)
        || detectedFirst.timestamp.date().year() != 2007)
        return 6;

    QTemporaryFile file;
    if (!file.open())
        return 7;
    QTextStream stream(&file);
    stream << samples.join(QLatin1Char('\n')) << '\n';
    stream.flush();
    const QString path = file.fileName();
    file.close();

    GuiAnalysisRequest request;
    request.filePaths = {path};
    auto cancelRequested = std::make_shared<std::atomic_bool>(false);
    GuiAnalysisResult result = LogAnalysisWorker::run(request, cancelRequested);
    if (!result.errorMessage.isEmpty() || result.entries.size() != 2)
        return 8;
    if (result.entries.at(0).timestamp.date().year() != 2007
        || result.entries.at(1).timestamp.date().year() != 2008)
        return 9;

    // Icerikte tam tarih capasi yoksa program artik HATA VERMEZ; son care olarak
    // dosyanin degistirilme yilini kullanip sonucu "TAHMINI" diye etiketler.
    QTemporaryFile noAnchorFile;
    if (!noAnchorFile.open())
        return 10;
    QTextStream noAnchorStream(&noAnchorFile);
    noAnchorStream << "Dec 31 23:59:59 host sshd[1]: no full date here\n";
    noAnchorStream << "Jan  1 00:00:01 host sshd[1]: still no year\n";
    noAnchorStream.flush();
    const QString noAnchorPath = noAnchorFile.fileName();
    noAnchorFile.close();

    GuiAnalysisRequest fallbackRequest;
    fallbackRequest.filePaths = {noAnchorPath};
    GuiAnalysisResult fallbackResult = LogAnalysisWorker::run(
        fallbackRequest, std::make_shared<std::atomic_bool>(false));
    if (!fallbackResult.errorMessage.isEmpty() || fallbackResult.entries.size() != 2)
        return 11;

    const int fileYear = QFileInfo(noAnchorPath).lastModified().date().year();
    if (fallbackResult.entries.at(0).timestamp.date().year() != fileYear)
        return 12;
    if (fallbackResult.detectedFormats.isEmpty()
        || !fallbackResult.detectedFormats.first().contains(QStringLiteral("TAHMINI")))
        return 13;

    // Aralik kaydi olmayan seyrek bir dosyada bile Kasim -> Ocak gecisi yeni yil
    // sayilmali. Detector ile parser ayni kurali kullandigi icin, icindeki capa
    // 2006 diyorsa Ocak satirlari da 2006 olmali (2005 degil).
    QTemporaryFile gapFile;
    if (!gapFile.open())
        return 14;
    QTextStream gapStream(&gapFile);
    gapStream << "Nov 20 10:00:00 host app[1]: november event\n";
    gapStream << "Nov 30 23:00:00 host app[1]: end of november\n";
    gapStream << "Jan  5 08:00:00 host app[1]: anchor at Thu Jan  5 08:00:00 2006\n";
    gapStream << "Jan  6 09:00:00 host app[1]: second january line\n";
    gapStream.flush();
    const QString gapPath = gapFile.fileName();
    gapFile.close();

    GuiAnalysisRequest gapRequest;
    gapRequest.filePaths = {gapPath};
    GuiAnalysisResult gapResult = LogAnalysisWorker::run(
        gapRequest, std::make_shared<std::atomic_bool>(false));
    if (!gapResult.errorMessage.isEmpty() || gapResult.entries.size() != 4)
        return 15;
    if (gapResult.entries.at(0).timestamp.date().year() != 2005
        || gapResult.entries.at(1).timestamp.date().year() != 2005)
        return 16;
    if (gapResult.entries.at(2).timestamp.date().year() != 2006
        || gapResult.entries.at(3).timestamp.date().year() != 2006)
        return 17;

    return 0;
}
