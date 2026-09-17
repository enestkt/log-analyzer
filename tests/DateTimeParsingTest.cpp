#include <QCoreApplication>
#include <QDateTime>
#include <QTextStream>

#include "core/DateTimeParsing.h"

// DateTimeParsing::parse, QDateTime::fromString'in hizli yoludur ve her
// durumda onunla BIREBIR ayni sonucu vermelidir. Bu test gecerli, gecersiz ve
// hizli yola uymayan girdileri iki fonksiyona da verip sonuclari karsilastirir.
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream err(stderr);

    const QString isoMs = QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz");
    const QString iso = QStringLiteral("yyyy-MM-dd HH:mm:ss");
    const QString us = QStringLiteral("MM/dd/yyyy HH:mm:ss");

    const QList<QPair<QString, QString>> cases = {
        // hizli yol: gecerli
        { QStringLiteral("2015-10-18 18:01:47.978"), isoMs },
        { QStringLiteral("2026-09-08 10:35:46.000"), isoMs },
        { QStringLiteral("2015-10-18 18:01:47"), iso },
        { QStringLiteral("2016-02-29 12:00:00"), iso },
        { QStringLiteral("10/18/2015 18:01:47"), us },
        { QStringLiteral("0999-01-01 00:00:00"), iso },
        // gecersiz tarih/saat: fromString'e dusmeli
        { QStringLiteral("2015-02-30 18:01:47.978"), isoMs },
        { QStringLiteral("2015-02-29 12:00:00"), iso },
        { QStringLiteral("2015-10-18 25:01:47.978"), isoMs },
        { QStringLiteral("2015-10-18 18:61:47"), iso },
        { QStringLiteral("2016-12-31 23:59:60.000"), isoMs },
        { QStringLiteral("0000-01-01 00:00:00"), iso },
        { QStringLiteral("13/18/2015 18:01:47"), us },
        // bicime birebir uymayanlar: fromString'e dusmeli
        { QStringLiteral("2015-10-18T18:01:47.978"), isoMs },
        { QStringLiteral("2015-10-18 18:01:47.9"), isoMs },
        { QStringLiteral("2015-10-18 18:01:47.978123"), isoMs },
        { QStringLiteral("2015/10/18 18:01:47"), iso },
        { QStringLiteral("ab/cd/efgh ij:kl:mn"), us },
        { QStringLiteral("2015-1a-18 18:01:47"), iso },
        { QString(), iso },
        // desteklenmeyen format: fromString'e dusmeli
        { QStringLiteral("18.10.2015 18:01"), QStringLiteral("dd.MM.yyyy HH:mm") },
    };

    bool ok = true;
    for (const auto &c : cases) {
        const QDateTime fast = DateTimeParsing::parse(c.first, c.second);
        const QDateTime slow = QDateTime::fromString(c.first, c.second);
        const bool same = fast.isValid() == slow.isValid()
            && fast == slow
            && fast.timeSpec() == slow.timeSpec()
            && fast.toString(Qt::ISODateWithMs) == slow.toString(Qt::ISODateWithMs);
        if (!same) {
            err << "FAILED: '" << c.first << "' / '" << c.second << "'  hizli="
                << fast.toString(Qt::ISODateWithMs) << " (gecerli=" << fast.isValid()
                << ")  fromString=" << slow.toString(Qt::ISODateWithMs)
                << " (gecerli=" << slow.isValid() << ")\n";
            ok = false;
        }
    }
    return ok ? 0 : 1;
}
