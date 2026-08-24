#include "LogFilter.h"
#include "LogEntry.h"

#include <QStringList>
#include <QVector>
#include <algorithm>

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

    const int distance = levenshteinDistance(word.toLower(), term.toLower());
    return distance <= fuzzyThreshold(term.length());
}

bool fuzzyContains(const QString &message, const QString &searchText)
{
    const QStringList terms = searchText.split(QRegularExpression(QStringLiteral("\\s+")),
                                                Qt::SkipEmptyParts);
    if (terms.isEmpty())
        return true;

    const QStringList words = message.split(QRegularExpression(QStringLiteral("\\W+")),
                                             Qt::SkipEmptyParts);

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
        const QString rawSearchText = m_searchPattern.pattern();
        if (looksLikeRegex(rawSearchText)) {
            if(!m_searchPattern.match(entry.message).hasMatch())
                return false;
        } else {
            if(!fuzzyContains(entry.message, rawSearchText))
                return false;
        }
    }
    return true;
}
