#include "FileLogReader.h"

bool FileLogReader::open(const QString &path, QString &error)
{
    m_file.setFileName(path);

    if(!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = m_file.errorString();
        return false;
    }
    m_stream.setDevice(&m_file);
    m_stream.setEncoding(QStringConverter::Utf8);
    return true;
}

bool FileLogReader::atEnd() const
{
    return m_stream.atEnd();
}

QString FileLogReader::readLine()
{

// QTextStream::readLine() tek bir satiri belleğe alir, readAll() gibi
// tum dosyayi yuklemez -- buyuk dosyalarda bellegin sabit kalmasini saglayan yer burasi.
    return m_stream.readLine();
}

void FileLogReader::close()
{
    m_file.close();
}