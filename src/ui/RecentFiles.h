#pragma once

#include <QObject>
#include <QStringList>

// TEK SORUMLULUK: son acilan log dosyalarinin listesini tutmak ve diske
// kalici olarak kaydetmek. Ekrani tanimaz; liste degisince sinyal yayar.
// (qt-media-player projesindeki RecentFiles ile ayni desen.)

class RecentFiles : public QObject
{
    Q_OBJECT

public:
    explicit RecentFiles(QObject *parent = nullptr);

    QStringList files() const;

public slots:
    void add(const QString &path);
    void clear();

signals:
    void changed();

private:
    void load();
    void save();

    QStringList m_files;
};
