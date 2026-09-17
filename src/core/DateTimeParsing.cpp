#include "DateTimeParsing.h"

#include <QDate>
#include <QTime>

namespace {

// text[pos, pos + count) araligini sayiya cevirir; rakam disi bir karakter
// varsa ok false olur.
int readDigits(const QString &text, qsizetype pos, qsizetype count, bool &ok)
{
    int value = 0;
    for (qsizetype i = pos; i < pos + count; ++i) {
        const char16_t c = text.at(i).unicode();
        if (c < u'0' || c > u'9') {
            ok = false;
            return 0;
        }
        value = value * 10 + (c - u'0');
    }
    return value;
}

// "HH:mm:ss" kismi (11. karakterden baslayarak) ve ayiricilari.
bool readTime(const QString &text, int &hour, int &minute, int &second)
{
    bool ok = text.at(10) == QLatin1Char(' ')
        && text.at(13) == QLatin1Char(':')
        && text.at(16) == QLatin1Char(':');
    if (!ok)
        return false;
    hour = readDigits(text, 11, 2, ok);
    minute = readDigits(text, 14, 2, ok);
    second = readDigits(text, 17, 2, ok);
    return ok;
}

} // namespace

QDateTime DateTimeParsing::parse(const QString &text, const QString &format)
{
    bool ok = false;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, msec = 0;

    const bool isoWithMs = format == QLatin1String("yyyy-MM-dd HH:mm:ss.zzz")
        && text.size() == 23 && text.at(19) == QLatin1Char('.');
    const bool isoNoMs = format == QLatin1String("yyyy-MM-dd HH:mm:ss") && text.size() == 19;
    const bool usDate = format == QLatin1String("MM/dd/yyyy HH:mm:ss") && text.size() == 19;

    if (isoWithMs || isoNoMs) {
        ok = text.at(4) == QLatin1Char('-') && text.at(7) == QLatin1Char('-');
        if (ok) {
            year = readDigits(text, 0, 4, ok);
            month = readDigits(text, 5, 2, ok);
            day = readDigits(text, 8, 2, ok);
            ok = ok && readTime(text, hour, minute, second);
            if (ok && isoWithMs)
                msec = readDigits(text, 20, 3, ok);
        }
    } else if (usDate) {
        ok = text.at(2) == QLatin1Char('/') && text.at(5) == QLatin1Char('/');
        if (ok) {
            month = readDigits(text, 0, 2, ok);
            day = readDigits(text, 3, 2, ok);
            year = readDigits(text, 6, 4, ok);
            ok = ok && readTime(text, hour, minute, second);
        }
    }

    if (ok) {
        const QDate date(year, month, day);
        const QTime time(hour, minute, second, msec);
        // Gecersiz tarih/saat (30 Subat, 25:00, 23:59:60 gibi) hizli yoldan
        // kurulmaz: QDateTime gecersiz saati sessizce gece yarisina cevirir,
        // fromString ise gecersiz dondurur. Bu durumlar asagida fromString'e
        // birakilir ki sonuc birebir ayni kalsin.
        if (date.isValid() && time.isValid())
            return QDateTime(date, time);
    }
    return QDateTime::fromString(text, format);
}
