#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QTextStream>
#include <QTime>

#include "core/LogEntry.h"
#include "core/LogFilter.h"

namespace {

LogEntry entryAt(const QDate &date, const QTime &time)
{
    LogEntry entry;
    entry.timestamp = QDateTime(date, time);
    entry.level = LogLevel::Info;
    entry.message = QStringLiteral("test");
    entry.isValid = true;
    return entry;
}

bool expect(bool condition, const QString &message)
{
    if (condition)
        return true;

    QTextStream(stderr) << "FAILED: " << message << '\n';
    return false;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QDate firstDay(2026, 8, 12);
    const QDate lastDay(2026, 8, 14);

    LogFilter filter;
    filter.setDateRange(firstDay, lastDay);

    bool ok = true;
    ok &= expect(!filter.matches(entryAt(firstDay.addDays(-1), QTime(23, 59, 59, 999))),
                 QStringLiteral("baslangictan onceki kayit disarida kalmali"));
    ok &= expect(filter.matches(entryAt(firstDay, QTime(0, 0, 0, 0))),
                 QStringLiteral("baslangic gununun ilk ani dahil olmali"));
    ok &= expect(filter.matches(entryAt(lastDay, QTime(23, 59, 59, 999))),
                 QStringLiteral("bitis gununun son ani dahil olmali"));
    ok &= expect(!filter.matches(entryAt(lastDay.addDays(1), QTime(0, 0, 0, 0))),
                 QStringLiteral("bitis gununden sonraki kayit disarida kalmali"));

    LogFilter sameDayFilter;
    sameDayFilter.setDateRange(firstDay, firstDay);
    ok &= expect(sameDayFilter.matches(entryAt(firstDay, QTime(18, 42, 7, 321))),
                 QStringLiteral("tek gun secimi gunun tamamini kapsamali"));

    LogFilter minuteFilter;
    minuteFilter.setTimeRange(QDateTime(firstDay, QTime(10, 15, 0, 0)),
                              QDateTime(firstDay, QTime(10, 30, 59, 999)));
    ok &= expect(!minuteFilter.matches(entryAt(firstDay, QTime(10, 14, 59, 999))),
                 QStringLiteral("baslangic dakikasindan onceki kayit disarida kalmali"));
    ok &= expect(minuteFilter.matches(entryAt(firstDay, QTime(10, 15, 0, 0))),
                 QStringLiteral("baslangic dakikasi dahil olmali"));
    ok &= expect(minuteFilter.matches(entryAt(firstDay, QTime(10, 30, 59, 999))),
                 QStringLiteral("bitis dakikasinin tamami dahil olmali"));
    ok &= expect(!minuteFilter.matches(entryAt(firstDay, QTime(10, 31, 0, 0))),
                 QStringLiteral("bitis dakikasindan sonraki kayit disarida kalmali"));

    return ok ? 0 : 1;
}
