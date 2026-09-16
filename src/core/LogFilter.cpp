#include "LogFilter.h"
#include "LogEntry.h"

#include <QStringList>
#include <QVector>
#include <algorithm>
#include <utility>

namespace {

bool looksLikeRegex(const QString &text)
{
    static const QString metaChars = QStringLiteral(".^$*+?()[]{}|\\");
    for (const QChar &ch : text) {
        if (metaChars.contains(ch))
            return true;
    }
    return false;
}

int levenshteinDistance(const QString &a, const QString &b)
{
    const int m = a.size();
    const int n = b.size();
    QVector<QVector<int>> dp(m + 1, QVector<int>(n + 1, 0));

    for (int i = 0; i <= m; ++i)
        dp[i][0] = i;
    for (int j = 0; j <= n; ++j)
        dp[0][j] = j;

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (a[i - 1] == b[j - 1])
                dp[i][j] = dp[i - 1][j - 1];
            else
                dp[i][j] = 1 + std::min({ dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1] });
        }
    }
    return dp[m][n];
}

// Elasticsearch'in "AUTO" fuzziness kuralina benzer: kisa kelimede tolerans yok,
// orta uzunlukta 1, uzun kelimede 2 harf hatasi kabul edilir.
int fuzzyThreshold(int termLength)
{
    if (termLength < 3)
        return 0;
    if (termLength <= 5)
        return 1;
    return 2;
}

bool fuzzyWordMatches(const QString &word, const QString &term)
{
    // Onceki davranisi (alt-dize arama) koruyoruz: "contain" yazinca "container" bulunsun.
    if (word.contains(term, Qt::CaseInsensitive))
        return true;

    // Levenshtein mesafesi en az iki kelimenin uzunluk farki kadardir. Fark
    // toleransi zaten asiyorsa pahali hesaba girmeye gerek yok; ham satirdaki
    // hex kimlikler, uzun sinif adlari gibi kelimelerin cogu burada elenir.
    const int threshold = fuzzyThreshold(term.length());
    if (qAbs(word.length() - term.length()) > threshold)
        return false;

    const int distance = levenshteinDistance(word.toLower(), term.toLower());
    return distance <= threshold;
}

