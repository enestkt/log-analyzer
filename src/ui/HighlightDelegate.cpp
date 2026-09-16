#include "HighlightDelegate.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QPainter>
#include <QTextDocument>

#include "../core/LogFilter.h"

HighlightDelegate::HighlightDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void HighlightDelegate::setSearchPattern(const QRegularExpression &pattern)
{
    m_searchPattern = pattern;
}

void HighlightDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QString text = opt.text;
    const auto spans = LogFilter::highlightSpans(text, m_searchPattern);
    if (spans.isEmpty()) {
        // Eslesme yoksa (ya da arama bos ise) varsayilan cizim yeterli.
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // 1) Hucrenin zeminini (satir arka plani, secim rengi) normal yoldan cizdir;
    //    metni bosaltiyoruz ki iki kez yazilmasin.
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    QStyleOptionViewItem background = opt;
    background.text.clear();
    style->drawControl(QStyle::CE_ItemViewItem, &background, painter, opt.widget);

    const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
    const qreal available = textRect.width();

    // 2) Metnin hangi kismi gorunecek? Eslesen karakterler kalin cizildigi icin
    //    genislik karakter karakter, dogru fontla olculur.
    QFont boldFont = opt.font;
    boldFont.setBold(true);
    const QFontMetricsF normalMetrics(opt.font);
    const QFontMetricsF boldMetrics(boldFont);

    QVector<bool> inMatch(text.size(), false);
    for (const auto &span : spans) {
        for (qsizetype i = span.first; i < span.first + span.second; ++i)
            inMatch[i] = true;
    }
    const auto charWidth = [&](qsizetype i) {
        return inMatch[i] ? boldMetrics.horizontalAdvance(text.at(i))
                          : normalMetrics.horizontalAdvance(text.at(i));
    };
    const QString ellipsis = QStringLiteral("…");
    const qreal ellipsisWidth = normalMetrics.horizontalAdvance(ellipsis);

    //    Ilk eslesme hucreye sigmiyorsa (uzun mesajin sonundaysa) metni bastan
    //    kirp: eslesmeden once alanin yaklasik ucte biri kadar baglam birak.
    //    Boylece kullanici satirin neden geldigini her zaman gorur.
    qsizetype start = 0;
    const qsizetype firstMatchStart = spans.first().first;
    const qsizetype firstMatchEnd = firstMatchStart + spans.first().second;
    qreal widthToFirstMatchEnd = 0;
    for (qsizetype i = 0; i < firstMatchEnd; ++i)
        widthToFirstMatchEnd += charWidth(i);
    if (widthToFirstMatchEnd > available) {
        start = firstMatchStart;
        qreal context = ellipsisWidth;
        while (start > 0 && context + charWidth(start - 1) <= available / 3) {
            --start;
            context += charWidth(start);
        }
    }

    //    Sigan kadar karakter al; tasan kisim varsa sona "..." birak.
    qreal used = start > 0 ? ellipsisWidth : 0;
    qsizetype end = start;
    while (end < text.size() && used + charWidth(end) <= available) {
        used += charWidth(end);
        ++end;
    }
    const bool cutEnd = end < text.size();
    if (cutEnd) {
        while (end > start && used + ellipsisWidth > available) {
            --end;
            used -= charWidth(end);
        }
    }

    // 3) Gorunen parcayi HTML olarak kur: eslesen araliklar sari zemin + kalin,
    //    geri kalan tablodaki normal metin renginde. toHtmlEscaped, log
    //    mesajindaki '<' '>' gibi karakterlerin HTML sanilmasini onler;
    //    white-space:pre ard arda gelen bosluklarin tek bosluga inmesini onler.
    QString html = QStringLiteral("<span style=\"color:#334155;white-space:pre;\">");
    if (start > 0)
        html += ellipsis;
    qsizetype cursor = start;
    for (const auto &span : spans) {
        const qsizetype spanStart = qMax(span.first, start);
        const qsizetype spanEnd = qMin(span.first + span.second, end);
        if (spanStart >= spanEnd)
            continue;
        html += text.mid(cursor, spanStart - cursor).toHtmlEscaped();
        html += QStringLiteral(
                    "<span style=\"background-color:#fde047;color:#713f12;font-weight:700;\">")
              + text.mid(spanStart, spanEnd - spanStart).toHtmlEscaped()
              + QStringLiteral("</span>");
        cursor = spanEnd;
    }
    html += text.mid(cursor, end - cursor).toHtmlEscaped();
    if (cutEnd)
        html += ellipsis;
    html += QStringLiteral("</span>");

    QTextDocument doc;
    doc.setDefaultFont(opt.font);
    doc.setDocumentMargin(0);
    doc.setHtml(html);
    doc.setTextWidth(-1); // tek satir, sarma yok -- tablo satirlari tek satirlik

    // 4) Hucrenin metin bolgesine, dikeyde ortalayarak ciz. Olcumdeki kucuk
    //    yuvarlama farklari icin bolge disi yine kirpilir.
    painter->save();
    painter->setClipRect(textRect);
    const qreal yOffset = (textRect.height() - doc.size().height()) / 2.0;
    painter->translate(textRect.left(), textRect.top() + qMax<qreal>(0.0, yOffset));
    doc.drawContents(painter);
    painter->restore();
}
