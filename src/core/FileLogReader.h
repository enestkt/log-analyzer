#pragma once

#include <QFile>
#include <QTextStream>

#include "ILogReader.h"

class FileLogReader : public ILogReader
{
public:
    bool open(const QString &path, QString &error) override;
    bool atEnd() const override;
    QString readLine() override;
    void close() override;

    // Dosyadan su ana kadar okunan bayt. QTextStream kendi tamponuna onceden
    // okudugu icin islenen satirlardan en fazla bir tampon kadar ileride olabilir;
    // ilerleme gostermek icin bu hassasiyet yeterli.
    qint64 bytesRead() const { return m_file.pos(); }

private:
    QFile m_file;
    QTextStream m_stream;
};
