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

private:
    QFile m_file;
    QTextStream m_stream;
};
