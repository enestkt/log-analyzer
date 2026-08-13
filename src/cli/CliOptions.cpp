#include "CliOptions.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>

#include "../core/RegexLogParser.h"

bool CliOptions::parse(const QCoreApplication &app, QString &error)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Buyuk log dosyalarini filtreleyip istatistik cikaran komut satiri araci."));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption fileOption(QStringList{ "file" },
                                        QStringLiteral("Log dosyasi yolu (zorunlu)."), QStringLiteral("path"));
    const QCommandLineOption fromOption(QStringList{ "from" },
                                        QStringLiteral("Baslangic zamani, ISO 8601 (orn. 2026-08-11T00:00:00)."), QStringLiteral("timestamp"));
    const QCommandLineOption toOption(QStringList{ "to" },
                                      QStringLiteral("Bitis zamani, ISO 8601."), QStringLiteral("timestamp"));
    const QCommandLineOption levelOption(QStringList{ "level" },
                                         QStringLiteral("Minimum seviye: TRACE/DEBUG/INFO/WARNING/ERROR/CRITICAL."), QStringLiteral("level"));
    const QCommandLineOption parserPatternOption(QStringList{ "parser-pattern" },
                                                 QStringLiteral("Ozel parse regex'i (timestamp/level/message named group'lari sart)."), QStringLiteral("regex"));
    const QCommandLineOption timestampFormatOption(QStringList{ "timestamp-format" },
                                                   QStringLiteral("Log icindeki zaman damgasi formati (Qt format stringi)."), QStringLiteral("format"),
                                                   RegexLogParser::defaultTimestampFormat());
    const QCommandLineOption searchOption(QStringList{ "search" },
                                          QStringLiteral("Mesaj icinde aranacak regex."), QStringLiteral("regex"));
    const QCommandLineOption exportOption(QStringList{ "export" },
                                          QStringLiteral("Disa aktarma formati: csv veya json."), QStringLiteral("format"));
    const QCommandLineOption outputOption(QStringList{ "output" },
                                          QStringLiteral("Disa aktarma dosyasi yolu."), QStringLiteral("path"));

    parser.addOption(fileOption);
    parser.addOption(fromOption);
    parser.addOption(toOption);
    parser.addOption(levelOption);
    parser.addOption(parserPatternOption);
    parser.addOption(timestampFormatOption);
    parser.addOption(searchOption);
    parser.addOption(exportOption);
    parser.addOption(outputOption);

    parser.process(app); // --help/--version burada otomatik yakalanip programi sonlandirir

    if (!parser.isSet(fileOption)) {
        error = QStringLiteral("--file zorunludur.");
        return false;
    }
    m_filePath = parser.value(fileOption);

    const QFileInfo fileInfo(m_filePath);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        error = QStringLiteral("Dosya bulunamadi ya da okunamiyor: %1").arg(m_filePath);
        return false;
    }

    if (parser.isSet(fromOption)) {
        m_from = QDateTime::fromString(parser.value(fromOption), Qt::ISODate);
        if (!m_from.isValid()) {
            error = QStringLiteral("--from gecersiz, ISO 8601 formatinda olmali (orn. 2026-08-11T00:00:00).");
            return false;
        }
    }

    if (parser.isSet(toOption)) {
        m_to = QDateTime::fromString(parser.value(toOption), Qt::ISODate);
        if (!m_to.isValid()) {
            error = QStringLiteral("--to gecersiz, ISO 8601 formatinda olmali.");
            return false;
        }
    }

    if (m_from.isValid() && m_to.isValid() && m_from > m_to) {
        error = QStringLiteral("--from, --to'dan sonra olamaz.");
        return false;
    }

    if (parser.isSet(levelOption)) {
        m_minLevel = logLevelFromString(parser.value(levelOption));
        if (m_minLevel == LogLevel::Unknown) {
            error = QStringLiteral("Gecersiz --level degeri: %1").arg(parser.value(levelOption));
            return false;
        }
    }

    m_parserPattern = parser.isSet(parserPatternOption)
                          ? QRegularExpression(parser.value(parserPatternOption))
                          : RegexLogParser::defaultPattern();
    if (!m_parserPattern.isValid()) {
        error = QStringLiteral("--parser-pattern gecersiz: %1").arg(m_parserPattern.errorString());
        return false;
    }

    m_timestampFormat = parser.value(timestampFormatOption);

    if (parser.isSet(searchOption)) {
        m_searchPattern = QRegularExpression(parser.value(searchOption));
        if (!m_searchPattern.isValid()) {
            error = QStringLiteral("--search gecersiz: %1").arg(m_searchPattern.errorString());
            return false;
        }
    }

    const bool exportSet = parser.isSet(exportOption);
    const bool outputSet = parser.isSet(outputOption);
    if (exportSet != outputSet) {
        error = QStringLiteral("--export ve --output birlikte kullanilmali.");
        return false;
    }

    if (exportSet) {
        const QString format = parser.value(exportOption).toLower();
        if (format != QStringLiteral("csv") && format != QStringLiteral("json")) {
            error = QStringLiteral("Gecersiz --export degeri: %1 (csv ya da json olmali).").arg(format);
            return false;
        }
        m_exportFormat = format;
        m_outputPath = parser.value(outputOption);
    }

    return true;
}