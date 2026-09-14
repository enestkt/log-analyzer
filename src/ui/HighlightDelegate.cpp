#include "HighlightDelegate.h"

#include <QApplication>
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

    // 2) Metni HTML olarak kur: eslesen araliklar mavi + kalin <span> icinde,
    //    geri kalan tablodaki normal metin renginde. toHtmlEscaped, log
    //    mesajindaki '<' '>' gibi karakterlerin HTML sanilmasini onler.
    QString html = QStringLiteral("<span style=\"color:#334155;\">");
    qsizetype cursor = 0;
    for (const auto &span : spans) {
        html += text.mid(cursor, span.first - cursor).toHtmlEscaped();
        html += QStringLiteral(
                    "<span style=\"background-color:#fde047;color:#713f12;font-weight:700;\">")
              + text.mid(span.first, span.second).toHtmlEscaped()
              + QStringLiteral("</span>");
        cursor = span.first + span.second;
    }
    html += text.mid(cursor).toHtmlEscaped();
    html += QStringLiteral("</span>");

    QTextDocument doc;
    doc.setDefaultFont(opt.font);
    doc.setDocumentMargin(0);
    doc.setHtml(html);
    doc.setTextWidth(-1); // tek satir, sarma yok -- tablo satirlari tek satirlik

    // 3) Hucrenin metin bolgesine, dikeyde ortalayarak ciz. Bolge disina tasan
    //    kisim kirpilir (uzun mesajlarda tablo zaten yatay kaydiriyor/kesiyor).
    const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
    painter->save();
    painter->setClipRect(textRect);
    const qreal yOffset = (textRect.height() - doc.size().height()) / 2.0;
    painter->translate(textRect.left(), textRect.top() + qMax<qreal>(0.0, yOffset));
    doc.drawContents(painter);
    painter->restore();
}
