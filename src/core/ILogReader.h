#pragma once

#include <QString>

// Bir log kaynagindan (dosya, ileride stdin/gzip...) satir satir okuma sozlesmesi.
// FileLogReader disinda baska bir kaynak eklemek istersen sadece yeni bir implementasyon yazarsin,
// main.cpp'deki akis mantigi degismez.

class ILogReader
{
public:
    virtual ~ILogReader() = default;

    virtual bool open(const QString &path, QString &error) = 0;

    // atEnd() true donene kadar readLine() cagirmak guvenlidir.
    // atEnd() true iken readLine() cagirmak tanimsizdir

    virtual bool atEnd() const = 0;
    virtual QString readLine() = 0;

    virtual void close() = 0;
};