bool fuzzyContains(const QString &message, const QString &searchText)
{
    // Her satirda yeniden derlenmesinler diye bir kez olusturuluyor.
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    static const QRegularExpression nonWord(QStringLiteral("\\W+"));

    const QStringList terms = searchText.split(whitespace, Qt::SkipEmptyParts);
    if (terms.isEmpty())
        return true;

    const QStringList words = message.split(nonWord, Qt::SkipEmptyParts);

    for (const QString &term : terms) {
        bool found = false;
        for (const QString &word : words) {
            if (fuzzyWordMatches(word, term)) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

} // namespace

void LogFilter::setTimeRange(const QDateTime &from, const QDateTime &to)
{
    m_from = from;
    m_to = to;
}

void LogFilter::setDateRange(const QDate &from, const QDate &to)
{
    m_from = from.isValid() ? QDateTime(from, QTime(0, 0, 0, 0)) : QDateTime();
    m_to = to.isValid() ? QDateTime(to, QTime(23, 59, 59, 999)) : QDateTime();
}

void LogFilter::setMinLevel(LogLevel level)
{
    m_minLevel = level;
}

void LogFilter::setSearchPattern(const QRegularExpression &pattern)
{
    m_searchPattern = pattern;
}

bool LogFilter::matches(const LogEntry &entry) const
{
    if(!entry.isValid)
        return false;

    if(m_from.isValid() && entry.timestamp < m_from)
        return false;
    if(m_to.isValid() && entry.timestamp > m_to)
        return false;

    if(m_minLevel != LogLevel::Unknown){
        if(entry.level == LogLevel::Unknown)
            return false;
        if(static_cast<int>(entry.level) < static_cast<int>(m_minLevel))
            return false;
    }

    if(m_searchPattern.isValid() && !m_searchPattern.pattern().isEmpty()) {
        // Arama ham satirin tamamina bakar. Ayristiricilar thread kimligi,
        // kategori gibi koseli parantezli alanlari mesajdan ayirip atiyor;
        // yalnizca mesaja bakilsaydi kullanici dosyada gordugu metni
        // bulamazdi (grep ile ayni davranis). Ham satiri olmayan, elle
        // olusturulmus kayitlarda mesaja bakilir.
        const QString &searchedText = entry.rawLine.isEmpty() ? entry.message : entry.rawLine;
        const QString rawSearchText = m_searchPattern.pattern();
        if (looksLikeRegex(rawSearchText)) {
            if(!m_searchPattern.match(searchedText).hasMatch())
                return false;
        } else {
            if(!fuzzyContains(searchedText, rawSearchText))
                return false;
        }
    }
    return true;
}

QVector<QPair<qsizetype, qsizetype>> LogFilter::highlightSpans(
    const QString &message, const QRegularExpression &searchPattern)
{
    QVector<QPair<qsizetype, qsizetype>> spans;
    if (!searchPattern.isValid() || searchPattern.pattern().isEmpty())
        return spans;

    const QString rawSearchText = searchPattern.pattern();
    if (looksLikeRegex(rawSearchText)) {
        // Regex modu: desenin mesajda eslestigi her parcayi isaretle.
        QRegularExpressionMatchIterator it = searchPattern.globalMatch(message);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            if (match.capturedLength(0) > 0)
                spans.append({ match.capturedStart(0), match.capturedLength(0) });
        }
    } else {
        // Duz kelime modu: fuzzyContains ile ayni mantik, ama kelimelerin
        // mesaj icindeki KONUMLARINA da ihtiyacimiz oldugu icin split yerine
        // globalMatch ile geziyoruz.
        static const QRegularExpression whitespace(QStringLiteral("\\s+"));
        const QStringList terms = rawSearchText.split(whitespace, Qt::SkipEmptyParts);
        static const QRegularExpression wordPattern(
            QStringLiteral("\\w+"), QRegularExpression::UseUnicodePropertiesOption);
        QRegularExpressionMatchIterator words = wordPattern.globalMatch(message);
        while (words.hasNext()) {
            const QRegularExpressionMatch wordMatch = words.next();
            const QString word = wordMatch.captured(0);
            for (const QString &term : terms) {
                // Alt-dize eslesmesi: sadece aranan parcayi boya ("info" ->
                // "information" kelimesinin ilk 4 harfi).
                const qsizetype inWord = word.indexOf(term, 0, Qt::CaseInsensitive);
                if (inWord >= 0) {
                    spans.append({ wordMatch.capturedStart(0) + inWord,
                                   qsizetype(term.size()) });
                    break;
                }
                // Yazim hatasi toleransi: kelimenin tamamini boya.
                const int threshold = fuzzyThreshold(term.length());
                if (qAbs(word.size() - term.size()) <= threshold
                    && levenshteinDistance(word.toLower(), term.toLower()) <= threshold) {
                    spans.append({ wordMatch.capturedStart(0),
                                   qsizetype(word.size()) });
                    break;
                }
            }
        }
    }

    // Ust uste binen/ic ice gecen araliklari birlestir -- boyama kodu sirali
    // ve ayrik araliklar bekler.
    std::sort(spans.begin(), spans.end());
    QVector<QPair<qsizetype, qsizetype>> merged;
    for (const auto &span : std::as_const(spans)) {
        if (!merged.isEmpty()
            && span.first <= merged.last().first + merged.last().second) {
            const qsizetype end = qMax(merged.last().first + merged.last().second,
                                       span.first + span.second);
            merged.last().second = end - merged.last().first;
        } else {
            merged.append(span);
        }
    }
    return merged;
}
