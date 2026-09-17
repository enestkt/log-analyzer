#include <QCoreApplication>
#include <QRegularExpression>
#include <QTextStream>

#include "core/LogEntry.h"
#include "core/LogFilter.h"

namespace {

// Genel ayristiricinin statusLog satirini kaydettigi hali: koseli parantezli
// alanlar mesajdan atilmis, ham satirda duruyor.
LogEntry statusLogEntry()
{
    LogEntry entry;
    entry.timestamp = QDateTime(QDate(2026, 9, 8), QTime(10, 35, 46, 311));
    entry.level = LogLevel::Unknown;
    entry.message = QStringLiteral("serialThr=0x5601e61a1f50 serialIfc=0x5601e6295fb0");
    entry.rawLine = QStringLiteral(
        "[2026-09-08 10:35:46.311][0x7f0f8eb03680][SER/SETUP_THREAD_STARTED][78550283ms] "
        "serialThr=0x5601e61a1f50 serialIfc=0x5601e6295fb0");
    entry.isValid = true;
    return entry;
}

bool searchMatches(const LogEntry &entry, const QString &pattern)
{
    LogFilter filter;
    filter.setSearchPattern(QRegularExpression(pattern));
    return filter.matches(entry);
}

bool passes(const LogEntry &entry, const QString &search, const QString &exclude)
{
    LogFilter filter;
    filter.setSearchPattern(QRegularExpression(search));
    filter.setExcludePattern(QRegularExpression(exclude));
    return filter.matches(entry);
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
    const LogEntry entry = statusLogEntry();

    bool ok = true;
    ok &= expect(searchMatches(entry, QStringLiteral("serialThr")),
                 QStringLiteral("mesajdaki kelime bulunmali"));
    ok &= expect(searchMatches(entry, QStringLiteral("SETUP_THREAD_STARTED")),
                 QStringLiteral("mesajdan atilan kategori alani ham satirda bulunmali"));
    ok &= expect(searchMatches(entry, QStringLiteral("0x7f0f8eb03680")),
                 QStringLiteral("mesajdan atilan thread kimligi ham satirda bulunmali"));
    ok &= expect(searchMatches(entry, QStringLiteral("SER/\\w+")),
                 QStringLiteral("regex arama da ham satira uygulanmali"));
    ok &= expect(!searchMatches(entry, QStringLiteral("ENCODER")),
                 QStringLiteral("satirda gecmeyen kelime eslesmemeli"));

    // Ham satiri olmayan, elle olusturulmus kayitta mesaja geri dusulmeli.
    LogEntry manual;
    manual.timestamp = QDateTime(QDate(2026, 9, 8), QTime(12, 0));
    manual.message = QStringLiteral("connection refused");
    manual.isValid = true;
    ok &= expect(searchMatches(manual, QStringLiteral("refused")),
                 QStringLiteral("ham satir bossa mesajda aranmali"));
    ok &= expect(!searchMatches(manual, QStringLiteral("timeout")),
                 QStringLiteral("ham satir bossa mesajda olmayan kelime eslesmemeli"));

    // Hariç tutma
    ok &= expect(!passes(entry, QString(), QStringLiteral("SER/SETUP_THREAD_STARTED")),
                 QStringLiteral("kategorisi hariç tutulan satir elenmeli"));
    ok &= expect(passes(entry, QString(), QStringLiteral("UG/TICK")),
                 QStringLiteral("satirda gecmeyen kategori hariç tutulunca satir kalmali"));
    ok &= expect(!passes(entry, QString(), QStringLiteral("UG/TICK ALIVE setup_thread")),
                 QStringLiteral("bosluklu kelimelerden herhangi biri (harf duyarsiz) eslesince elenmeli"));
    ok &= expect(passes(entry, QString(), QStringLiteral("serialThx")),
                 QStringLiteral("hariç tutmada yazim hatasi toleransi olmamali"));
    ok &= expect(!passes(entry, QString(), QStringLiteral("SER/\\w+")),
                 QStringLiteral("regex hariç tutma calismali"));
    ok &= expect(!passes(entry, QStringLiteral("serialThr"), QStringLiteral("0x7f0f8eb03680")),
                 QStringLiteral("arama eslesse bile hariç tutulan satir elenmeli"));
    ok &= expect(passes(entry, QStringLiteral("serialThr"), QString()),
                 QStringLiteral("bos hariç tutma deseni devre disi olmali"));
    ok &= expect(!passes(manual, QString(), QStringLiteral("refused")),
                 QStringLiteral("ham satir bossa hariç tutma mesaja bakmali"));

    return ok ? 0 : 1;
}
