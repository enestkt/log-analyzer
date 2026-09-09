#pragma once

#include <QHash>
#include <QString>

// Klasik Syslog basliginda yil bulunmaz. Bu sinif, ayni satirin mesajinda
// tekrarlanan tam tarihleri (orn. "Sat Jun 11 03:28:22 2005") capalama
// noktasi olarak kullanip dosyanin ilk kaydinin yilini hesaplar.
class SyslogYearDetector
{
public:
    void inspectLine(const QString &line);
    int inferredFirstYear() const;

private:
    int m_previousMonth = 0;
    int m_rolloversSeen = 0;
    QHash<int, int> m_candidateCounts;
};

// Dosya adinda ayri bir dort haneli yil varsa onu dondurur.
int syslogYearFromFileName(const QString &path);
