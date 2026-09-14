#pragma once

#include <QRegularExpression>
#include <QStyledItemDelegate>

// Sonuc tablosunun "Mesaj" sutununu cizen ozel delegate. Arama kutusundaki
// kelime/regex ile eslesen kisimlari mavi + kalin gosterir; boylece kullanici
// satirin NEDEN filtreden gectigini bir bakista gorur.
//
// Hangi araligin eslestigini core'daki LogFilter::highlightSpans hesaplar --
// bu sinif sadece boyar. Boylece "eslesme" tanimi filtreyle her zaman ayni kalir.
class HighlightDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit HighlightDelegate(QObject *parent = nullptr);

    // Her aramada guncel desen buraya verilir; bos desen vurgulamayi kapatir.
    void setSearchPattern(const QRegularExpression &pattern);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    QRegularExpression m_searchPattern;
};
